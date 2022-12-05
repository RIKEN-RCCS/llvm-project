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

#include "AArch64SwplTargetMachine.h"
#include "AArch64TargetTransformInfo.h"
#include "SWPipeliner.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace swpl;

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
  if (!DebugOutput) return;
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
    ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop cannot be software pipelined because the loop contains a branch instruction.";
    });
    break;
  case MsgID_swpl_many_insts:
    ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop is not software pipelined because the loop contains too many instructions.";
    });
    break;
  case MsgID_swpl_many_memory_insts:
    ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop is not software pipelined because the loop contains too many instructions accessing memory.";
    });
    break;
  case MsgID_swpl_not_covered_inst:
    ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "NotSoftwarePipleined",
                                               L.getStartLoc(), L.getHeader())
             << "This loop cannot be software pipelined because the loop contains an instruction, such as function call,which is not supported by software pipelining.";
    });
    break;
  case MsgID_swpl_not_covered_loop_shape:
    ORE->emit([&]() {
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
    if (TII->isNonTargetMI4SWPL(*I)) {
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
      const auto phiIter=MRI->def_instr_begin(I->getOperand(1).getReg());
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
    if (TII->isNonTargetMI4SWPL(*I)) {
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
    if (TII) {
      dbgs() << TII->getName(I->getOpcode());
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
}


namespace swpl {

/// 利用資源パターンを生成するための生成過程で使われるデータ構造
struct work_node {
  StmResourceId id; ///< 利用資源
  int startCycle=0; ///< 開始サイクル
  llvm::SmallVector<work_node*, 8> nodes; ///< 次の利用資源

  /// constructor
  explicit work_node(StmResourceId id):id(id){}

  /// destructor
  ~work_node() {
    /// 次の利用資源を削除する
    for (auto *n:nodes) delete n;
  }

  /// 前の利用資源につなげる
  ///
  /// \param [in] p つなげる利用資源
  void append(work_node*p) {
    if (p==nullptr) return;
    nodes.push_back(p);
  }

  /// 資源の利用パターンを生成する
  ///
  /// \param [in] SM SchedModel
  /// \param [out] stmPipelines 生成結果
  void gen_patterns(llvm::TargetSchedModel&SM, StmPipelinesImpl &stmPipelines) {
    std::vector<StmResourceId> ptn;
    std::vector<int> cycle;
    StmPatternId patternId=0;
    gen_pattern(SM, patternId, ptn, cycle, stmPipelines);
  }

  /// 資源の利用パターンを生成する
  ///
  /// \param [in] SM SchedModel
  /// \param [in] patternId 利用資源パターンのID
  /// \param [in] ptn 利用資源パターン
  /// \param [in] cycle 開始サイクル
  /// \param [out] stmPipelines 生成結果
  void gen_pattern(llvm::TargetSchedModel&SM, StmPatternId &patternId, std::vector<StmResourceId> ptn, std::vector<int> cycle,
                   StmPipelinesImpl &stmPipelines) {
    // 引数：ptnはコピーコンストラクタで複製させている。
    if (id!=llvm::A64FXRes::PortKind::P_NULL) {
      ptn.push_back(id);
      cycle.push_back(startCycle);
      if (nodes.empty()) {
        AArch64StmPipeline *t=new AArch64StmPipeline(SM);
        stmPipelines.push_back(t);
        t->patternId=patternId++;
        for (auto resource:ptn) {
          t->resources.push_back(resource);
        }
        for (auto stage:cycle) {
          t->stages.push_back(stage);
        }
        return;
      }
    }
    for ( work_node* c : nodes ) {
      // and-node
      c->gen_pattern(SM, patternId, ptn, cycle, stmPipelines);
    }
  }
};

}

using nodelist=std::vector<work_node*>;

/// 利用資源パターン生成の準備
/// \param [in] sm SchedModel
/// \param [in,out] next_target 次の資源をつなげる親
/// \param [in] target
/// \param [in] portKindList
/// \param [in] cycle 開始サイクル
static void makePrePattern(llvm::TargetSchedModel sm, nodelist&next_target, nodelist&target, const llvm::SmallVectorImpl<A64FXRes::PortKind> &portKindList, int cycle) {
  for (const auto portKind: portKindList) {
    for (auto *t:target) {
      work_node *p=new work_node(portKind);
      p->startCycle=cycle;
      t->nodes.push_back(p);
      next_target.push_back(p);
    }

  }
}

/// 利用資源パターン生成の準備
/// \param [in] sm SchedModel
/// \param [in] resInfo
/// \param [in] mi 対象のMachineInstr
/// \return 利用資源パターンを生成するための中間表現
/// @note
/// stage値はAArch64A64FXResInfoがstartCycleを真面目に対応しているため、歯抜けになることがある。<br>
/// スケジューリングの際に問題が出たら、歯抜けステージ値が無くなるよう、「0,1,7」から「0,1,2」に変更する。（2020年11月30日鎌塚氏より指示）<br>
/// 例：FMAXNMPv2i64pという命令では、以下のようにstage値が0,1,7と歯抜けになる。<br>
/// (パターン=0) stage/resource: 0/FLA, 1/FLA, 7/FLA<br>
/// (パターン=1) stage/resource: 0/FLA, 1/FLA, 7/FLB
static work_node* makePrePatterns(llvm::TargetSchedModel& sm, const llvm::AArch64A64FXResInfo& resInfo,  const llvm::MachineInstr& mi) {
  work_node *root=nullptr;
  nodelist *target=nullptr;
  nodelist *next_target=nullptr;
  auto *IResDesc = resInfo.getInstResDesc(mi);
  if (IResDesc==nullptr) {
    llvm_unreachable("AArch64A64FXResInfo::getInstResDesc() is nullptr");
  }
  assert(IResDesc!=nullptr);
  const auto &cycleList=IResDesc->getStartCycleList();
  int ix=0;
  for (const auto &PRE : IResDesc->getResList()) {

    if (root==nullptr) {
      root=new work_node(llvm::A64FXRes::PortKind::P_NULL);
      target=new nodelist;
      next_target=new nodelist;
      target->push_back(root);
    }
    makePrePattern(sm, *next_target, *target, PRE, cycleList[ix++]);
    nodelist* t=target;
    target=next_target;
    next_target=t;
    t->clear();
  }
  if  (target!=nullptr) {
    target->clear();
    delete target;
  }
  if (next_target!=nullptr) {
    next_target->clear();
    delete next_target;
  }
  return root;
}

void AArch64SwplTargetMachine::initialize(const llvm::MachineFunction &mf) {
  if (MF==nullptr) {
    const llvm::TargetSubtargetInfo &ST = mf.getSubtarget();
    SM.init(&ST);
    ResInfo=new AArch64A64FXResInfo(ST);
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_FLA]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_FLB]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EXA]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EXB]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EAGA]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EAGB]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_PRX]=1;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_BR]=1;
    numResource=8; // 資源管理がSchedModelとは別になったので、ハードコードする
  }
  MF=&mf;
  MRI=&(MF->getRegInfo());
}

unsigned int AArch64SwplTargetMachine::getFetchBandwidth(void) const {
  return getRealFetchBandwidth()+OptionVirtualFetchWidth;
}

unsigned int AArch64SwplTargetMachine::getRealFetchBandwidth(void) const {
  return OptionRealFetchWidth;
}

int AArch64SwplTargetMachine::getNumSameKindResources(StmResourceId resource) {
  int num=tmNumSameKindResources[resource];
  assert(num && "AArch64SwplTargetMachine::getNumSameKindResources: invalid resource");
  return num;
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

const char *AArch64StmPipeline::getResourceName(StmResourceId resource) {
  const char *name="";
  switch (resource) {
  case llvm::A64FXRes::PortKind::P_FLA:name="FLA";break;
  case llvm::A64FXRes::PortKind::P_FLB:name="FLB";break;
  case llvm::A64FXRes::PortKind::P_EXA:name="EXA";break;
  case llvm::A64FXRes::PortKind::P_EXB:name="EXB";break;
  case llvm::A64FXRes::PortKind::P_EAGA:name="EAGA";break;
  case llvm::A64FXRes::PortKind::P_EAGB:name="EAGB";break;
  case llvm::A64FXRes::PortKind::P_PRX:name="PRX";break;
  case llvm::A64FXRes::PortKind::P_BR:name="BR";break;
  default:
    llvm_unreachable("unknown resourceid");
  }
  return name;
}

int AArch64SwplTargetMachine::getNumSlot(void) const {
  return getFetchBandwidth();
}

int AArch64SwplTargetMachine::computeMemFlowDependence(const llvm::MachineInstr *, const llvm::MachineInstr *) const {
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

const AArch64StmPipeline *
AArch64SwplTargetMachine::getPipeline(const MachineInstr &mi,
                                  StmPatternId patternid) {
  auto *tps= getPipelines(mi);
  if (tps==nullptr) return nullptr;
  if (patternid >= tps->size()) return nullptr;
  auto *t=(*tps)[patternid];
  return t;
}

StmPipelinesImpl *
AArch64SwplTargetMachine::generateStmPipelines(const MachineInstr &mi) {

  work_node *t=nullptr;
  if (isImplimented(mi)) {
    t=makePrePatterns(SM, *ResInfo, mi);
    if (t==nullptr) {
      if (DebugStm)
        dbgs() << "DBG(AArch64SwplTargetMachine::generateStmPipelines): makePrePatterns() is nullptr. MIR=" << mi;
      return nullptr;
    }
  }
  auto *pipelines=new StmPipelines;
  if (t) {
    t->gen_patterns(SM, *pipelines);
    delete t;
  } else {
    pipelines->push_back(new AArch64StmPipeline(SM));
  }
  if (DebugStm) {
    for (auto*pipeline:*pipelines) {
      pipeline->print(dbgs());
    }
  }
  return pipelines;
}
int AArch64SwplTargetMachine::computeRegFlowDependence(const llvm::MachineInstr* def, const llvm::MachineInstr* use) const {
  const auto *IResDesc=ResInfo->getInstResDesc(*def);
  if (IResDesc==nullptr) return 1;
  return IResDesc->getLatency();
}

int AArch64SwplTargetMachine::computeMemAntiDependence(const llvm::MachineInstr *, const llvm::MachineInstr *) const {
  return 1;
}

int AArch64SwplTargetMachine::computeMemOutputDependence(const llvm::MachineInstr *, const llvm::MachineInstr *) const {
  return 1;
}

int AArch64SwplTargetMachine::getMinNResources(StmOpcodeId opcode, StmResourceId resource) {
  auto *tps= stmPipelines[opcode];
  int min_n=INT_MAX;
  for (auto *t:*tps) {
    int n=t->getNResources(resource);
    if (n)
      min_n=std::min(min_n, n);
  }
  if (min_n==INT_MAX)
    return 0;
  return min_n;
}

unsigned int AArch64SwplTargetMachine::getNumResource(void) const {
  return numResource;
}

bool AArch64SwplTargetMachine::isImplimented(const llvm::MachineInstr&mi) const {
  if (OptionCopyIsVirtual) {
    if (mi.isCopy()) return false;
  }

  return ResInfo->getInstResDesc(mi)!=nullptr;
}

bool AArch64SwplTargetMachine::isPseudo(const MachineInstr &mi) const {
  return !isImplimented(mi);
}

#ifdef STMTEST

void StmX4StmTest::init(llvm::MachineFunction&mf, bool first, int TestID) {
  AArch64SwplTargetMachine::initialize(mf);
  if (TestID==1) {

    const char *result="OK";
    const char *firstcall=first?"first-call":"not first-call";

    if (MF!=&mf || MRI==nullptr || tmNumSameKindResources.empty()) {
      result="NG";
    }
    dbgs() << "<<<TEST: 001 AArch64SwplTargetMachine>>>\n";
    dbgs() << "DBG(StmX::init): " << firstcall << " is  " << result << "\n";
  }
}

#define DEF_RESOURCE(N) {llvm::A64FXRes::PortKind::P_##N, #N}

void StmTest::run(llvm::MachineFunction&MF) {
  bool firstCall= STM.getMachineRegisterInfo()==nullptr;
  DebugStm = true;
  STM.init(MF, firstCall, TestID);
  const std::map<StmResourceId, const char *> resources={
          DEF_RESOURCE(FLA),
          DEF_RESOURCE(FLB),
          DEF_RESOURCE(EXA),
          DEF_RESOURCE(EXB),
          DEF_RESOURCE(EAGA),
          DEF_RESOURCE(EAGB),
          DEF_RESOURCE(PRX),
          DEF_RESOURCE(BR)
  };
  const std::vector<StmResourceId> resourceIds= {
          llvm::A64FXRes::PortKind::P_FLA,
          llvm::A64FXRes::PortKind::P_FLB,
          llvm::A64FXRes::PortKind::P_EXA,
          llvm::A64FXRes::PortKind::P_EXB,
          llvm::A64FXRes::PortKind::P_EAGA,
          llvm::A64FXRes::PortKind::P_EAGB,
          llvm::A64FXRes::PortKind::P_PRX,
          llvm::A64FXRes::PortKind::P_BR
  };
  if (TestID==100) {
    // MIRに対応した資源情報が揃っているか確認する
    dbgs() << "<<<TEST: 100>>>\n";
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        dbgs() << "*************************\nmir:" << instr;
        if (STM.isPseudo(instr)) {
          dbgs() << "Pseudo\n";
        }
        (void)STM.getPipelines(instr);
        dbgs() << "latency:" << STM.computeRegFlowDependence(&instr, nullptr) << "\n";
      }
    }
  } else if (TestID==1) {
    if (!firstCall) return;
    dbgs() << "<<<TEST: 001-01 latency>>>\n";
    int defaultStoreLatency = OptionStoreLatency;
    int defaultFlowDep = OptionFlowDep;
    dbgs() << "option:swpl-store-latency is default, swpl-flow-dep is default \n";
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is default, swpl-flow-dep is 0 \n";
    OptionFlowDep = 0;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is default, swpl-flow-dep is 1 \n";
    OptionFlowDep = 1;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";

    OptionStoreLatency = 0;
    dbgs() << "option:swpl-store-latency is 0, swpl-flow-dep is default \n";
    OptionFlowDep = defaultFlowDep;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 0, swpl-flow-dep is 0 \n";
    OptionFlowDep = 0;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 0, swpl-flow-dep is 1 \n";
    OptionFlowDep = 1;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";

    OptionStoreLatency = 1;
    dbgs() << "option:swpl-store-latency is 1, swpl-flow-dep is default \n";
    OptionFlowDep = defaultFlowDep;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 1, swpl-flow-dep is 0 \n";
    OptionFlowDep = 0;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 1, swpl-flow-dep is 1 \n";
    OptionFlowDep = 1;
    dbgs() << "AArch64SwplTargetMachine::computeMemFlowDependence():" << STM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemAntiDependence():" << STM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "AArch64SwplTargetMachine::computeMemOutputDependence():" << STM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    OptionFlowDep = defaultFlowDep;
    OptionStoreLatency = defaultStoreLatency;

    dbgs() << "<<<TEST: 001-02 Bandwidth>>>\n";
    int defaultRealFetchWidth = OptionRealFetchWidth;
    int defaultVirtualFetchWidth = OptionVirtualFetchWidth;
    dbgs() << "option:swpl-real-fetch-width is default, swpl-virtual-fetch-width is default\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 0;
    dbgs() << "option:swpl-real-fetch-width is default, swpl-virtual-fetch-width is 0\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 1;
    dbgs() << "option:swpl-real-fetch-width is default, swpl-virtual-fetch-width is 1\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";

    OptionRealFetchWidth = 0;
    OptionVirtualFetchWidth = defaultVirtualFetchWidth;
    dbgs() << "option:swpl-real-fetch-width is 0, swpl-virtual-fetch-width is default\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 0;
    dbgs() << "option:swpl-real-fetch-width is 0, swpl-virtual-fetch-width is 0\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 1;
    dbgs() << "option:swpl-real-fetch-width is 0, swpl-virtual-fetch-width is 1\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";

    OptionRealFetchWidth = 1;
    OptionVirtualFetchWidth = defaultVirtualFetchWidth;
    dbgs() << "option:swpl-real-fetch-width is 1, swpl-virtual-fetch-width is default\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 0;
    dbgs() << "option:swpl-real-fetch-width is 1, swpl-virtual-fetch-width is 0\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 1;
    dbgs() << "option:swpl-real-fetch-width is 1, swpl-virtual-fetch-width is 1\n";
    dbgs() << "AArch64SwplTargetMachine::getFetchBandwidth():" << STM.getFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getRealFetchBandwidth():" << STM.getRealFetchBandwidth() << ", ";
    dbgs() << "AArch64SwplTargetMachine::getNumSlot():" << STM.getNumSlot() << "\n";

    OptionRealFetchWidth = defaultRealFetchWidth;
    OptionVirtualFetchWidth = defaultVirtualFetchWidth;


    dbgs() << "<<<TEST: 001-03 getNumResource>>>\n";
    dbgs() << "AArch64SwplTargetMachine::getNumResource():" << STM.getNumResource() << "\n";
    dbgs() << "<<<TEST: 001-04 getNumSameResource>>>\n";
    for (const auto &rc:resourceIds) {
      // 同じ資源数を出力
      dbgs() << "AArch64SwplTargetMachine::getNumSameKindResources(" << resources.at(rc) << "):" << STM.getNumSameKindResources(rc) << "\n";
    }

    dbgs() << "<<<TEST: 001-05 by MachineInstr/>>>\n";
    bool alreadyLD1W=false;
    bool alreadyST1W=false;
    bool alreadySUBSXri=false;
    bool alreadyPHI=false;
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        bool target=false;
        if (instr.getOpcode()==AArch64::LD1W && !alreadyLD1W) {
          alreadyLD1W = true;
          target = true;
        } else if (instr.getOpcode()==AArch64::ST1W && !alreadyST1W) {
          alreadyST1W = true;
          target = true;
        } else if (instr.getOpcode()==AArch64::SUBSXri && !alreadySUBSXri) {
          alreadySUBSXri = true;
          target = true;
        } else if (instr.getOpcode()==AArch64::PHI && !alreadyPHI) {
          alreadyPHI = true;
          target = true;
        }
        if (target) {
          bool isPseudo = STM.isPseudo(instr);
          dbgs() << "**\nmir:" << instr;
          if (!isPseudo)
            dbgs() << "AArch64SwplTargetMachine::computeRegFlowDependence(): " << STM.computeRegFlowDependence(&instr, &instr) << ", ";
          dbgs() << "AArch64SwplTargetMachine::isIssuedOneByOne(): " << STM.isIssuedOneByOne(instr) << ", ";
          dbgs() << "AArch64SwplTargetMachine::isPseudo(): " << isPseudo << "\n";
        }
      }
    }

  } else if (TestID==2) {
    dbgs() << "<<<TEST: 002 StmRegKindNum>>>\n";
    dbgs() << "<<<TEST: 002-01 getKindNum>>>\n";
    dbgs() << "AArch64StmRegKind::getKindNum():" << AArch64StmRegKind(*MRI).getKindNum() << "\n";
    dbgs() << "<<<TEST: 002-02 >>>\n";
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        dbgs() << "**\nmir:" << instr;
        int opix = 0;
        for (const auto &op:instr.operands()) {
          opix++;
          dbgs() << "operator(ix=" << opix << "): " << op << "\n";
          if (!op.isReg())
            continue;
          // register kind
          const auto usereg = op.getReg();
          auto regkind = TII->getRegKind(*MRI, usereg);
          regkind->print(dbgs());
          dbgs() << "AArch64StmRegKind::isInteger:" << regkind->isInteger() << " isFloating:" << regkind->isFloating()
                 << " isPredicate:" << regkind->isPredicate() << " isisCCRegister:" << regkind->isCCRegister()
                 << " isIntegerCCRegister:" << regkind->isIntegerCCRegister()
                 << " isAllocated:" << regkind->isAllocalted() << " getNum:" << regkind->getNum() << "\n";
          delete regkind;
        }
      }
    }
  } else if (TestID==3) {
    bool alreadyLD1W = false;
    bool alreadyST1W = false;
    bool alreadySUBSXri = false;
    bool alreadyFMAXNMPv2i64p = false;
    bool alreadyADDVv4i32v = false;
    bool alreadyST2Twov4s_POST = false;
    bool alreadyBFMXri = false;
    dbgs() << "<<<TEST: 003 AArch64StmPipeline>>>\n";
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        bool target = false;
        if (instr.getOpcode() == AArch64::LD1W && !alreadyLD1W) {
          alreadyLD1W = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::ST1W && !alreadyST1W) {
          alreadyST1W = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::SUBSXri && !alreadySUBSXri) {
          alreadySUBSXri = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::FMAXNMPv2i64p && !alreadyFMAXNMPv2i64p) {
          alreadyFMAXNMPv2i64p = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::ADDVv4i32v && !alreadyADDVv4i32v) {
          alreadyADDVv4i32v = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::ST2Twov4s_POST && !alreadyST2Twov4s_POST) {
          alreadyST2Twov4s_POST = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::BFMXri && !alreadyBFMXri) {
          alreadyBFMXri = true;
          target = true;
        }
        if (target) {
          dbgs() << "OPCODE: " << TII->getName(instr.getOpcode()) << "\n";
          const auto *pipes = STM.getPipelines(instr);
          assert(pipes && pipes->size() >= 1);
          dbgs() << "AArch64StmPipeline::getPipeline(" << TII->getName(instr.getOpcode()) << ", 0): ";
          const AArch64StmPipeline *p = STM.getPipeline(instr, 0);
          p->print(dbgs());
          p = STM.getPipeline(instr, 1000);
          dbgs() << "AArch64StmPipeline::getPipeline(" << TII->getName(instr.getOpcode()) << ", 1000): " << (p?"NG\n":"OK\n");
          for (const auto &rc:resourceIds) {
            int ptn = 0;
            dbgs() << "AArch64StmPipeline::getNResources(" << resources.at(rc) << "):";
            char sep = ' ';
            for (const auto *pipe:*pipes) {
              dbgs() << sep << " ptn=" << ptn++ << "/N=" << pipe->getNResources(rc);
              sep = ',';
            }
            dbgs() << "\n";
            dbgs() << "AArch64SwplTargetMachine::getMinNResources(" << resources.at(rc) << "): "
                   << STM.getMinNResources(instr.getOpcode(), rc) << "\n";
          }
        }
      }
    }
  }
}
#endif
