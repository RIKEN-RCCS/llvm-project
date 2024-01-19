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
#include "llvm/Support/Regex.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpipeliner"
#undef ALLOCATED_IS_CCR_ONLY

static cl::opt<bool> DebugStm("swpl-debug-tm",cl::init(false), cl::ReallyHidden);
static cl::opt<int> OptionStoreLatency("swpl-store-latency",cl::init(6), cl::ReallyHidden);
static cl::opt<int> OptionFlowDep("swpl-flow-dep",cl::init(10), cl::ReallyHidden);
static cl::opt<bool> OptionCopyIsVirtual("swpl-copy-is-virtual",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableSensitiveCheck("swpl-sensitive-check",cl::init(false), cl::ReallyHidden);
static cl::opt<unsigned> MaxInstNum("swpl-max-inst-num",cl::init(500), cl::ReallyHidden);
static cl::opt<unsigned> MaxMemNum("swpl-max-mem-num",cl::init(400), cl::ReallyHidden);
static cl::opt<bool> DisableRegAlloc("swpl-disable-reg-alloc",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableRegAlloc("swpl-enable-reg-alloc",cl::init(true), cl::ReallyHidden);
static cl::opt<bool> DisableProEpiCopy("swpl-disable-proepi-copy",cl::init(false), cl::ReallyHidden);

// TargetLoopのMI出力オプション（swpl処理は迂回）
static cl::opt<bool> OptionDumpTargetLoopOnly("swpl-debug-dump-targetloop-only",cl::init(false), cl::ReallyHidden);
// TargetLoopのMI出力オプション
static cl::opt<bool> OptionDumpTargetLoop("swpl-debug-dump-targetloop",cl::init(false), cl::ReallyHidden);
// MIのリソース情報出力オプション
static cl::opt<std::string> OptionDumpResource("swpl-debug-dump-resource-filter",cl::init(""), cl::ReallyHidden);

static void printDebug(const char *f, const StringRef &msg, const MachineLoop &L) {
  if (!SWPipeliner::isDebugOutput()) return;
  errs() << "DBG(" << f << ") " << msg << ":";
  L.getStartLoc().print(errs());
  errs() <<"\n";
}

/**
 * \brief outputRemarkMissed
 *        RemarkMissed用のメッセージを生成する。
 *
 * \param[in] L 対象のMachineLoop
 * \return なし
 */
enum MsgID {
  MsgID_swpl_branch_not_for_loop,
  MsgID_swpl_many_insts,
  MsgID_swpl_many_memory_insts,
  MsgID_swpl_not_covered_inst,
  MsgID_swpl_not_covered_loop_shape,
  MsgID_swpl_multiple_inst_update_CCR,
  MsgID_swpl_multiple_inst_reference_CCR,
  MsgID_swpl_inst_update_FPCR
};
static void outputRemarkMissed(MachineLoop &L, int msg_id) {
  switch (msg_id) {
  case MsgID_swpl_branch_not_for_loop:
    SWPipeliner::Reason = "This loop cannot be software pipelined"
                          " because the loop contains a branch instruction.";
    break;
  case MsgID_swpl_many_insts:
    SWPipeliner::Reason = "This loop is not software pipelined"
                          " because the loop contains too many instructions.";
    break;
  case MsgID_swpl_many_memory_insts:
    SWPipeliner::Reason = "This loop is not software pipelined"
                          " because the loop contains too many instructions accessing memory.";
    break;
  case MsgID_swpl_not_covered_inst:
    SWPipeliner::Reason = "This loop cannot be software pipelined"
                          " because the loop contains an instruction, such as function call,"
                          " which is not supported by software pipelining.";
    break;
  case MsgID_swpl_not_covered_loop_shape:
    SWPipeliner::Reason = "This loop cannot be software pipelined"
                          " because the shape of the loop is not covered by software pipelining.";
    break;
  case MsgID_swpl_multiple_inst_update_CCR:
    SWPipeliner::Reason = "This loop cannot be software pipelined"
                          " because multiple instructions to update CCR.";
    break;
  case MsgID_swpl_multiple_inst_reference_CCR:
    SWPipeliner::Reason = "This loop cannot be software pipelined"
                          " because multiple instructions to reference CCR.";
    break;
  case MsgID_swpl_inst_update_FPCR:
    SWPipeliner::Reason = "This loop cannot be software pipelined"
                          " because instruction to update FPCR.";
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
    outputRemarkMissed(L, MsgID_swpl_many_insts);
    return true;
  }

  for (; I != E; --I) {
    // Callである
    if (I->isCall()) {
      printDebug(__func__, "pipeliner info:found call", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    if (EnableSensitiveCheck && I->mayRaiseFPException()) {
      printDebug(__func__, "pipeliner info:found mayRaiseFPException", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    if (EnableSensitiveCheck && I->hasUnmodeledSideEffects()) {
      printDebug(__func__, "pipeliner info:found hasUnmodeledSideEffects", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    if (EnableSensitiveCheck && I->hasOrderedMemoryRef() &&  (!I->mayLoad() || !I->isDereferenceableInvariantLoad())) {
      printDebug(__func__, "pipeliner info:found hasOrderedMemoryRef", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    // fenceもしくはgnuasm命令である
    if (SWPipeliner::TII->isNonTargetMI4SWPL(*I)) {
      printDebug(__func__, "pipeliner info:found non-target-inst or gnuasm", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    // volatile属性を含む命令である
    for (MachineMemOperand *MMO : I->memoperands()) {
      if (MMO->isVolatile()) {
        printDebug(__func__, "pipeliner info:found volataile operand", L);
        outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
        return true;
      }
    }
    /* CCを更新する命令が複数出現した。 */
    if (CompMI && hasRegisterImplicitDefOperand (&*I, AArch64::NZCV)) {
      printDebug(__func__, "pipeliner info:multi-defoperand==NZCV", L);
      outputRemarkMissed(L, MsgID_swpl_multiple_inst_update_CCR);
      return true;
    }
    /* FPCRを更新する命令が出現した。 */
    if (hasRegisterImplicitDefOperand (&*I, AArch64::FPCR)) {
      printDebug(__func__, "pipeliner info:defoperand==FPCR", L);
      outputRemarkMissed(L, MsgID_swpl_inst_update_FPCR);
      return true;
    }
    /* CCを参照する命令が複数出現した。 */
    if (BccMI && I->hasRegisterImplicitUseOperand(AArch64::NZCV)) {
      printDebug(__func__, "pipeliner info:multi-refoperand==NZCV", L);
      outputRemarkMissed(L, MsgID_swpl_multiple_inst_reference_CCR);
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
        outputRemarkMissed(L, MsgID_swpl_not_covered_loop_shape);
        return true;
      }
    } else if (BccMI && isCompMI(&*I, CC)) {
      /* Bccが先に見つかっていて、かつループ終了条件比較命令が見つかった。 */
      if (CompMI) {
        /* ループ終了条件比較命令が複数見つかった。 */
        printDebug(__func__, "pipeliner info:not found SUBSXri", L);
        outputRemarkMissed(L, MsgID_swpl_branch_not_for_loop);
        return true;
      }
      const auto phiIter=SWPipeliner::MRI->def_instr_begin(I->getOperand(1).getReg());
      if (!phiIter->isPHI()) {
        /* 正規化されたループ制御変数がない（PHIとSUBの間に命令が存在する）。 */
        printDebug(__func__, "pipeliner info:not found induction var", L);
        outputRemarkMissed(L, MsgID_swpl_not_covered_loop_shape);
        return true;
      }
      CompMI = &*I;
    }
    if ((!I->memoperands_empty()) && (--mem_counter)<=0) {
      printDebug(__func__, "pipeliner info:over mem limit num", L);
      outputRemarkMissed(L, MsgID_swpl_many_memory_insts);
      return true;
    }
  }
  if (!(BccMI && CompMI)) {
    printDebug(__func__, "pipeliner info:not found (BCC || SUBSXri)", L);
    outputRemarkMissed(L, MsgID_swpl_not_covered_loop_shape);
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
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    if (EnableSensitiveCheck && I->mayRaiseFPException()) {
      printDebug(__func__, "pipeliner info:found mayRaiseFPException", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    if (EnableSensitiveCheck && I->hasUnmodeledSideEffects()) {
      printDebug(__func__, "pipeliner info:found hasUnmodeledSideEffects", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    if (EnableSensitiveCheck && I->hasOrderedMemoryRef() &&  (!I->mayLoad() || !I->isDereferenceableInvariantLoad())) {
      printDebug(__func__, "pipeliner info:found hasOrderedMemoryRef", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    // fenceもしくはgnuasm命令である
    if (SWPipeliner::TII->isNonTargetMI4SWPL(*I)) {
      printDebug(__func__, "pipeliner info:found non-target-inst or gnuasm", L);
      outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    // volatile属性を含む命令である
    for (MachineMemOperand *MMO : I->memoperands()) {
      if (MMO->isVolatile()) {
        printDebug(__func__, "pipeliner info:found volataile operand", L);
        outputRemarkMissed(L, MsgID_swpl_not_covered_inst);
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
    outputRemarkMissed(L, MsgID_swpl_not_covered_loop_shape);
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

static const char *getResourceName(StmResourceId resource) {
  const char *name="";
  switch (resource) {
  case AArch64SwplSchedA64FX::FLA:name="FLA";break;
  case AArch64SwplSchedA64FX::FLB:name="FLB";break;
  case AArch64SwplSchedA64FX::EXA:name="EXA";break;
  case AArch64SwplSchedA64FX::EXB:name="EXB";break;
  case AArch64SwplSchedA64FX::EAGA:name="EAGA";break;
  case AArch64SwplSchedA64FX::EAGB:name="EAGB";break;
  case AArch64SwplSchedA64FX::PRX:name="PRX";break;
  case AArch64SwplSchedA64FX::BR:name="BR";break;
  case AArch64SwplSchedA64FX::LSU1:name="LSU1";break;
  case AArch64SwplSchedA64FX::LSU2:name="LSU2";break;
  case AArch64SwplSchedA64FX::FLA_C:name="FLA_C";break;
  case AArch64SwplSchedA64FX::FLB_C:name="FLB_C";break;
  case AArch64SwplSchedA64FX::EXA_C:name="EXA_C";break;
  case AArch64SwplSchedA64FX::EXB_C:name="EXB_C";break;
  case AArch64SwplSchedA64FX::EAGA_C:name="EAGA_C";break;
  case AArch64SwplSchedA64FX::EAGB_C:name="EAGB_C";break;
  case AArch64SwplSchedA64FX::FLA_E:name="FLA_E";break;
  case AArch64SwplSchedA64FX::EXA_E:name="EXA_E";break;
  case AArch64SwplSchedA64FX::EXB_E:name="EXB_E";break;
  default:
    llvm_unreachable("unknown resourceid");
  }
  return name;

}

static StmPipelines forPseudoMI;
static StmPipelines forNoImplMI;
Regex RDumpResource;
StringSet<> dumpedMIResource;

AArch64SwplTargetMachine::~AArch64SwplTargetMachine() {
  for (auto *p:forPseudoMI) delete p;
  for (auto *p:forNoImplMI) delete p;
  forPseudoMI.clear();
  forNoImplMI.clear();
  for (auto &tms: stmPipelines) {
    if (tms.getSecond()) {
      for (auto *t:*(tms.getSecond())) {
        delete const_cast<StmPipeline *>(t);
      }
      delete tms.getSecond();
    }
  }
}

void AArch64SwplTargetMachine::initialize(const MachineFunction &mf) {
  if (MF==nullptr) {
    const TargetSubtargetInfo &ST = mf.getSubtarget();
    SM.init(&ST);
    numResource = AArch64SwplSchedA64FX::END - 1;

    forPseudoMI.push_back(new StmPipeline());
    auto *p = new StmPipeline();
    p->stages.push_back(0);
    p->resources.push_back(AArch64SwplSchedA64FX::BR);
    forNoImplMI.push_back(p);

  }
  MF=&mf;

  // 属性VLをMFから取り出し、AArch64SwplSchedA64FXに設定する
  const AArch64Subtarget &Subtarget = mf.getSubtarget<AArch64Subtarget>();
  SwplSched.VectorLength = Subtarget.getMaxSVEVectorSizeInBits();

  if (SwplSched.VectorLength > 512 || SwplSched.VectorLength == 0) {
    SwplSched.VectorLength = 512;
  }
  for (auto &mbb:mf) {
    DebugLoc loc;
    if (mbb.empty()) continue;
    for (auto &mi : mbb) {
      auto &l = mi.getDebugLoc();
      loc=l;
      if (l.get()==nullptr) continue;
      break;
    }
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "VectorLength", loc, &mbb)
             << "SVE instruction latency is calculated at " << ore::NV("VL", SwplSched.VectorLength) << "bit.";
    });
    break;
  }

  // swpl-debug-dump-resourceで設定された正規表現をコンパイルする
  RDumpResource = Regex(OptionDumpResource);
  dumpedMIResource.clear();
}

const char *AArch64SwplTargetMachine::getResourceName(StmResourceId resource) const {
  return ::getResourceName(resource);
}

int AArch64SwplTargetMachine::computeMemFlowDependence(const MachineInstr *, const MachineInstr *) const {
  if (OptionStoreLatency > 0) return OptionStoreLatency;
  if (OptionFlowDep > 0) return OptionFlowDep;
  return 1;
}


const StmPipelinesImpl *
AArch64SwplTargetMachine::getPipelines(const MachineInstr &mi) const {

  // swpl-debug-dump-resourceで指定されていて、まだdumpしていないMIのみdumpする
  bool dump = false;
  if (!(OptionDumpResource.empty())){
    // miを文字列にして既にdumpしたかを判断する
    std::string mistr;
    raw_string_ostream mistream(mistr);
    mi.print(mistream);
    if (RDumpResource.match(SWPipeliner::TII->getName(mi.getOpcode())) &&
        !(dumpedMIResource.contains(mistr))){
      dump = true;
      dumpedMIResource.insert(mistr);
    }
  }

  if (isPseudo(mi)) {
    if (dump) {
      dbgs() << "DBG(AArch64SwplTargetMachine::getPipelines): Pseudo-instr: "
             << SWPipeliner::TII->getName(mi.getOpcode()) << "\n";
    }
    return &forPseudoMI;
  } else if (!isImplimented(mi)) {
    if (dump || SWPipeliner::isDebugOutput()) {
      dbgs() << "DBG(AArch64SwplTargetMachine::getPipelines): undefined machine-instr: "
             << SWPipeliner::TII->getName(mi.getOpcode()) << "\n";
    }
    return &forNoImplMI;
  }
  auto id=SwplSched.getRes(mi);
  const StmPipelinesImpl *p=SwplSched.getPipelines(id);
  if (dump) {
    dbgs() << "DBG(AArch64SwplTargetMachine::getPipelines): MI: " << mi;
    const char *q="";
    switch ((id&0xfffff000)) {
    case AArch64SwplSchedA64FX::INT_OP:q="INT_OP"; break;
    case AArch64SwplSchedA64FX::INT_LD:q="INT_LD"; break;
    case AArch64SwplSchedA64FX::INT_ST:q="INT_ST"; break;
    case AArch64SwplSchedA64FX::SIMDFP_SVE_OP:q="SIMDFP_SVE_OP"; break;
    case AArch64SwplSchedA64FX::SIMDFP_SVE_LD:q="SIMDFP_SVE_LD"; break;
    case AArch64SwplSchedA64FX::SIMDFP_SVE_ST:q="SIMDFP_SVE_ST"; break;
    case AArch64SwplSchedA64FX::SVE_CMP_INST:q="SVE_CMP_INST"; break;
    case AArch64SwplSchedA64FX::PREDICATE_OP:q="PREDICATE_OP"; break;
    case AArch64SwplSchedA64FX::PREDICATE_LD:q="PREDICATE_LD"; break;
    case AArch64SwplSchedA64FX::PREDICATE_ST:q="PREDICATE_ST"; break;
    }
    dbgs() << "  ResourceID: " << q << "+" << (id&0xfff) << "\n";
    dbgs() << "  latency: " << SwplSched.getLatency(id) << "\n";
    dbgs() << "  seqDecode: " << (SwplSched.isSeqDecode(id) ? "true" : "false") << "\n";
    for (const auto *s: *p) {
      print(dbgs(), *s);
    }
  }
  return p;
}

int AArch64SwplTargetMachine::computeRegFlowDependence(const MachineInstr* def, const MachineInstr* use) const {
  if (isPseudo(*def)) {
    return 0;
  }
  if (isImplimented(*def)) {
    return SwplSched.getLatency(SwplSched.getRes(*def));
  }
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
  if (SwplSched.getRes(mi) == AArch64SwplSchedA64FX::MI_NA)
    return false;
  else
    return true;
}

bool AArch64SwplTargetMachine::isPseudo(const MachineInstr &mi) const {
  if (OptionCopyIsVirtual && mi.isCopy()) return true;
  return SwplSched.isPseudo(mi);
}

void AArch64SwplTargetMachine::print(llvm::raw_ostream &ost, const StmPipeline &pipeline) const {
  ost << "  stage/resource(" << "): ";
  int last=pipeline.stages.size();
  const char *sep="";
  for (int ix=0; ix<last; ix++) {
    ost << sep << pipeline.stages[ix] << "/" << getResourceName(pipeline.resources[ix]);
    sep=", ";
  }
  ost << "\n";
}
bool AArch64SwplTargetMachine::isDisableRegAlloc(void) const {
  if (DisableRegAlloc) return true;
  return !EnableRegAlloc;
}

unsigned AArch64SwplTargetMachine::getInstType(const MachineInstr &mi) const {
  auto id=SwplSched.getRes(mi);
  return (id&0xfffff000);
}

const char* AArch64SwplTargetMachine::getInstTypeString(unsigned insttypeid) const {
  switch(insttypeid) {
  case AArch64SwplSchedA64FX::INT_OP: return "INT_OP";break;
  case AArch64SwplSchedA64FX::INT_LD: return "INT_LD"; break;
  case AArch64SwplSchedA64FX::INT_ST: return "INT_ST"; break;
  case AArch64SwplSchedA64FX::SIMDFP_SVE_OP: return "SIMDFP_SVE_OP"; break;
  case AArch64SwplSchedA64FX::SIMDFP_SVE_LD: return "SIMDFP_SVE_LD"; break;
  case AArch64SwplSchedA64FX::SIMDFP_SVE_ST: return "SIMDFP_SVE_ST"; break;
  case AArch64SwplSchedA64FX::SVE_CMP_INST: return "SVE_CMP_INST"; break;
  case AArch64SwplSchedA64FX::PREDICATE_OP: return "PREDICATE_OP"; break;
  case AArch64SwplSchedA64FX::PREDICATE_LD: return "PREDICATE_LD"; break;
  case AArch64SwplSchedA64FX::PREDICATE_ST: return "PREDICATE_ST"; break;
  case 0: return "undefined"; break;
  default:
    llvm_unreachable("unknown InstTypeId");
  }
}

unsigned AArch64SwplTargetMachine::calcPenaltyByInsttypeAndDependreg(const MachineInstr& prod,
                                                                     const MachineInstr& cons,
                                                                     const llvm::Register& reg) const {
  /// \todo 現在 I/Fのみ。
  ///       命令種と依存レジスタによるペナルティを算出する処理を実装し、
  ///       スケジューラがこれを用いてスケジューリングするように実装する。
  return 0;
}

bool AArch64SwplTargetMachine::isEnableRegAlloc(void) const {
  if (DisableRegAlloc) return false;
  return EnableRegAlloc;
}

bool AArch64SwplTargetMachine::isEnableProEpiCopy(void) const {
  return !DisableProEpiCopy;
}