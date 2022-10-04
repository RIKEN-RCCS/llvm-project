//=- AArch64LoopDataExtraction.cpp - SWPL LOOP -*- c++ -*--------------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 Loop Data Extraction (SwplLoop)
//
//===----------------------------------------------------------------------===//
//=== Copyright FUJITSU LIMITED 2021  and FUJITSU LABORATORIES LTD. 2021   ===//
//===----------------------------------------------------------------------===//

#include "AArch64.h"

#include "llvm/CodeGen/MachineLoopInfo.h"
#include "AArch64SWPipeliner.h"
#include <iostream>
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "AArch64TargetTransformInfo.h"

using namespace llvm;
using namespace swpl;
#define DEBUG_TYPE "fj-swp-loop"

#define UNKNOWN_MEM_DIFF INT_MIN        /* 0 の正反対 */

static cl::opt<bool> DebugLoop("swpl-debug-loop",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> NoUseAA("swpl-no-use-aa",cl::init(false), cl::ReallyHidden);

using BasicBlocks = std::vector<MachineBasicBlock *>;
using BasicBlocksIterator = std::vector<MachineBasicBlock *>::iterator ;
static MachineBasicBlock *getPredecessorBB(MachineBasicBlock &BB);
static void follow_single_predecessor_MBBs(MachineLoop *L, BasicBlocks *BBs);
static void construct_use(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO, SwplInsts &insts);
static void construct_mem_use(Register2SwplRegMap &rmap, SwplInst &inst, const MachineMemOperand *MMO, SwplInsts &insts,
                              SwplMems *mems, SwplMems *memsOtherBody);
static void construct_def(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO);
static bool isNonTarget(MachineInstr &inst);

void SwplReg::inheritReg(SwplReg *former_reg, SwplReg *latter_reg) {
  assert (former_reg->Successor == NULL);
  assert (latter_reg->Predecessor == NULL);

  former_reg->Successor = latter_reg;
  latter_reg->Predecessor = former_reg;

  return;
}

void SwplReg::getDefPort( SwplInst **p_def_inst, int *p_def_index) const {
  *p_def_inst = DefInst;
  *p_def_index = (int)DefIndex;
}

void SwplReg::getDefPort( SwplInst **p_def_inst, int *p_def_index) {
  *p_def_inst = DefInst;
  *p_def_index = (int)DefIndex;
}

void SwplReg::getDefPortInLoopBody( SwplInst **p_def_inst, int *p_def_index) {
  getDefPort(p_def_inst, p_def_index);
  while(*p_def_inst != nullptr && (*p_def_inst)->isPhi()) {
    const SwplReg Reg = (*p_def_inst)->getPhiUseRegFromIn();
    SwplInst *inst = *p_def_inst;
    Reg.getDefPort(p_def_inst, p_def_index);
    if (*p_def_inst == inst) {
      // Body内にdef_instが存在しない 
      *p_def_inst = nullptr;
      return;
    }
  }
  if (!((*p_def_inst)->isInLoop())) {
    *p_def_inst = nullptr;
  }
}

/// \note レジスタのサイズではないことに注意
int SwplReg::getRegSize() const {
  /// llvmではtuple型の内部表現は存在せず、レジスタそれぞれが独立している。
  return 1;
}

bool SwplInst::isDefinePredicate() const {
  for (auto *reg:getDefRegs()) {
    TmRegKind rk=TM.getRegKind(reg->getReg());
    if (rk.isPredicate()) return true;
  }
  return false;
}

bool SwplInst::isFloatingPoint() const {
  for (auto *reg:getDefRegs()) {
    TmRegKind rk=TM.getRegKind(reg->getReg());
    if (rk.isFloating()) return true;
  }
  return false;
}

const SwplReg &SwplInst::getPhiUseRegFromIn() const {
  assert(isPhi());
  assert(getSizeUseRegs() == 2);
  // getUseRegs(0)が外ブロックからの場合
  // getUseRegs(1)がループブロックからの場合
  return getUseRegs(1);
}

SwplLoop *SwplLoop::Initialize(MachineLoop &L, const UseMap& LiveOutReg) {
  SwplLoop *loop = new SwplLoop(L);
  Register2SwplRegMap rmap;

  /// convertSSAtoNonSSA() を呼び出し、対象ループの非SSA化を行う。
  loop->convertSSAtoNonSSA(L, LiveOutReg);

  /// makePreviousInsts() を呼び出し、ループボディで使われているレジスタを定義しているループ外の命令の一覧
  /// SwplLoop::PreviousInsts を作成する。
  loop->makePreviousInsts(rmap);
  /// makePhiInsts() を呼び出し、Phi命令の情報 SwplLoop::PhiInsts を作成する。
  loop->makePhiInsts(rmap);
  /// makeBodyInsts() を呼び出し、ループボディ内の命令一覧 SwplLoop::BodyInsts を作成する。
  loop->makeBodyInsts(rmap);
  /// reconstructPhi() を呼び出し、次のイテレーションで使われるレジスタの情報をPhi命令のuse情報として追加する。
  loop->reconstructPhi(rmap);
  
  /// makeMemIncrementMap() を呼び出し、メモリとアドレス増分値のマップ SwplLoop::MemIncrementMap を作成する。
  loop->makeMemIncrementMap();

  /// collectRegs() を呼び出し、メモリ開放するときのためのRegの情報 SwplReg を収集する。
  loop->collectRegs();
  
  if (DebugLoop) {
    loop->print();
  }
  return loop;
}

/// ループボディで使われているレジスタを定義しているループ外の命令の一覧 SwplLoop::PreviousInsts を作成する。
void SwplLoop::makePreviousInsts(Register2SwplRegMap &rmap) {
  BasicBlocks BBs;

  follow_single_predecessor_MBBs(getML(), &BBs);

  for (auto *BB:BBs) {
    for (auto &MI:BB->instrs()) {
      if (MI.isDebugInstr()) { continue; }
      SwplInst *inst = new SwplInst(&MI, (SwplLoop *)nullptr);
      inst->InitializeWithDefUse(&MI, (SwplLoop *)nullptr, rmap, getPreviousInsts(), (SwplMems *)nullptr, &(getMemsOtherBody()));
      addPreviousInsts(inst);
    }
  }

  // 非SSA化で生成したCopy命令の情報を収集する
  MachineBasicBlock *bodyMBB = getNewBodyMBB();
  MachineBasicBlock *preMBB = getNewBodyPreMBB(bodyMBB);
  for (auto &MI:preMBB->instrs()) {
    if (MI.isDebugInstr()) { continue; }
    SwplInst *inst = new SwplInst(&MI, (SwplLoop *)nullptr);
    inst->InitializeWithDefUse(&MI, (SwplLoop *)nullptr, rmap, getPreviousInsts(), (SwplMems *)nullptr, &(getMemsOtherBody()));
    addPreviousInsts(inst);
  }
}

/// ループ内で定義があるレジスタに対して SwplLoop::PhiInsts を生成する。
void SwplLoop::makePhiInsts(Register2SwplRegMap &rmap) {
  /// 対象は必ず NewBodyMBB 内にあるCopy命令。
  /// Copies に収集してあるので、その命令を対象にする。
  for (auto &MI:getCopies()) {
    assert(MI->getOpcode() == TargetOpcode::COPY);
    const MachineOperand &MO = MI->getOperand(0);
    assert(MO.isReg() && MO.isDef());
    llvm::Register Reg = MO.getReg();
    assert(Reg.isVirtual());

    SwplReg *swpl_reg;
    auto itr = rmap.find(Reg);
    if (itr == rmap.end()) {
      SwplInst *d_inst = new SwplInst(nullptr, nullptr);
      swpl_reg = new SwplReg(Reg, *(d_inst), 0, false);
      d_inst->addDefRegs(swpl_reg);
      addPreviousInsts(d_inst);
    } else {
      swpl_reg = itr->second;
    }
    SwplInst *phi = new SwplInst(nullptr, this);
    swpl_reg->addUseInsts(phi);
    phi->addUseRegs(swpl_reg);

    SwplReg *swpl_defreg = new SwplReg(Reg, *phi, 0, false);
    phi->addDefRegs(swpl_defreg);
    rmap[Reg] = swpl_defreg;
    addPhiInsts(phi);
  }
}

/// ループボディ内の命令一覧 SwplLoop::BodyInsts を作成する。
void SwplLoop::makeBodyInsts(Register2SwplRegMap &rmap) {
  MachineInstr*branch=nullptr;
  MachineInstr*cmp=nullptr;
  MachineInstr*addsub=nullptr;
  (void)LindaScr(*(getML())).findGensForLoop(&branch, &cmp, &addsub);
  for (auto &MI:getNewBodyMBB()->instrs()) {
    if (MI.isDebugInstr()) { continue; }
    if (&MI == branch) { continue; }
    if (&MI == cmp && &MI != addsub) { continue; }
    if (MI.isBranch()) { continue; }
    SwplInst *inst = new SwplInst(&MI, &*this);
    inst->InitializeWithDefUse(&MI, &*this, rmap, getPreviousInsts(), &(getMems()), &(getMemsOtherBody()));
    addBodyInsts(inst);
  }
}

/// 次のイテレーションで使われるレジスタの情報をPhi命令のuse情報として追加する。
void SwplLoop::reconstructPhi(Register2SwplRegMap &rmap) {
  for (auto *phi:getPhiInsts()) {
    for (auto *swpl_reg:phi->getDefRegs()) {
      llvm::Register Reg = swpl_reg->getReg();
      auto itr = rmap.find(Reg);
      SwplReg *tmp_swpl_reg = itr->second;
      tmp_swpl_reg->addUseInsts(phi);
      phi->addUseRegs(tmp_swpl_reg);
    }
  }
}

void SwplInst::InitializeWithDefUse(llvm::MachineInstr *MI, SwplLoop *loop, Register2SwplRegMap &rmap,
                                    SwplInsts &insts, SwplMems *mems, SwplMems *memsOtherBody) {
  if (MI == nullptr) {
    llvm_unreachable("InitializeWithDefUse: not MI");
  }
  assert(MI->getNumDefs()<10);
  llvm::MachineOperand *DefMO[10];
  int def_i = 0;
  int use_i = 0;
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    if (!MI->getOperand(i).isReg()) { continue; }
    if (MI->getOperand(i).isDef()) {
      DefOpMap[def_i]=i+1;
      // Defの設定
      DefMO[def_i] = &(MI->getOperand(i));
      def_i++;
    } else {
      UseOpMap[use_i]=i+1;
      use_i++;
      construct_use(rmap, *this, MI->getOperand(i), insts);
    }
  }

  if (MI->mayLoad() && MI->getNumMemOperands()==0) {
    //
    construct_mem_use(rmap, *this, nullptr, insts, mems, memsOtherBody);
  }

  for (MachineMemOperand *MMO : MI->memoperands()) {
    construct_mem_use(rmap, *this, MMO, insts, mems, memsOtherBody);
  }
  for (int i=0; i<def_i; ++i) {
    construct_def(rmap, *this, *DefMO[i]);
  }
}

bool SwplInst::isRecurrence() const {
  // 複数回定義される場合はfalseを返す
  if (getSizeDefRegs() != 1) { return false; }

  SwplReg def_reg = getDefRegs(0);
  SwplInst *phi = nullptr; 
  for (auto *inst:def_reg.getUseInsts()) {
    if (inst->isPhi()) {
      phi = inst;
    }
  }

  if (phi != nullptr) {
    SwplReg phi_def_reg = phi->getDefRegs(0);
    for (auto *inst:phi_def_reg.getUseInsts()) {
      if (inst == this) {
        return true;
      }
    }
  }  
  return false;
}


size_t SwplLoop::getSizeBodyRealInsts() const {
  size_t n = 0;
  for (auto *inst:getBodyInsts()) {
    const llvm::MachineInstr *MI = inst->getMI();
    if (swpl::TM.isPseudo(*MI)) { continue; }
    n++;
  }
  return n;
}

/// メモリとアドレス増分値のマップ SwplLoop::MemIncrementMap を作成する。
void SwplLoop::makeMemIncrementMap() {
  for (auto *mem:getMems()) {
    int increment = mem->calcEachMemAddressIncrement();
    addMemIncrementMap(mem, increment);
  }
}

bool SwplLoop::areBodyInstsOrder(SwplInst *first, SwplInst *second) {
  int state=0;
  for ( auto*inst:getBodyInsts()) {
    if (state==0) {
      if (inst==first) state=1;
    } else {
      if (inst==second) return true;
    }
  }
  return false;
}

bool SwplLoop::areMemsOrder(SwplMem *first, SwplMem *second) {
  assert(first != second);
  int state=0;
  for ( auto*mem:getMems()) {
    if (state==0) {
      if (mem==first) state=1;
    } else {
      if (mem==second) return true;
    }
  }
  return false;
}

/// メモリアクセス間の差を計算する
/// \param [in] former_mi MachineInstr
/// \param [in] latter_mi MachineInstr
/// \return 差
static int mems_initial_diff(const MachineInstr* former_mi, const MachineInstr *latter_mi) {
  const MachineOperand *formerBaseOp;
  int64_t formerOffset;
  bool formerOffsetIsScalable;
  if (!TII->getMemOperandWithOffset(*former_mi, formerBaseOp, formerOffset, formerOffsetIsScalable, TRI))
    return UNKNOWN_MEM_DIFF;
  if (formerOffsetIsScalable)
    return UNKNOWN_MEM_DIFF;
  if (!formerBaseOp->isReg())
    return UNKNOWN_MEM_DIFF;

  const MachineOperand *latterBaseOp;
  int64_t latterOffset;
  bool latterOffsetIsScalable;
  if (!TII->getMemOperandWithOffset(*latter_mi, latterBaseOp, latterOffset, latterOffsetIsScalable, TRI))
    return UNKNOWN_MEM_DIFF;
  if (latterOffsetIsScalable)
    return UNKNOWN_MEM_DIFF;
  if (!latterBaseOp->isReg())
    return UNKNOWN_MEM_DIFF;

  if (formerBaseOp->getReg() != latterBaseOp->getReg())
    return UNKNOWN_MEM_DIFF;
  int64_t offset=formerOffset-latterOffset;
  int64_t absoffset=labs(offset);
  return (absoffset > INT_MAX)? UNKNOWN_MEM_DIFF:offset;
}

/// former_memとＮ回後のlatter_memが重なる可能性の否定できない最小のＮを返す。 \n
/// ただし、Ｎは、former_mem latter_mem の順に並んでいる時は０以上、逆の時は１以上とする。
unsigned SwplLoop::getMemsMinOverlapDistance(SwplMem *former_mem, SwplMem *latter_mem) {
  unsigned smallest_distance;
  int former_increment, latter_increment;

  const auto * former_mi=NewMI2OrgMI.at(former_mem->getInst()->getMI());
  const auto * latter_mi=NewMI2OrgMI.at(latter_mem->getInst()->getMI());

  if (!NoUseAA && !former_mi->mayAlias(AA, *latter_mi, false)) {
    const auto *memop1=former_mem->getMO();
    const auto *memop2=latter_mem->getMO();
    // MachineInstr::mayAlias の結果は信頼性が低いため、さらに、メモリ間で重なる可能性があるか確認する
    if (memop1!=nullptr && memop2!=nullptr && memop1->getValue()!=nullptr && memop2->getValue()!=nullptr) {
      if (AA->isNoAlias(
              MemoryLocation::getAfter(memop1->getValue(), memop1->getAAInfo()),
              MemoryLocation::getAfter(memop2->getValue(), memop2->getAAInfo()))) {
         if (DebugOutput) { dbgs() << " aliasAnalysis is NoAlias\n"; }
         return SwplMem::MAX_LOOP_DISTANCE;
      }
    }
  }

  // 最小の値を取得
  smallest_distance = (areMemsOrder(former_mem, latter_mem) ? 0 : 1);

  former_increment = getMemIncrement(former_mem);
  latter_increment = getMemIncrement(latter_mem);
  if (DebugOutput)
    dbgs() << "DBG(getMemIncrement): former_increment=" << former_increment << " , latter_increment=" << latter_increment << "\n";

  if (former_increment == UNKNOWN_MEM_DIFF
      || latter_increment == UNKNOWN_MEM_DIFF
      || former_increment != latter_increment) {
    return smallest_distance;
  }
  assert(former_increment == latter_increment);

  /* 最初にループに入った時のアドレスの差を求める。
   *  i = b;          ─┐この差のこと.
   *  j = b + 4;      ─┘
   * ┌→
   * │
   * │...  = a[i] op ...
   * │
   * │...  = a[j] op ...
   * │
   * │i = i + 1
   * │j = j + 1
   * └─
   */

  int distance;
  int initial_diff = mems_initial_diff (former_mi, latter_mi);
  if (DebugOutput)
    dbgs() << "DBG(mems_initial_diff): initial_diff=" << initial_diff
     << ", former_size=" << former_mem->getSize()
     << ", latter_size=" << latter_mem->getSize() << "\n";

  if (initial_diff == UNKNOWN_MEM_DIFF) {
    return smallest_distance;
  }

  /* アクセスするメモリサイズを取得し、
   */
  int former_size = former_mem->getSize();
  int latter_size = latter_mem->getSize();

  /* 両者のアドレス増分が0の場合は、初期アドレスの差とアクセスサイズを考慮して
   * 重ならなければ、何回転先の命令であっても重ならない */
  if (initial_diff != 0 && latter_increment == 0) {
    if (!((initial_diff + (int) former_size > 0) && (initial_diff - (int) latter_size < 0))) {
      return SwplMem::MAX_LOOP_DISTANCE;
    }
  }
  /* 初期アドレスの差を考慮して、何回転先に重なるかを計算する */
  for (distance = smallest_distance; distance < SwplMem::MAX_LOOP_DISTANCE; ++distance) {
    int diff;

    /* 重なるのは、
       (former_address + former_size > latter_address)
       && (former_address < latter_address + latter_size)

       former_address := former_initial_address
       latter_address := latter_initial_address + disntace * latter_increment
       initial_diff := former_initial_address - latter_initial_address
       diff := former_address - latter_address
     */

    diff = initial_diff - distance * latter_increment;
    if ((diff + (int) former_size > 0) && (diff - (int) latter_size < 0)) {
      return distance;
    }
  }
  return distance;
}

/// スケジューリング結果反映機能のMVE処理やKERNEL分離処理においてPhiを特別視したいため、
/// SwplDdg を作成後に SwplLoop::BodyInsts に含まれるPhi命令を SwplLoop::PhiInsts に収集し直し、
/// SwplLoop::BodyInsts からは削除する。\n
void SwplLoop::recollectPhiInsts() {
  auto S = bodyinsts_begin();
  auto E = bodyinsts_end();

  auto itr = S;
  while (itr != E) {
    SwplInst *inst = *itr;
    if (inst->isPhi()) {
      addPhiInsts(inst);
      itr = eraseBodyInsts(itr);
    } else {
      itr++;
    }
  }
}

void SwplLoop::convertNonSSA(llvm::MachineBasicBlock *body, llvm::MachineBasicBlock *pre, const DebugLoc &dbgloc,
                             llvm::MachineBasicBlock *org, const UseMap &LiveOutReg) {
  std::vector<MachineInstr*> phis;

  for (auto &phi:body->phis()) {
    phis.push_back(&phi);
  }
  /// (1). Phi命令を検索し、Phi命令の単位に以下の処理を行う。
  for (auto *phi:phis) {
    unsigned opix;
    unsigned num=phi->getNumOperands();
    llvm::Register def_r=0, Reg=0, own_r=0, in_r=0;

    ///   (1)-1. Phiの参照レジスタのうち、自身のMBBで定義されるレジスタ(own_r)とLiveInのレジスタ(in_r)をおさえておく。
    def_r = phi->getOperand(0).getReg();
    for (opix=1; opix<num; opix++) {
      Reg = phi->getOperand(opix).getReg();
      opix++;
      assert(phi->getOperand(opix).isMBB());
      auto *mbb=phi->getOperand(opix).getMBB();
      if (mbb==org) {
        assert(own_r == 0);
        own_r = Reg;
      } else {
        assert(in_r == 0);
        in_r = Reg;
      }
    }
    ///   (1)-2. Phiの定義を参照する命令を検索し2.の参照レジスタに置きかえる。 \n
    ///      defregはliveoutしているかを確認する。liveoutしていれば、PHIから生成されるCOPYの定義レジスタは新規を利用。
    const auto *org_phi=NewMI2OrgMI[phi];
    auto org_def_r=org_phi->getOperand(0).getReg();
    if (LiveOutReg.count(org_def_r)!=0) {
      auto backup_def_r=def_r;
      def_r=MRI->cloneVirtualRegister(def_r);
      MachineInstr *Copy = BuildMI(*body, body->getFirstNonPHI(), dbgloc,TII->get(AArch64::COPY), backup_def_r)
              .addReg(def_r);
      NewMI2OrgMI[Copy]=org_phi;
    }


    ///      PHIを元にCOPY命令を生成する際、COPYの定義レジスタクラスはPHIの定義レジスタクラスであるべき。

    ///      phiを２つのCOPY命令に変換し、predecessorとbodyの下方に挿入する。
    ///````
    ///   body :
    ///          phi_def = PHI own_r, in_r
    ///          inst
    ///          inst ...
    ///   ↓
    ///   predecessor of body :
    ///          inst
    ///          inst ...
    ///      --> phi_def = COPY own_r
    ///   body :
    ///          inst
    ///          inst ...
    ///      --> phi_def = COPY in_r
    ///````

    ///   (1)-3. preにin_rからown_rへのCopy命令を挿入する(def_r = Copy in_r)。
    MachineInstr *Copy = BuildMI(*pre, pre->getFirstTerminator(), dbgloc,TII->get(AArch64::COPY), def_r)
                                .addReg(in_r);
    addCopies(Copy);

    ///   (1)-4. preにin_rからown_rへのCopy命令を挿入する(def_r = Copy own_r)。
    MachineInstr *c=BuildMI(*body, body->getFirstTerminator(), dbgloc,TII->get(AArch64::COPY), def_r)
            .addReg(own_r);
    NewMI2OrgMI[c]=org_phi;

    ///          OrgMI2NewMIがorgのphiとnewのphiとなっているので、new側をCopy命令に変更する。
    for (auto itr: getOrgMI2NewMI()) {
      if (itr.second == phi) {
        addOrgMI2NewMI(itr.first, const_cast<llvm::MachineInstr *>(Copy));
      }
    }  
  }

  /// (2). Phi命令を削除する。
  for (auto *phi:phis) {
    phi->eraseFromParent();
  }
}

/// orgの定義レジスタを収集しレジスタを複写する。
/// \note この時点ではまだrenamingせずにorg2newへの登録のみをおこなう。
/// \param[in]  &orgMI コピー元のMI
/// \param[inout]  org2new 定義するレジスタをKeyに複製したレジスタをValueに持つMap.
static void collectDefReg(llvm::MachineInstr &orgMI, RegMap &org2new) {
  for (unsigned i = 0, e = orgMI.getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = orgMI.getOperand(i);
    if (!(MO.isReg())) { continue; }
    llvm::Register orgReg = MO.getReg();
    if (!(orgReg.isVirtual())) { continue; }
    if (MO.isDef()) {
      llvm::Register newReg = MRI->cloneVirtualRegister(orgReg);
      // 多重定義はないはず。
      assert(org2new.find(orgReg) == org2new.end());
      org2new[orgReg] = newReg;
    }
  }
}

void SwplLoop::renameReg() {
  for (auto m_itr: getOrgMI2NewMI()) {
    const MachineInstr *orgMI = m_itr.first;
    MachineInstr *newMI = m_itr.second;
    for (unsigned i = 0, e = orgMI->getNumOperands(); i != e; ++i) {
      const MachineOperand &MO = orgMI->getOperand(i);
      if (!(MO.isReg())) { continue; }
      llvm::Register orgReg = MO.getReg();
      auto r_itr = getOrgReg2NewReg().find(orgReg);
      if (r_itr != getOrgReg2NewReg().end()) {
        newMI->getOperand(i).setReg(r_itr->second);
      }
    }
  }
}

void SwplLoop::cloneBody(llvm::MachineBasicBlock*newBody, llvm::MachineBasicBlock*oldBody) {
  auto S=oldBody->begin();
  auto E=oldBody->end();

  for (auto J=S; J!=E;) {
    MachineInstr *org=&*J++;
    const MachineInstr *mi = const_cast<MachineInstr *>(org);
    // orgMIをベースにnewMIをduplicate
    MachineInstr &newMI = TII->duplicate(*newBody, newBody->getFirstTerminator(), *mi);
    // レジスタリネーミング向けに定義レジスタを複製し、orgとnewのレジスタをmapにため込む
    collectDefReg(*org, getOrgReg2NewReg());
    addOrgMI2NewMI(mi, &newMI);
  }
  // レジスタを変換する
  renameReg();
}

void SwplLoop::convertSSAtoNonSSA(MachineLoop &L, const UseMap &LiveOutReg) {
  MachineBasicBlock *ob=L.getTopBlock();
  const BasicBlock *ob_bb=ob->getBasicBlock();
  MachineFunction *MF=ob->getParent();
  const auto &dbgloc=L.getTopBlock()->begin()->getDebugLoc();
  
  /// MachineBasicBlockの追加
  MachineBasicBlock *new_bb=MF->CreateMachineBasicBlock(ob_bb);
  setNewBodyMBB(new_bb);
  /// MBBをMFの連鎖の最後に挿入する。制御フローは未設定。
  auto itr = MF->end();
  MF->insert(itr, new_bb);

  /// 非SSA用の直前のMachineBasicBlockを用意。
  MachineBasicBlock *pre=MF->CreateMachineBasicBlock(ob_bb);
  /// new_bbの直前に挿入する。制御フローは未設定。
  MF->insert(new_bb->getIterator(), pre);

  pre->addSuccessor(new_bb);
  
  /// cloneBody() を呼び出し、MachineBasickBlockの複製とレジスタリネーミングを行う。
  cloneBody(new_bb, ob);
  /// convertNonSSA() を呼び出し、非SSA化と複製したMachineBasicBlockの回収を行う。
  convertNonSSA(new_bb, pre, dbgloc, ob, LiveOutReg);
}

void SwplInst::pushAllRegs(SwplLoop *loop) {
  for (auto *reg:getUseRegs()) {
    loop->addRegs(reg);
  }
  for (auto *reg:getDefRegs()) {
    loop->addRegs(reg);
  }
}

/// 全ての SwplInst を走査し SwplInst::DefRegs / SwplInst::UseRegs を走査し SwplLoop::Regs に登録する。
/// \note SwplLoop 生成時rmapを使用しているが、すべての SwplReg が登録されているわけではない。\n
///       そのため最後に SwplInst が使用する SwplReg を収集する。
void SwplLoop::collectRegs() {
  for (auto *inst:getPreviousInsts()) {
    inst->pushAllRegs(this);
  }
  for (auto *inst:getPhiInsts()) {
    inst->pushAllRegs(this);
  }
  for (auto *inst:getBodyInsts()) {
    inst->pushAllRegs(this);
  }
  // 同一要素の削除
  SwplRegs &regs = getRegs();
  std::sort(regs.begin(), regs.end());
  regs.erase(std::unique(regs.begin(), regs.end()), regs.end());
}

void SwplLoop::deleteNewBodyMBB() {
  MachineBasicBlock *newMBB = getNewBodyMBB();
  if (newMBB == nullptr) { return; }
  MachineBasicBlock *preMBB = getNewBodyPreMBB(newMBB);

  preMBB->removeSuccessor(newMBB);
  preMBB->eraseFromParent();
  newMBB->eraseFromParent();

  setNewBodyMBB(nullptr);
}

int SwplReg::calcEachRegIncrement() {
  SwplInst *def_inst = nullptr;
  SwplInst *induction_inst = nullptr;
  int index_dummy = -1;
  const auto *target=this;

  getDefPort(&def_inst, &index_dummy);
  if (!def_inst->isInLoop()) {
    return 0;
  }
  if (def_inst->isADDXrr()) {
    const auto &r1=def_inst->getUseRegs(0);
    const auto &r2=def_inst->getUseRegs(1);
    if (r1.DefInst->isPhi()) {
      if (r2.DefInst->isInLoop())
        return UNKNOWN_MEM_DIFF;
      def_inst=r1.DefInst;
      target=&r1;
    } else if (r2.DefInst->isPhi()) {
      if (r1.DefInst->isInLoop())
        return UNKNOWN_MEM_DIFF;
      def_inst=r2.DefInst;
      target=&r2;
    } else {
      return UNKNOWN_MEM_DIFF;
    }
  }

  if (def_inst->isPhi()) {
    const SwplReg& induction_reg = def_inst->getPhiUseRegFromIn();
    induction_reg.getDefPort(&induction_inst, &index_dummy);
    while (induction_inst->isCopy()) {
      const auto &def_reg=induction_inst->getUseRegs(0);
      induction_inst=def_reg.DefInst;
    }

    if (induction_inst->getSizeUseRegs() == 1) {
      int sign = 0;
      const auto *mi=induction_inst->getMI();
      auto op=mi->getOpcode();
      if (op == AArch64::ADDSXri || op == AArch64::ADDXri)
        sign = 1;
      else if (op == AArch64::SUBSXri || op == AArch64::SUBXri)
        sign = -1;

      if (sign == 0)
        return UNKNOWN_MEM_DIFF;

      if (&(induction_inst->getUseRegs(0)) == target) {
        const auto& mo=mi->getOperand(2);
        if (!mo.isImm()) return UNKNOWN_MEM_DIFF;
        return mo.getImm();
      }
    } else {
      return UNKNOWN_MEM_DIFF;
    }
  } else {
    if (def_inst->isLoad())
      return UNKNOWN_MEM_DIFF;

    while (def_inst->getSizeUseRegs() == 1) {
      def_inst->getUseRegs(0).getDefPort(&def_inst, &index_dummy);
      if (!def_inst->isInLoop()) {
        return 0;
      }
      if (def_inst->isPhi()) {
        return UNKNOWN_MEM_DIFF;
      }
    }
    return UNKNOWN_MEM_DIFF;
  }
  return UNKNOWN_MEM_DIFF;
}

int SwplMem::calcEachMemAddressIncrement() {
  int sum = 0;
  int old_sum = 0;
  if (DebugOutput) {
    if (Inst->getMI()==nullptr)
      dbgs() << "DBG(calcEachMemAddressIncrement): \n";
    else
      dbgs() << "DBG(calcEachMemAddressIncrement): " << *(Inst->getMI());
  }
  for (auto *reg:getUseRegs()) {
    int increment = reg->calcEachRegIncrement();
    if (DebugOutput) dbgs() << " calcEachRegIncrement(" << printReg(reg->getReg(), TRI)  << "): " << increment << "\n";
    if (increment == UNKNOWN_MEM_DIFF) {
      return increment;
    }
    old_sum = sum;
    sum += increment;

    // 計算結果の溢れチェック
    if (increment > 0) {
      if (sum < old_sum) {
        return UNKNOWN_MEM_DIFF;
      }
    } else if (increment < 0) {
      if (sum > old_sum) {
        return UNKNOWN_MEM_DIFF;
      }
    }
  }
  return sum;
}

void SwplLoop::print() {
  dbgs() << "DBG(SwplLoop::print) SwplLoop Print. \n";
  dbgs() << "DBG(SwplLoop::print) ------------------ NewPreBodyMBB. \n";
  dbgs() << *getNewBodyPreMBB(NewBodyMBB);
  dbgs() << "DBG(SwplLoop::print) ------------------ NewBodyMBB. \n";
  dbgs() << *NewBodyMBB;
  dbgs() << "\n";

  dbgs() << "DBG(SwplLoop::print) ------------------ PreviousInsts. \n";
  for ( auto *inst:getPreviousInsts()) {
    inst->print();
    dbgs() << "\n";
  }
  dbgs() << "DBG(SwplLoop::print) ------------------ SwplLoop PhiInsts. \n";
  for ( auto *inst:getPhiInsts()) {
    inst->print();
    dbgs() << "\n";
  }
  dbgs() << "DBG(SwplLoop::print) ------------------ SwplLoop BodyInsts. \n";
  for ( auto *inst:getBodyInsts()) {
    inst->print();
    dbgs() << "\n";
  }
}

void SwplInst::print() {
  dbgs() << "DBG(SwplInst::print) SwplInst. \n";
  if (getMI() != nullptr) {
    dbgs() << "  ### MI: " << *getMI();
  } else {
    dbgs() << "  ### MI: MI == null\n";
  }

  dbgs() << "  ### dump UseRegs \n";
  for ( auto *reg:getUseRegs()) {
    dbgs() << "    ";
    reg->print();
  }  
  dbgs() << "  ### dump DefRegs \n";
  for ( auto *reg:getDefRegs()) {
    dbgs() << "    ";
    reg->print();
  }  
}

void SwplLoop::dumpOrgMI2NewMI() {
  for (auto itr: getOrgMI2NewMI()) {
    dbgs() << "OldMI=" << *itr.first << "\n";
    dbgs() << "NewMI=" << *itr.second << "\n";
  }
}

void SwplLoop::dumpOrgReg2NewReg() {
  for (auto itr: getOrgReg2NewReg()) {
    dbgs() << "OldReg=" << printReg(itr.first, TRI) << "NewReg=" << printReg(itr.second, TRI) << "\n";
  }
}

void SwplLoop::dumpCopies() {
  for (auto itr: getCopies()) {
    dbgs() << "MI=" << *itr << "\n";
  }
}

void SwplLoop::printRegID(llvm::Register r) {
  dbgs() << "RegID=" << printReg(r, TRI) << "\n";
}

void SwplReg::print() {
  dbgs() << "DBG(SwplReg::print) SwplReg. : ";
  dbgs() << "SwplReg=" << this << ": RegID=" << printReg(getReg(), TRI) << "\n";
}

void SwplReg::printDefInst() {
  dbgs() << *(getDefInst()->getMI());
}

void SwplMem::printInst() {
  dbgs() << *(getInst()->getMI());
}

/// Immediate-Predecessor が一つである限り、次々たどって、スタックに入れる
/// \param [in] L MachineLoop
/// \param [in,out] BBs BasicBlocks
static void follow_single_predecessor_MBBs(MachineLoop *L, BasicBlocks *BBs) {
  MachineBasicBlock *LoopBB = L->getTopBlock();

  // 対象ループ判定でbodyのbasicBlockが一つであることをチェック済み
  assert(L->getBlocks().size() == 1);
  // predecessorは自分自身とPreheaderの２つ(Preheaderはない場合があるので)
  if (LoopBB->pred_size() != 2) {
    return;
  }

  MachineBasicBlock *pred_BB = nullptr;
  for (auto *MBB:LoopBB->predecessors()) {
    if (LoopBB == MBB) { continue; }
    pred_BB = MBB;
  }
  
  MachineBasicBlock *BB = pred_BB;
  while(BB) {
    for (auto &MI:BB->instrs()) {
      if (isNonTarget(MI)) { return; }
    }
    BBs->push_back(BB);
    BB = getPredecessorBB(*BB);
  }
  
  return;
}

/// PredecessorのBBを取得
/// \param [in] BB MachineBasicBlock
/// \return MachineBasicBlock
static MachineBasicBlock *getPredecessorBB(MachineBasicBlock &BB) {
  if (BB.pred_size() != 1) { return nullptr; }
  for (MachineBasicBlock *S : BB.predecessors())
    return S;

  llvm_unreachable("getPredecessorBB: not_reach_here");
  return nullptr;
}

/// オペランドのuse情報を設定する
/// \param [in,out] rmap llvm::Register と SwplReg のマップ
/// \param [in,out] inst SwplInst
/// \param [in] MO MachineOperand
/// \param [in,out] insts SwplInsts
static void construct_use(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO, SwplInsts &insts) {
  SwplReg *reg;

  auto itr = rmap.find(MO.getReg());
  if (itr != rmap.end()) {
    // SwplRegが構築済み
    reg = itr->second;
  } else {
    // SwplRegを生成しrmapとinstを登録する。
    SwplInst *d_inst = new SwplInst(nullptr, nullptr);
    llvm::Register r = MO.getReg();
    reg = new SwplReg(r, *(d_inst), 0, false);
    rmap[r] = reg;
    insts.push_back(d_inst);
  }

  reg->addUseInsts(&inst);
  inst.addUseRegs(reg);
}

/// MachineMemOperand から SwplMem を生成する
/// \param [in,out] rmap llvm::Register と SwplReg のマップ
/// \param [in,out] inst SwplInst
/// \param [in] MMO MachineMemOperand
/// \param [in,out] insts SwplInsts
/// \param [in,out] mem SwplMems
/// \param [in,out] memsOtherBody SwplMems
static void construct_mem_use(Register2SwplRegMap &rmap, SwplInst &inst, const MachineMemOperand *MMO, SwplInsts &insts,
                              SwplMems *mems, SwplMems *memsOtherBody) {
  SwplMem *mem = new SwplMem(MMO, inst, (MMO==nullptr)?0:MMO->getSize());

  const MachineOperand *BaseOp=nullptr;
  int64_t Offset=0;
  bool OffsetIsScalable=false;
  if (TII->getMemOperandWithOffset(*(inst.getMI()), BaseOp, Offset, OffsetIsScalable, TRI) && BaseOp->isReg()) {
    mem->addUseReg(BaseOp->getReg(), rmap);
  }


  // 登録はBodyInstのみ
  if (mems != nullptr) {
    mems->push_back(mem);
  } else {
    // メモリ解放用にmemsOtherBodyにmemを登録する。
    memsOtherBody->push_back(mem);
  }
}

/// オペランドのdef情報を設定する
/// \param [in,out] rmap llvm::Register と SwplReg のマップ
/// \param [in,out] inst SwplInst
/// \param [in] MO MachineOperand
static void construct_def(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO) {
  size_t indx = inst.getSizeDefRegs();
  llvm::Register r = MO.getReg();
  SwplReg *reg = new SwplReg(r, inst, indx, MO.isEarlyClobber());
  inst.addDefRegs(reg);
  rmap[r] = reg;
}

/// 対象命令でないかどうかの判定
/// \param [in] inst
/// \retval true 対象命令でない
/// \retval false 対象命令である
static bool isNonTarget(MachineInstr &inst) {
  switch(inst.getOpcode()) {
  case AArch64::DMB: /// fence相当
  case AArch64::INLINEASM:
  case AArch64::INLINEASM_BR:
    return true;
  default:
    return false;
  }
}

void SwplLoop::destroy() {
  for (auto *inst:PreviousInsts) {
    delete inst;
  }
  for (auto *inst:PhiInsts) {
    delete inst;
  }
  for (auto *inst:BodyInsts) {
    delete inst;
  }
  for (auto *mem:Mems) {
    delete mem;
  }
  for (auto *mem:MemsOtherBody) {
    delete mem;
  }
  for (auto *reg:Regs) {
    delete reg;
  }
  deleteNewBodyMBB();
}
