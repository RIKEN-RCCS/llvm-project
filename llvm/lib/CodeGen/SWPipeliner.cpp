//=- SWPipeliner.cpp - Machine Software Pipeliner Pass -*- c++ -*------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Machine Software Pipeliner Pass.
//
//===----------------------------------------------------------------------===//


#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/SwplTargetMachine.h"
#include "llvm/InitializePasses.h"

#include "SWPipeliner.h"
#include "SwplPlan.h"
#include "SwplScr.h"
#include "SwplTransformMIR.h"
#include "llvm/Support/FormatVariadic.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpipeliner"

static cl::opt<bool> OptionDumpPlan("swpl-debug-dump-plan",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnablesplitInst_swp_pre("swpl-splitinst-swppre",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableSwpl("swpl-disable",cl::init(false), cl::ReallyHidden);


/// Pragmaによるswpのon/offの代わりにSWPL化Loopを絞り込む
static cl::opt<int> TargetLoop("swpl-choice-loop",cl::init(0), cl::ReallyHidden);
static cl::opt<bool> DebugOutput("swpl-debug",cl::init(false), cl::ReallyHidden);




namespace llvm {

bool SWPipeliner::isDebugOutput() {
  return ::DebugOutput;
}

MachineOptimizationRemarkEmitter *SWPipeliner::ORE = nullptr;
const TargetInstrInfo *SWPipeliner::TII = nullptr;
const TargetRegisterInfo *SWPipeliner::TRI = nullptr;
MachineRegisterInfo *SWPipeliner::MRI = nullptr;
SwplTargetMachine *SWPipeliner::STM = nullptr;
AliasAnalysis *SWPipeliner::AA = nullptr;
std::string SWPipeliner::Reason;
SwplLoop *SWPipeliner::currentLoop = nullptr;

/// loop normalization pass for SWPL
struct SWPipelinerPre : public MachineFunctionPass {
public:
  static char ID;               ///< PassのID

  /**
   * \brief SWPipelinerPreのコンストラクタ
   */
  SWPipelinerPre() : MachineFunctionPass(ID) {
    initializeSWPipelinerPrePass(*PassRegistry::getPassRegistry());
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
    return "Pre Software Pipeliner";
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

char SWPipeliner::ID = 0;

INITIALIZE_PASS_BEGIN(SWPipeliner, DEBUG_TYPE,
                      "Software Pipeliner", false, false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(SWPipeliner, DEBUG_TYPE,
                    "Software Pipeliner", false, false)

namespace llvm {

/**
 * \brief SWPipelinerを生成する
 *
 * \retval FunctionPass 生成されたSWPipeliner
 */
FunctionPass *createSWPipelinerPass() {
  return new SWPipeliner();
}
}

/**
 * \brief runOnMachineFunction
 *
 * \param MF 対象のMachineFunction
 * \retval true  MF に変更を加えたことを示す
 * \retval false MF を変更していないことを示す
 */
bool SWPipeliner::runOnMachineFunction(MachineFunction &mf) {
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
  STM = TII->getSwplTargetMachine();
  Reason = "";

  STM->initialize(*MF);

  for (auto &L : *MLI) {
    scheduleLoop(*L);
    assert(Reason.empty());
  }

  return Modified;
}

bool SWPipeliner::doFinalization(Module &) {
  return false;
}

static void printDebug(const char *f, const StringRef &msg, const MachineLoop &L) {
  if (!SWPipeliner::isDebugOutput()) return;
  errs() << "DBG(" << f << ") " << msg << ":";
  L.getStartLoc().print(errs());
  errs() <<"\n";
}

void SWPipeliner::remarkMissed(const char *msg, MachineLoop &L) {
  std::string msg1=msg;
  if (!SWPipeliner::Reason.empty()) {
    msg1 = SWPipeliner::Reason;
    SWPipeliner::Reason = "";
  }

  ORE->emit([&]() {
    return MachineOptimizationRemarkMissed(DEBUG_TYPE, "NotSoftwarePipleined",
                                           L.getStartLoc(), L.getHeader())
           << msg1;
  });
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
bool SWPipeliner::scheduleLoop(MachineLoop &L) {
  bool Changed = false;
  for (auto &InnerLoop : L)
    Changed |= scheduleLoop(*InnerLoop);

  /* 自ループが最内でなければ処理対象としない */
  if (L.getSubLoops().size() != 0) {
    return Changed;
  }
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


  if (!TII->canPipelineLoop(L)) {
    printDebug(__func__, "!!! Can not pipeline loop.", L);
    remarkMissed("Failed to pipeline loop", L);

    return Changed;
  }

  loopCountForDebug++;
  // TargetLoopが指定された場合、それ以外のSWPL処理はおこなわない
  if (TargetLoop>0 && TargetLoop!=loopCountForDebug) return Changed;

  SwplScr swplScr(L);
  SwplScr::UseMap liveOutReg;
  // SwplLoop::Initialize()でLoop複製されるため、その前に出口Busyレジスタ情報を収集する
  swplScr.collectLiveOut(liveOutReg);

  // データ抽出
  currentLoop = SwplLoop::Initialize(L, liveOutReg);
  SwplDdg *ddg = SwplDdg::Initialize(*currentLoop);

  // スケジューリング
  SwplPlan* plan = SwplPlan::generatePlan(*ddg);
  if (plan != NULL) {
    if ( OptionDumpPlan ) {
      plan->dump( dbgs() );
    }
    if (plan->getPrologCycles() == 0) {
      remarkMissed("This loop is not software pipelined because the software pipelining does not improve the performance.",
                   *currentLoop->getML());
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "        : Loop isn't software pipelined because prologue is 0 cycle.\n";
      }

    } else {
      SwplTransformMIR tran(*MF, *plan, liveOutReg);
      Changed = tran.transformMIR();
      if (!Changed) {
        remarkMissed("This loop is not software pipelined because no schedule is obtained.", *currentLoop->getML());
      }
    }
    SwplPlan::destroy( plan );
  } else {
    remarkMissed("This loop is not software pipelined because no schedule is obtained.", *currentLoop->getML());
    if (SWPipeliner::isDebugOutput()) {
      dbgs() << "        : Loop isn't software pipelined because plan is NULL.\n";
    }
  }
  delete ddg;
  delete currentLoop;
  currentLoop = nullptr;

  return Changed;
}


/**
 * \brief shouldOptimize
 *        対象のループに対するSwpl最適化指示を判定する。
 *
 * \param[in] L 対象のMachineLoop
 * \retval true  Swpl最適化対象指示がある
 * \retval false Swpl最適化対象指示がない。もしくは最適化抑止指示がある。
 */
bool SWPipeliner::shouldOptimize(MachineLoop &L) {
  MachineBasicBlock *MBB = L.getTopBlock();

  const BasicBlock *BB = MBB->getBasicBlock();
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  const Loop *BBLoop = LI->getLoopFor(BB);

  if (!enableSWP(BBLoop)) {
    return false;
  }
  return true;
}

char SWPipelinerPre::ID = 0;

INITIALIZE_PASS_BEGIN(SWPipelinerPre, "swpipelinerpre",
                      "Software Pipeliner Pre", false, false)
  INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
  INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(SWPipelinerPre, "swpipelinerpre",
                    "Software Pipeliner Pre", false, false)

namespace llvm {

/**
 * \brief SWPipelinerPreを生成する
 *
 * \retval FunctionPass 生成されたSWPipelinerPre
 */
FunctionPass *createSWPipelinerPrePass() {
  return new SWPipelinerPre();
}
}

/**
 * \brief runOnMachineFunction
 *
 * \param MF 対象のMachineFunction
 * \retval true  MF に変更を加えたことを示す
 * \retval false MF を変更していないことを示す
 */
bool SWPipelinerPre::runOnMachineFunction(MachineFunction &mf) {
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

bool SWPipelinerPre::check(MachineLoop &L) {
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
  if (!enableSWP(BBLoop)) return Changed;

  if (!hasPreHeader(L)) Changed|=createPreHeader(L);
  if (!hasExit(L)) Changed|=createExit(L);

  if (EnablesplitInst_swp_pre) {
    SmallVector<MachineInstr *, 2> delete_mi;
    for (auto &mi:*body) {
      if (TII->splitPrePostIndexInstr(*body, mi, nullptr, nullptr)) {
        Changed=true;

        // MBBから削除する命令の収集
        delete_mi.push_back(&mi);
      }
    }
    for (auto *m : delete_mi)
      m->eraseFromParent();
  }

  return Changed;
}

bool SWPipelinerPre::doFinalization(Module &) {
  return false;
}
bool SWPipelinerPre::hasPreHeader(MachineLoop &L) {
  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "DBG(SWPipelinerPre::hasPreHeader: " << L;
  }
  auto *body=L.getTopBlock();
  return body->pred_size()==2;
}
bool SWPipelinerPre::hasExit(MachineLoop &L) {
  auto *body=L.getTopBlock();
  return body->succ_size()==2;
}
bool SWPipelinerPre::createPreHeader(MachineLoop &L) {
  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "DBG(SWPipelinerPre::createPreHeader: loop-body mir(before)\n";
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
    auto phPhi=BuildMI(ph, phi->getDebugLoc(), TII->get(TargetOpcode::PHI), newReg);
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
    BuildMI(*body, phi, phi->getDebugLoc(), TII->get(TargetOpcode::PHI), phi->getOperand(0).getReg())
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
  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "DBG(SWPipelinerPre::createPreHeader: loop-body-mir(after)\n";
    dbgs() << *(L.getTopBlock());
  }

  return true;
}

bool SWPipelinerPre::createExit(MachineLoop &L) {
  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "DBG(SWPipelinerPre::createExit\n";
    dbgs() << *(L.getTopBlock());
  }
  return false;
}
