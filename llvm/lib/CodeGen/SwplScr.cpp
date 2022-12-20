//=- SwplScr.cpp -  Swpl Scheduling common function -*- C++ -*---------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Swpl Scheduling common function
//
//===----------------------------------------------------------------------===//

#include "SwplScr.h"
#include "SWPipeliner.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// private

bool SwplScr::getCoefficient(const MachineOperand &op, int *coefficient) const {
  if (op.isImm()) {
    *coefficient= op.getImm();
  } else if (op.isCImm()) {
    const auto *cimm= op.getCImm();
    if (cimm->getBitWidth()>31) {
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "DBG(SwplScr::getCoefficient): size(constant)>31\n";
      }
      *coefficient=0;
      return false;
    }
    *coefficient=cimm->getZExtValue();
  } else {
    llvm_unreachable("noConstant!");
  }
  return true;
}

void SwplScr::makeBypass(const SwplTransformedMIRInfo &tmi,
                     const DebugLoc& dbgloc,
                     MachineBasicBlock &skip_kernel_from,
                     MachineBasicBlock &skip_kernel_to,
                     MachineBasicBlock &skip_mod_from,
                     MachineBasicBlock &skip_mod_to) {

  SWPipeliner::TII->makeBypassKernel (
      *SWPipeliner::MRI,
      tmi.originalDoInitVar, dbgloc, skip_kernel_from, skip_kernel_to,
      tmi.requiredKernelIteration * tmi.coefficient + tmi.minConstant);
  SWPipeliner::TII->makeBypassMod(tmi.updateDoVRegMI->getOperand(0).getReg(),
                     dbgloc, tmi.branchDoVRegMI->getOperand(0), skip_mod_from, skip_mod_to);
}


bool SwplScr::getDoInitialValue(SwplTransformedMIRInfo &TMI) const {
  ///
  /// ```
  /// PH: %reg1=MOVi32imm imm OR %regs=MOVi64imm imm
  ///     %reg2=SUBREG_TO_REG 0, reg1 OR reg2=COPY reg1
  /// L:  %reg3=PHI %reg2, PH, %reg5, L
  /// ```
  ///
  const auto phiIter=SWPipeliner::MRI->def_instr_begin(TMI.originalDoVReg);
  TMI.initDoVRegMI =&*phiIter;
  const MachineOperand *t;
  assert(TMI.initDoVRegMI->isPHI());

  if (TMI.initDoVRegMI->getOperand(2).getMBB()==ML.getTopBlock())
    t=&(TMI.initDoVRegMI->getOperand(3));
  else
    t=&(TMI.initDoVRegMI->getOperand(1));

  TMI.originalDoInitVar=t->getReg();
  assert (t->isReg());

  // 物理レジスタは仮引数のため、探索を中断する
  while(t->getReg().isVirtual())  {
    const auto defIter= SWPipeliner::MRI->def_instr_begin(t->getReg());
    const MachineInstr*mi=&*defIter;
    t=&(mi->getOperand(1));
    if (mi->isMoveImmediate()) break;
    if (!mi->isSubregToReg() && !mi->isCopy()) return false;
    if (mi->isSubregToReg()) t=&(mi->getOperand(2));
    assert(t->isReg());
  }

  if (t->isImm() || t->isCImm()) {
    TMI.isIterationCountConstant=getCoefficient(*t, &(TMI.doVRegInitialValue));
    return TMI.isIterationCountConstant;
  }
  return false;
}

void SwplScr::moveBody(llvm::MachineBasicBlock*newBody, llvm::MachineBasicBlock*oldBody) {
  auto I=newBody->getFirstTerminator();
  auto E=oldBody->end();
  for (auto J=oldBody->begin(); J!=E;) {
    MachineInstr *mi=&*J++;
    oldBody->remove(mi);
    newBody->insert(I,mi);
  }
}

// public

bool SwplScr::findBasicInductionVariable(SwplTransformedMIRInfo &TMI) const {
  /// SwplTransformedMIRInfoの以下メンバを設定する(括弧内は下記例のどれかを示す)
  /// - isIterationCountConstant(immを抽出成功)
  /// - doVRegInitialValue(imm)
  /// - originalKernelIteration((doVRegInitialValue-constant)/coefficient)
  /// - originalDoVReg(reg3:updateDoVRegMIの比較対象レジスタ)
  /// - updateDoVRegMI(SUBSXri)
  /// - branchDoVRegMI(Bcc)
  /// - coefficient(1)
  /// - constant(下の場合は0-coefficient)
  ///
  /// ```
  /// PH: %reg1=MOVi32imm imm OR %regs=MOVi64imm imm
  ///     %reg2=SUBREG_TO_REG 0, reg1 OR reg2=COPY reg1
  /// L:  %reg3=PHI %reg2, PH, %reg5, L
  ///     etc
  ///     %reg4=SUBSXri %reg3, 1, 0, implicit-def $nzcv
  ///     %reg5=COPY %reg4
  ///     Bcc 1, %bb.2, implicit $nzcv
  /// ```
  ///


  MachineInstr*branch=nullptr;
  MachineInstr*cmp=nullptr;
  MachineInstr*addsub=nullptr;

  /// (1) findMIsForLoop() : loop制御に関わる言を探す(branch, cmp, addsub)
  if (!SWPipeliner::TII->findMIsForLoop(*ML.getTopBlock(), &branch, &cmp, &addsub)) return false;

  unsigned imm = branch->getOperand(0).getImm();

  if (SWPipeliner::TII->isNE(imm)) {
    /// (2) ラッチ言がbneccの場合
    /// (2-1)getCoefficient() : 制御変数の増減値を取得する
    if (!getCoefficient(addsub->getOperand(2), &(TMI.coefficient))) {
      return false;
    }

    /// (2-2)比較と制御変数の増減を同一命令で行っているため、,0との比較になる(constant=0)
    /// \note 対象ループ判定で、SUBWSXriで減算＋比較の場合のみ対象としている
    TMI.minConstant = 0;

  } else if (SWPipeliner::TII->isGE(imm)) {
    /// (3) ラッチ言がbgeccの場合
    /// (3-1)getCoefficient() : 制御変数の増減値を取得する
    if (!getCoefficient(addsub->getOperand(2), &(TMI.coefficient))) {
      return false;
    }

    /// (3-2)比較と制御変数の増減を同一命令で行っているため、0との比較になる(constant=0-coefficient)
    /// \note 対象ループ判定で、SUBWSXriで減算＋比較の場合のみ対象としている
    TMI.minConstant = 0 - TMI.coefficient;
    /// (3-3)mi=比較言(LLVMではaddsubと同一命令)のop1
  }
  TMI.originalDoVReg = addsub->getOperand(1).getReg();

  TMI.updateDoVRegMI =addsub;
  TMI.branchDoVRegMI =branch;
  if (getDoInitialValue(TMI))
    TMI.originalKernelIteration=(TMI.doVRegInitialValue - TMI.minConstant)/ TMI.coefficient;

  return true;
}


/// LOOPをSWPL用に変形する
/// 1. original loop
/// \dot
/// digraph G1 {
///
///     graph [rankdir="TB"];
///     node [shape=box] op ob oe;
///     op [label="org preheader"];
///     ob [label="org body"];
///     oe [label="org exit "];
///     op->ob->oe;
///     ob->ob [tailport = s, headport = n];
/// }
/// \enddot
/// 2. 変形後loop
/// \dot
/// digraph G2 {
///
/// graph [
///     rankdir="LR"
///     ];
/// node [shape=box] op c1 ob c2 np nb ne oe;
/// op [label="op:org preheader"];
/// c1 [label="c1:check1(回転数が足りているか)"];
/// ob [label="ob:org body(SWPL対象)"];
/// c2 [label="c2:check2(余りLOOP必要か)"];
/// np [label="np:new preheader(誘導変数用のPHI生成)"];
/// nb [label="nb:new body(ORG Body)"];
/// ne [label="ne:new exit(出口BUSY変数のPHI生成)"];
/// oe [label="oe:org exit "];
/// {rank = same; op; c1; ob; c2; np; nb; ne; oe;}
/// op->c1
/// c1->ob [label="Y"]
/// c1->np [taillabel="N", tailport = w, headport = w]
/// ob->c2
/// c2->np [label="Y"]
/// c2->ne [taillabel="N", tailport = e, headport = e]
/// np->nb->ne->oe;
/// ob->ob [tailport = s, headport = n];
/// nb->nb [tailport = s, headport = n];
/// }
/// \enddot
void SwplScr::prepareCompensationLoop(SwplTransformedMIRInfo &tmi) {

  MachineBasicBlock* ob=ML.getTopBlock();
  const BasicBlock*ob_bb=ob->getBasicBlock();
  MachineBasicBlock* op=nullptr;
  MachineBasicBlock* oe=nullptr;
  MachineFunction *MF=ob->getParent();
  const auto &dbgloc=ML.getTopBlock()->begin()->getDebugLoc();

  for (auto *mbb:ob->predecessors()) {
    if (mbb!=ob) {
      op=mbb;
      break;
    }
  }
  assert(op!=nullptr);

  for (auto *mbb:ob->successors()) {
    if (mbb!=ob) {
      oe=mbb;
      break;
    }
  }
  assert(oe!=nullptr);

  tmi.OrgPreHeader=op;
  tmi.OrgBody=ob;
  tmi.OrgExit=oe;

  // MBBを生成する
  MachineBasicBlock* prolog=MF->CreateMachineBasicBlock(ob_bb);
  MachineBasicBlock* epilog=MF->CreateMachineBasicBlock(ob_bb);
  MachineBasicBlock* c1=MF->CreateMachineBasicBlock(ob_bb);
  MachineBasicBlock* c2=MF->CreateMachineBasicBlock(ob_bb);
  MachineBasicBlock* np=MF->CreateMachineBasicBlock(ob_bb);
  MachineBasicBlock* nb=MF->CreateMachineBasicBlock(ob_bb);
  MachineBasicBlock* ne=MF->CreateMachineBasicBlock(ob_bb);

  // neとoeは直列に並んでいない場合があるため、Branch命令で明にSuccessorを指定する
  const auto &debugLoc=ob->getFirstTerminator()->getDebugLoc();
  SWPipeliner::TII->insertUnconditionalBranch(*ne, oe, debugLoc);

  tmi.Prolog=prolog;
  tmi.Epilog=epilog;
  tmi.Check1=c1;
  tmi.Check2=c2;
  tmi.NewPreHeader=np;
  tmi.NewBody=nb;
  tmi.NewExit=ne;

  // MBBを並べる(オブジェクトの並びを整理しているのみで、実行経路は未設定)、op~oeは直列に並んでいない場合がある。
  // [op-]ob[-oe]
  MF->insert(ob->getIterator(), c1);
  MF->insert(ob->getIterator(), prolog);
  auto ip=oe->getIterator();
  if (oe!=ob->getNextNode()) {
    if (ob->getNextNode()==nullptr)
      ip=MF->end();
    else
      ip=ob->getNextNode()->getIterator();
  }
  // [op-]c1-prolog-ob[-oe]
  MF->insert(ip, epilog);
  // [op-]c1-prolog-ob-epilog[-oe]
  MF->insert(ip, c2);
  // [op-]c1-prolog-ob-epilog-c2[-oe]
  MF->insert(ip, np);
  // [op-]c1-prolog-ob-epilog-c2-np[-oe]
  MF->insert(ip, nb);
  // [op-]c1-prolog-ob-epilog-c2-np-nb[-oe]
  MF->insert(ip, ne);
  // [op-]c1-prolog-ob-epilog-c2-np-nb-ne[-oe]

  // [op--]c1--prolog--ob--epilog--c2--np--nb--ne[--oe]
  //                  ^ |
  //                  +-+

  // original body内の命令をnew bodyに移動する
  // phiの飛び込み元、branch先は元のママ
  moveBody(nb, ob);

  // nbのSuccessorをobと同じにする（命令と同じにする）。ReplaceUsesOfBlockWith()を利用するため。
  nb->addSuccessor(ob);
  nb->addSuccessor(oe);

  // new bodyのSuccessorおよびbranch先をまとめて変更する
  nb->ReplaceUsesOfBlockWith(ob, nb);
  nb->ReplaceUsesOfBlockWith(oe, ne);

  // PHI飛び込み元を変更する
  np->addSuccessor(nb);
  nb->replacePhiUsesWith(op, np);
  nb->replacePhiUsesWith(ob, nb);
  // op--c1--prolog--ob--epilog--c2--np-->nb--ne--oe
  //                ^ |                  ^ |
  //               +-+                   +-+

  // 残りのSuccessorおよびbranch先を変更する
  op->ReplaceUsesOfBlockWith(ob, c1);
  c1->addSuccessor(prolog);
  prolog->addSuccessor(ob);
  ob->replaceSuccessor(oe, epilog);
  epilog->addSuccessor(c2);
  c2->addSuccessor(np);
  ne->addSuccessor(oe);
  oe->replacePhiUsesWith(ob, ne);
  // op-->c1-->prolog-->ob-->epilog-->c2-->np-->nb-->ne-->oe
  //                   ^ |                     ^ |
  //                   +-+                     +-+

  makeBypass(tmi, dbgloc, *c1, *np,*c2, *ne);
  //                                  +-------------+
  //                                  |             v
  // op-->c1-->prolog-->ob-->epilog-->c2-->np-->nb-->ne-->oe
  //      |            ^ |                 ^   ^ |
  //      |            +-+                 |   +-+
  //      +-------------------------------+
}

void SwplScr::postSSA(SwplTransformedMIRInfo &tmi) {
  if (!tmi.isNecessaryBypassKernel()) {
    removeMBB(tmi.Check1, tmi.OrgPreHeader, tmi.Prolog);
    tmi.Check1=nullptr;
  }
  if (!tmi.isNecessaryBypassMod()) {
    // 余りループ実行判断は不要：余りループは必ず実行する。または必ず実行しない。
    // 余りループを実行しない（削除）場合、合流点のPHIを書き換える必要がある
    removeMBB(tmi.Check2, tmi.Epilog, tmi.NewPreHeader, !tmi.isNecessaryMod());
    tmi.Check2=nullptr;
  }
  if (tmi.isNecessaryMod()) {
    /// 余りループ通過回数が1の場合は分岐が不要
    if (!tmi.isNecessaryModIterationBranch()) {
      removeIterationBranch(tmi.branchDoVRegMI, tmi.NewBody);
    }
  } else {
      removeMBB(tmi.NewBody, tmi.NewPreHeader, tmi.NewExit);
      tmi.NewBody=nullptr;
  }
  if (!tmi.isNecessaryKernelIterationBranch()) {
    removeIterationBranch(tmi.branchDoVRegMIKernel, tmi.OrgBody);
  }
}

void SwplScr::removeMBB(llvm::MachineBasicBlock *target, llvm::MachineBasicBlock*from, llvm::MachineBasicBlock *to, bool unnecessaryMod) {
  MachineBasicBlock *rmSucc=nullptr;
  for (auto *p:target->successors()) {
    if (p!=target && p!=to) {
      rmSucc=p;
      break;
    }
  }
  if (unnecessaryMod) {
    assert(rmSucc!=nullptr);
    for (auto &phi:rmSucc->phis()) {
      // 出口BUSYレジスタの書き換え
      if (phi.getOperand(2).getMBB()==target) {
        auto *t=&phi.getOperand(3);
        t->setReg(phi.getOperand(1).getReg());
      } else {
        auto *t=&phi.getOperand(1);
        t->setReg(phi.getOperand(3).getReg());
      }
    }
  }
  from->ReplaceUsesOfBlockWith(target, to);
  to->replacePhiUsesWith(target, from);
  target->removeSuccessor(to);
  if (rmSucc!=nullptr) {
    target->removeSuccessor(rmSucc);
    removePredFromPhi(rmSucc, target);
  }
  target->eraseFromParent();
}

void SwplScr::removeIterationBranch(MachineInstr *br, MachineBasicBlock *body) {
  /// latch命令を削除する
  br->eraseFromParent();
  body->removeSuccessor(body);
  removePredFromPhi(body, body);
}

void SwplScr::removePredFromPhi(MachineBasicBlock *fromMBB,
                            MachineBasicBlock *removeMBB) {
  std::vector<MachineInstr*> phis;
  for (auto &phi:fromMBB->phis()) {
    phis.push_back(&phi);
  }
  for (auto *phi:phis) {
    auto mi=BuildMI(*fromMBB, phi, phi->getDebugLoc(), phi->getDesc(), phi->getOperand(0).getReg());
    Register r;
    for (auto &op:phi->operands()) {
      if (op.isReg())
        r=op.getReg();
      else {
        auto *mbb=op.getMBB();
        if (mbb==removeMBB) continue;
        mi.addReg(r).addMBB(mbb);
      }
    }
  }
  for (auto *phi:phis) {
    phi->eraseFromParent();
  }
}

#define PRINT_MBB(name) \
  llvm::dbgs() << #name ": "; \
  if (name==nullptr)    \
    llvm::dbgs() << "nullptr\n"; \
  else                  \
    llvm::dbgs() << llvm::printMBBReference(*name) << "\n";

void SwplTransformedMIRInfo::print() {
  llvm::dbgs() << "originalDoVReg:" << printReg(originalDoVReg, SWPipeliner::TRI) << "\n"
               << "originalDoInitVar:" << printReg(originalDoInitVar, SWPipeliner::TRI) << "\n"
               << "doVReg:" << printReg(doVReg, SWPipeliner::TRI) << "\n"
               << "iterationInterval:" << iterationInterval << "\n"
               << "minimumIterationInterval:" << minimumIterationInterval << "\n"
               << "coefficient: " << coefficient << "\n"
               << "minConstant: " << minConstant << "\n"
               << "expansion: " << expansion << "\n"
               << "nVersions: " << nVersions << "\n"
               << "nCopies: " << nCopies << "\n"
               << "requiredKernelIteration: " << requiredKernelIteration << "\n"
               << "prologEndIndx: " << prologEndIndx << "\n"
               << "kernelEndIndx: " << kernelEndIndx << "\n"
               << "epilogEndIndx: " << epilogEndIndx << "\n"
               << "isIterationCountConstant: " << isIterationCountConstant << "\n"
               << "doVRegInitialValue: " << doVRegInitialValue << "\n"
               << "originalKernelIteration: " << originalKernelIteration << "\n"
               << "transformedKernelIteration: " << transformedKernelIteration << "\n"
               << "transformedModIteration: " << transformedModIteration << "\n";
  if (updateDoVRegMI !=nullptr)
    llvm::dbgs() << "updateDoVRegMI:" << *updateDoVRegMI;
  else
    llvm::dbgs() << "updateDoVRegMI: nullptr\n";
  if (branchDoVRegMI !=nullptr)
    llvm::dbgs() << "branchDoVRegMI:" << *branchDoVRegMI;
  else
    llvm::dbgs() << "branchDoVRegMI: nullptr\n";
  if (branchDoVRegMIKernel !=nullptr)
    llvm::dbgs() << "branchDoVRegMIKernel:" << *branchDoVRegMIKernel;
  else
    llvm::dbgs() << "branchDoVRegMIKernel: nullptr\n";
  PRINT_MBB(OrgPreHeader);
  PRINT_MBB(Check1);
  PRINT_MBB(Prolog);
  PRINT_MBB(OrgBody);
  PRINT_MBB(Epilog);
  PRINT_MBB(NewPreHeader);
  PRINT_MBB(Check2);
  PRINT_MBB(NewBody);
  PRINT_MBB(NewExit);
  PRINT_MBB(OrgExit);
  llvm::dbgs() << "mis:\n";
  int i=0;
  for (const auto *mi: mis) {
    llvm::dbgs() << i++ << *mi;
  }
}

void SwplScr::collectLiveOut(UseMap &usemap) {
  auto*mbb=ML.getTopBlock();
  for (auto& mi: *(mbb)) {
    for (auto& op: mi.operands()){
      if (!op.isReg() || !op.isDef()) continue;
      auto r=op.getReg();
      if (r.isPhysical()) continue;
      for (auto &u:SWPipeliner::MRI->use_operands(r)) {
        auto *umi=u.getParent();
        if (umi->getParent()==mbb) continue;
        usemap[r].push_back(&u);
      }
    }
  }
}
