//=- AArch64SWPipeliner.cpp - Machine Software Pipeliner Pass -*- c++ -*-----=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 Machine Software Pipeliner Pass.
//
//===----------------------------------------------------------------------===//

#include "AArch64.h"

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/InitializePasses.h"
#include "AArch64TargetTransformInfo.h"

#include "AArch64SWPipeliner.h"
#include "AArch64SwplDataDependenceAnalysis.h"
#include "AArch64SwplPlan.h"
#include "AArch64SwplScr.h"
#include "AArch64SwplTargetMachine.h"
#include "AArch64SwplTransformMIR.h"

using namespace llvm;
using namespace swpl;
#define DEBUG_TYPE "aarch64-swpipeliner"

static cl::opt<bool> DisableSwpl("swpl-disable",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableSensitiveCheck("swpl-sensitive-check",cl::init(false), cl::ReallyHidden);
static cl::opt<unsigned> MaxInstNum("swpl-max-inst-num",cl::init(500), cl::ReallyHidden);
static cl::opt<unsigned> MaxMemNum("swpl-max-mem-num",cl::init(400), cl::ReallyHidden);
static cl::opt<int> TestStm("swpl-test-tm",cl::init(0), cl::ReallyHidden);
static cl::opt<bool> OptionDumpPlan("swpl-debug-dump-plan",cl::init(false), cl::ReallyHidden);

/// Pragmaによるswpのon/offの代わりにSWPL化Loopを絞り込む
static cl::opt<int> TargetLoop("swpl-choice-loop",cl::init(0), cl::ReallyHidden);

namespace swpl {
cl::opt<bool> DebugOutput("swpl-debug",cl::init(false), cl::ReallyHidden);
MachineOptimizationRemarkEmitter *ORE = nullptr;
const TargetInstrInfo *TII = nullptr;
const TargetRegisterInfo *TRI = nullptr;
MachineRegisterInfo *MRI = nullptr;
SwplTargetMachine STM;
AliasAnalysis *AA;

/// swpl-choice-loopで対象Loop特定に利用する
int loopCountForDebug=0;
}

static bool hasRegisterImplicitDefOperand(MachineInstr *MI, unsigned Reg);
static bool isCompMI(MachineInstr *MI, AArch64CC::CondCode CC);

namespace {

struct AArch64SWPipeliner : public MachineFunctionPass {
public:
  static char ID;               ///< PassのID

  MachineFunction *MF = nullptr;
  const MachineLoopInfo *MLI = nullptr;

  /**
   * \brief AArch64SWPipelinerのコンストラクタ
   */
  AArch64SWPipeliner() : MachineFunctionPass(ID) {
    initializeAArch64SWPipelinerPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &mf) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<MachineLoopInfo>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<MachineOptimizationRemarkEmitterPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool doFinalization (Module &) override;

  /**
   * \brief Pass名を取得する
   *
   * -debug-passオプションを使用した際などに表示されるPassの
   * 名前を指定する。
   */
  StringRef getPassName() const override {
    return "AArch64 Software Pipeliner";
  }
private:
  bool canPipelineLoop(MachineLoop &L);
  bool scheduleLoop(MachineLoop &L);
  bool isNonTargetLoop(MachineLoop &L);
  void outputRemarkAnalysis(MachineLoop &L, int msg_id);
  bool shouldOptimize(MachineLoop &L);
  swpl::StmTest *stmTest =nullptr; ///< Stmのテスト用領域
};

struct AArch64PreSWPipeliner : public MachineFunctionPass {
public:
  static char ID;               ///< PassのID

  /**
   * \brief AArch64SWPipelinerのコンストラクタ
   */
  AArch64PreSWPipeliner() : MachineFunctionPass(ID) {
    initializeAArch64PreSWPipelinerPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &mf) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfo>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<MachineOptimizationRemarkEmitterPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool doFinalization (Module &) override;

  /**
   * \brief Pass名を取得する
   *
   * -debug-passオプションを使用した際などに表示されるPassの
   * 名前を指定する。
   */
  StringRef getPassName() const override {
    return "AArch64 Pre Software Pipeliner";
  }
private:
  bool check(MachineLoop &L);
  bool hasPreHeader(MachineLoop &L);
  bool createPreHeader(MachineLoop &L);
  bool hasExit(MachineLoop &L);
  bool createExit(MachineLoop &L);
  MachineRegisterInfo *MRI=nullptr;
  MachineOptimizationRemarkEmitter *ORE = nullptr;
  const TargetInstrInfo *TII = nullptr;
  MachineFunction *MF = nullptr;
};
} // end of anonymous namespace

char AArch64SWPipeliner::ID = 0;

INITIALIZE_PASS_BEGIN(AArch64SWPipeliner, DEBUG_TYPE,
                      "AArch64 Software Pipeliner", false, false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(AArch64SWPipeliner, DEBUG_TYPE,
                    "AArch64 Software Pipeliner", false, false)

/**
 * \brief AArch64SWPipelinerを生成する
 *
 * \retval FunctionPass 生成されたAArch64SWPipeliner
 */
FunctionPass *llvm::createAArch64SWPipelinerPass() {
  return new AArch64SWPipeliner();
}

/**
 * \brief runOnMachineFunction
 *
 * \param MF 対象のMachineFunction
 * \retval true  MF に変更を加えたことを示す
 * \retval false MF を変更していないことを示す
 */
bool AArch64SWPipeliner::runOnMachineFunction(MachineFunction &mf) {
  bool Modified = false;
  loopCountForDebug=0;
  if (!mf.getSubtarget().getSchedModel().hasInstrSchedModel()) {
    // schedmodelを持たないプロセッサ用のコードなので、本最適化は適用しない。
    // llcでmcpuを指定しても意味がない（Clangで-mcpu=a64fx指定が必須）のようだ
    return false;
  }
  MF = &mf;
  MLI = &getAnalysis<MachineLoopInfo>();
  ORE = &getAnalysis<MachineOptimizationRemarkEmitterPass>().getORE();
  TII = MF->getSubtarget().getInstrInfo();
  TRI = MF->getSubtarget().getRegisterInfo();
  MRI = &(MF->getRegInfo());
  AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();

#ifdef STMTEST
  if (TestStm) {
    // Tmの動作テスト(SchedModel確認のため、ここでテストを動作させている)
    if (stmTest ==nullptr) {
      stmTest =new swpl::StmTest(TestStm);
    }
    stmTest->run(mf);
    return false;
  }
#endif

  STM.initialize(*MF);

  for (auto &L : *MLI)
    scheduleLoop(*L);

  return Modified;
}

bool AArch64SWPipeliner::doFinalization(Module &) {
#ifdef STMTEST
  if (TestStm && stmTest !=nullptr) {
    // testで利用する領域の解放
    delete stmTest;
    stmTest =nullptr;
  }
#endif
  return false;
}

static void printDebug(const char *f, const StringRef &msg, const MachineLoop &L) {
  if (!DebugOutput) return;
  errs() << "DBG(" << f << ") " << msg << ":";
  L.getStartLoc().print(errs());
  errs() <<"\n";
}

/**
 * \brief scheduleLoop
 *        Swpl最適化を実施する。
 *        ・対象ループ判定
 *        ・データ抽出
 *        ・スケジューリング
 *        ・スケジューリング結果反映
 *       
 * \param[in] L 対象のMachineLoop
 * \retval true  Swpl最適化を適用した。
 * \retval false Swpl最適化を適用しなかった。
 */
bool AArch64SWPipeliner::scheduleLoop(MachineLoop &L) {
  bool Changed = false;
  for (auto &InnerLoop : L)
    Changed |= scheduleLoop(*InnerLoop);

  /* 自ループが最内でなければ処理対象としない */
  if (L.getSubLoops().size() != 0) {
    return Changed;
  }

  if (!canPipelineLoop(L)) {
    printDebug(__func__, "!!! Can not pipeline loop.", L);
    ORE->emit([&]() {
      return MachineOptimizationRemarkMissed(DEBUG_TYPE, "NotSoftwarePipleined",
                                             L.getStartLoc(), L.getHeader())
             << "Failed to pipeline loop";
    });

    return Changed;
  }

  loopCountForDebug++;
  // TargetLoopが指定された場合、それ以外のSWPL処理はおこなわない
  if (TargetLoop>0 && TargetLoop!=loopCountForDebug) return Changed;

  SwplScr swplScr(L);
  UseMap liveOutReg;
  // SwplLoop::Initialize()でLoop複製されるため、その前に出口Busyレジスタ情報を収集する
  swplScr.collectLiveOut(liveOutReg);

  // データ抽出
  swpl::SwplLoop *loop = SwplLoop::Initialize(L, liveOutReg);
  swpl::SwplDdg *ddg = SwplDdg::Initialize(*loop);

  // スケジューリング
  swpl::SwplPlan* plan = SwplPlan::generatePlan(*ddg);
  if (plan != NULL) {
    if (plan->getPrologCycles() == 0) {
      swpl::ORE->emit([&]() {
        return MachineOptimizationRemarkMissed(DEBUG_TYPE, "NotSoftwarePipleined",
                                               loop->getML()->getStartLoc(),
                                               loop->getML()->getHeader())
                << "This loop is not software pipelined because the software pipelining does not improve the performance.";
      });
      if (DebugOutput) {
        dbgs() << "        : Loop isn't software pipelined because prologue is 0 cycle.\n";
      }

    } else {
      if ( OptionDumpPlan ) {
        plan->dump( dbgs() );
      }
      swpl::SwplTransformMIR tran(*MF, *plan, liveOutReg);
      Changed = tran.transformMIR();
    }
    SwplPlan::destroy( plan );
  } else {
    swpl::ORE->emit([&]() {
      return MachineOptimizationRemarkMissed(DEBUG_TYPE, "NotSoftwarePipleined",
                                             loop->getML()->getStartLoc(),
                                             loop->getML()->getHeader())
              << "This loop is not software pipelined because no schedule is obtained.";
    });
    if (DebugOutput) {
      dbgs() << "        : Loop isn't software pipelined because plan is NULL.\n";
    }
  }
  delete ddg;
  delete loop;

  return Changed;
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
void AArch64SWPipeliner::outputRemarkAnalysis(MachineLoop &L, int msg_id) {
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

/**
 * \brief shouldOptimize
 *        対象のループに対するSwpl最適化指示を判定する。
 *
 * \param[in] L 対象のMachineLoop
 * \retval true  Swpl最適化対象指示がある
 * \retval false Swpl最適化対象指示がない。もしくは最適化抑止指示がある。
 */
bool AArch64SWPipeliner::shouldOptimize(MachineLoop &L) {
  MachineBasicBlock *MBB = L.getTopBlock();
  
  const BasicBlock *BB = MBB->getBasicBlock();
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  const Loop *BBLoop = LI->getLoopFor(BB);

  if (!llvm::enableSWP(BBLoop)) {
    return false;
  }
  return true;
}

/**
 * \brief canPipelineLoop
 *        対象のループがSwpl最適化対象かどうかを判定する。
 *
 * \param[in] L 対象のMachineLoop
 * \retval true  Swpl最適化対象のループである。
 * \retval false Swpl最適化対象のループではない。
 */
bool AArch64SWPipeliner::canPipelineLoop(MachineLoop &L) {
  // ローカルオプションによる機能抑止
  if (DisableSwpl) {
    printDebug(__func__, "[canPipelineLoop:NG] Specified Swpl disable by local option. ", L);
    return false;
  }
  
  // 最適化指示の判定
  if (!shouldOptimize(L)) {
    printDebug(__func__, "[canPipelineLoop:NG] Specified Swpl disable by option/pragma. ", L);
    return false;
  }
  
  // Simd/Unrollの余りループは最適化抑止
  // I/F未定

  // ループ内のBasicBlockが一つではない場合は最適化抑止
  if (L.getNumBlocks() != 1) {
    printDebug(__func__, "[canPipelineLoop:NG] Not a single basic block. ", L);
    outputRemarkAnalysis(L, MsgID_swpl_not_covered_loop_shape);
    return false;
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
bool AArch64SWPipeliner::isNonTargetLoop(MachineLoop &L) {
  MachineBasicBlock *LoopBB = L.getTopBlock();

  AArch64CC::CondCode _NE = AArch64CC::NE;
  AArch64CC::CondCode _GE = AArch64CC::GE;
  MachineBasicBlock::iterator I = LoopBB->getFirstTerminator();
  MachineBasicBlock::iterator E = LoopBB->getFirstNonDebugInstr();
  MachineInstr *BccMI = nullptr;
  MachineInstr *CompMI = nullptr;
  int mem_counter=MaxMemNum;
  AArch64CC::CondCode CC;

  // 命令数はBasickBlockから取得できる
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
    // gnuasm命令である
    auto Op = I->getOpcode();
    if (Op == AArch64::INLINEASM || Op == AArch64::INLINEASM_BR) {
      printDebug(__func__, "pipeliner info:found gnuasm", L);
      outputRemarkAnalysis(L, MsgID_swpl_not_covered_inst);
      return true;
    }
    // DMB命令（fence相当）である
    if (Op == AArch64::DMB) {
      printDebug(__func__, "pipeliner info:found non-target-inst", L);
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


char AArch64PreSWPipeliner::ID = 0;

INITIALIZE_PASS_BEGIN(AArch64PreSWPipeliner, "aarch64-preswpipeliner",
                      "AArch64 Pre Software Pipeliner", false, false)
  INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
  INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(AArch64PreSWPipeliner, "aarch64-preswpipeliner",
                    "AArch64 Pre Software Pipeliner", false, false)

/**
 * \brief AArch64PreSWPipelinerを生成する
 *
 * \retval FunctionPass 生成されたAArch64PreSWPipeliner
 */
FunctionPass *llvm::createAArch64PreSWPipelinerPass() {
  return new AArch64PreSWPipeliner();
}

/**
 * \brief runOnMachineFunction
 *
 * \param MF 対象のMachineFunction
 * \retval true  MF に変更を加えたことを示す
 * \retval false MF を変更していないことを示す
 */
bool AArch64PreSWPipeliner::runOnMachineFunction(MachineFunction &mf) {
  bool Modified = false;
  if (!mf.getSubtarget().getSchedModel().hasInstrSchedModel()) {
    // schedmodelを持たないプロセッサ用のコードなので、本祭最適化は適用しない。
    // llcでmcpuを指定しても意味がない（Clangで-mcpu=a64fx指定が必須）のようだ
    return false;
  }
  auto mli = &getAnalysis<MachineLoopInfo>();
  ORE = &getAnalysis<MachineOptimizationRemarkEmitterPass>().getORE();
  TII = mf.getSubtarget().getInstrInfo();
  MF = &mf;
  MRI=&(mf.getRegInfo());

  for (auto *L :*mli) Modified|=check(*L);

  return Modified;
}

bool AArch64PreSWPipeliner::check(MachineLoop &L) {
  bool Changed = false;
  for (auto &InnerLoop : L)
    Changed |= check(*InnerLoop);

  /* 自ループが最内でなければ処理対象としない */
  if (L.getSubLoops().size() != 0) {
    return Changed;
  }

  auto *body=L.getTopBlock();

  // Loop本体のMBBは２つ以上は無視
  if (L.getNumBlocks()>1) return Changed;

  // 無限Loop等異常なLoopは無視
  if (body->pred_size()==1 || body->succ_size()==1) return Changed;

  // CALLを含んでいれば無視
  bool hasCall=false;
  for (const auto &mi:*body) {
    if (mi.isCall()) {
      hasCall=true;
      break;
    }
  }
  if (hasCall) return Changed;

  // SWPオプションの確認
  const BasicBlock *BB = body->getBasicBlock();
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  const Loop *BBLoop = LI->getLoopFor(BB);
  if (!llvm::enableSWP(BBLoop)) return Changed;

  if (!hasPreHeader(L)) Changed|=createPreHeader(L);
  if (!hasExit(L)) Changed|=createExit(L);

  return Changed;
}

bool AArch64PreSWPipeliner::doFinalization(Module &) {
  return false;
}
bool AArch64PreSWPipeliner::hasPreHeader(MachineLoop &L) {
  if (DebugOutput) {
    dbgs() << "DBG(AArch64PreSWPipeliner::hasPreHeader: " << L;
  }
  auto *body=L.getTopBlock();
  return body->pred_size()==2;
}
bool AArch64PreSWPipeliner::hasExit(MachineLoop &L) {
  auto *body=L.getTopBlock();
  return body->succ_size()==2;
}
bool AArch64PreSWPipeliner::createPreHeader(MachineLoop &L) {
  if (DebugOutput) {
    dbgs() << "DBG(AArch64PreSWPipeliner::createPreHeader: loop-body mir(before)\n";
    dbgs() << *(L.getTopBlock());
  }
  auto *body=L.getTopBlock();
  auto *ph = MF->CreateMachineBasicBlock(body->getBasicBlock());
  auto It = body->getIterator();

  // Bodyの前にpreheaderを挿入
  MF->insert(It, ph);

  // loop-bodyのpred情報収集
  std::vector<MachineBasicBlock*> preds;
  for (auto *p: body->predecessors()) {
    if (p==body) continue;
    preds.push_back(p);
  }
  // predのSuccessorをつなぎなおす
  for (auto *p: preds) {
    // predのBranch命令の飛び先も変更してくれる
    p->ReplaceUsesOfBlockWith(body, ph);
  }

  // bodyのphiを集める
  std::vector<MachineInstr*> phis;
  for (auto &phi:body->phis()) {
    phis.push_back(&phi);
  }

  for (auto *phi:phis) {
    // preheaderに追加するPHIで定義するレジスタ
    Register newReg=MRI->cloneVirtualRegister(phi->getOperand(0).getReg());
    // preheaderに追加するPHI
    auto phPhi=BuildMI(ph, phi->getDebugLoc(), TII->get(AArch64::PHI), newReg);
    Register iv;
    Register t;
    for (auto &op:phi->operands()) {
      if (op.isReg()) t=op.getReg();
      else if (op.isMBB()) {
        if (op.getMBB()==body) {
          // loop-bodyに追加するPHIで参照する誘導変数
          iv=t;
        } else {
          // preheaderに追加するPHIは、loop-body以外の参照を追加する
          phPhi.addReg(t).addMBB(op.getMBB());
        }
      }
    }
    // loop-bodyに追加するPHI
    BuildMI(*body, phi, phi->getDebugLoc(), TII->get(AArch64::PHI), phi->getOperand(0).getReg())
        .addReg(newReg)
        .addMBB(ph)
        .addReg(iv)
        .addMBB(body);
  }
  // loop-bodyの古いPHIを削除する
  for (auto *phi:phis) {
    phi->eraseFromParent();
  }
  // 最後にPHのSuccをBodyに変更する
  ph->addSuccessor(body);
  if (DebugOutput) {
    dbgs() << "DBG(AArch64PreSWPipeliner::createPreHeader: loop-body-mir(after)\n";
    dbgs() << *(L.getTopBlock());
  }

  return true;
}

bool AArch64PreSWPipeliner::createExit(MachineLoop &L) {
  if (DebugOutput) {
    dbgs() << "DBG(AArch64PreSWPipeliner::createExit\n";
    dbgs() << *(L.getTopBlock());
  }
  return false;
}
