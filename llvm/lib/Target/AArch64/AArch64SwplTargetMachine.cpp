//=- AArch64SwplTargetMachine.cpp - Target Machine Info for SWP -*- C++ -*---=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Target Machine Info for SWP.
//
//===----------------------------------------------------------------------===//

#include "AArch64.h"

#include "../../CodeGen/SWPipeliner.h"
#include "AArch64SwplTargetMachine.h"
#include "AArch64TargetTransformInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpipeliner"
#undef ALLOCATED_IS_CCR_ONLY

static cl::opt<bool> DebugStm("swpl-debug-tm",cl::init(false), cl::ReallyHidden);
static cl::opt<int> OptionStoreLatency("swpl-store-latency",cl::init(6), cl::ReallyHidden);
static cl::opt<int> OptionFlowDep("swpl-flow-dep",cl::init(10), cl::ReallyHidden);
static cl::opt<int> OptionRealFetchWidth("swpl-real-fetch-width",cl::init(4), cl::ReallyHidden);
static cl::opt<int> OptionVirtualFetchWidth("swpl-virtual-fetch-width",cl::init(4), cl::ReallyHidden);
static cl::opt<bool> OptionCopyIsVirtual("swpl-copy-is-virtual",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableSensitiveCheck("swpl-sensitive-check",cl::init(false), cl::ReallyHidden);
static cl::opt<unsigned> MaxInstNum("swpl-max-inst-num",cl::init(500), cl::ReallyHidden);
static cl::opt<unsigned> MaxMemNum("swpl-max-mem-num",cl::init(400), cl::ReallyHidden);

// TargetLoopのMI出力オプション（swpl処理は迂回）
static cl::opt<bool> OptionDumpTargetLoopOnly("swpl-debug-dump-targetloop-only",cl::init(false), cl::ReallyHidden);
// TargetLoopのMI出力オプション
static cl::opt<bool> OptionDumpTargetLoop("swpl-debug-dump-targetloop",cl::init(false), cl::ReallyHidden);

static void printDebug(const char *f, const StringRef &msg, const MachineLoop &L) {
  if (!SWPipeliner::isDebugOutput()) return;
  errs() << "DBG(" << f << ") " << msg << ":";
  L.getStartLoc().print(errs());
  errs() <<"\n";
}

/**
 * \brief outputRemarkAnalysis
 *        RemarkAnalysisオブジェクトを生成する。
 *
 * \param[in] L 対象のMachineLoop
 * \return なし
 */
enum MsgID {
  MsgID_swpl_branch_not_for_loop,
  MsgID_swpl_many_insts,
  MsgID_swpl_many_memory_insts,
  MsgID_swpl_not_covered_inst,
  MsgID_swpl_not_covered_loop_shape
};
static void outputRemarkAnalysis(MachineLoop &L, int msg_id) {
  switch (msg_id) {
  case MsgID_swpl_branch_not_for_loop:
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop cannot be software pipelined because the loop contains a branch instruction.";
    });
    break;
  case MsgID_swpl_many_insts:
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop is not software pipelined because the loop contains too many instructions.";
    });
    break;
  case MsgID_swpl_many_memory_insts:
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop is not software pipelined because the loop contains too many instructions accessing memory.";
    });
    break;
  case MsgID_swpl_not_covered_inst:
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop cannot be software pipelined because the loop contains an instruction, such as function call,which is not supported by software pipelining.";
    });
    break;
  case MsgID_swpl_not_covered_loop_shape:
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop cannot be software pipelined because the shape of the loop is not covered by software pipelining.";
    });
    break;
  }
  return;
}


static bool hasRegisterImplicitDefOperand(MachineInstr *MI, unsigned Reg) {
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    if (MO.isReg() && MO.isDef() && MO.isImplicit() && MO.getReg() == Reg)
      return true;
  }
  return false;
}

static bool isCompMI(MachineInstr *MI, AArch64CC::CondCode CC) {

  assert(CC == AArch64CC::GE || CC == AArch64CC::NE);
  if (MI->getOpcode() != AArch64::SUBSXri) {
    return false;
  }

  const MachineOperand &MO = MI->getOperand(0);
  if (CC == AArch64CC::NE) {
    if (MO.isReg()
        && MI->getOperand(1).isReg()
        && (MI->getOperand(2).isImm() && MI->getOperand(2).getImm() == 1)
        && hasRegisterImplicitDefOperand (MI, AArch64::NZCV)) {
      return true;
    } else {
      return false;
    }
  } else if (CC == AArch64CC::GE) {
    if (MO.isReg()
        && MI->getOperand(1).isReg()
        && (MI->getOperand(2).isImm())
        && hasRegisterImplicitDefOperand (MI, AArch64::NZCV)) {
      return true;
    } else {
      return false;
    }
  }
  return false;
}

/**
 * \brief isNonTargetLoop
 *        対象ループ内のbodyのBasicVBlock内の命令探索し、
 *        以下の判定を実施する。
 *        ・ループ内の命令判定
 *        ・ループ制御命令の判定
 *        ・ループ内の命令数/メモリアクセス命令数の判定
 *
\verbatim
 1. BasicBlock内の命令数がMaxInstNumを超えている場合は最適化抑止する。
 2. BasicBlock内の命令をFirstTerminator()～FirstNonDebugInstr()までループする。
  2.1 ループ内の命令判定
     以下の命令を検知した場合、最適化を抑止する。
     - Call命令
     - gnuasm命令
     - DMB命令（fence相当）
     - volatile属性を含む命令
  2.2 ループ制御命令の判定
    ループ分岐命令、ループ終了条件比較命令（NZCVレジスタを更新する命令）を捉えて以下を判定する。
    ・ 以下に該当するループ分岐命令の判定
       - Bcc命令であること
       - NZCVレジスタを参照していること
       - 第一オペランドの種別がAArch64CC::NEまたはGEであること
    ・ ループ終了条件比較命令の判定
       - 命令がAArch64::SUBSXriであること
       - NZCVレジスタを更新していること
       - CC種別がNEの場合に第３ぺランドが定数の１であること
       - CC種別がGEの場合に第３ぺランドが定数であること
    ・ ループ分岐命令、ループ終了条件比較命令以外の命令が以下に該当しないこと
        - CC(NZCVレジスタ)を参照する
        - CC(NZCVレジスタ)を更新する

  2.3 ループ内の命令数/メモリアクセス命令数の判定
    ・ループ内の命令数がMaxInstNumを超えない。MaxInstNumはローカルオプション：-swpl-max-inst-numで制御可能。
    ・ループ内の命令数がMaxMemNumを超えない。MaxMemNumはローカルオプション：-swpl-max-mem-numで制御可能。

\endverbatim
 *
 *
 * \param[in] L 対象のMachineLoop
 * \retval true  最適化対象ループではない。
 * \retval false 最適化対象ループである。
 */
static bool isNonTargetLoop(MachineLoop &L) {
  MachineBasicBlock *LoopBB = L.getTopBlock();

  AArch64CC::CondCode _NE = AArch64CC::NE;
  AArch64CC::CondCode _GE = AArch64CC::GE;
  MachineBasicBlock::iterator I = LoopBB->getFirstTerminator();
  MachineBasicBlock::iterator E = LoopBB->getFirstNonDebugInstr();
  MachineInstr *BccMI = nullptr;
  MachineInstr *CompMI = nullptr;
  int mem_counter=MaxMemNum;
  AArch64CC::CondCode CC;

  //命令数はBasickBlockから取得できる
  if (LoopBB->size() > MaxInstNum) {
    printDebug(__func__, "pipeliner info:over inst limit num", L);
    outputRemarkAnalysis(L, MsgID_swpl_many_insts);
    return true;
  }

  for (; I != E; --I) {
    // Callである
    if (I->isCall()) {
      printDebug(__func__, "pipeliner info:found call", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    if (EnableSensitiveCheck && I->mayRaiseFPException()) {
      printDebug(__func__, "pipeliner info:found mayRaiseFPException", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    if (EnableSensitiveCheck && I->hasUnmodeledSideEffects()) {
      printDebug(__func__, "pipeliner info:found hasUnmodeledSideEffects", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    if (EnableSensitiveCheck && I->hasOrderedMemoryRef() &&  (!I->mayLoad() || !I->isDereferenceableInvariantLoad())) {
      printDebug(__func__, "pipeliner info:found hasOrderedMemoryRef", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    // fenceもしくはgnuasm命令である
    if (SWPipeliner::TII->isNonTargetMI4SWPL(*I)) {
      printDebug(__func__, "pipeliner info:found non-target-inst or gnuasm", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    // volatile属性を含む命令である
    for (MachineMemOperand *MMO : I->memoperands()) {
      if (MMO->isVolatile()) {
        printDebug(__func__, "pipeliner info:found volataile operand", L);
        outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
        return true;
      }
    }
    /* CCを更新する命令が複数出現した。 */
    if (CompMI && hasRegisterImplicitDefOperand (&*I, AArch64::NZCV)) {
      printDebug(__func__, "pipeliner info:multi-defoperand==NZCV", L);
      outputRemarkAnalysis(L, MsgID_swpl_branch_not_for_loop);
      return true;
    }
    /* CCを参照する命令が複数出現した。 */
    if (BccMI && I->hasRegisterImplicitUseOperand(AArch64::NZCV)) {
      printDebug(__func__, "pipeliner info:multi-refoperand==NZCV", L);
      outputRemarkAnalysis(L, MsgID_swpl_branch_not_for_loop);
      return true;
    }
    /* ループ分岐命令を捕捉 */
    if (I->getOpcode() == AArch64::Bcc
        && I->hasRegisterImplicitUseOperand(AArch64::NZCV)) {
      BccMI = &*I;
      CC = (AArch64CC::CondCode)BccMI->getOperand(0).getImm();
      /* CCの比較種別が対象外 */
      if (CC != _NE && CC != _GE) {
        printDebug(__func__, "pipeliner info:BCC condition!=NE && GE", L);
        outputRemarkAnalysis(L, MsgID_swpl_not_covered_loop_shape);
        return true;
      }
    } else if (BccMI && isCompMI(&*I, CC)) {
      /* Bccが先に見つかっていて、かつループ終了条件比較命令が見つかった。 */
      if (CompMI) {
        /* ループ終了条件比較命令が複数見つかった。 */
        printDebug(__func__, "pipeliner info:not found SUBSXri", L);
        outputRemarkAnalysis(L, MsgID_swpl_branch_not_for_loop);
        return true;
      }
      const auto phiIter=SWPipeliner::MRI->def_instr_begin(I->getOperand(1).getReg());
      if (!phiIter->isPHI()) {
        /* 正規化されたループ制御変数がない（PHIとSUBの間に命令が存在する）。 */
        printDebug(__func__, "pipeliner info:not found induction var", L);
        outputRemarkAnalysis(L, MsgID_swpl_not_covered_loop_shape);
        return true;
      }
      CompMI = &*I;
    }
    if ((!I->memoperands_empty()) && (--mem_counter)<=0) {
      printDebug(__func__, "pipeliner info:over mem limit num", L);
      outputRemarkAnalysis(L, MsgID_swpl_many_memory_insts);
      return true;
    }
  }
  if (!(BccMI && CompMI)) {
    printDebug(__func__, "pipeliner info:not found (BCC || SUBSXri)", L);
    outputRemarkAnalysis(L, MsgID_swpl_not_covered_loop_shape);
    return true;
  }
  return false;
}

/**
 * \brief isNonTargetLoopForInstDump
 *        ローカルオプションによるMI出力対象かを判定する
 * \details 判定内容は、isNonTargetLoopの判定のうち、
 *          以下の判定を除いたものとしている。
 *          - ループ制御命令の判定
 *          - ループ内の命令数/メモリアクセス命令数の判定
 * \param[in] L 対象のMachineLoop
 * \retval true  ローカルオプションによるMI出力対象ループでない
 * \retval false ローカルオプションによるMI出力対象ループである
 */
static bool isNonTargetLoopForInstDump(MachineLoop &L) {
  MachineBasicBlock *LoopBB = L.getTopBlock();
  MachineBasicBlock::iterator I = LoopBB->getFirstTerminator();
  MachineBasicBlock::iterator E = LoopBB->getFirstNonDebugInstr();

  for (; I != E; --I) {
    // Callである
    if (I->isCall()) {
      printDebug(__func__, "pipeliner info:found call", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    if (EnableSensitiveCheck && I->mayRaiseFPException()) {
      printDebug(__func__, "pipeliner info:found mayRaiseFPException", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    if (EnableSensitiveCheck && I->hasUnmodeledSideEffects()) {
      printDebug(__func__, "pipeliner info:found hasUnmodeledSideEffects", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    if (EnableSensitiveCheck && I->hasOrderedMemoryRef() &&  (!I->mayLoad() || !I->isDereferenceableInvariantLoad())) {
      printDebug(__func__, "pipeliner info:found hasOrderedMemoryRef", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    // fenceもしくはgnuasm命令である
    if (SWPipeliner::TII->isNonTargetMI4SWPL(*I)) {
      printDebug(__func__, "pipeliner info:found non-target-inst or gnuasm", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    // volatile属性を含む命令である
    for (MachineMemOperand *MMO : I->memoperands()) {
      if (MMO->isVolatile()) {
        printDebug(__func__, "pipeliner info:found volataile operand", L);
        outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
        return true; // 対象でない
      }
    }
  }
  return false; // 対象である
}

/**
 * \brief dumpLoopInst
 *        MachineLoopのFirstNonDebugInstrからFirstTerminatorの命令をdumpする
 * \details 命令のdumpは、以下の情報をTSVで出力する。
 *  - 関数名
 *  - 行番号
 *  - opcode名
 *  - Pseudo命令であれば"PSEUDO"。Pseudo命令でない場合は空フィールド
 *  - 命令が持つメモリオペランド数を出力
 *  - MI.dump()
 * \param[in] L 対象のMachineLoop
 * \return なし
 */
static void dumpLoopInst(MachineLoop &L) {
  auto topblock = L.getTopBlock();

  // 関数名を取得
  auto funcname = topblock->getParent()->getName();

  MachineBasicBlock::iterator I = topblock->getFirstNonDebugInstr();
  MachineBasicBlock::iterator E = topblock->getFirstTerminator();
  for (; I != E; I++) {
    // 関数名を出力
    dbgs() << funcname;
    dbgs() << "\t";

    // 行番号を出力
    auto DL = I->getDebugLoc();
    if( DL.get() ) {
      dbgs() << DL.getLine();
    }
    else {
      dbgs() << " ";
    }
    dbgs() << "\t";

    // opcode名を出力
    if (SWPipeliner::TII) {
      dbgs() << SWPipeliner::TII->getName(I->getOpcode());
    }
    else {
      dbgs() << "UNKNOWN";
    }
    dbgs() << "\t";

    // Pseudo命令かを出力
    if( I->isPseudo() ) {
      dbgs() << "PSEUDO";
    }
    else {
      dbgs() << " ";
    }
    dbgs() << "\t";

    // 命令が持つメモリオペランド数を出力
    dbgs() << I->getNumMemOperands();
    dbgs() << "\t";

    // MachineInstrをダンプ
    I->print(dbgs());
  }
}


namespace llvm {

bool AArch64InstrInfo::canPipelineLoop(MachineLoop &L) const {

  // Simd/Unrollの余りループは最適化抑止
  // I/F未定

  // ループ内のBasicBlockが一つではない場合は最適化抑止
  if (L.getNumBlocks() != 1) {
    printDebug(__func__, "[canPipelineLoop:NG] Not a single basic block. ", L);
    outputRemarkAnalysis(L, MsgID_swpl_not_covered_loop_shape);
    return false;
  }

  // ローカルオプションによるTargetLoopのMI出力
  if( (OptionDumpTargetLoop || OptionDumpTargetLoopOnly) && !(isNonTargetLoopForInstDump(L)) ) {
    dumpLoopInst(L);
    if (OptionDumpTargetLoopOnly) {
      // DumpLoopのみ行う場合は、flaeを返して処理を終了させる（RpassMissedとなる）
      return false;
    }
  }

  // BasicBlock内の命令チェック
  if (isNonTargetLoop(L)) {
    printDebug(__func__, "[canPipelineLoop:NG] Unable to analyzeLoop, can NOT pipeline Loop. ", L);
    return false;
  }

  // 対象ループ判定だけして最適化は未実施
  printDebug(__func__, "[canPipelineLoop:OK] Passed all checks. ", L);

  return true;
}

int AArch64InstrInfo::calcEachRegIncrement(const SwplReg *r) const {
//  SwplInst *def_inst = nullptr;
  const SwplInst *induction_inst = nullptr;
  int index_dummy = -1;
  const SwplReg *target=r;
  const SwplInst *def_inst;
  r->getDefPort(&def_inst, &index_dummy);

  if (!def_inst->isInLoop()) {
    return 0;
  }
  if (def_inst->isInLoop() && def_inst->getMI() != nullptr && def_inst->getMI()->getOpcode()==AArch64::ADDXrr) {
    const auto &r1=def_inst->getUseRegs(0);
    const auto &r2=def_inst->getUseRegs(1);
    if (r1.getDefInst()->isPhi()) {
      if (r2.getDefInst()->isInLoop())
        return SwplDdg::UNKNOWN_MEM_DIFF;
      def_inst=r1.getDefInst();
      target=&r1;
    } else if (r2.getDefInst()->isPhi()) {
      if (r1.getDefInst()->isInLoop())
        return SwplDdg::UNKNOWN_MEM_DIFF;
      def_inst=r2.getDefInst();
      target=&r2;
    } else {
      return SwplDdg::UNKNOWN_MEM_DIFF;
    }
  }

  if (def_inst->isPhi()) {
    const SwplReg& induction_reg = def_inst->getPhiUseRegFromIn();
    induction_reg.getDefPort(&induction_inst, &index_dummy);
    while (induction_inst->isCopy()) {
      const auto &def_reg=induction_inst->getUseRegs(0);
      induction_inst=def_reg.getDefInst();
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
        return SwplDdg::UNKNOWN_MEM_DIFF;

      if (&(induction_inst->getUseRegs(0)) == target) {
        const auto& mo=mi->getOperand(2);
        if (!mo.isImm()) return SwplDdg::UNKNOWN_MEM_DIFF;
        return mo.getImm();
      }
    } else {
      return SwplDdg::UNKNOWN_MEM_DIFF;
    }
  } else {
    if (def_inst->isLoad())
      return SwplDdg::UNKNOWN_MEM_DIFF;

    while (def_inst->getSizeUseRegs() == 1) {
      def_inst->getUseRegs(0).getDefPort(&def_inst, &index_dummy);
      if (!def_inst->isInLoop()) {
        return 0;
      }
      if (def_inst->isPhi()) {
        return SwplDdg::UNKNOWN_MEM_DIFF;
      }
    }
    return SwplDdg::UNKNOWN_MEM_DIFF;
  }
  return SwplDdg::UNKNOWN_MEM_DIFF;
}

SwplTargetMachine *AArch64InstrInfo::getSwplTargetMachine() const {
  SwplTargetMachine *p=new AArch64SwplTargetMachine();
  return p;
}

}

namespace llvm {

void AArch64SwplTargetMachine::initialize(const MachineFunction &mf) {
  if (MF==nullptr) {
    const TargetSubtargetInfo &ST = mf.getSubtarget();
    SM.init(&ST);
    ResInfo=new AArch64A64FXResInfo(ST);
    tmNumSameKindResources[A64FXRes::PortKind::P_FLA]=2;
    tmNumSameKindResources[A64FXRes::PortKind::P_FLB]=2;
    tmNumSameKindResources[A64FXRes::PortKind::P_EXA]=2;
    tmNumSameKindResources[A64FXRes::PortKind::P_EXB]=2;
    tmNumSameKindResources[A64FXRes::PortKind::P_EAGA]=2;
    tmNumSameKindResources[A64FXRes::PortKind::P_EAGB]=2;
    tmNumSameKindResources[A64FXRes::PortKind::P_PRX]=1;
    tmNumSameKindResources[A64FXRes::PortKind::P_BR]=1;
    numResource=8; // 資源管理がSchedModelとは別になったので、ハードコードする
  }
  MF=&mf;
}

unsigned int AArch64SwplTargetMachine::getFetchBandwidth(void) const {
  return getRealFetchBandwidth()+OptionVirtualFetchWidth;
}

unsigned int AArch64SwplTargetMachine::getRealFetchBandwidth(void) const {
  return OptionRealFetchWidth;
}

int AArch64StmPipeline::getNResources(StmResourceId resource) const {
  int counter=0;
  for (StmResourceId r: resources) {
    if (r==resource) counter++;
  }
  return counter;
}

void AArch64StmPipeline::print(raw_ostream &ost) const {
  ost << "DBG(AArch64StmPipeline::print) stage/resource("<< patternId << "): ";
  int last=stages.size();
  const char *sep="";
  for (int ix=0; ix<last; ix++) {
    ost << sep << stages[ix] << "/" << getResourceName(resources[ix]);
    sep=", ";
  }
  ost << "\n";
}

const char *AArch64StmPipeline::getResourceName(StmResourceId resource) const {
  // @todo AArch64SwplTargetMachine::getResourceName()を呼び出すようにすべきor本メソッド削除
  const char *name="";
  switch (resource) {
  case A64FXRes::PortKind::P_FLA:name="FLA";break;
  case A64FXRes::PortKind::P_FLB:name="FLB";break;
  case A64FXRes::PortKind::P_EXA:name="EXA";break;
  case A64FXRes::PortKind::P_EXB:name="EXB";break;
  case A64FXRes::PortKind::P_EAGA:name="EAGA";break;
  case A64FXRes::PortKind::P_EAGB:name="EAGB";break;
  case A64FXRes::PortKind::P_PRX:name="PRX";break;
  case A64FXRes::PortKind::P_BR:name="BR";break;
  default:
    llvm_unreachable("unknown resourceid");
  }
  return name;
}

const char *AArch64SwplTargetMachine::getResourceName(StmResourceId resource) const {
  const char *name="";
  switch (resource) {
  case A64FXRes::PortKind::P_FLA:name="FLA";break;
  case A64FXRes::PortKind::P_FLB:name="FLB";break;
  case A64FXRes::PortKind::P_EXA:name="EXA";break;
  case A64FXRes::PortKind::P_EXB:name="EXB";break;
  case A64FXRes::PortKind::P_EAGA:name="EAGA";break;
  case A64FXRes::PortKind::P_EAGB:name="EAGB";break;
  case A64FXRes::PortKind::P_PRX:name="PRX";break;
  case A64FXRes::PortKind::P_BR:name="BR";break;
  default:
    llvm_unreachable("unknown resourceid");
  }
  return name;
}

int AArch64SwplTargetMachine::computeMemFlowDependence(const MachineInstr *, const MachineInstr *) const {
  if (OptionStoreLatency > 0) return OptionStoreLatency;
  if (OptionFlowDep > 0) return OptionFlowDep;
  return 1;
}

const StmPipelinesImpl *
AArch64SwplTargetMachine::getPipelines(const MachineInstr &mi) {
  auto *tps= stmPipelines[mi.getOpcode()];
  if (tps==nullptr) {
    tps= generateStmPipelines(mi);
    stmPipelines[mi.getOpcode()]=tps;
  }
  return tps;
}

StmPipelinesImpl *
AArch64SwplTargetMachine::generateStmPipelines(const MachineInstr &mi) {
  // @todo 作りが必要
  auto *pipelines=new StmPipelines;
  if (mi.isPseudo()) {
    pipelines->push_back(new AArch64StmPipeline());
  } else {
    auto *p = new AArch64StmPipeline();
    p->stages.push_back(0);
    p->resources.push_back(A64FXRes::PortKind::P_BR);
    pipelines->push_back(p);
  }
  return pipelines;
}

}
int AArch64SwplTargetMachine::computeRegFlowDependence(const MachineInstr* def, const MachineInstr* use) const {
  // @todo 作り必要
  return 1;
}

int AArch64SwplTargetMachine::computeMemAntiDependence(const MachineInstr *, const MachineInstr *) const {
  return 1;
}

int AArch64SwplTargetMachine::computeMemOutputDependence(const MachineInstr *, const MachineInstr *) const {
  return 1;
}

unsigned int AArch64SwplTargetMachine::getNumResource(void) const {
  return numResource;
}

bool AArch64SwplTargetMachine::isImplimented(const MachineInstr&mi) const {
  if (OptionCopyIsVirtual) {
    if (mi.isCopy()) return false;
  }
  // @todo 作り必要
  return false;
}

bool AArch64SwplTargetMachine::isPseudo(const MachineInstr &mi) const {
  // @todo 作り必要
  return mi.isPseudo();
//  return !isImplimented(mi);
}

