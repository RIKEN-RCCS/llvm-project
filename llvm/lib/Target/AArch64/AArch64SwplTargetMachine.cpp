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
#include "AArch64InstrInfo.h"
#include "MCTargetDesc/AArch64AddressingModes.h"
#include "llvm/Support/FormatVariadic.h"
#include <unordered_set>
#include "llvm/CodeGen/SwplTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpipeliner"
#undef ALLOCATED_IS_CCR_ONLY

static cl::opt<bool> DebugStm("swpl-debug-tm",cl::init(false), cl::ReallyHidden);
static cl::opt<int> OptionStoreLatency("swpl-store-latency",cl::init(6), cl::ReallyHidden);
static cl::opt<int> OptionFlowDep("swpl-flow-dep",cl::init(10), cl::ReallyHidden);
static cl::opt<bool> OptionCopyIsVirtual("swpl-copy-is-virtual",cl::init(false), cl::ReallyHidden);
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
    SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_many_insts);
    return true;
  }

  for (; I != E; --I) {
    // Callである
    if (I->isCall()) {
      printDebug(__func__, "pipeliner info:found call", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
      return true;
    }
    // fenceもしくはgnuasm命令である
    if (SWPipeliner::TII->isNonTargetMI4SWPL(*I)) {
      printDebug(__func__, "pipeliner info:found non-target-inst or gnuasm", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
      return true;
    }
    // volatile属性を含む命令である
    for (MachineMemOperand *MMO : I->memoperands()) {
      if (MMO->isVolatile()) {
        printDebug(__func__, "pipeliner info:found volataile operand", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
        return true;
      }
    }
    /* CCを更新する命令が複数出現した。 */
    if (CompMI && hasRegisterImplicitDefOperand (&*I, AArch64::NZCV)) {
      printDebug(__func__, "pipeliner info:multi-defoperand==NZCV", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_multiple_inst_update_CCR);
      return true;
    }
    /* FPCRを更新する命令が出現した。 */
    if (hasRegisterImplicitDefOperand (&*I, AArch64::FPCR)) {
      printDebug(__func__, "pipeliner info:defoperand==FPCR", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_inst_update_FPCR);
      return true;
    }
    /* CCを参照する命令が複数出現した。 */
    if (BccMI && I->hasRegisterImplicitUseOperand(AArch64::NZCV)) {
      printDebug(__func__, "pipeliner info:multi-refoperand==NZCV", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_multiple_inst_reference_CCR);
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
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_loop_shape);
        return true;
      }
    } else if (BccMI && isCompMI(&*I, CC)) {
      /* Bccが先に見つかっていて、かつループ終了条件比較命令が見つかった。 */
      if (CompMI) {
        /* ループ終了条件比較命令が複数見つかった。 */
        printDebug(__func__, "pipeliner info:not found SUBSXri", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_branch_not_for_loop);
        return true;
      }
      const auto phiIter=SWPipeliner::MRI->def_instr_begin(I->getOperand(1).getReg());
      if (!phiIter->isPHI()) {
        /* 正規化されたループ制御変数がない（PHIとSUBの間に命令が存在する）。 */
        printDebug(__func__, "pipeliner info:not found induction var", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_loop_shape);
        return true;
      }
      CompMI = &*I;
    }
    if ((!I->memoperands_empty()) && (--mem_counter)<=0) {
      printDebug(__func__, "pipeliner info:over mem limit num", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_many_memory_insts);
      return true;
    }
  }
  if (!(BccMI && CompMI)) {
    printDebug(__func__, "pipeliner info:not found (BCC || SUBSXri)", L);
    SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_loop_shape);
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
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    // fenceもしくはgnuasm命令である
    if (SWPipeliner::TII->isNonTargetMI4SWPL(*I)) {
      printDebug(__func__, "pipeliner info:found non-target-inst or gnuasm", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
      return true; // 対象でない
    }
    // volatile属性を含む命令である
    for (MachineMemOperand *MMO : I->memoperands()) {
      if (MMO->isVolatile()) {
        printDebug(__func__, "pipeliner info:found volataile operand", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
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
    SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_loop_shape);
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

bool AArch64InstrInfo::isNonScheduleInstr(MachineLoop &L) const {
  MachineBasicBlock *LoopMBB = L.getTopBlock();

  bool DefMI = false;
  bool UseMI = false;

  for (auto &mi:*LoopMBB) {
    // Call
    if (mi.isCall()) {
      printDebug(__func__, "pipeliner info:found call", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
      return true;
    }
    // fence or gnuasm command
    if (SWPipeliner::TII->isNonTargetMI4SWPL(mi)) {
      printDebug(__func__, "pipeliner info:found non-target-inst or gnuasm", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
      return true;
    }
    // an instruction that includes volatile attribute
    for (MachineMemOperand *MMO : mi.memoperands()) {
      if (MMO->isVolatile()) {
        printDebug(__func__, "pipeliner info:found volataile operand", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_inst);
        return true;
      }
    }
    /* Multiple instructions to update CC appeared */
    if (hasRegisterImplicitDefOperand (&mi, AArch64::NZCV)) {
      if (DefMI) {
        printDebug(__func__, "pipeliner info:multi-defoperand==NZCV", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_multiple_inst_update_CCR);
        return true;
      }
      DefMI = true;
    }
    /* An instruction to update the FPCR has appeared */
    if (hasRegisterImplicitDefOperand (&mi, AArch64::FPCR)) {
      printDebug(__func__, "pipeliner info:defoperand==FPCR", L);
      SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_inst_update_FPCR);
      return true;
    }
    /* Multiple instructions that reference CC appeared */
    if (mi.hasRegisterImplicitUseOperand(AArch64::NZCV)) {
      if (UseMI) {
        printDebug(__func__, "pipeliner info:multi-refoperand==NZCV", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_multiple_inst_reference_CCR);
        return true;
      }
      UseMI = true;
    }  
  }
  return false;
}

bool AArch64InstrInfo::isNonNormalizeLoop(MachineLoop &L) const{
  MachineBasicBlock *LoopMBB = L.getTopBlock();

  AArch64CC::CondCode _NE = AArch64CC::NE;
  AArch64CC::CondCode _GE = AArch64CC::GE;
  MachineBasicBlock::iterator I = LoopMBB->getFirstTerminator();
  MachineBasicBlock::iterator E = LoopMBB->getFirstNonDebugInstr();
  MachineInstr *BccMI = nullptr;
  MachineInstr *CompMI = nullptr;
  AArch64CC::CondCode CC;

  for (; I != E; --I) {
    /* Capture loop branch instructions */
    if (I->getOpcode() == AArch64::Bcc
        && I->hasRegisterImplicitUseOperand(AArch64::NZCV)) {
      BccMI = &*I;
      CC = (AArch64CC::CondCode)BccMI->getOperand(0).getImm();
      /* CC comparison type is not applicable */
      if (CC != _NE && CC != _GE) {
        printDebug(__func__, "pipeliner info:BCC condition!=NE && GE", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_loop_shape);
        return true;
      }
    } else if (BccMI && isCompMI(&*I, CC)) {
      /* Bcc was found first, and the loop end condition comparison instruction was found */
      if (CompMI) {
        /* Multiple loop termination condition comparison instructions were found */
        printDebug(__func__, "pipeliner info:not found SUBSXri", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_branch_not_for_loop);
        return true;
      }
      const auto phiIter=SWPipeliner::MRI->def_instr_begin(I->getOperand(1).getReg());
      if (!phiIter->isPHI()) {
        /* No normalized loop control variables (instruction exists between PHI and SUB */
        printDebug(__func__, "pipeliner info:not found induction var", L);
        SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_loop_shape);
        return true;
      }
      CompMI = &*I;
    }    
  }
  if (!(BccMI && CompMI)) {
    printDebug(__func__, "pipeliner info:not found (BCC || SUBSXri)", L);
    SWPipeliner::setRemarkMissedReason(SWPipeliner::MsgID_swpl_not_covered_loop_shape);
    return true;
  }
  return false;
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

using P_=AArch64SwplSchedA64FX::PortKind;

#define LATENCY_NA 1

unsigned AArch64SwplSchedA64FX::VectorLength;

/// INT_OP
static StmPipeline RES_INT_OP_001_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_001_02 = {{0}, {P_::EXB}};
static StmPipeline RES_INT_OP_001_03 = {{0}, {P_::EAGA}};
static StmPipeline RES_INT_OP_001_04 = {{0}, {P_::EAGB}};
static StmPipeline RES_INT_OP_002_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_002_02 = {{0}, {P_::EXB}};
static StmPipeline RES_INT_OP_003_01 = {{0, 1}, {P_::EXA, P_::EXA}};
static StmPipeline RES_INT_OP_003_02 = {{0, 1}, {P_::EXB, P_::EXB}};
static StmPipeline RES_INT_OP_004_01 = {{0, 1, 2}, {P_::EXA, P_::EXA, P_::EXA}};
static StmPipeline RES_INT_OP_004_02 = {{0, 1, 2}, {P_::EXB, P_::EXB, P_::EXB}};
static StmPipeline RES_INT_OP_005_01 = {{0}, {P_::EAGA}};
static StmPipeline RES_INT_OP_005_02 = {{0}, {P_::EAGB}};
static StmPipeline RES_INT_OP_006_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_006_02 = {{0}, {P_::EXB}};
static StmPipeline RES_INT_OP_007_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_008_01 = {{0, 5}, {P_::EXA, P_::EXA}};

/// INT_LD
static StmPipeline RES_INT_LD_001_01 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGA, P_::EXA, P_::LSU1, P_::EXA_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_02 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGA, P_::EXB, P_::LSU1, P_::EXB_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_03 = {
  {0, 0, 1, 2, 5}, 
  {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_04 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_05 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGB, P_::EXA, P_::LSU1, P_::EXA_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_001_06 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGB, P_::EXB, P_::LSU1, P_::EXB_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_001_07 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGB, P_::EAGA, P_::LSU1, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_001_08 = {
  {0, 0, 1, 2, 5}, 
  {P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_001_09 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGA, P_::EXA, P_::LSU2, P_::EXA_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_10 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGA, P_::EXB, P_::LSU2, P_::EXB_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_11 = {
  {0, 0, 1, 2, 5}, 
  {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_12 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_INT_LD_001_13 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGB, P_::EXA, P_::LSU2, P_::EXA_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_001_14 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGB, P_::EXB, P_::LSU2, P_::EXB_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_001_15 = {
  {0, 0, 0, 1, 5}, 
  {P_::EAGB, P_::EAGA, P_::LSU2, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_001_16 = {
  {0, 0, 1, 2, 5}, 
  {P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_INT_LD_002_01 = {{0, 0, 5}, {P_::EAGA,P_::LSU1,P_::EAGA_C}};
static StmPipeline RES_INT_LD_002_02 = {{0, 0, 5}, {P_::EAGB,P_::LSU1,P_::EAGB_C}};
static StmPipeline RES_INT_LD_002_03 = {{0, 0, 5}, {P_::EAGA,P_::LSU2,P_::EAGA_C}};
static StmPipeline RES_INT_LD_002_04 = {{0, 0, 5}, {P_::EAGB,P_::LSU2,P_::EAGB_C}};

/// INT_ST
static StmPipeline RES_INT_ST_001_01 = {{0, 0, 0, 0}, {P_::EAGA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_001_02 = {{0, 0, 0, 0}, {P_::EAGB, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_002_01 = {
  {0, 0, 0, 0, 0},
  {P_::EAGA, P_::FLA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_002_02 = {
  {0, 0, 0, 0, 0},
  {P_::EAGA, P_::FLA, P_::EXB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_002_03 = {
  {0, 0, 0, 0, 0},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_002_04 = {
  {0, 0, 0, 0, 0},
  {P_::EAGB, P_::FLA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_002_05 = {
  {0, 0, 0, 0, 0},
  {P_::EAGB, P_::FLA, P_::EXB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_002_06 = {
  {0, 0, 0, 0, 0},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU1, P_::LSU2}};

/// SIMDFP_SVE_OP
static StmPipeline RES_SIMDFP_SVE_OP_001_01 = {{0}, {P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_001_02 = {{0}, {P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_002_01 = {{0, 4}, {P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_002_02 = {{0, 4}, {P_::FLB, P_::FLB_C}};
static StmPipeline RES_SIMDFP_SVE_OP_003_01 = {{0}, {P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_004_01 = {{0, 6}, {P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_005_01 = {{0, 4, 8}, {P_::EXA, P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_006_01 = {{0, 6}, {P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_006_02 = {{0, 6}, {P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_007_01 = {{0, 4}, {P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_008_01 = {{0, 1, 1, 5}, {P_::FLA, P_::FLA_C, P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_009_01 = {
  {0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42},
  {P_::FLA, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E}};
static StmPipeline RES_SIMDFP_SVE_OP_010_01 = {
  {0,  1,  7,  9,  13, 18, 19, 25, 27, 31, 36, 37,  43,  45,  49, 54,
    55, 61, 63, 67, 72, 73, 79, 81, 85, 90, 99, 108, 117, 126, 135},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_010_02 = {
  {0,  0,  6,  9,  12, 18, 18, 24, 27, 30, 36, 36,  42,  45,  48, 54,
    54, 60, 63, 66, 72, 72, 78, 81, 84, 90, 99, 108, 117, 126, 135},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB, P_::FLB, P_::FLB, P_::FLB, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_011_01 = {{0, 4}, {P_::EXA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_012_01 = {{0, 1, 9}, {P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_012_02 = {{0, 1, 9}, {P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_012_03 = {{0, 0, 9}, {P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_012_04 = {{0, 0, 9}, {P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_013_01 = {
  {0, 1, 7, 9, 13, 18, 27},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_013_02 = {
  {0, 0, 6, 9, 12, 18, 27},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_014_01 = {
  {0, 1, 7, 9, 13, 18, 19, 25, 27, 31, 36, 37, 45, 54, 63},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_014_02 = {
  {0, 0, 6, 9, 12, 18, 18, 24, 27, 30, 36, 36, 45, 54, 63},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_015_01 = {
  {0, 1, 7},
  {P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_015_02 = {
  {0, 1, 7},
  {P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_016_01 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_016_02 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_016_03 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_016_04 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_016_05 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_016_06 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_016_07 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_016_08 = {
  {0, 4, 10, 19, 25}, 
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_01 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_02 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_03 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_04 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_05 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_06 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_07 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_08 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_09 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_10 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_11 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_12 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_13 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_14 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_15 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_16 = {
  {0, 4, 10, 19, 25, 34, 40}, 
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_01 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_02 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_03 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_04 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_05 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_06 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_07 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_08 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_09 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_10 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_11 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_12 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_13 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_14 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_15 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_16 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_17 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_18 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_19 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_20 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_21 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_22 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_23 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_24 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_25 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_26 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_27 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_28 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_29 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_30 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_31 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_32 = {
  {0, 4, 10, 19, 25, 34, 40, 49, 55},
  {P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_019_01 = {
  {0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
    63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79},
  {P_::FLA, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E}};
static StmPipeline RES_SIMDFP_SVE_OP_020_01 = {
  {0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
    14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
    28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
    42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,
    56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
    70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,
    84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,
    98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
    126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
    140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153},
  {P_::FLA, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E}};
static StmPipeline RES_SIMDFP_SVE_OP_021_01 = {{0, 6}, {P_::FLB, P_::FLB_C}};
static StmPipeline RES_SIMDFP_SVE_OP_022_01 = {
  {0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28},
  {P_::FLA, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E}};
static StmPipeline RES_SIMDFP_SVE_OP_023_01 = {
  {0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51},
  {P_::FLA, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E}};
static StmPipeline RES_SIMDFP_SVE_OP_024_01 = {
  {0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96, 97},
  {P_::FLA, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E, P_::FLA_E,
    P_::FLA_E, P_::FLA_E
    }};
static StmPipeline RES_SIMDFP_SVE_OP_025_01 = {
  {0, 4}, 
  {P_::EXA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_026_01 = {
  {0, 4, 10}, 
  {P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_026_02 = {
  {0, 4, 10}, 
  {P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_026_03 = {
  {0, 4, 10}, 
  {P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_026_04 = {
  {0, 4, 10}, 
  {P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_027_01 = {
  {0, 10}, 
  {P_::FLA, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_OP_027_02 = {
  {0, 10}, 
  {P_::FLA, P_::EAGB}};

/// SIMDFP_SVE_LD
static StmPipeline RES_SIMDFP_SVE_LD_001_01 = {
  {0, 0},
  {P_::EAGA, P_::LSU1}};
static StmPipeline RES_SIMDFP_SVE_LD_001_02 = {
  {0, 0},
  {P_::EAGB, P_::LSU1}};
static StmPipeline RES_SIMDFP_SVE_LD_001_03 = {
  {0, 0},
  {P_::EAGA, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_LD_001_04 = {
  {0, 0},
  {P_::EAGB, P_::LSU2}};

static StmPipeline RES_SIMDFP_SVE_LD_002_01 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EXA, P_::LSU1, P_::EXA_C,P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_02 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EXB, P_::LSU1, P_::EXB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_03 = {
  {0, 0, 1, 2, 8},
  {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_04 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_05 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EXA, P_::LSU1, P_::EXA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_06 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EXB, P_::LSU1, P_::EXB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_07 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EAGA, P_::LSU1, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_08 = {
  {0, 0, 1, 2, 8},
  {P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_09 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EXA, P_::LSU2, P_::EXA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_10 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EXB, P_::LSU2, P_::EXB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_11 = {
  {0, 0, 1, 2, 8},
  {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_12 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_13 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EXA, P_::LSU2, P_::EXA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_14 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EXB, P_::LSU2, P_::EXB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_15 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EAGA, P_::LSU2, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_16 = {
  {0, 0, 1, 2, 8},
  {P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_003_01 = {
  {0, 0, 8}, {P_::EAGA, P_::LSU1, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_003_02 = {
  {0, 0, 8}, {P_::EAGB, P_::LSU1, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_003_03 = {
  {0, 0, 8}, {P_::EAGA, P_::LSU2, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_003_04 = {
  {0, 0, 8}, {P_::EAGB, P_::LSU2, P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_004_01 = {
  {0, 0, 4, 8, 8, 9, 9},
  {P_::EXA, P_::LSU1, P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_004_02 = {
  {0, 0, 4, 8, 8, 9, 9},
  {P_::EXA, P_::LSU2, P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};

static StmPipeline RES_SIMDFP_SVE_LD_005_01 = {
  {0, 0, 4, 4, 5, 5},
  {P_::FLA, P_::LSU1, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_005_02 = {
  {0, 0, 4, 4, 5, 5},
  {P_::FLA, P_::LSU2, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};

static StmPipeline RES_SIMDFP_SVE_LD_006_01 = {
  {0, 0, 1, 2, 8},
  {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_02 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_03 = {
  {0, 0, 1, 2, 8},
  {P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_04 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EAGA, P_::LSU1, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_05 = {
  {0, 0, 1, 2, 8},
  {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_06 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_07 = {
  {0, 0, 1, 2, 8},
  {P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_08 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EAGA, P_::LSU2, P_::EAGA_C, P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_007_01 = {
  {0, 0, 0, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_007_02 = {
  {0, 0, 0, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_007_03 = {
  {0, 0, 0, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU2, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_007_04 = {
  {0, 0, 0, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU2, P_::FLA_C,P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_008_01 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::EAGA, P_::EAGA_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_02 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU1, P_::EAGA_C, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_03 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU1, P_::EAGB_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_04 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::EAGB, P_::EAGB_C, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_05 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU2, P_::EAGA, P_::EAGA_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_06 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU2, P_::EAGA_C, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_07 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU2, P_::EAGB_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_08 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU2, P_::EAGB, P_::EAGB_C, P_::FLA_C, P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_009_01 = {
  {0, 0, 0},
  {P_::EAGA, P_::EAGB, P_::LSU1}};
static StmPipeline RES_SIMDFP_SVE_LD_009_02 = {
  {0, 0, 0},
  {P_::EAGA, P_::EAGB, P_::LSU2}}; 
static StmPipeline RES_SIMDFP_SVE_LD_010_01 = {
  {0, 0, 1, 1},
  {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_010_02 = {
  {0, 0, 1, 1},
  {P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_010_03 = {
  {0, 0, 1, 1},
  {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_010_04 = {
  {0, 0, 1, 1},
  {P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGB}};

static StmPipeline RES_SIMDFP_SVE_LD_011_01 = {
  {0, 0, 4, 5, 9, 9, 10, 10, 11, 11, 12, 12},
  {P_::EXA, P_::LSU1, P_::FLA, P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA,
    P_::EAGB, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_011_02 = {
  {0, 0, 4, 5, 9, 9, 10, 10, 11, 11, 12, 12},
  {P_::EXA, P_::LSU2, P_::FLA, P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA,
    P_::EAGB, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_012_01 = {
  {0, 0, 0, 1},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_012_02 = {
  {0, 0, 0, 1},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_012_03 = {
  {0, 0, 0, 1},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_012_04 = {
  {0, 0, 0, 1},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_013_01 = {
  {0, 0, 0, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_013_02 = {
  {0, 0, 0, 1, 1},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_013_03 = {
  {0, 0, 0, 1, 1},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_013_04 = {
  {0, 0, 0, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_013_05 = {
  {0, 0, 0, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_013_06 = {
  {0, 0, 0, 1, 1},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_013_07 = {
  {0, 0, 0, 1, 1},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_013_08 = {
  {0, 0, 0, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_014_01 = {
  {0, 0, 0, 1, 1},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_014_02 = {
  {0, 0, 0, 1, 1},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_015_01 = {
  {0, 0, 1, 1, 2, 2},
  {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_015_02 = {
  {0, 0, 1, 1, 2, 2},
  {P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_015_03 = {
  {0, 0, 1, 1, 2, 2},
  {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_015_04 = {
  {0, 0, 1, 1, 2, 2},
  {P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_016_01 = {
  {0, 0, 0, 1, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGB, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_016_02 = {
  {0, 0, 0, 1, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGB, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_016_03 = {
  {0, 0, 0, 1, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGB, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_016_04 = {
  {0, 0, 0, 1, 1, 2},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGB, P_::EAGB}};

/// SIMDFP_SVE_ST
static StmPipeline RES_SIMDFP_SVE_ST_001_01 = {
  {0, 0, 0, 0},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_001_02 = {
  {0, 0, 0, 0},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::LSU2}};

static StmPipeline RES_SIMDFP_SVE_ST_002_01 = {
  {0, 0, 0, 0, 0},
  {P_::EAGA, P_::FLA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_02 = {
  {0, 0, 0, 0, 0},
  {P_::EAGA, P_::FLA, P_::EXB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_03 = {
  {0, 0, 0, 0, 0},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_04 = {
  {0, 0, 0, 0, 0},
  {P_::EAGB, P_::FLA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_05 = {
  {0, 0, 0, 0, 0},
  {P_::EAGB, P_::FLA, P_::EXB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_06 = {
  {0, 0, 0, 0, 0},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU1, P_::LSU2}};

static StmPipeline RES_SIMDFP_SVE_ST_003_01 = {
  {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 9, 10, 10, 11, 11},
  {P_::EXA, P_::LSU1, P_::LSU2, P_::EXA, P_::EXA, P_::EXA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB,
    P_::EAGA, P_::EAGB}};

static StmPipeline RES_SIMDFP_SVE_ST_004_01 = {
  {0, 0, 0, 0, 1, 1},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_ST_004_02 = {
  {0, 0, 0, 0, 0, 1},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU1, P_::LSU2,  P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_ST_004_03 = {
  {0, 0, 0, 0, 0, 1},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU1, P_::LSU2,  P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_ST_004_04 = {
  {0, 0, 0, 0, 1, 1},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGB, P_::FLA}};

static StmPipeline RES_SIMDFP_SVE_ST_005_01 = {
  {0, 0, 0, 0, 1, 1, 1},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGA, P_::FLA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_ST_005_02 = {
  {0, 0, 0, 0, 1, 1, 1},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGA, P_::FLA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_ST_006_01 = {
  {0, 0, 0, 1,  2,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  8,
   9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15},
  {P_::EXA,  P_::LSU1, P_::LSU2, P_::EXA,  P_::EXA,  P_::EXA,  P_::FLA,
   P_::EXA,  P_::FLA,  P_::EXA,  P_::FLA,  P_::EXA,  P_::FLA,  P_::EXA,
   P_::EAGA, P_::EAGB, P_::FLA,  P_::EAGA, P_::EAGB, P_::FLA,  P_::EAGA,
   P_::EAGB, P_::FLA,  P_::EAGA, P_::EAGB, P_::FLA,  P_::EAGA, P_::EAGB,
   P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};

/// SVE_CMP_INST
static StmPipeline RES_SVE_CMP_INST_001_01 = {{0, 4}, {P_::FLA, P_::FLA_C}};
static StmPipeline RES_SVE_CMP_INST_002_01 = {{0, 0, 4}, {P_::PRX, P_::FLA, P_::FLA_C}};

/// PREDICATE_OP
static StmPipeline RES_PREDICATE_OP_001_01 = {{0}, {P_::PRX}};
static StmPipeline RES_PREDICATE_OP_002_01 = {{0, 1}, {P_::EXA, P_::PRX}};

/// 利用資源IDと利用資源情報のmap
std::map<AArch64SwplSchedA64FX::ResourceID, AArch64SwplSchedA64FX::SchedResource>
  AArch64SwplSchedA64FX::ResInfo={
  {MI_INT_OP_001,  /// Pipeline:EX* | EAG*  Latency:1
    {{&RES_INT_OP_001_01, &RES_INT_OP_001_02, 
      &RES_INT_OP_001_03, &RES_INT_OP_001_04},1}},
  {MI_INT_OP_002,  /// Pipeline:EX*  Latency:1
    {{&RES_INT_OP_002_01, &RES_INT_OP_002_02}, 1}},
  {MI_INT_OP_003,  /// Pipeline:(EXA + EXA) | (EXB + EXB)  Latency:1+1  Blocking:P
    {{&RES_INT_OP_003_01, &RES_INT_OP_003_02}, 2}},
  {MI_INT_OP_004,  /// Pipeline:(EXA + EXA) | (EXB + EXB)  Latency:2+1  Blocking:P
    {{&RES_INT_OP_004_01, &RES_INT_OP_004_02}, 3}},
  {MI_INT_OP_005,  /// Pipeline:EAG*  Latency:NA
    {{&RES_INT_OP_005_01, &RES_INT_OP_005_02}, LATENCY_NA}},
  {MI_INT_OP_006,  /// Pipeline:EX*  Latency:2
    {{&RES_INT_OP_006_01, &RES_INT_OP_006_02}, 2}},
  {MI_INT_OP_007,  /// Pipeline:EXA  Latency:5
    {{&RES_INT_OP_007_01}, 5}},
  {MI_INT_OP_008,  /// Pipeline:EXA / EXA  Latency:5 + [1]1
    {{&RES_INT_OP_008_01}, 6}},
  {MI_INT_LD_001,  /// Pipeline:EAG* / EX*| EAG*  Latency:5 / 1
    {{&RES_INT_LD_001_01, &RES_INT_LD_001_02,
      &RES_INT_LD_001_03, &RES_INT_LD_001_04,
      &RES_INT_LD_001_05, &RES_INT_LD_001_06,
      &RES_INT_LD_001_07, &RES_INT_LD_001_08,
      &RES_INT_LD_001_09, &RES_INT_LD_001_10,
      &RES_INT_LD_001_11, &RES_INT_LD_001_12,
      &RES_INT_LD_001_13, &RES_INT_LD_001_14,
      &RES_INT_LD_001_15, &RES_INT_LD_001_16}, 5}},
  {MI_INT_LD_002,  /// Pipeline:EAG*  Latency:5
    {{&RES_INT_LD_002_01, &RES_INT_LD_002_02,
      &RES_INT_LD_002_03, &RES_INT_LD_002_04}, 5}},
  {MI_INT_ST_001,  /// Pipeline:EAG*, EXA  Latency:NA, NA
    {{&RES_INT_ST_001_01, &RES_INT_ST_001_02}, LATENCY_NA}},
  {MI_INT_ST_002,  /// Pipeline:EAG*, FLA / EX*| EAG*  Latency:NA,NA / 1
    {{&RES_INT_ST_002_01, &RES_INT_ST_002_02,
      &RES_INT_ST_002_03, &RES_INT_ST_002_04,
      &RES_INT_ST_002_05, &RES_INT_ST_002_06},
    LATENCY_NA}},
  {MI_SIMDFP_SVE_OP_001,  /// Pipeline:FL*  Latency:9
    {{&RES_SIMDFP_SVE_OP_001_01, &RES_SIMDFP_SVE_OP_001_02}, 9}},
  {MI_SIMDFP_SVE_OP_002,  /// Pipeline:FL*  Latency:4
    {{&RES_SIMDFP_SVE_OP_002_01, &RES_SIMDFP_SVE_OP_002_02}, 4}},
  {MI_SIMDFP_SVE_OP_003,  /// Pipeline:FLA   Latency:9
    {{&RES_SIMDFP_SVE_OP_003_01}, 9}},
  {MI_SIMDFP_SVE_OP_004,  /// Pipeline:FLA   Latency:6
    {{&RES_SIMDFP_SVE_OP_004_01}, 6}},
  {MI_SIMDFP_SVE_OP_005,  /// Pipeline:EXA + NULL + FLA  Latency:1+3+4
    {{&RES_SIMDFP_SVE_OP_005_01}, 8}},
  {MI_SIMDFP_SVE_OP_006,  /// Pipeline:FLA / FL*  Latency:6 / [1]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_006_01, &RES_SIMDFP_SVE_OP_006_02}, 15, true}},
  {MI_SIMDFP_SVE_OP_007,  /// Pipeline:FLA  Latency:4
    {{&RES_SIMDFP_SVE_OP_007_01}, 4}},
  {MI_SIMDFP_SVE_OP_008,  /// Pipeline:FLA+FLA  Latency:1+4
    {{&RES_SIMDFP_SVE_OP_008_01}, 5}},
  {MI_SIMDFP_SVE_OP_009,  /// Pipeline:FLA  Latency:43  Blocking:E
    {{&RES_SIMDFP_SVE_OP_009_01}, 43}},
  {MI_SIMDFP_SVE_OP_010,  /// Pipeline:FL* / FLA / (FL* / FLA) x 14 / FL*  Latency:9 / 6 / ([1,2]9 / [2]6) x 14 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_010_01, &RES_SIMDFP_SVE_OP_010_02}, 144, true}},
  {MI_SIMDFP_SVE_OP_011,  /// Pipeline:EXA + NULL + FLA  Latency:1+3+9
    {{&RES_SIMDFP_SVE_OP_011_01}, 13}},
  {MI_SIMDFP_SVE_OP_012,  /// Pipeline:FL* / FLA / FL*  Latency:9 / 6 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_012_01, &RES_SIMDFP_SVE_OP_012_02, 
      &RES_SIMDFP_SVE_OP_012_03, &RES_SIMDFP_SVE_OP_012_04}, 18, true}},
  {MI_SIMDFP_SVE_OP_013,  /// Pipeline:FL* / FLA / (FL* / FLA) x 2 / FL*  Latency:9 / 6 / ([1,2]9 / [2]6) x 2 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_013_01, &RES_SIMDFP_SVE_OP_013_02}, 36, true}},
  {MI_SIMDFP_SVE_OP_014,  /// Pipeline:FL* / FLA / (FL* / FLA) x 6 / FL*  Latency:9 / 6 / ([1,2]9 / [2]6) x 6 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_014_01, &RES_SIMDFP_SVE_OP_014_02}, 72, true}},
  {MI_SIMDFP_SVE_OP_015,  /// Pipeline:FLA / FLA / FL*  Latency:6 / 6 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_015_01, &RES_SIMDFP_SVE_OP_015_02}, 16, true}},
  {MI_SIMDFP_SVE_OP_016,  /// Pipeline:FL* / (FLA / FL*) x 2  Latency:4 / ([1]6 / [1,2]9) x 2  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_016_01, &RES_SIMDFP_SVE_OP_016_02,
      &RES_SIMDFP_SVE_OP_016_03, &RES_SIMDFP_SVE_OP_016_04,
      &RES_SIMDFP_SVE_OP_016_05, &RES_SIMDFP_SVE_OP_016_06,
      &RES_SIMDFP_SVE_OP_016_07, &RES_SIMDFP_SVE_OP_016_08},
    34, true}},
  {MI_SIMDFP_SVE_OP_017,  /// Pipeline:FL* / (FLA / FL*) x 3  Latency:4 / ([1]6 / [1,2]9) x 3  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_017_01, &RES_SIMDFP_SVE_OP_017_02,
      &RES_SIMDFP_SVE_OP_017_03, &RES_SIMDFP_SVE_OP_017_04,
      &RES_SIMDFP_SVE_OP_017_05, &RES_SIMDFP_SVE_OP_017_06,
      &RES_SIMDFP_SVE_OP_017_07, &RES_SIMDFP_SVE_OP_017_08,
      &RES_SIMDFP_SVE_OP_017_09, &RES_SIMDFP_SVE_OP_017_10,
      &RES_SIMDFP_SVE_OP_017_11, &RES_SIMDFP_SVE_OP_017_12,
      &RES_SIMDFP_SVE_OP_017_13, &RES_SIMDFP_SVE_OP_017_14,
      &RES_SIMDFP_SVE_OP_017_15, &RES_SIMDFP_SVE_OP_017_16},
    49, true}},
  {MI_SIMDFP_SVE_OP_018,  /// Pipeline:FL* / (FLA / FL*) x 4  Latency:4 / ([1]6 / [1,2]9) x 4  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_018_01, &RES_SIMDFP_SVE_OP_018_02,
      &RES_SIMDFP_SVE_OP_018_03, &RES_SIMDFP_SVE_OP_018_04,
      &RES_SIMDFP_SVE_OP_018_05, &RES_SIMDFP_SVE_OP_018_06,
      &RES_SIMDFP_SVE_OP_018_07, &RES_SIMDFP_SVE_OP_018_08,
      &RES_SIMDFP_SVE_OP_018_09, &RES_SIMDFP_SVE_OP_018_10,
      &RES_SIMDFP_SVE_OP_018_11, &RES_SIMDFP_SVE_OP_018_12,
      &RES_SIMDFP_SVE_OP_018_13, &RES_SIMDFP_SVE_OP_018_14,
      &RES_SIMDFP_SVE_OP_018_15, &RES_SIMDFP_SVE_OP_018_16,
      &RES_SIMDFP_SVE_OP_018_17, &RES_SIMDFP_SVE_OP_018_18,
      &RES_SIMDFP_SVE_OP_018_19, &RES_SIMDFP_SVE_OP_018_20,
      &RES_SIMDFP_SVE_OP_018_21, &RES_SIMDFP_SVE_OP_018_22,
      &RES_SIMDFP_SVE_OP_018_23, &RES_SIMDFP_SVE_OP_018_24,
      &RES_SIMDFP_SVE_OP_018_25, &RES_SIMDFP_SVE_OP_018_26,
      &RES_SIMDFP_SVE_OP_018_27, &RES_SIMDFP_SVE_OP_018_28,
      &RES_SIMDFP_SVE_OP_018_29, &RES_SIMDFP_SVE_OP_018_30,
      &RES_SIMDFP_SVE_OP_018_31, &RES_SIMDFP_SVE_OP_018_32},
    64, true}},
  {MI_SIMDFP_SVE_OP_019,  /// Pipeline:FLA  Latency:80  Blocking:E
    {{&RES_SIMDFP_SVE_OP_019_01}, 80}},
  {MI_SIMDFP_SVE_OP_020,  /// Pipeline:FLA  Latency:154  Blocking:E
    {{&RES_SIMDFP_SVE_OP_020_01}, 154}},
  {MI_SIMDFP_SVE_OP_021,  /// Pipeline:FLB  Latency:6
    {{&RES_SIMDFP_SVE_OP_021_01}, 6}},
  {MI_SIMDFP_SVE_OP_022,  /// Pipeline:FLA  Latency:29  Blocking:E
    {{&RES_SIMDFP_SVE_OP_022_01}, 29}},
  {MI_SIMDFP_SVE_OP_023,  /// Pipeline:FLA  Latency:52  Blocking:E
    {{&RES_SIMDFP_SVE_OP_023_01}, 52}},
  {MI_SIMDFP_SVE_OP_024,  /// Pipeline:FLA  Latency:98  Blocking:E
    {{&RES_SIMDFP_SVE_OP_024_01}, 98}},
  {MI_SIMDFP_SVE_OP_025,  /// Pipeline:EXA + NULL + FLA  Latency:1+3+6
    {{&RES_SIMDFP_SVE_OP_025_01}, 10}},
  {MI_SIMDFP_SVE_OP_026,  /// Pipeline:FL* / FLA / FL*  Latency:4 / [1]6 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_026_01, &RES_SIMDFP_SVE_OP_026_02,
      &RES_SIMDFP_SVE_OP_026_03, &RES_SIMDFP_SVE_OP_026_04},
    19, true}},
  {MI_SIMDFP_SVE_OP_027,  /// Pipeline:FLA + NULL ; EAG*  Latency:9+1 ; 15
    {{&RES_SIMDFP_SVE_OP_027_01, &RES_SIMDFP_SVE_OP_027_02},
    25}},
  {MI_SIMDFP_SVE_LD_001,  /// Pipeline:EAG*, FLA  Latency:11
    {{&RES_SIMDFP_SVE_LD_001_01, &RES_SIMDFP_SVE_LD_001_02,
      &RES_SIMDFP_SVE_LD_001_03, &RES_SIMDFP_SVE_LD_001_04},
    11}},
  {MI_SIMDFP_SVE_LD_002,  /// Pipeline:EAG*, FLA / EX*| EAG*  Latency:8/1
    {{&RES_SIMDFP_SVE_LD_002_01, &RES_SIMDFP_SVE_LD_002_02,
      &RES_SIMDFP_SVE_LD_002_03, &RES_SIMDFP_SVE_LD_002_04,
      &RES_SIMDFP_SVE_LD_002_05, &RES_SIMDFP_SVE_LD_002_06,
      &RES_SIMDFP_SVE_LD_002_07, &RES_SIMDFP_SVE_LD_002_08,
      &RES_SIMDFP_SVE_LD_002_09, &RES_SIMDFP_SVE_LD_002_10,
      &RES_SIMDFP_SVE_LD_002_11, &RES_SIMDFP_SVE_LD_002_12,
      &RES_SIMDFP_SVE_LD_002_13, &RES_SIMDFP_SVE_LD_002_14,
      &RES_SIMDFP_SVE_LD_002_15, &RES_SIMDFP_SVE_LD_002_16},
    8}},
  {MI_SIMDFP_SVE_LD_003,  /// Pipeline:EAG*  Latency:8
    {{&RES_SIMDFP_SVE_LD_003_01, &RES_SIMDFP_SVE_LD_003_02,
      &RES_SIMDFP_SVE_LD_003_03, &RES_SIMDFP_SVE_LD_003_04},
    8}},
  {MI_SIMDFP_SVE_LD_004,  /// Pipeline:EXA + NULL + FLA +Pipe((EAGA & EAGB), 2)  Latency:1+3+4+Pipe(11, 2)
    {{&RES_SIMDFP_SVE_LD_004_01, &RES_SIMDFP_SVE_LD_004_02},
    20}},
  {MI_SIMDFP_SVE_LD_005,  /// Pipeline:FLA + Pipe((EAGA & EAGB), 2)  Latency:4+Pipe(11, 2)
    {{&RES_SIMDFP_SVE_LD_005_01, &RES_SIMDFP_SVE_LD_005_02},
    16}},
  {MI_SIMDFP_SVE_LD_006,  /// Pipeline:EAG* / EAG*  Latency:8 / 6  
    {{&RES_SIMDFP_SVE_LD_006_01, &RES_SIMDFP_SVE_LD_006_02,
      &RES_SIMDFP_SVE_LD_006_03, &RES_SIMDFP_SVE_LD_006_04,
      &RES_SIMDFP_SVE_LD_006_05, &RES_SIMDFP_SVE_LD_006_06,
      &RES_SIMDFP_SVE_LD_006_07, &RES_SIMDFP_SVE_LD_006_08},
    8}},
  {MI_SIMDFP_SVE_LD_007,  /// Pipeline:EAG* /FLA  Latency:8 / 1  Seq-decode:true
    {{&RES_SIMDFP_SVE_LD_007_01, &RES_SIMDFP_SVE_LD_007_02,
      &RES_SIMDFP_SVE_LD_007_03, &RES_SIMDFP_SVE_LD_007_04},
    8, true}},
  {MI_SIMDFP_SVE_LD_008,  /// Pipeline:EAG* /FLA / EAG*  Latency:8 / 6 / 1  Seq-decode:true
    {{&RES_SIMDFP_SVE_LD_008_01, &RES_SIMDFP_SVE_LD_008_02,
      &RES_SIMDFP_SVE_LD_008_03, &RES_SIMDFP_SVE_LD_008_04,
      &RES_SIMDFP_SVE_LD_008_05, &RES_SIMDFP_SVE_LD_008_06,
      &RES_SIMDFP_SVE_LD_008_07, &RES_SIMDFP_SVE_LD_008_08},
    8, true}},
  {MI_SIMDFP_SVE_LD_009,  /// Pipeline:EAG* /EAG*  Latency:11 / 11
    {{&RES_SIMDFP_SVE_LD_009_01, &RES_SIMDFP_SVE_LD_009_02},
    11}},
  {MI_SIMDFP_SVE_LD_010,  /// Pipeline:EAG* /EAG*/ EAG*  Latency:1 / [1]11 / [2]11
    {{&RES_SIMDFP_SVE_LD_010_01, &RES_SIMDFP_SVE_LD_010_02,
      &RES_SIMDFP_SVE_LD_010_03, &RES_SIMDFP_SVE_LD_010_04},
    12}},
  {MI_SIMDFP_SVE_LD_011,  /// Pipeline:EXA + NULL + FLA + FLA + Pipe((EAGA & EAGB), 4)  Latency:1+3+1+4+Pipe(11, 4)
    {{&RES_SIMDFP_SVE_LD_011_01, &RES_SIMDFP_SVE_LD_011_02},
    23}},
  {MI_SIMDFP_SVE_LD_012,  /// Pipeline:EAG* / EAG* / EAG*  Latency:11 / 11 / 11
    {{&RES_SIMDFP_SVE_LD_012_01, &RES_SIMDFP_SVE_LD_012_02,
      &RES_SIMDFP_SVE_LD_012_03, &RES_SIMDFP_SVE_LD_012_04},
    12}},
  {MI_SIMDFP_SVE_LD_013,  /// Pipeline:EAG* / EAG* / EAG* / EAG*  Latency:11 / 11 / 11 / 1
    {{&RES_SIMDFP_SVE_LD_013_01, &RES_SIMDFP_SVE_LD_013_02,
      &RES_SIMDFP_SVE_LD_013_03, &RES_SIMDFP_SVE_LD_013_04,
      &RES_SIMDFP_SVE_LD_013_05, &RES_SIMDFP_SVE_LD_013_06},
    12}},
  {MI_SIMDFP_SVE_LD_014,  /// Pipeline:EAG* / EAG* / EAG* / EAG*  Latency:11 / 11 / 11 / 11
    {{&RES_SIMDFP_SVE_LD_014_01, &RES_SIMDFP_SVE_LD_014_02},
    12}},
  {MI_SIMDFP_SVE_LD_015,  /// Pipeline:EAG* / (EAG*) x 4  Latency:1 / [1/2/3/4](11) x 4
    {{&RES_SIMDFP_SVE_LD_015_01, &RES_SIMDFP_SVE_LD_015_02,
      &RES_SIMDFP_SVE_LD_015_03, &RES_SIMDFP_SVE_LD_015_04},
    13}},
  {MI_SIMDFP_SVE_LD_016,  /// Pipeline:EAG* / EAG* / EAG* / EAG* / EAG*  Latency:11 / 11 / 11 / 11 / 1
    {{&RES_SIMDFP_SVE_LD_016_01, &RES_SIMDFP_SVE_LD_016_02,
      &RES_SIMDFP_SVE_LD_016_03, &RES_SIMDFP_SVE_LD_016_04},
    12}},
  {MI_SIMDFP_SVE_ST_001,  /// Pipeline:EAG*, FLA  Latency:NA,NA
    {{&RES_SIMDFP_SVE_ST_001_01, &RES_SIMDFP_SVE_ST_001_02}, LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_002,  /// Pipeline:EAG*, FLA / EX*| EAG*  Latency:NA,NA / 1
    {{&RES_SIMDFP_SVE_ST_002_01, &RES_SIMDFP_SVE_ST_002_02,
      &RES_SIMDFP_SVE_ST_002_03, &RES_SIMDFP_SVE_ST_002_04,
      &RES_SIMDFP_SVE_ST_002_05, &RES_SIMDFP_SVE_ST_002_06},
    LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_003,  /// Pipeline:(EXA + NULL + FLA + Pipe((EAGA & EAGB), 2) / EXA + NULL + FLA) x 2  Latency:(1+3+4+Pipe(NA, 2) / 1+3+NA) x 2
    {{&RES_SIMDFP_SVE_ST_003_01}, 11 + LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_004,  /// Pipeline:EAG*, FLA / EAG*, FLA  Latency:NA, NA / NA, NA
    {{&RES_SIMDFP_SVE_ST_004_01, &RES_SIMDFP_SVE_ST_004_02,
      &RES_SIMDFP_SVE_ST_004_03, &RES_SIMDFP_SVE_ST_004_04},
      1 + LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_005,  /// Pipeline:EAG* / EAG*, FLA / EAG*, FLA  Latency:1 / NA, NA / NA, NA
    {{&RES_SIMDFP_SVE_ST_005_01, &RES_SIMDFP_SVE_ST_005_02},
      1 + LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_006,  /// Pipeline:(EXA + NULL + FLA + Pipe((EAGA & EAGB), 2) / EXA + NULL + FLA) x 4  Latency:(1+3+4+Pipe(NA, 2) / 1+3+NA) x 4
    {{&RES_SIMDFP_SVE_ST_006_01},
      15 + LATENCY_NA}},
  {MI_SVE_CMP_INST_001,  /// Pipeline:FLA  Latency:4
    {{&RES_SVE_CMP_INST_001_01}, 4}},
  {MI_SVE_CMP_INST_002,  /// Pipeline:PRX, FLA  Latency:4
    {{&RES_SVE_CMP_INST_002_01}, 4}},
  {MI_PREDICATE_OP_001,  /// Pipeline:PRX  Latency:3
    {{&RES_PREDICATE_OP_001_01}, 3}},
  {MI_PREDICATE_OP_002,  /// Pipeline:EXA + PRX  Latency:1+3
    {{&RES_PREDICATE_OP_002_01}, 4}}
  };

/// Map of commands and used resource IDs
llvm::DenseMap<unsigned int, AArch64SwplSchedA64FX::ResourceID> AArch64SwplSchedA64FX::MIOpcodeInfo{
  // Base
  {AArch64::ADDSXri, MI_INT_OP_002},
  {AArch64::ADDWri, MI_INT_OP_001},
  {AArch64::ADDWrr, MI_INT_OP_001},
  {AArch64::ADDXri, MI_INT_OP_001},
  {AArch64::ADDXrr, MI_INT_OP_001},
  {AArch64::ANDSWri, MI_INT_OP_002},
  {AArch64::ANDWri, MI_INT_OP_001},
  {AArch64::ANDXri, MI_INT_OP_001},
  {AArch64::CCMPXr, MI_INT_OP_003},
  {AArch64::CSELWr, MI_INT_OP_002},
  {AArch64::CSELXr, MI_INT_OP_002},
  {AArch64::CSINCWr, MI_INT_OP_002},
  {AArch64::EORXri, MI_INT_OP_001},
  {AArch64::EORXrr, MI_INT_OP_001},
  {AArch64::MSUBWrrr, MI_INT_OP_008},
  {AArch64::ORRWri, MI_INT_OP_001},
  {AArch64::ORRWrr, MI_INT_OP_001},
  {AArch64::ORRXri, MI_INT_OP_001},
  {AArch64::PRFMui, MI_INT_OP_005},
  {AArch64::PRFUMi, MI_INT_OP_005},
  {AArch64::SUBSXri, MI_INT_OP_002},
  {AArch64::SUBSXrr, MI_INT_OP_002},
  {AArch64::SUBWri, MI_INT_OP_001},
  {AArch64::SUBWrr, MI_INT_OP_001},
  {AArch64::SUBXri, MI_INT_OP_001},
  {AArch64::SUBXrr, MI_INT_OP_001},

  {AArch64::LDRBBroX, MI_INT_LD_002},
  {AArch64::LDRHHui, MI_INT_LD_002},
  {AArch64::LDRSWpost, MI_INT_LD_001},
  {AArch64::LDRSWroX, MI_INT_LD_002},
  {AArch64::LDRSWui, MI_INT_LD_002},
  {AArch64::LDRWpost, MI_INT_LD_001},
  {AArch64::LDRWroX, MI_INT_LD_002},
  {AArch64::LDRWui, MI_INT_LD_002},
  {AArch64::LDRXroX, MI_INT_LD_002},
  {AArch64::LDRXui, MI_INT_LD_002},
  {AArch64::LDURHHi, MI_INT_LD_002},
  {AArch64::LDURSWi, MI_INT_LD_002},
  {AArch64::LDURWi, MI_INT_LD_002},

  {AArch64::STRBBroX, MI_INT_ST_001},
  {AArch64::STRHHui, MI_INT_ST_001},
  {AArch64::STRWpost, MI_INT_ST_002},
  {AArch64::STRWroX, MI_INT_ST_001},
  {AArch64::STRWui, MI_INT_ST_001},
  {AArch64::STRXroX, MI_INT_ST_001},
  {AArch64::STRXui, MI_INT_ST_001},
  
  // SIMD&FP
  {AArch64::ADDv2i64, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADDv4i32, MI_SIMDFP_SVE_OP_002},
  {AArch64::DUPi32, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUPi64, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUPv2i32lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUPv2i64gpr, MI_SIMDFP_SVE_OP_025},
  {AArch64::DUPv2i64lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUPv4i32lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::EXTv16i8, MI_SIMDFP_SVE_OP_004},
  {AArch64::FABSDr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FABSSr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FADDDrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDPv2i32p, MI_SIMDFP_SVE_OP_006},
  {AArch64::FADDPv2i64p, MI_SIMDFP_SVE_OP_006},
  {AArch64::FADDPv4f32, MI_SIMDFP_SVE_OP_015},
  {AArch64::FADDSrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDv2f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDv2f64, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FCMPDri, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCMPDrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCMPSri, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCMPSrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCSELDrrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCSELSrrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCVTDSr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FCVTLv2i32, MI_SIMDFP_SVE_OP_021},
  {AArch64::FCVTLv4i32, MI_SIMDFP_SVE_OP_021},
  {AArch64::FCVTSDr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FCVTZSUXSr, MI_SIMDFP_SVE_OP_027},
  {AArch64::FDIVDrr, MI_SIMDFP_SVE_OP_009},
  {AArch64::FDIVSrr, MI_SIMDFP_SVE_OP_022},
  {AArch64::FDIVv2f32, MI_SIMDFP_SVE_OP_022},
  {AArch64::FDIVv2f64, MI_SIMDFP_SVE_OP_009},
  {AArch64::FDIVv4f32, MI_SIMDFP_SVE_OP_022},
  {AArch64::FMADDDrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMADDSrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMAXNMDrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMAXNMSrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMINNMDrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMINNMSrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMLAv1i64_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLAv2f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLAv2f64, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLAv2i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLAv2i64_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLAv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLAv4i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLSv1i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLSv2f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLSv2i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLSv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMSUBDrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMSUBSrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULDrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULSrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULv1i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMULv1i64_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMULv2f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULv2f64, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULv2i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMULv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FNEGDr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FSQRTDr, MI_SIMDFP_SVE_OP_009},
  {AArch64::FSQRTSr, MI_SIMDFP_SVE_OP_022},
  {AArch64::FSQRTv2f32, MI_SIMDFP_SVE_OP_022},
  {AArch64::FSQRTv2f64, MI_SIMDFP_SVE_OP_009},
  {AArch64::FSQRTv4f32, MI_SIMDFP_SVE_OP_022},
  {AArch64::FSUBDrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUBSrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUBv2f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUBv2f64, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUBv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::INSvi32lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::INSvi64lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::MULv4i32, MI_SIMDFP_SVE_OP_001},
  {AArch64::MULv8i16, MI_SIMDFP_SVE_OP_001},
  {AArch64::SCVTFUWDri, MI_SIMDFP_SVE_OP_011},
  {AArch64::SCVTFUWSri, MI_SIMDFP_SVE_OP_011},
  {AArch64::SHLv2i64_shift, MI_SIMDFP_SVE_OP_002},
  {AArch64::UZP1v8i16, MI_SIMDFP_SVE_OP_004},
  {AArch64::UZP2v8i16, MI_SIMDFP_SVE_OP_004},
  {AArch64::ZIP1v2i32, MI_SIMDFP_SVE_OP_004},
  {AArch64::ZIP1v2i64, MI_SIMDFP_SVE_OP_004},
  {AArch64::ZIP1v4i32, MI_SIMDFP_SVE_OP_004},
  {AArch64::ZIP2v2i32, MI_SIMDFP_SVE_OP_004},
  {AArch64::ZIP2v2i64, MI_SIMDFP_SVE_OP_004},

  {AArch64::LD1i32, MI_SIMDFP_SVE_LD_007},
  {AArch64::LD1i32_POST, MI_SIMDFP_SVE_LD_008},
  {AArch64::LD1i64, MI_SIMDFP_SVE_LD_007},
  {AArch64::LD1i64_POST, MI_SIMDFP_SVE_LD_008},
  {AArch64::LD1Rv2d, MI_SIMDFP_SVE_LD_003},
  {AArch64::LD1Rv2d_POST, MI_SIMDFP_SVE_LD_006},
  {AArch64::LD1Rv2s, MI_SIMDFP_SVE_LD_003},
  {AArch64::LD1Rv2s_POST, MI_SIMDFP_SVE_LD_006},
  {AArch64::LD3Threev2s_POST, MI_SIMDFP_SVE_LD_013},
  {AArch64::LD3Threev4s, MI_SIMDFP_SVE_LD_012},
  {AArch64::LD3Threev4s_POST, MI_SIMDFP_SVE_LD_013},
  {AArch64::LD4Fourv2d, MI_SIMDFP_SVE_LD_014},
  {AArch64::LD4Fourv2d_POST, MI_SIMDFP_SVE_LD_016},
  {AArch64::LD4Fourv2s, MI_SIMDFP_SVE_LD_014},
  {AArch64::LD4Fourv4s, MI_SIMDFP_SVE_LD_014},
  {AArch64::LD4Fourv4s_POST, MI_SIMDFP_SVE_LD_016},
  {AArch64::LDRDpost, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRDpre, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRDroW, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDRDroX, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDRDui, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRQpost, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRQroX, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDRQui, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSpre, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSroX, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDRSpost, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSui, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSroW, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDURDi, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDURQi, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDURSi, MI_SIMDFP_SVE_LD_003},
  
  {AArch64::ST1i32, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1i64, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST2Twov2d, MI_SIMDFP_SVE_ST_004},
  {AArch64::ST2Twov2d_POST, MI_SIMDFP_SVE_ST_005},
  {AArch64::ST2Twov4s, MI_SIMDFP_SVE_ST_004},
  {AArch64::ST2Twov4s_POST, MI_SIMDFP_SVE_ST_005},
  {AArch64::ST2Twov8h, MI_SIMDFP_SVE_ST_004},
  {AArch64::STRDpost, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRDroX, MI_SIMDFP_SVE_ST_001},
  {AArch64::STRDui, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRQpost, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRQroX, MI_SIMDFP_SVE_ST_001},
  {AArch64::STRQui, MI_SIMDFP_SVE_ST_001},
  {AArch64::STRSpost, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRSroX, MI_SIMDFP_SVE_ST_001},
  {AArch64::STRSui, MI_SIMDFP_SVE_ST_002},
  {AArch64::STURDi, MI_SIMDFP_SVE_ST_001},
  {AArch64::STURQi, MI_SIMDFP_SVE_ST_001},
  {AArch64::STURSi, MI_SIMDFP_SVE_ST_001},
  
  // SVE
  {AArch64::ADD_ZI_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADD_ZI_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADD_ZPmZ_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADD_ZZZ_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADD_ZZZ_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADR_LSL_ZZZ_D_2, MI_SIMDFP_SVE_OP_008},
  {AArch64::ADR_LSL_ZZZ_D_3, MI_SIMDFP_SVE_OP_008},
  {AArch64::AND_PPzPP, MI_PREDICATE_OP_001},
  {AArch64::AND_ZI, MI_SIMDFP_SVE_OP_007},
  {AArch64::BIC_PPzPP, MI_PREDICATE_OP_001},
  {AArch64::CMPHI_PPzZZ_D, MI_SVE_CMP_INST_002},
  {AArch64::CPY_ZPmV_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUP_ZR_D, MI_SIMDFP_SVE_OP_005},
  {AArch64::DUP_ZR_S, MI_SIMDFP_SVE_OP_005},
  {AArch64::DUP_ZZI_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUP_ZZI_S, MI_SIMDFP_SVE_OP_004},
  {AArch64::EOR_PPzPP, MI_PREDICATE_OP_001},
  {AArch64::EXT_ZZI, MI_SIMDFP_SVE_OP_004},
  {AArch64::FABS_ZPmZ_D_UNDEF, MI_SIMDFP_SVE_OP_002},
  {AArch64::FABS_ZPmZ_S_UNDEF, MI_SIMDFP_SVE_OP_002},
  {AArch64::FADD_ZPmZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADD_ZPZI_D_UNDEF, MI_SIMDFP_SVE_OP_003},
  {AArch64::FADD_ZPZI_S_UNDEF, MI_SIMDFP_SVE_OP_003},
  {AArch64::FADD_ZPZZ_D_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADD_ZPZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADD_ZZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADD_ZZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FCMEQ_PPzZ0_D, MI_SVE_CMP_INST_001},
  {AArch64::FCMEQ_PPzZ0_S, MI_SVE_CMP_INST_001},
  {AArch64::FCMGE_PPzZZ_D, MI_SVE_CMP_INST_001},
  {AArch64::FCMGE_PPzZZ_S, MI_SVE_CMP_INST_001},
  {AArch64::FCMGT_PPzZ0_D, MI_SVE_CMP_INST_001},
  {AArch64::FCMGT_PPzZ0_S, MI_SVE_CMP_INST_001},
  {AArch64::FCMGT_PPzZZ_D, MI_SVE_CMP_INST_001},
  {AArch64::FCMGT_PPzZZ_S, MI_SVE_CMP_INST_001},
  {AArch64::FCMLE_PPzZ0_D, MI_SVE_CMP_INST_001},
  {AArch64::FCMLE_PPzZ0_S, MI_SVE_CMP_INST_001},
  {AArch64::FCMLT_PPzZ0_D, MI_SVE_CMP_INST_001},
  {AArch64::FCMLT_PPzZ0_S, MI_SVE_CMP_INST_001},
  {AArch64::FCMNE_PPzZ0_D, MI_SVE_CMP_INST_001},
  {AArch64::FCMNE_PPzZ0_S, MI_SVE_CMP_INST_001},
  {AArch64::FMAD_ZPmZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMAD_ZPmZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMAXNM_ZPZZ_D_UNDEF, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMAXNM_ZPZZ_S_UNDEF, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMINNM_ZPZZ_D_UNDEF, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMINNM_ZPZZ_S_UNDEF, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMLA_ZPmZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLA_ZPmZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLA_ZPZZZ_D_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLA_ZPZZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLS_ZPZZZ_D_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLS_ZPZZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMSB_ZPmZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZPmZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZPZI_D_UNDEF, MI_SIMDFP_SVE_OP_003},
  {AArch64::FMUL_ZPZI_S_UNDEF, MI_SIMDFP_SVE_OP_003},
  {AArch64::FMUL_ZPZZ_D_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZPZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FNEG_ZPmZ_D_UNDEF, MI_SIMDFP_SVE_OP_002},
  {AArch64::FNMLS_ZPZZZ_D_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FNMLS_ZPZZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FRSQRTE_ZZ_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::FSUB_ZPZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUB_ZZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUB_ZZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::INDEX_IR_D, MI_SIMDFP_SVE_OP_011},
  {AArch64::INDEX_RI_S, MI_SIMDFP_SVE_OP_011},
  {AArch64::LSL_ZZI_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::LSR_ZZI_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::LSR_ZZI_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::MLS_ZPmZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::MLS_ZPmZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::MLS_ZPZZZ_D_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::MLS_ZPZZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::MUL_ZI_D, MI_SIMDFP_SVE_OP_003},
  {AArch64::MUL_ZI_S, MI_SIMDFP_SVE_OP_003},
  {AArch64::PTEST_PP, MI_PREDICATE_OP_001},
  {AArch64::REV_ZZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::REV_ZZ_S, MI_SIMDFP_SVE_OP_004},
  {AArch64::SCVTF_ZPmZ_StoD_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::SCVTF_ZPmZ_StoS_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::SCVTF_ZPmZ_DtoD_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::SEL_PPPP, MI_PREDICATE_OP_001},
  {AArch64::SEL_ZPZZ_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::SEL_ZPZZ_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::SPLICE_ZPZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::SPLICE_ZPZ_S, MI_SIMDFP_SVE_OP_004},
  {AArch64::SUB_ZZZ_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::SUB_ZZZ_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::SUNPKLO_ZZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::UMULH_ZPZZ_D_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::UMULH_ZPZZ_S_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::UUNPKHI_ZZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::UUNPKLO_ZZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::UZP1_ZZZ_S, MI_SIMDFP_SVE_OP_004},
  {AArch64::WHILELO_PXX_D, MI_PREDICATE_OP_002},
  {AArch64::WHILELT_PWW_S, MI_PREDICATE_OP_002},
  {AArch64::WHILELT_PXX_D, MI_PREDICATE_OP_002},
  {AArch64::WHILELT_PXX_S, MI_PREDICATE_OP_002},
  
  {AArch64::GLD1B_S_SXTW, MI_SIMDFP_SVE_LD_011},
  {AArch64::GLD1D, MI_SIMDFP_SVE_LD_004},
  {AArch64::GLD1D_IMM, MI_SIMDFP_SVE_LD_005},
  {AArch64::GLD1D_SCALED, MI_SIMDFP_SVE_LD_004},
  {AArch64::GLD1D_SXTW_SCALED, MI_SIMDFP_SVE_LD_004},
  {AArch64::GLD1W_D, MI_SIMDFP_SVE_LD_004},
  {AArch64::GLD1W_D_IMM, MI_SIMDFP_SVE_LD_005},
  {AArch64::GLD1W_D_SCALED, MI_SIMDFP_SVE_LD_004},
  {AArch64::GLD1W_SXTW, MI_SIMDFP_SVE_LD_011},
  {AArch64::GLD1W_SXTW_SCALED, MI_SIMDFP_SVE_LD_011},
  {AArch64::LD1B, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1D, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1D_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1RD_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1RW_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1SW_D, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1SW_D_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W_D, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W_D_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD2D, MI_SIMDFP_SVE_LD_010},
  {AArch64::LD2D_IMM, MI_SIMDFP_SVE_LD_009},
  {AArch64::LD2W, MI_SIMDFP_SVE_LD_010},
  {AArch64::LD2W_IMM, MI_SIMDFP_SVE_LD_009},
  {AArch64::LD4D, MI_SIMDFP_SVE_LD_015},
  {AArch64::LD4D_IMM, MI_SIMDFP_SVE_LD_014},
  {AArch64::LD4W, MI_SIMDFP_SVE_LD_015},
  {AArch64::LD4W_IMM, MI_SIMDFP_SVE_LD_014},
  
  {AArch64::SST1B_S_SXTW, MI_SIMDFP_SVE_ST_006},
  {AArch64::SST1D, MI_SIMDFP_SVE_ST_003},
  {AArch64::SST1D_SCALED, MI_SIMDFP_SVE_ST_003},
  {AArch64::SST1W_D_SCALED, MI_SIMDFP_SVE_ST_003},
  {AArch64::SST1W_SXTW, MI_SIMDFP_SVE_ST_006},
  {AArch64::SST1W_SXTW_SCALED, MI_SIMDFP_SVE_ST_006},
  {AArch64::ST1B, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1D, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1D_IMM, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W_D, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W_IMM, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST2D, MI_SIMDFP_SVE_ST_005},
  {AArch64::ST2D_IMM, MI_SIMDFP_SVE_ST_004},
  {AArch64::ST2W, MI_SIMDFP_SVE_ST_005},
  {AArch64::ST2W_IMM, MI_SIMDFP_SVE_ST_004},
  {AArch64::STNT1D_ZRI, MI_SIMDFP_SVE_ST_001},
  {AArch64::STNT1D_ZRR, MI_SIMDFP_SVE_ST_001},
  {AArch64::STNT1W_ZRI, MI_SIMDFP_SVE_ST_001},
  {AArch64::STNT1W_ZRR, MI_SIMDFP_SVE_ST_001},
};

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::getRes(
  const MachineInstr &mi) const {
  ResourceID id = AArch64SwplSchedA64FX::searchRes(mi);
  return id;
}

const StmPipelinesImpl *AArch64SwplSchedA64FX::getPipelines(
  ResourceID id) const {
  return &ResInfo.at(id).pipelines;
}

bool AArch64SwplSchedA64FX::isPseudo(const MachineInstr &mi) const {
  // アセンブラ出力時までに無くなる可能性が大きいものはPseudoとして扱う
  switch (mi.getOpcode()) {
    case AArch64::SUBREG_TO_REG:
    case AArch64::INSERT_SUBREG:
    case AArch64::REG_SEQUENCE:
      return true;
  }
  return false;
}

unsigned AArch64SwplSchedA64FX::getLatency(ResourceID id) const {
  return ResInfo.at(id).latency;
}

bool AArch64SwplSchedA64FX::isSeqDecode(ResourceID id) const {
  return ResInfo.at(id).seqdecode;
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchRes(
  const MachineInstr &mi) {

  unsigned Opcode = mi.getOpcode();
  auto it = MIOpcodeInfo.find(Opcode);
  if (it != MIOpcodeInfo.end()) {
    return it->second;
  }

  // COPY命令はレジスタ種別で利用する資源が異なるため
  // レジスタ種別ごとに代表的なレイテンシ・演算器で定義する
  if (Opcode == AArch64::COPY) {
    Register r = mi.getOperand(0).getReg();
    auto [rkind, n] = SWPipeliner::TII->getRegKindId(*SWPipeliner::MRI, r);
    if (rkind == StmRegKind::getFloatRegID()) {
      return MI_SIMDFP_SVE_OP_002;
    } else if (rkind == StmRegKind::getPredicateRegID()) {
      return MI_PREDICATE_OP_001;
    } else {
      return MI_INT_OP_001;
    }
  }

  // ADDXrs/SUBXrs命令の判断
  if (Opcode == AArch64::ADDXrs || Opcode == AArch64::SUBXrs){
    return AArch64SwplSchedA64FX::searchResADDSUBShiftReg(mi);
  }

  // ADDXrx命令の判断
  if (Opcode == AArch64::ADDXrx){
    return AArch64SwplSchedA64FX::searchResADDExtendReg(mi);
  }

  // MADD/UMADDL命令の判断
  if (Opcode == AArch64::MADDWrrr || Opcode == AArch64::MADDXrrr ||
      Opcode == AArch64::UMADDLrrr){
    return AArch64SwplSchedA64FX::searchResMADD(mi);
  }

  // ORRWrs/ORRXrs命令の判断
  if (Opcode == AArch64::ORRWrs || Opcode == AArch64::ORRXrs){
    return AArch64SwplSchedA64FX::searchResORRShiftReg(mi);
  }

  // SBFM/UBFM命令の判断
  if (Opcode == AArch64::SBFMXri || Opcode == AArch64::SBFMWri ||
      Opcode == AArch64::UBFMXri || Opcode == AArch64::UBFMWri){
    return AArch64SwplSchedA64FX::searchResSBFM(mi);
  }

  // FADDA命令の判断
  if (Opcode == AArch64::FADDA_VPZ_D){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_012;
    case 256:
      return MI_SIMDFP_SVE_OP_013;
    default:
      return MI_SIMDFP_SVE_OP_014;
    }
  }
  if (Opcode == AArch64::FADDA_VPZ_S){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_013;
    case 256:
      return MI_SIMDFP_SVE_OP_014;
    default:
      return MI_SIMDFP_SVE_OP_010;
    }
  }

  // FADDV命令の判断
  if (Opcode == AArch64::FADDV_VPZ_D){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_026;
    case 256:
      return MI_SIMDFP_SVE_OP_016;
    default:
      return MI_SIMDFP_SVE_OP_017;
    }
  }
  if (Opcode == AArch64::FADDV_VPZ_S){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_016;
    case 256:
      return MI_SIMDFP_SVE_OP_017;
    default:
      return MI_SIMDFP_SVE_OP_018;
    }
  }

  // FDIV命令,FSQRT命令の判断
  if (Opcode == AArch64::FDIV_ZPZZ_D_UNDEF || Opcode == AArch64::FSQRT_ZPmZ_D_UNDEF){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_009;
    case 256:
      return MI_SIMDFP_SVE_OP_019;
    default:
      return MI_SIMDFP_SVE_OP_020;
    }
  }
  if (Opcode == AArch64::FDIV_ZPZZ_S_UNDEF || Opcode == AArch64::FSQRT_ZPmZ_S_UNDEF){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_022;
    case 256:
      return MI_SIMDFP_SVE_OP_023;
    default:
      return MI_SIMDFP_SVE_OP_024;
    }
  }
  
  return AArch64SwplSchedA64FX::MI_NA;
}


AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResADDSUBShiftReg(const MachineInstr &mi) {
  // 参考：AArch64InstrInfo.cpp AArch64InstrInfo::isFalkorShiftExtFast

  // Immが存在しない場合、シフト量：0
  if (mi.getNumOperands() < 4)
    return MI_INT_OP_001;

  unsigned Imm = mi.getOperand(3).getImm();
  unsigned ShiftVal = AArch64_AM::getShiftValue(Imm);
  if (ShiftVal == 0)
    return MI_INT_OP_001;
  else if (ShiftVal <= 4 && AArch64_AM::getShiftType(Imm) == AArch64_AM::LSL)
    return MI_INT_OP_003;
  else
    return MI_INT_OP_004;
}


AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResADDExtendReg(const MachineInstr &mi) {
  // 参考：AArch64InstrInfo.cpp AArch64InstrInfo::isFalkorShiftExtFast

  unsigned Opcode = mi.getOpcode();
  // Immが存在しない場合、LSL、拡張後のシフト量：0
  if (mi.getNumOperands() < 4){
    if (Opcode == AArch64::ADDWrx)
      return MI_INT_OP_002;
    else
      return MI_INT_OP_003;
  }
  
  unsigned Imm = mi.getOperand(3).getImm();
  
  if (Opcode == AArch64::ADDWrx){ /// 32bit(sf = 0)
    switch (AArch64_AM::getArithExtendType(Imm)){
    default:
      return MI_INT_OP_003;
    case AArch64_AM::UXTW:
    case AArch64_AM::UXTX:
    case AArch64_AM::SXTW:
    case AArch64_AM::SXTX:
      return MI_INT_OP_002;
    }
  } else {
    switch (AArch64_AM::getArithExtendType(Imm)){
    default:
      return MI_INT_OP_003;
    case AArch64_AM::UXTX:
    case AArch64_AM::SXTX:
      return MI_INT_OP_002;
    }
  }
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResMADD(const MachineInstr &mi) {
  unsigned Opcode = mi.getOpcode();
  Register r = mi.getOperand(3).getReg();
  
  // MADD:32bit Wa==WZRの場合、MULと等価
  if (Opcode == AArch64::MADDWrrr && r == AArch64::WZR){
    return MI_INT_OP_007;
  }
  // MADD:64bit Xa==XZRの場合、MULと等価
  if (Opcode == AArch64::MADDXrrr && r == AArch64::XZR){
    return MI_INT_OP_007;
  }
  // UMADDL Xa==XZRの場合、UMULLと等価
  if (Opcode == AArch64::UMADDLrrr && r == AArch64::XZR){
    return MI_INT_OP_007;
  }

  return MI_INT_OP_008;
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResORRShiftReg(const MachineInstr &mi) {
  // 参考: AArch64InstPrinter.cpp AArch64InstPrinter::printInst
  // AArch64::ORRWrs and AArch64::ORRXrs with WZR/XZR reg
  // and zero immediate operands used as an alias for mov instruction.

  // Immが存在しない場合、LSL、拡張後のシフト量：0
  if (mi.getNumOperands() < 4){
      return MI_INT_OP_001;
  }

  unsigned Imm = mi.getOperand(3).getImm();
  unsigned ShiftVal = AArch64_AM::getShiftValue(Imm);

  if (ShiftVal == 0) {
    return MI_INT_OP_001;
  } else {
    return MI_INT_OP_004;
  }
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResSBFM(const MachineInstr &mi) {
  // 参考: AArch64InstPrinter.cpp AArch64InstPrinter::printInst
  unsigned Opcode = mi.getOpcode();

  const MachineOperand Op2 = mi.getOperand(2);
  const MachineOperand Op3 = mi.getOperand(3);

  bool IsSigned = (Opcode == AArch64::SBFMXri || Opcode == AArch64::SBFMWri);
  bool Is64Bit = (Opcode == AArch64::SBFMXri || Opcode == AArch64::UBFMXri);
  if (Op2.isImm() && Op2.getImm() == 0 && Op3.isImm()) {
    switch (Op3.getImm()) {
    default:
      break;
    case 7:
      if (IsSigned || (!Is64Bit)){
        // SXTB/UXTB
        return MI_INT_OP_002;
      }
      break;
    case 15:
      if (IsSigned || (!Is64Bit)){
        // SXTH/UXTH
        return MI_INT_OP_002;
      }
      break;
    case 31:
      // *xtw is only valid for signed 64-bit operations.
      if (Is64Bit && IsSigned){
        // SXTW
        return MI_INT_OP_002;
      }
      break;
    }
  }

  // All immediate shifts are aliases, implemented using the Bitfield
  // instruction. In all cases the immediate shift amount shift must be in
  // the range 0 to (reg.size -1).
  if (Op2.isImm() && Op3.isImm()) {
    int shift = 0;
    int64_t immr = Op2.getImm();
    int64_t imms = Op3.getImm();
    if (Opcode == AArch64::UBFMWri && imms != 0x1F && ((imms + 1) == immr)) {
      // LSL
      shift = 31 - imms;
      if (1 <= shift && shift <= 4) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::UBFMXri && imms != 0x3f &&
                ((imms + 1 == immr))) {
      // LSL
      shift = 63 - imms;
      if (1 <= shift && shift <= 4) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::UBFMWri && imms == 0x1f) {
      // LSR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::UBFMXri && imms == 0x3f) {
      // LSR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::SBFMWri && imms == 0x1f) {
      // ASR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::SBFMXri && imms == 0x3f) {
      // ASR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    }
  }

  // SBFIZ/UBFIZ or SBFX/UBFX
  return MI_INT_OP_004;
}

#define UNAVAILABLE_REGS 7
#define PRIO_ASC_ORDER 0
#define PRIO_UNUSE 1

static cl::opt<bool> DebugSwplRegAlloc("swpl-debug-reg-alloc",cl::init(false), cl::ReallyHidden);
static cl::opt<int> SwplRegAllocPrio("swpl-reg-alloc-prio",cl::init(PRIO_ASC_ORDER), cl::ReallyHidden);
static cl::opt<bool> EnableSetrenamable("swpl-enable-reg-alloc-setrenamable",cl::init(false), cl::ReallyHidden);

// 使ってはいけない物理レジスタのリスト
static DenseSet<unsigned> nousePhysRegs {
    (unsigned)AArch64::XZR,  // ゼロレジスタ
    (unsigned)AArch64::SP,   // スタックポインタ
    (unsigned)AArch64::FP,   // フレームポインタ(X29)
    (unsigned)AArch64::LR,   // リンクレジスタ(X30)
    (unsigned)AArch64::WZR,  // ゼロレジスタ
    (unsigned)AArch64::WSP,  // スタックポインタ
    (unsigned)AArch64::W29,  // フレームポインタ(X29)相当
    (unsigned)AArch64::W30   // リンクレジスタ(X30)相当
};


/**
 * @brief エピローグ部の仮想レジスタリストを作成する
 * @param [in]     mi llvm::MachineInstr
 * @param [in]     num_mi llvm::MachineInstrの通し番号
 * @param [in,out] ekri_tbl カーネルループ外のレジスタ情報の表
 */
static void createVRegListEpi(MachineInstr *mi, unsigned num_mi,
                              SwplExcKernelRegInfoTbl &ekri_tbl) {
  assert(mi);

  // MIのオペランド数でループ
  for (MachineOperand& mo:mi->operands()) {
    // レジスタオペランド かつ レジスタ番号が 0 でない かつ
    // 仮想レジスタであればリストに加える
    unsigned reg = 0;
    if ((!mo.isReg()) || ((reg = mo.getReg()) == 0) ||
        (!Register::isVirtualRegister(reg)))
      continue;

    // 割り当て済みレジスタ情報を当該仮想レジスタで検索
    auto ri = ekri_tbl.getWithVReg(reg);

    // TODO: 以下、同じような処理が続くため、関数化した方が良い
    // 当該オペランドが"定義"か
    if (mo.isDef()) {
      if (ri != nullptr) {
        if( ri->num_def == -1 ||
            ri->num_use == -1 ) {
          // 当該仮想レジスタは登録済みなのでMI通し番号を更新
          ri->updateNumDef(num_mi);
        }
      } else {
        // 当該仮想レジスタは登録されていないため新規登録
        ekri_tbl.addExcKernelRegInfo(reg, num_mi, -1);
      }
    }
    // 当該オペランドが"参照"か
    if (mo.isUse()) {
      if (ri != nullptr) {
        if( ri->num_def == -1 ||
            ri->num_use == -1 ||
            ri->num_use > ri->num_def ) {
          // 当該仮想レジスタは登録済みなのでMI通し番号を更新
          ri->updateNumUse(num_mi);
        }
      } else {
        // 当該仮想レジスタは登録されていないため新規登録
        ekri_tbl.addExcKernelRegInfo(reg, -1, num_mi);
      }
    }
  }

  return;
}

/**
 * @brief Create a COPY from a virtual register to a physical register
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out] p_post_mis COPY instruction for prolog post
 * @param [in]     vreg virtual register number
 * @param [in]     preg physical register number
 */
static void addCopyMIpre(MachineFunction &MF,
                         DebugLoc dl,
                         std::vector<MachineInstr*> &p_post_mis,
                         unsigned vreg, unsigned preg) {
  assert(vreg && preg);
  if( DebugSwplRegAlloc ) {
    dbgs() << " Copy " << printReg(vreg, SWPipeliner::TRI)
           << " to " << printReg(preg, SWPipeliner::TRI) << "\n";
  }

  // Create COPY from virtual register to physical register
  llvm::MachineInstr *copy =
      BuildMI(MF, dl, SWPipeliner::TII->get(TargetOpcode::COPY))
      .addReg(preg, RegState::Define).addReg(vreg);
  p_post_mis.push_back(copy);

  return;
}

/**
 * @brief Create a COPY from a physical register to a virtual register
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out] e_pre_mis COPY instruction for epilog pre
 * @param [in]     vreg virtual register number
 * @param [in]     preg physical register number
 */
static void addCopyMIpost(MachineFunction &MF,
                          DebugLoc dl,
                          std::vector<MachineInstr*> &e_pre_mis,
                          unsigned vreg, unsigned preg) {
  assert(vreg && preg);
  if( DebugSwplRegAlloc ) {
    dbgs() << " Copy " << printReg(preg, SWPipeliner::TRI)
           << " to " << printReg(vreg, SWPipeliner::TRI) << "\n";
  }

  // Create COPY from physical register to virtual register
  llvm::MachineInstr *copy =
      BuildMI(MF, dl, SWPipeliner::TII->get(TargetOpcode::COPY))
      .addReg(vreg, RegState::Define).addReg(preg);
  e_pre_mis.push_back(copy);

  return;
}

/**
 * @brief Create SWPLIVEIN instruction
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out]    livein_mi MI for livein
 * @param [in]     copy_mis COPY instruction for livein/liveout
 * @param [in]     vreg_loc Location of physical register for COPY instruction
 */
static void createSwpliveinMI(MachineFunction &MF,
                          DebugLoc dl,
                          MachineInstr **livein_mi,
                          std::vector<MachineInstr*> &copy_mis,
                          unsigned vreg_loc) {

  // Create SWPLIVEIN instruction
  MachineInstrBuilder mib = BuildMI(MF, dl, SWPipeliner::TII->get(AArch64::SWPLIVEIN));
  for(auto *copy:copy_mis){
    mib.addReg(copy->getOperand(vreg_loc).getReg(), RegState::ImplicitDefine);
  }
  *livein_mi = mib.getInstr();
  return;
}

/**
 * @brief Add registers to the SWPLIVEOUT instruction
 * @param [in,out] mib MachineInstrBuilder for SWPLIVEOUT
 * @param [in]     copy_mis COPY instruction for livein/liveout
 * @param [in]     vreg_loc Location of physical register for COPY instruction
 * @param [in,out] added_regs Registers already added to the SWPLIVEOUT instruction
 */
static void addRegSwpliveoutMI(MachineInstrBuilder &mib,
                              std::vector<MachineInstr*> &copy_mis,
                              unsigned vreg_loc,
                              std::vector<Register> &added_regs) {
  for (auto *copy:copy_mis) {
    Register reg = copy->getOperand(vreg_loc).getReg();
    bool added = false;

    // Already added registers?
    for (auto &added_reg:added_regs) {
      if (reg == added_reg){
        added = true;
        break;
      }    
    }

    if(!added){
      mib.addReg(reg, RegState::Implicit);
      added_regs.push_back(reg);
    }  
  }
  return;
}

/**
 * @brief Create Prolog SWPLIVEOUT instruction
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out]    liveout_mi MI for liveout
 * @param [in]     livein_copy_mis COPY instruction for livein
 */
static void createPrologSwpliveoutMI(MachineFunction &MF,
                          DebugLoc dl,
                          MachineInstr **liveout_mi,
                          std::vector<MachineInstr*> &livein_copy_mis) {

  // Create SWPLIVEOUT instruction
  MachineInstrBuilder mib = BuildMI(MF, dl, SWPipeliner::TII->get(AArch64::SWPLIVEOUT));
  std::vector<Register> added_regs;
  addRegSwpliveoutMI(mib, livein_copy_mis, 0, added_regs);
  *liveout_mi = mib.getInstr();
  return;
}

/**
 * @brief Create Kernel SWPLIVEOUT instruction
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out]    liveout_mi MI for liveout
 * @param [in]     livein_copy_mis COPY instruction for livein
 * @param [in]     liveout_copy_mis COPY instruction for liveout
 */
static void createKernelSwpliveoutMI(MachineFunction &MF,
                          DebugLoc dl,
                          MachineInstr **liveout_mi,
                          std::vector<MachineInstr*> &livein_copy_mis,
                          std::vector<MachineInstr*> &liveout_copy_mis) {

  // Create SWPLIVEOUT instruction
  MachineInstrBuilder mib = BuildMI(MF, dl, SWPipeliner::TII->get(AArch64::SWPLIVEOUT));
  std::vector<Register> added_regs;
  // The livein register is added to extend the live range.
  addRegSwpliveoutMI(mib, livein_copy_mis, 0, added_regs);
  addRegSwpliveoutMI(mib, liveout_copy_mis, 1, added_regs);
  *liveout_mi = mib.getInstr();
  return;
}

/**
 * @brief  Create software pipelining pseudo instructions.
 * @param  [in,out] tmi Information for the SwplTransformMIR
 * @param  [in]     MF  MachineFunction
 */
void AArch64InstrInfo::createSwplPseudoMIs(SwplTransformedMIRInfo *tmi,MachineFunction &MF) const {
  // Get DebugLoc from kernel start MI
  DebugLoc firstDL;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    firstDL = mi->getDebugLoc();
    break;
  }
  
  // Generate SWPLIVEIN, SWPLIVEOUT instructions
  createPrologSwpliveoutMI(MF, firstDL, &tmi->prolog_liveout_mi, tmi->livein_copy_mis);
  createSwpliveinMI(MF, firstDL, &tmi->kernel_livein_mi, tmi->livein_copy_mis, 0);
  createKernelSwpliveoutMI(MF, firstDL, &tmi->kernel_liveout_mi, tmi->livein_copy_mis, tmi->liveout_copy_mis);
  createSwpliveinMI(MF, firstDL, &tmi->epilog_livein_mi, tmi->liveout_copy_mis, 1);
}

/**
 * @brief  Whether it is a software pipelining pseudo instruction
 * @param  [in]  MI MachineInstr
 * @retval true Software pipelining pseudo instruction
 * @retval false Not software pipelining pseudo instruction
 */
bool AArch64InstrInfo::isSwplPseudoMI(MachineInstr &MI) const {
  unsigned op = MI.getOpcode();
  return (op == AArch64::SWPLIVEIN || op == AArch64::SWPLIVEOUT);
}

/**
 * @brief  物理レジスタ割り付け対象外の仮想レジスタ一覧を作成する
 * @param  [in]     mi llvm::MachineInstr
 * @param  [in]     mo llvm::MachineOperand
 * @param  [in,out] ex_vreg 割り付け対象外仮想レジスタ
 * @param  [in]     reg getReg()で取得したレジスタ番号
 * @note   制約のある命令は、仮対処として、物理レジスタ割り付け対象外とする。
 */
static void createExcludeVReg(MachineInstr *mi, MachineOperand &mo,
                              std::unordered_set<unsigned> &ex_vreg, unsigned reg) {
  if ((!Register::isVirtualRegister(reg)) ||
      (ex_vreg.find(reg) != ex_vreg.end())) {
    // 仮想レジスタでない or 除外リストに追加済み なら何もしない
    return;
  }

  // 対象のオペランドなら除外リストに追加
  assert(mi);
  switch (mi->getOpcode()) {
  case TargetOpcode::EXTRACT_SUBREG:
  case TargetOpcode::INSERT_SUBREG:
  case TargetOpcode::REG_SEQUENCE:
  case TargetOpcode::SUBREG_TO_REG:
    if( DebugSwplRegAlloc ) {
      dbgs() << "Excluded operand: " << mi->getOpcode()
             << ", reg=" << printReg(reg, SWPipeliner::TRI) << "\n";
    }
    ex_vreg.insert(reg);
    break;
  default:
    break;
  }
}

/**
 * @brief  仮想レジスタごとに生存区間リストを作成する
 * @param  [in]     mi llvm::MachineInstr
 * @param  [in]     idx moのindex
 * @param  [in]     mo llvm::MachineOperand
 * @param  [in]     reg getReg()で取得したレジスタ番号
 * @param  [in,out] rai_tbl 割り当て済みレジスタ情報の表
 * @param  [in]     num_mi llvm::MachineInstrの通し番号
 * @param  [in]     total_mi llvm::MachineInstrの総数
 * @param  [in]     ex_vreg 割り付け対象外仮想レジスタ
 * @param  [in]     doVReg renaming後の制御変数
 * @retval 0 正常終了
 * @retval 0以外 異常終了
 */
static int createLiveRange(MachineInstr *mi, unsigned idx,
                           MachineOperand &mo, unsigned reg,
                           SwplRegAllocInfoTbl &rai_tbl,
                           int num_mi, int total_mi,
                           std::unordered_set<unsigned> &ex_vreg,
                           Register doVReg) {
  int ret = 0;

  /*
   * 以下に気を付ける
   * - liveinの場合はいきなり参照になる
   * - liveoutの場合
   * - 参照後、ループの冒頭に戻った後での定義関係
   * - 定義のみ(あるのか？)
   * - vectorレジスタ/subreg/複数参照
   * - 物理レジスタ領域の重なり($x0と$w0など)
   */

  // getReg()で取得したレジスタが仮想か物理かを判定する
  if (MCRegister::isPhysicalRegister(reg)) {
    // 物理レジスタである
    // 割り当て済みレジスタ情報を当該物理レジスタで検索
    if( !(rai_tbl.isUsePReg(reg)) ) {
      // 物理レジスタ割り付け前に登場する物理レジスタは使用することができない。
      // 無効な情報として、レジスタ情報になければ新規登録するのみ。
      rai_tbl.addRegAllocInfo(0, 1, total_mi, &mo, 0, reg);
    }
  } else if (Register::isVirtualRegister(reg)) {
    // 仮想レジスタである

    // 除外リストに含まれていない限り、処理続行
    if (ex_vreg.find(reg) == ex_vreg.end()) {

      // 割り当て済みレジスタ情報を当該仮想レジスタで検索
      auto rai = rai_tbl.getWithVReg( reg );

      assert(mi);
      unsigned tied_vreg = 0;
      // TODO: 以下、同じような処理が続くため、関数化した方が良い
      // 当該オペランドが"定義"か
      if (mo.isDef()) {
        // 参照オペランドとtiedならそのオペランドを取得
        unsigned tied_idx;
        if ((mo.isTied()) && (mi->isRegTiedToUseOperand(idx, &tied_idx))) {
          MachineOperand &tied_mo = mi->getOperand(tied_idx);
          if (ex_vreg.find(tied_mo.getReg()) != ex_vreg.end()) {
            // tiedの相手が除外リストに含まれるなら自分も除外リストへ
            if (DebugSwplRegAlloc) {
              dbgs() << "Excluded register. current reg="
                    << printReg(reg, SWPipeliner::TRI) << ", tied(excluded)="
                    << printReg(tied_mo.getReg(), SWPipeliner::TRI) << "\n";
            }
            ex_vreg.insert(reg);
            return ret;
          }
          if ((tied_mo.isReg()) && (tied_mo.getReg()) && (tied_mo.isTied())) {
            // レジスタオペランド かつ レジスタ値が0より大きい かつ
            // tiedフラグが立っているならtied仮想レジスタとして使用する
            tied_vreg = tied_mo.getReg();
          }
        }

        if (rai != nullptr) {
          // 同じ仮想レジスタの定義は、複数存在しないことが前提
          // 現在 assertとしているが、想定外としてエラーとすべきか。
          assert( rai->num_def == -1 );

          // 当該仮想レジスタは登録済みなのでMI通し番号を更新
          if (reg == doVReg) {
            // doVRegに該当する場合、当該仮想レジスタに割り当てる物理レジスタは
            // 再利用されて欲しくないため、liverangeをMAX値で更新
            rai->updateNumDefNoReuse( num_mi );
          } else {
            rai->updateNumDef( num_mi );
          }
          rai->addMo(&mo);
          if (tied_vreg != 0) {
            rai->updateTiedVReg(tied_vreg);
          }
        } else {
          // 当該仮想レジスタは登録されていないため新規登録
          rai_tbl.addRegAllocInfo( reg, num_mi, -1, &mo, tied_vreg );
        }
      }
      // 当該オペランドが"参照"か
      if (mo.isUse()) {
        // 定義オペランドとtiedならそのオペランドを取得
        unsigned tied_idx;
        if ((mo.isTied()) && (mi->isRegTiedToDefOperand(idx, &tied_idx))) {
          MachineOperand &tied_mo = mi->getOperand(tied_idx);
          if (ex_vreg.find(tied_mo.getReg()) != ex_vreg.end()) {
            // tiedの相手が除外リストに含まれるなら自分も除外リストへ
            if (DebugSwplRegAlloc) {
              dbgs() << "Excluded register. current reg="
                    << printReg(reg, SWPipeliner::TRI) << ", tied(excluded)="
                    << printReg(tied_mo.getReg(), SWPipeliner::TRI) << "\n";
            }
            ex_vreg.insert(reg);
            return ret;
          }
          if ((tied_mo.isReg()) && (tied_mo.getReg()) && (tied_mo.isTied())) {
            // レジスタオペランド かつ レジスタ値が0より大きい かつ
            // tiedフラグが立っているならtied仮想レジスタとして使用する
            tied_vreg = tied_mo.getReg();
          }
        }

        if (rai != nullptr) {
          if( rai->num_def == -1 ||
              rai->num_use == -1 ||
              rai->num_use > rai->num_def ) {
            // 当該仮想レジスタは登録済みなのでMI通し番号を更新
            if (reg == doVReg) {
              // doVRegに該当する場合、当該仮想レジスタに割り当てる物理レジスタは
              // 再利用されて欲しくないため、liverangeをMAX値で更新
              rai->updateNumUseNoReuse( num_mi );
            } else {
              rai->updateNumUse( num_mi );
            }
          }
          rai->addMo(&mo);
          if (tied_vreg != 0) {
            rai->updateTiedVReg(tied_vreg);
          }
        } else {
          // 当該仮想レジスタは登録されていないため新規登録
          rai_tbl.addRegAllocInfo( reg, -1, num_mi, &mo, tied_vreg );
        }
      }
    }
  }  else {
    // do nothing. 仮想レジスタでも物理レジスタでもない
  }

  return ret;
}

/**
 * @brief Liverangeの延長処理
 * @param [in,out] tmi 命令列変換情報
 */
static void extendLiveRange(SwplTransformedMIRInfo *tmi) {
  assert(tmi);
  auto e = tmi->swplRAITbl->length();
  for (size_t i = 0; i < e; i++) {
    RegAllocInfo *rinfo = tmi->swplRAITbl->getWithIdx(i);
    // 後に呼び出すcallSetReg()内でカーネル終端にCOPY命令を追加する
    // 条件に合致したレジスタはnum_useを当該COPY命令に設定して
    // liverangeを伸ばす
    if ((rinfo->vreg > 0) && (rinfo->preg > 0) &&                         // vreg および preg が 0 より大きい
        ((rinfo->num_def > -1) &&                                         // (定義がある) &&
         ((rinfo->num_use == -1) || (rinfo->num_def < rinfo->num_use)) && // (参照がないor定義<参照である) &&
         ((unsigned)rinfo->num_use < rinfo->total_mi)) &&                 // (参照がカーネル終端未満である)
        (tmi->swplEKRITbl->isUseFirstVRegInExcK(rinfo->vreg))) {          // エピローグで参照から始まっている
      /*
       *   bb.5.for.body1: (kernel loop)
       *     $x2(%11) = xxx $x1(%10), 1
       *     $x3(%12) = xxx $x2(%11), 1
       *     |
       *     | この間で$x2が再利用されないよう、liverangeを伸ばす
       *     |
       *     %13 = COPY $x2(%11)    <- callSetReg()にてliveout向けCOPYを追加する
       *   bb.11.for.body1:
       *     %14 = ADD %13, 1
       */
      if( DebugSwplRegAlloc ) {
        dbgs() << "num_use: " << rinfo->num_use << " to " << rinfo->total_mi << "\n";
      }
      rinfo->updateNumUseNoReuse(rinfo->total_mi);
    }
  }
  return;
}

/**
 * @brief  レジスタ情報からレジスタ割り付け対象外であるかをチェックする
 * @param  [in] rai 割り当て済みレジスタ情報
 * @param  [in] ekri_tbl カーネル外のレジスタ情報の表
 * @retval true レジスタ割り付け対象でない
 * @retval false レジスタ割り付け対象である
 */
static bool checkNoPhysRegAlloc(RegAllocInfo *rai, SwplExcKernelRegInfoTbl &ekri_tbl) {
  assert(rai);
  /*
   * 以下の1～3のいずれかに該当する仮想レジスタは、物理レジスタ割り付け対象としない
   * 1. 仮想レジスタ番号が 0
   * 2. isVirtualRegister()の返却値がfalseである
   * 3. カーネル内に定義しかない かつ カーネル外にて参照から始まっていない
   *   上記3.の例）
   *   bb.5.for.body1: (kernel loop)
   *     %10 = ADD $x1, 1  <- %10はカーネル内外で参照されないため、
   *                          割り付ける意味がなく、対象としない
   *   bb.11.for.body1: (epilogue)
   *     %10 = ADD %11, 1
   */
  if ((rai->vreg == 0) || (!Register::isVirtualRegister(rai->vreg)) ||
      ((rai->num_def > -1) && (rai->num_use == -1) &&
       (!ekri_tbl.isUseFirstVRegInExcK(rai->vreg)))) {
    return true;
  }
  return false;
}

/**
 * @brief  自分およびtied相手がレジスタ割り付け対象外であるかの判定
 * @param  [in] own 割り当て済みレジスタ情報
 * @param  [in] rai_tbl 割り当て済みレジスタ情報の表
 * @param  [in] ekri_tbl カーネル外のレジスタ情報の表
 * @retval true レジスタ割り付け対象でない
 * @retval false レジスタ割り付け対象である
 */
static bool isNoPhysRegAlloc(RegAllocInfo *own, SwplRegAllocInfoTbl &rai_tbl,
                             SwplExcKernelRegInfoTbl &ekri_tbl) {
  bool ret = false;
  // 自分が割り付け対象かを調べる
  ret |= checkNoPhysRegAlloc(own, ekri_tbl);

  RegAllocInfo* tied;
  if ((own->tied_vreg != 0) && (tied = rai_tbl.getWithVReg(own->tied_vreg))) {
    if( DebugSwplRegAlloc ) {
      dbgs() << " tied-register. [current](vreg: "
             << printReg(own->vreg, SWPipeliner::TRI) << ", tied_vreg: "
             << printReg(own->tied_vreg, SWPipeliner::TRI) << "), [tied](vreg: "
             << printReg(tied->vreg, SWPipeliner::TRI) << ", tied_vreg: "
             << printReg(tied->tied_vreg, SWPipeliner::TRI) << ", preg: "
             << printReg(tied->preg, SWPipeliner::TRI) << ")\n";
    }
    assert(tied->tied_vreg == own->vreg);
    // tied相手が存在すれば、tied相手に合わせて自分も割り付け対象外となるか否かを調べる
    ret |= checkNoPhysRegAlloc(tied, ekri_tbl);
    // tied相手によって既に自分にも物理レジスタ割り当て済みなら自分は割り当て処理しない
    ret |= ((tied->preg > 0) && (own->preg > 0) && (tied->preg == own->preg));
  }

  return ret;
}

/**
 * @brief  生存区間を基に仮想レジスタへ物理レジスタを割り付ける
 * @param  [in] preg 物理レジスタ
 * @param  [in] rai 割り当て済みレジスタ情報
 * @param  [in] ekri_tbl カーネル外のレジスタ情報の表
 */
static void assignPReg(unsigned preg, RegAllocInfo *rai, SwplRegAllocInfoTbl &rai_tbl) {
  assert(rai);
  rai->preg = preg;
  if (rai->tied_vreg != 0) {
    RegAllocInfo *tied = rai_tbl.getWithVReg(rai->tied_vreg);
    if (tied) tied->preg = preg;
  }
  return;
}

/**
 * @brief  生存区間を基に仮想レジスタへ物理レジスタを割り付ける
 * @param  [in,out] rai_tbl 割り当て済みレジスタ情報の表
 * @param  [in]     ekri_tbl カーネル外のレジスタ情報の表
 * @param  [in]     total_mi カーネル部の総MI数
 * @retval 0 正常終了
 * @retval 0以外 異常終了
 * @details レジスタ再利用時の「最も過去」とは、「出現した順に古い方から」という意
 */
static int physRegAllocWithLiveRange(SwplRegAllocInfoTbl &rai_tbl,
                                     SwplExcKernelRegInfoTbl &ekri_tbl,
                                     unsigned total_mi) {
  int ret = 0;

  // 割り当て済みレジスタ情報でループ
  auto e = rai_tbl.length();
  for (size_t i = 0; i < e; i++) {
    RegAllocInfo *itr_cur =rai_tbl.getWithIdx(i);
    bool allocated = false;

    // レジスタ割り付け対象か
    if (isNoPhysRegAlloc(itr_cur, rai_tbl, ekri_tbl)) {
      if( DebugSwplRegAlloc ) {
        dbgs() << " Following register will be not allocated a physcal register. (vreg: "
               << printReg(itr_cur->vreg, SWPipeliner::TRI) << ", preg: "
               << printReg(itr_cur->preg, SWPipeliner::TRI) << ")\n";
      }
      continue;
    }

    // オプション指定があった場合は、空いている物理レジスタから優先して割り付け
    if ((!allocated) && (SwplRegAllocPrio == PRIO_UNUSE)) {
      const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(itr_cur->vreg);
      for (auto &preg:*trc) {
        // 物理レジスタ番号取得

        // 使ってはいけない物理レジスタは割付けない
        if (nousePhysRegs.contains(preg)) continue;

        // 当該物理レジスタが割り当て可能なレジスタでなければcontinue
        if ((preg == 0) || (!SWPipeliner::MRI->isAllocatable(preg)))
          continue;

        // 割り当て済みリストに登録されていないなら当該物理レジスタを使用
        if( !(rai_tbl.isUsePReg( preg )) ) {
          assignPReg(preg, itr_cur, rai_tbl);
          allocated = true;
          break;
        }
      }
    }

    // 仮想レジスタのレジスタクラスに該当する物理レジスタ群を取得。
    // 当該物理レジスタをキーにレジスタ管理情報の表を見ていき、
    // 「生存区間が自分と重ならない」ものが見つかり次第、当該物理レジスタを割り付け。
    if (!allocated) {
      unsigned preg = rai_tbl.getReusePReg( itr_cur );
      if( preg!=0 ) {
        assignPReg(preg, itr_cur, rai_tbl);
        allocated = true;
      }
    }

    // それでも物理レジスタ割り当てできなかったら異常復帰
    if (!allocated) {
      if( DebugSwplRegAlloc ) {
        dbgs() << " failed to allocate a physical register for virtual register "
               << printReg(itr_cur->vreg) << "\n";
        rai_tbl.dump();
      }
      std::string mistr;
      raw_string_ostream mistream(mistr);

      mistream << "Failed to allocate physical register. VReg="
               << printReg(itr_cur->vreg, SWPipeliner::TRI, 0, SWPipeliner::MRI)
               << ':'
               << printRegClassOrBank(itr_cur->vreg, *SWPipeliner::MRI, SWPipeliner::TRI);
      SWPipeliner::Reason = mistr;
      ret = -1;
      break;
    }
  }

  return ret;
}

/**
 * @brief  レジスタ情報を基にMachineOperand::setReg()を呼び出す
 * @param  [in]     MF MachineFunction
 * @param  [in]     pre_dl カーネルpre処理のデバッグ情報の位置
 * @param  [in]     post_dl カーネルpost処理のデバッグ情報の位置
 * @param  [in,out] tmi 命令列変換情報
 */
static void callSetReg(MachineFunction &MF,
                       DebugLoc pre_dl,
                       DebugLoc post_dl,
                       SwplTransformedMIRInfo *tmi) {
  assert(tmi);
  auto e = tmi->swplRAITbl->length();
  for (size_t i = 0; i < e; i++) {
    RegAllocInfo *rinfo = tmi->swplRAITbl->getWithIdx(i);
    // 仮想レジスタと物理レジスタが有効な値でない限り何もしない
    if ((rinfo->vreg == 0) || (rinfo->preg == 0))
      continue;

    /*
     * Dealing with registers that span the MBB
     *   bb.10.for.body1:
     *     %11 = COPY %10
     *     $x1 = COPY %11             <- Add COPY defining $x1 (livein)
     *     SWPLIVEOUT implicit $x1
     *   bb.5.for.body1: (kernel loop)
     *     SWPLIVEIN implicit-def $x1
     *     $x2(%12) = ADD $x1(%11), 1
     *     SWPLIVEOUT implicit $x2
     *   bb.11.for.body1:
     *     SWPLIVEIN implicit-def $x2
     *     %12 = COPY $x2(%12)        <- Add COPY defining %12 (liveout)
     *     %13 = ADD %12, 1
     */
    if ((rinfo->num_def == -1) || (rinfo->num_def > rinfo->num_use))
      addCopyMIpre(MF, pre_dl, tmi->livein_copy_mis, rinfo->vreg, rinfo->preg);    // livein
    if (((rinfo->num_def > -1) && (tmi->swplEKRITbl->isUseFirstVRegInExcK(rinfo->vreg)))
        || SWPipeliner::currentLoop->containsLiveOutReg(rinfo->vreg))
      addCopyMIpost(MF, post_dl, tmi->liveout_copy_mis, rinfo->vreg, rinfo->preg); // liveout
    // 当該物理レジスタを使用するMachineOperandすべてにsetReg()する
    for (auto *mo : rinfo->vreg_mo) {
      auto preg = rinfo->preg;
      auto subRegIdx = mo->getSubReg();
      if (subRegIdx != 0) {
        preg = SWPipeliner::TRI->getSubReg(rinfo->preg, subRegIdx);
        mo->setSubReg(0);
      }
      mo->setReg(preg);
      mo->setIsRenamable(EnableSetrenamable);
    }
  }
  return;
}

/**
 * @brief  カーネル部のMachineInstrをprintする
 * @param  [in] MF MachineFunction
 * @param  [in] start kernel部の開始インデックス
 * @param  [in] end kernel部の終了インデックス
 * @param  [in] mis mi_tableの情報
 */
static void dumpKernelInstrs(const MachineFunction &MF,
                             const size_t start, const size_t end,
                             const std::vector<MachineInstr*> mis) {
  for (size_t i = start; i < end; ++i) {
    MachineInstr *mi = mis[i];
    if (mi != nullptr) {
      /*
       * SWPLの結果反映中のMIは、MFから紐づいていないため、
       * mi->print(dbgs())でOpcodeがUNKNOWNで出力されてしまう。
       * よって、結果反映前のMFを用いる。
       */
      mi->print(dbgs(),
                /* IsStandalone= */ true,
                /* SkipOpers= */ false,
                /* SkipDebugLoc= */ false,
                /* AddNewLine= */ true,
                MF.getSubtarget().getInstrInfo() );
    }
  }
}

/**
 * @brief  SWPLpass内における仮想レジスタへの物理レジスタ割り付け
 * @param  [in,out] tmi 命令列変換情報
 * @param  [in]     MF  MachineFunction
 * @retval true  物理レジスタ割り付け成功
 * @retval false 物理レジスタ割り付け失敗
 * @note   カーネルループのみが対象。
 */
bool AArch64InstrInfo::SwplRegAlloc(SwplTransformedMIRInfo *tmi,
                                    MachineFunction &MF) const {
  DebugLoc firstDL;
  DebugLoc lastDL;
  std::unordered_set<unsigned> exclude_vreg;

  /// epilogue部の仮想レジスタのリストを作成する
  tmi->swplEKRITbl = new SwplExcKernelRegInfoTbl();
  unsigned num_mi = 0;
  for (size_t i = tmi->kernelEndIndx; i < tmi->epilogEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi != nullptr) {
      ++num_mi;
      createVRegListEpi(mi, num_mi, *(tmi->swplEKRITbl));
    }
  }

  /// kernel部のMachineInstrを対象とする
  /// まず初めに有効な総MI数を数えつつ、割り付け対象としないレジスタのリストを作成する
  unsigned total_mi = 0;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    if (total_mi == 0)
      firstDL = mi->getDebugLoc();
    ++total_mi;

    // MIのオペランド数でループ
    for (MachineOperand& mo:mi->operands()) {
      // レジスタオペランドなら以降の処理継続
      // レジスタ番号 0 はいかなる種類のレジスタも表していない
      unsigned reg = 0;
      if ((!mo.isReg()) || ((reg = mo.getReg()) == 0))
        continue;

      // TODO: 割り付け対象としないレジスタ一覧を作る(仮対処)
      createExcludeVReg(mi, mo, exclude_vreg, reg);
    }
  }

  if( DebugSwplRegAlloc  ) {
    dumpKernelInstrs(MF, tmi->prologEndIndx, tmi->kernelEndIndx, tmi->mis);
  }

  tmi->swplRAITbl = new SwplRegAllocInfoTbl(total_mi);

  num_mi = 0;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    ++num_mi;
    if (num_mi==total_mi)
      lastDL = mi->getDebugLoc();

    // MIのオペランド数でループ
    for (unsigned opIdx = 0, e = mi->getNumOperands(); opIdx != e; ++opIdx) {
      MachineOperand &mo = mi->getOperand(opIdx);
      // レジスタオペランドなら以降の処理継続
      // レジスタ番号 0 はいかなる種類のレジスタも表していない
      unsigned reg = 0;
      if ((!mo.isReg()) || ((reg = mo.getReg()) == 0))
        continue;

      // 各仮想レジスタの生存区間リストを作成する
      if (createLiveRange(mi, opIdx, mo, reg, *(tmi->swplRAITbl), num_mi,
                          total_mi, exclude_vreg, tmi->doVReg) != 0) {
        assert(0 && "createLiveRange() failed");
        return false;
      }
    }
  }

  extendLiveRange(tmi);

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete createLiveRange()\n";
    (*tmi->swplRAITbl).dump();
  }

  // 物理レジスタ情報を割り当てる
  if (physRegAllocWithLiveRange(*(tmi->swplRAITbl),
                                *(tmi->swplEKRITbl), total_mi) != 0) {
    // 割り当て失敗
    return false;
  }

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete physRegAllocWithLiveRange()\n";
  }

  // setReg()呼び出し
  callSetReg(MF, firstDL, lastDL, tmi);

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete callSetReg()\n";
    (*tmi->swplRAITbl).dump();
    (*tmi->swplEKRITbl).dump();
    dumpKernelInstrs(MF, tmi->prologEndIndx, tmi->kernelEndIndx, tmi->mis);
  }

  return true;
}

/**
 * @brief  定義点と参照点から生存区間を求める(再利用禁止版)
 * @details 仮想レジスタごとの生存区間を返す。
 *          定義 < 参照の場合、当該仮想レジスタに割り当たる物理レジスタの
 *          再利用を禁止するため、参照点を最大値(total_mi)にする。
 * @retval -1より大きい値 計算した生存区間
 * @retval -1            計算不能
 */
int RegAllocInfo::calcLiveRangeNoReuse() {
  if ((num_def != -1) && (num_use != -1) && (num_def < num_use)) {
    // 定義の時点で参照がある or 参照の時点で定義がある and
    // 定義の後に参照があるケースでは、再利用させないための値を設定する
    num_use = total_mi;
  }
  return calcLiveRange();
}

/**
 * @brief  定義点と参照点から生存区間を求める
 * @details 仮想レジスタごとの生存区間を求める。
 *          定義点、参照点のどちらかが存在しない場合、計算不能として-1を返す。
 * @retval -1より大きい値 計算した生存区間
 * @retval -1            計算不能
 */
int RegAllocInfo::calcLiveRange() {
  if ( num_def != -1 && num_use != -1 ) {
    // 定義の時点で参照がある or 参照の時点で定義がある
    if (num_def < num_use) {
      // 定義の後に参照があるケース
      return (num_use - num_def);
    } else {
      // 参照の後に定義があるケース
      return (total_mi - num_def + num_use);
    }
  }

  // 定義の時点で参照がない or 参照の時点で定義がない
  return -1;
}

/**
 * @brief  Determine if the LiveRange of rows in the Survival Interval Table overlap.
 * @param  [in]  reginfo1 RegAllocInfo for comparison
 * @param  [in]  reginfo2 RegAllocInfo for comparison
 * @retval true  LiveRange overlap
 * @retval false LiveRange does not overlap
 */
bool SwplRegAllocInfoTbl::isOverlapLiveRange( RegAllocInfo *reginfo1, RegAllocInfo *reginfo2) {
  int def1 = reginfo1->num_def;
  int use1 = reginfo1->num_use;
  
  int def2 = reginfo2->num_def;
  int use2 = reginfo2->num_use;

  if(SWPipeliner::STM->isEnableProEpiCopy()) {
    // // livein case, make sure that the register is not destroyed.
    if (def1<0 || def2<0) return true;
  }

  for(int i=1; i<=(int)total_mi; i++) {    // num_def/num_use starts counting from 1
    bool overlap1 = false;
    bool overlap2 = false;
    if(def1>use1) {
      if( i<=use1 || i>=def1 ) overlap1=true;
    }
    else {
      if( def1<=i && i<=use1 ) overlap1=true;
    }
    
    if(def2>use2) {
      if( i<=use2 || i>=def2 ) overlap2=true;
    }
    else {
      if( def2<=i && i<=use2 ) overlap2=true;
    }
      
    if( overlap1 && overlap2 ) {
      return true;
    }
  }
  return false;
}

/**
 * @brief  SwplExcKernelRegInfoTblにExcKernelRegInfoエントリを追加する
 * @param  [in] vreg 仮想レジスタ番号
 * @param  [in] num_def 定義番号
 * @param  [in] num_use 参照番号
 */
void SwplExcKernelRegInfoTbl::addExcKernelRegInfo(
                                     unsigned vreg,
                                     int num_def,
                                     int num_use) {
  ekri_tbl.push_back({vreg, num_def, num_use});
  return;
}

/**
 * @brief  テーブルの中から、指定された仮想レジスタに該当するエントリを返す
 * @param  [in] vreg 仮想レジスタ番号
 * @retval 非nullptr 指定された仮想レジスタに該当するExcKernelRegInfoへのポインタ
 * @retval nullptr   指定された仮想レジスタに該当するExcKernelRegInfoは存在しない
 */
ExcKernelRegInfo* SwplExcKernelRegInfoTbl::getWithVReg(unsigned vreg) {
  std::vector<ExcKernelRegInfo>::iterator itr =
    find_if(ekri_tbl.begin(), ekri_tbl.end(),
            [&](ExcKernelRegInfo &info){
              return(info.vreg == vreg);
            });
  if (itr == ekri_tbl.end())
    return nullptr;

  return &(*itr);
}

/**
 * @brief  指定された仮想レジスタがカーネル外で参照から始まっている行の有無を返す
 * @param  [in] vreg 仮想レジスタ番号
 * @retval true 指定された仮想レジスタが参照から始まっている行あり
 * @retval false 指定された仮想レジスタが参照から始まっている行なし
 */
bool SwplExcKernelRegInfoTbl::isUseFirstVRegInExcK(unsigned vreg) {
  std::vector<ExcKernelRegInfo>::iterator itr =
    find_if(ekri_tbl.begin(), ekri_tbl.end(),
            [&](ExcKernelRegInfo &info){
              return((info.vreg == vreg) && (info.num_use > -1) &&
                     ((info.num_def == -1) ||           // 参照のみか
                      (info.num_def > info.num_use)));  // 定義>参照か
            });
  if (itr == ekri_tbl.end())
    return false;

  return true;
}

/**
 * @brief  カーネル外レジスタ情報 SwplExcKernelRegInfoTbl のデバッグプリント
 */
void SwplExcKernelRegInfoTbl::dump() {
  dbgs() << "Virtual registers of the epilogue.\n"
         << "No.\tvreg\tdef\tuse\n";
  auto e = ekri_tbl.size();
  for (size_t i = 0; i < e; i++) {
    ExcKernelRegInfo *ri_p = &(ekri_tbl[i]);
    dbgs() << i << "\t"
           << printReg(ri_p->vreg, SWPipeliner::TRI) << "\t" // 仮想レジスタ
           << ri_p->num_def << "\t"      // 仮想レジスタの定義番号
           << ri_p->num_use << "\n";     // 仮想レジスタの参照番号
  }
  return;
}

/**
 * @brief  SwplRegAllocInfoTblにRegAllocInfoエントリを追加する
 * @param  [in] vreg 仮想レジスタ番号
 * @param  [in] num_def 定義番号
 * @param  [in] num_use 参照番号
 * @param  [in] mo llvm::MachineOperand
 * @param  [in] tied_vreg tiedの場合、その相手となる仮想レジスタ
 * @param  [in] preg 物理レジスタ番号
 */
void SwplRegAllocInfoTbl::addRegAllocInfo( unsigned vreg,
                                           int num_def,
                                           int num_use,
                                           MachineOperand *mo,
                                           unsigned tied_vreg,
                                           unsigned preg) {
  const TargetRegisterClass *trc;

  assert(mo);

  unsigned clsid = 0;
  if( vreg != 0 ) {
    trc = SWPipeliner::MRI->getRegClass(vreg);
    clsid = trc->getID();
  }
  rai_tbl.push_back({vreg, preg, num_def, num_use, -1, tied_vreg, clsid, {mo}, total_mi});
  return;
}

/**
 * @brief  テーブルの中から、指定された仮想レジスタに該当するエントリを返す
 * @param  [in] vreg 仮想レジスタ番号
 * @retval 非nullptr 指定された仮想レジスタに該当するRegAllocInfoへのポインタ
 * @retval nullptr   指定された仮想レジスタに該当するRegAllocInfoは存在しない
 */
RegAllocInfo* SwplRegAllocInfoTbl::getWithVReg( unsigned vreg ) {
  auto itr =
    find_if(rai_tbl.begin(), rai_tbl.end(),
            [&](RegAllocInfo &info){
              return(info.vreg == vreg);
            });
  if (itr == rai_tbl.end())
    return nullptr;
  
  return &(*itr);
}

/**
 * @brief  テーブルの中から、再利用可能な物理レジスタ番号を返す
 * @param  [in] rai 割り当て対象のRegAllocInfo
 * @retval 非0 再利用可能な物理レジスタ番号
 * @retval 0   再利用可能な物理レジスタは見つからなかった
 */
unsigned SwplRegAllocInfoTbl::getReusePReg( RegAllocInfo* rai ) {
  assert(rai->preg==0);

  // TODO：現状、翻訳性能を考慮しない作りとなっている。
  // TODO：現在は、表の上から見つかったものを返している。
  //       どの行を再利用対象にするかの論理を入れたい。
  // TODO：物理レジスタに生存区間を紐づけたようなデータを活用するような形で、
  //       この検索を早く実施できるようにしたい。

  // tied相手のレジスタ情報を取得する
  RegAllocInfo *tied = nullptr;
  if (rai->tied_vreg != 0)
    tied = getWithVReg(rai->tied_vreg);

  // レジスタクラスに該当するレジスタごとに、
  // 既に割り当て済みの行を収集し、そのすべての生存区間が重なっていなければ、
  // 再利用可能とする。
  const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(rai->vreg);
  for (auto itr = trc->begin(), itr_end = trc->end();
       itr != itr_end; ++itr) {
    unsigned candidate_preg = *itr;
    std::vector<RegAllocInfo*> ranges;

    // 当該物理レジスタが割り当て可能なレジスタでなければcontinue
    if ((nousePhysRegs.contains(candidate_preg)) || (candidate_preg == 0) ||
        (!SWPipeliner::MRI->isAllocatable(candidate_preg))) {
      continue;
    }

    auto e = rai_tbl.size();
    for (size_t j = 0; j < e; j++) {
      RegAllocInfo *wk_reginfo = &(rai_tbl[j]);
      if( isPRegOverlap( candidate_preg, wk_reginfo->preg ) ) {
        ranges.push_back( wk_reginfo );
      }
    }

    bool isoverlap = false;
    auto ranges_e = ranges.size();
    for(unsigned i=0; i<ranges_e; i++) {
      if ((isOverlapLiveRange(ranges[i], rai)) ||
          ((tied) && (isOverlapLiveRange(ranges[i], tied)))) {
         // 「自分のliverange」もしくは「tied相手のliverange」のどちらかがチェック対象と重なる
        isoverlap = true;
        break;
      }
    }
    
    if(isoverlap == false) {
      return candidate_preg;
    }
  }
  
  return 0;
}

/**
 * @brief  二つの物理レジスタの物理領域が重複するかを判定する
 * @param  [in] preg1 チェック対象の物理レジスタ
 * @param  [in] preg2 チェック対象の物理レジスタ
 * @retval true  重複している
 * @retval false 重複していない
 */
bool SwplRegAllocInfoTbl::isPRegOverlap( unsigned preg1, unsigned preg2 ) {
  return SWPipeliner::TRI->regsOverlap(preg1, preg2);
}

/**
 * @brief  指定された物理レジスタが割り当てに使用されているか否かを判定する
 * @param  [in] preg チェック対象の物理レジスタ
 * @retval true  使用されている
 * @retval false 使用されていない
 */
bool SwplRegAllocInfoTbl::isUsePReg( unsigned preg ) {
  auto e = rai_tbl.size();
  for(unsigned i=0; i<e; i++) {
    unsigned usedpreg = rai_tbl[i].preg;
    // 物理的に重複するレジスタは"使用されている"
    if(isPRegOverlap(usedpreg, preg))
      return true;
  }
  return false;
}

/**
 * @brief  レジスタ割り当て情報 SwplRegAllocInfoTbl のデバッグプリント
 */
void SwplRegAllocInfoTbl::dump() {
  dbgs() << "Information of the registers of the kernel. MI=" << total_mi << "\n"
         << "No.\tvreg\tpreg\tdef\tuse\trange\ttied\tclass\tMO\n";
  auto e = rai_tbl.size();
  for (size_t i = 0; i < e; i++) {
    RegAllocInfo *ri_p = &(rai_tbl[i]);
    dbgs() << i << "\t"
           << printReg(ri_p->vreg, SWPipeliner::TRI) << "\t" // 割り当て済み仮想レジスタ
           << printReg(ri_p->preg, SWPipeliner::TRI) << "\t" // 割り当て済み物理レジスタ
           << ri_p->num_def << "\t"             // 仮想レジスタの定義番号
           << ri_p->num_use << "\t"             // 仮想レジスタの参照番号
           << ri_p->liverange  << "\t"          // 生存区間(参照番号-定義番号)
           << printReg(ri_p->tied_vreg, SWPipeliner::TRI) << "\t";  // tied仮想レジスタ
    // 仮想レジスタのレジスタクラス
    if (ri_p->vreg > 0) {
      const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(ri_p->vreg);
      const char* reg_class_name = SWPipeliner::TRI->getRegClassName(trc);
      dbgs() << reg_class_name << "\t";
    } else {
      dbgs() << ri_p->vreg_classid  << "\t";
    }
    dbgs() << "[ ";
    auto e = ri_p->vreg_mo.size();
    for (size_t l = 0; l < e; l++) {
      // 仮想レジスタのMachineOperand
      MachineOperand *m = ri_p->vreg_mo[l];
      // レジスタオペランドなら出力内容を分かり易くする
      if (m->isReg()) {
        dbgs() << printReg(m->getReg(), SWPipeliner::TRI, m->getSubReg());
      } else {
        m->print(dbgs());
      }
      dbgs() << " ";
    }
    dbgs() << " ]\n";
  }
  return;
}

/**
 * @brief SwplRegAllocInfoTblの初期化関数
 * @param [in] num_of_mi MIの総数
 */
SwplRegAllocInfoTbl::SwplRegAllocInfoTbl(unsigned num_of_mi) {
  total_mi=num_of_mi;
  rai_tbl={};
};

/**
 * @brief レジスタ割り付け情報からliverangeを求める
 * @param [in] range        liverange
 * @param [in] regAllocInfo レジスタ割り付け情報
 */
void SwplRegAllocInfoTbl::setRangeReg(std::vector<int> *range, RegAllocInfo& regAllocInfo, unsigned unitNum) {
  if (regAllocInfo.num_def < 0) {
    for (int i = 1; i <= regAllocInfo.num_use; i++) {
      range->at(i)+=unitNum;
    }
  } else if (regAllocInfo.num_use > 0 && regAllocInfo.num_def >= regAllocInfo.num_use) {
    for (int i=1; i<=regAllocInfo.num_use; i++) {
      range->at(i)+=unitNum;
    }
    for (unsigned i = regAllocInfo.num_def; i <= total_mi ; i++) {
      range->at(i)+=unitNum;
    }
  } else if (regAllocInfo.num_use < 0) {
    for (unsigned i = regAllocInfo.num_def; i <= total_mi ; i++) {
      range->at(i)+=unitNum;
    }
  } else {
    for (int i = regAllocInfo.num_def; i <= regAllocInfo.num_use; i++) {
      range->at(i)+=unitNum;
    }
  }
}

/**
 * @brief Integer/Floating-point/Predicateレジスタに割り当てた最大レジスタ数を数える
 */
void SwplRegAllocInfoTbl::countRegs() {
  std::vector<int> ireg;
  std::vector<int> freg;
  std::vector<int> preg;

  ireg.resize(total_mi+1);
  freg.resize(total_mi+1);
  preg.resize(total_mi+1);

  unsigned rk, units;

  for (auto &regAllocInfo:rai_tbl) {
    if (regAllocInfo.vreg==0) {
      // SWPL化前から実レジスタが割り付けてある
      std::tie(rk, units) = SWPipeliner::TII->getRegKindId(*SWPipeliner::MRI, regAllocInfo.preg);
      if (rk == StmRegKind::getCCRegID() || rk == 0) {
        // CCレジスタは計算から除外する
        continue;
      }
      if (rk == StmRegKind::getIntRegID()) {
        if (nousePhysRegs.contains(regAllocInfo.preg)) continue;
        // レジスタ割付処理で割付禁止レジスタが、割り付けてあるため、計算から除外する
      }
    } else {
      if (regAllocInfo.preg == 0) {
        // 実レジスタを割り付けていないので、計算から除外する
        continue;
      }
      std::tie(rk, units) = SWPipeliner::TII->getRegKindId(*SWPipeliner::MRI, regAllocInfo.vreg);
    }
    if (rk == StmRegKind::getIntRegID()) {
      setRangeReg(&ireg, regAllocInfo, units);
    } else if (rk == StmRegKind::getFloatRegID()) {
      setRangeReg(&freg, regAllocInfo, units);
    } else if (rk == StmRegKind::getPredicateRegID()) {
      setRangeReg(&preg, regAllocInfo, units);
    } else {
      llvm_unreachable("unnown reg class");
    }
  }
  // レジスタ割付では、なぜか１から始まるので、以下のforでは0番目を処理しているがほんの少し無駄になっている
  for (int i:ireg) num_ireg = std::max(num_ireg, i);
  for (int i:freg) num_freg = std::max(num_freg, i);
  for (int i:preg) num_preg = std::max(num_preg, i);
}

/**
 * @brief  Integerレジスタに割り当てた最大レジスタ数を返す
 * @retval 0以上の値 割り当てた最大レジスタ数
 */
int SwplRegAllocInfoTbl::countIReg() {
  if (num_ireg < 0) countRegs();
  return num_ireg;
}

/**
 * @brief  Floating-pointレジスタに割り当てた最大レジスタ数を返す
 * @retval 0以上の値 割り当てた最大レジスタ数
 */
int SwplRegAllocInfoTbl::countFReg()  {
  if (num_freg < 0) countRegs();
  return num_freg;
}

/**
 * @brief  Predicateレジスタに割り当てた最大レジスタ数を返す
 * @retval 0以上の値 割り当てた最大レジスタ数
 */
int SwplRegAllocInfoTbl::countPReg()  {
  if (num_preg < 0) countRegs();
  return num_preg;
}

/**
 * @brief  割り当て可能なIntegerレジスタの数を返す
 * @retval 0以上の値 利用可能なレジスタ数
 */
int SwplRegAllocInfoTbl::availableIRegNumber() const {
  // UNAVAILABLE_REGがXおよびWを定義しているため、数は半分にしている
  return AArch64::GPR64RegClass.getNumRegs()-(UNAVAILABLE_REGS/2);
}

/**
 * @brief  割り当て可能なFloating-pointレジスタの数を返す
 * @retval 0以上の値 利用可能なレジスタ数
 */
int SwplRegAllocInfoTbl::availableFRegNumber() const {
  return AArch64::FPR64RegClass.getNumRegs();
}

/**
 * @brief  割り当て可能なPredicateレジスタの数を返す
 * @retval 0以上の値 利用可能なレジスタ数
 */
int SwplRegAllocInfoTbl::availablePRegNumber() const {
  return AArch64::PPR_3bRegClass.getNumRegs();
}
