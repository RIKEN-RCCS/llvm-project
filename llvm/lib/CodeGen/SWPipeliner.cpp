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
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/SwplTargetMachine.h"
#include "llvm/InitializePasses.h"

#include "SWPipeliner.h"
#include "SwplScheduling.h"
#include "SwplTransformMIR.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/ErrorHandling.h"

#include "llvm/CodeGen/BasicTTIImpl.h"
#include <iostream>

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpipeliner"

static cl::opt<bool> OptionDumpPlan("swpl-debug-dump-plan",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableSwpl("swpl-disable",cl::init(false), cl::ReallyHidden);


static cl::opt<bool> DebugOutput("swpl-debug",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DebugDdgOutput("swpl-debug-ddg",cl::init(false), cl::ReallyHidden);

static cl::opt<unsigned> OptionMinIIBase("swpl-minii",cl::init(0), cl::ReallyHidden);
static cl::opt<unsigned> OptionMaxIIBase("swpl-maxii",cl::init(0), cl::ReallyHidden);

static cl::opt<std::string> OptionExportDDG("export-swpl-dep-mi",cl::init(""), cl::ReallyHidden);
static cl::opt<std::string> OptionImportDDG("import-swpl-dep-mi",cl::init(""), cl::ReallyHidden);

static cl::opt<int> OptionRealFetchWidth("swpl-real-fetch-width",cl::init(4), cl::ReallyHidden);
static cl::opt<int> OptionVirtualFetchWidth("swpl-virtual-fetch-width",cl::init(4), cl::ReallyHidden);

namespace llvm {

bool SWPipeliner::isDebugOutput() {
  return ::DebugOutput;
}

bool SWPipeliner::isDebugDdgOutput() {
  return ::DebugDdgOutput;
}

unsigned SWPipeliner::min_ii_for_retry=0;
unsigned SWPipeliner::loop_number=0;

unsigned SWPipeliner::nOptionMinIIBase() {
  return min_ii_for_retry ? min_ii_for_retry : ::OptionMinIIBase;
}

unsigned SWPipeliner::nOptionMaxIIBase() {
#define DEFAULT_MAXII_BASE 1000

  return ::OptionMaxIIBase ? ::OptionMaxIIBase : DEFAULT_MAXII_BASE;
}

static void printDebugM(const char *f, const StringRef &msg) {
  if (!SWPipeliner::isDebugOutput()) return;
  errs() << "DBG(" << f << ") " << msg << "\n";
}
void SWPipeliner::remarkMissed(const char *msg, MachineFunction &mf) {
  auto &F=mf.getFunction();
  auto &mbb=mf.front();
    ORE->emit([&]() {
      DebugLoc Loc;
      if (auto *SP = F.getSubprogram())
        Loc = DILocation::get(SP->getContext(), SP->getLine(), 1, SP);
      MachineOptimizationRemarkMissed R(DEBUG_TYPE, "NotSoftwarePipleined", Loc, &mbb);
      R << msg;
      return R;
    });
};
bool SWPipeliner::doInitialization(Module &m) {
  if (isExportDDG()) {
    // Clear the contents of the file when initializing as additional dependency information will be written.）
    std::error_code EC;
    raw_fd_ostream OutStrm(SWPipeliner::getDDGFileName(), EC);
  }
  return false;
}

bool SWPipeliner::isExportDDG() {
  return !OptionExportDDG.empty();
}
bool SWPipeliner::isImportDDG() {
  return !OptionImportDDG.empty();

}
StringRef SWPipeliner::getDDGFileName() {
  if (isImportDDG()) return OptionImportDDG;
  if (isExportDDG()) return OptionExportDDG;
  return "";
}



MachineOptimizationRemarkEmitter *SWPipeliner::ORE = nullptr;
const TargetInstrInfo *SWPipeliner::TII = nullptr;
const TargetRegisterInfo *SWPipeliner::TRI = nullptr;
MachineRegisterInfo *SWPipeliner::MRI = nullptr;
SwplTargetMachine *SWPipeliner::STM = nullptr;
AliasAnalysis *SWPipeliner::AA = nullptr;
std::string SWPipeliner::Reason;
SwplLoop *SWPipeliner::currentLoop = nullptr;
unsigned SWPipeliner::FetchBandwidth = 0;
unsigned SWPipeliner::RealFetchBandwidth = 0;

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

  FetchBandwidth = OptionRealFetchWidth + OptionVirtualFetchWidth;
  RealFetchBandwidth = OptionRealFetchWidth;

  if (skipFunction(mf.getFunction())) {
    if (isDebugOutput()) {
      dbgs() << "SWPipeliner: Not processed because skipFunction() is true.\n";
    }
    return false;
  }
  loop_number=0;
  bool Modified = false;
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
  //Check when minii/maxii is specified in the option.
  //When it becomes possible to specify minii/maxii for each loop by pragma etc., add consideration to that.
  if ( !DisableSwpl && (SWPipeliner::nOptionMinIIBase() > 0) && (SWPipeliner::nOptionMaxIIBase() > 0) ) {
    if ( SWPipeliner::nOptionMinIIBase() >= SWPipeliner::nOptionMaxIIBase() ) {
      printDebugM(__func__, "[canPipelineLoop:NG] Bypass SWPL processing. The specified minii/maxii is invalid. (It must be minii<maxii) ");
      remarkMissed("Bypass SWPL processing. The specified minii/maxii is invalid. (It must be minii<maxii)", mf);
      DisableSwpl = true;
      return false;
    }
  }

  STM->initialize(*MF);

  for (auto &L : *MLI) {
    scheduleLoop(*L);
    assert(Reason.empty());
  }
  delete STM;
  STM = nullptr;
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

void SWPipeliner::remarkAnalysis(const char *msg, MachineLoop &L, const char *Name) {
  ORE->emit([&]() {
    return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, Name,
                                           L.getStartLoc(), L.getHeader())
           << msg;
  });
}
static cl::opt<SWPipeliner::SwplRestrictionsFlag> DisableRestrictionsCheck("swpl-disable-restrictions-check",
                                               cl::init(SWPipeliner::SwplRestrictionsFlag::All),
                                               cl::ValueOptional, cl::ReallyHidden,
                                              cl::values(clEnumValN(SWPipeliner::SwplRestrictionsFlag::None, "0", ""),
                                                         clEnumValN(SWPipeliner::SwplRestrictionsFlag::MultipleReg, "1", "multiple register"),
                                                         clEnumValN(SWPipeliner::SwplRestrictionsFlag::MultipleDef, "2", "multiple defined operator"),
                                                         clEnumValN(SWPipeliner::SwplRestrictionsFlag::All, "", "")
                                                                ));

bool SWPipeliner::isDisableRestrictionsCheck(SwplRestrictionsFlag f) {

  return (f== DisableRestrictionsCheck ||
          DisableRestrictionsCheck == SwplRestrictionsFlag::All);
}

void SWPipeliner::makeMissedMessage_RestrictionsDetected(const MachineInstr &target) {
  std::string msg;
  raw_string_ostream StrOS(msg);
  StrOS << "SWPL is not performed because a restriction is detected. MI:";
  target.print(StrOS);
  Reason = msg;
}

bool SWPipeliner::scheduleLoop(MachineLoop &L) {
  bool Changed = false;
  Reason = "";
  MachineBasicBlock *MBB = L.getTopBlock();
  const BasicBlock *BB = MBB->getBasicBlock();
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  const Loop *BBLoop = LI->getLoopFor(BB);

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
  if (!shouldOptimize(BBLoop)) {
    printDebug(__func__, "[canPipelineLoop:NG] Specified Swpl disable by option/pragma. ", L);
    return false;
  }

  if (!TII->canPipelineLoop(L)) {
    printDebug(__func__, "!!! Can not pipeline loop.", L);
    remarkMissed("Failed to pipeline loop", L);

    return Changed;
  }

  loop_number++;
  SwplScr swplScr(L);
  SwplScr::UseMap liveOutReg;
  // SwplLoop::Initialize()でLoop複製されるため、その前に出口Busyレジスタ情報を収集する
  swplScr.collectLiveOut(liveOutReg);

  // データ抽出
  currentLoop = SwplLoop::Initialize(L, liveOutReg);
  if (!Reason.empty()) {
    // 何かしらのエラーが発生した
    remarkMissed("", *currentLoop->getML());
    if (SWPipeliner::isDebugOutput()) {
      printDebug(__func__, "!!! Can not pipeline loop. Loops with restricting MI", L);
    }
    delete currentLoop;
    currentLoop = nullptr;
    return Changed;
  }
  bool Nodep = false;
  if (enableNodep(BBLoop) && enableSWP(BBLoop, false)){
    remarkAnalysis("Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.",
                   *currentLoop->getML(), "scheduleLoop");
    Nodep = true;
  }
  SwplDdg *ddg = SwplDdg::Initialize(*currentLoop,Nodep);

  // スケジューリング
  bool redo;
  bool SWPLApplicationFailure = false;
  do {
    redo = false;

    SwplPlan* plan = SwplPlan::generatePlan(*ddg);
    if (plan != NULL) {
      if ( OptionDumpPlan ) {
        plan->dump( dbgs() );
      }
      if (plan->getPrologCycles() == 0) {
        SWPLApplicationFailure = true;
        remarkMissed("This loop is not software pipelined because the software pipelining does not improve the performance.",
                     *currentLoop->getML());
        if (SWPipeliner::isDebugOutput()) {
          dbgs() << "        : Loop isn't software pipelined because prologue is 0 cycle.\n";
        }

      } else {
        SwplTransformMIR tran(*MF, *plan, liveOutReg);
        Changed = tran.transformMIR();
        if (!Changed) {
          if (!plan->existsPragma && ((plan->getIterationInterval() + 1) < nOptionMaxIIBase()) ) {
            min_ii_for_retry = plan->getIterationInterval() + 1;
            redo = true;
            Reason = "";
            if (SWPipeliner::isDebugOutput()) {
              dbgs() << "        : Reschedule with minii set to " << min_ii_for_retry << " because ran out of physical registers.\n";
            }
          } else {
            SWPLApplicationFailure = true;
            remarkMissed("This loop is not software pipelined because no schedule is obtained.", *currentLoop->getML());
            if (SWPipeliner::isDebugOutput()) {
              if (plan->existsPragma) {
                dbgs() << "        : No rescheduling because II is specified in the pragma.\n";
              } else {
                dbgs() << "        : No rescheduling because II:" << nOptionMaxIIBase() << " reached maxii.\n";
              }
            }
          }
        }
      }
      SwplPlan::destroy( plan );
    } else {
      SWPLApplicationFailure = true;
      remarkMissed("This loop is not software pipelined because no schedule is obtained.", *currentLoop->getML());
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "        : Loop isn't software pipelined because plan is NULL.\n";
      }
    }
  } while ( redo );

  if (SWPLApplicationFailure && llvm::enableLS()) {
    // LS
    dbgs() << "start LocalScheduler!\n";
  }

  min_ii_for_retry = 0;
  SwplDdg::destroy(ddg);
  delete currentLoop;
  currentLoop = nullptr;

  return Changed;
}


bool SWPipeliner::shouldOptimize(const Loop *BBLoop) {

  if (!enableSWP(BBLoop, false)) {
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
  if (skipFunction(mf.getFunction())) {
      return false;
  }
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
  if (!enableSWP(BBLoop, false)) return Changed;

  if (!hasPreHeader(L)) Changed|=createPreHeader(L);
  if (!hasExit(L)) Changed|=createExit(L);

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

static cl::opt<bool> DebugDumpDdg("swpl-debug-dump-ddg",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableInstDep("swpl-enable-instdep",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableRegDep4tied("swpl-disable-regdep-4-tied",cl::init(false), cl::ReallyHidden);

static void update_distance_and_delay(SwplDdg &ddg, SwplInst &former_inst, SwplInst &latter_inst, int distance, int delay);

SwplDdg *SwplDdg::Initialize (SwplLoop &loop, bool Nodep) {
  SwplDdg *ddg = new SwplDdg(loop);

  /// 1. Call generateInstGraph() to initialize instruction dependency graph information SwplInstGraph.
  ddg->generateInstGraph();
  /// 2. Call analysisRegDependence() to analyze dependency information between registers.
  ddg->analysisRegDependence();
  /// 3. Call analysisMemDependence() to analyze dependency information between memories.
  if (!Nodep)
    ddg->analysisMemDependence();
  /// 4. Call analysisInstDependence() to analyze instruction dependency information.
  if (EnableInstDep)
    ddg->analysisInstDependence();

  /// 5. Call SwplLoop::recollectPhiInsts() to recollect Phi contained in SwplLoop::BodyInsts
  /// into PhiInsts after generating SwplDdg.
  loop.recollectPhiInsts();

  // Importing dependency information will not be performed here.
  // The edited dependency information is imported within create ModuloDelayMap,
  // so that it only affects SWPL scheduling.

  // Export yaml of DDG
  if (SWPipeliner::isExportDDG())
    ddg->exportYaml();

  if (DebugDumpDdg) {
    ddg->print();
  }  
  return ddg;
}

/// Phiの初期値(Register)、Loopで定義した値（Register）を取得する
/// \param [in] Phi
/// \param [in] Loop
/// \param [out] InitVal
/// \param [out] LoopVal
/// \note MachinePipelinerより移植
static void getPhiRegs(const MachineInstr &Phi, const MachineBasicBlock *Loop,
                       unsigned &InitVal, unsigned &LoopVal) {
  assert(Phi.isPHI() && "Expecting a Phi.");

  InitVal = 0;
  LoopVal = 0;
  for (unsigned i = 1, e = Phi.getNumOperands(); i != e; i += 2)
    if (Phi.getOperand(i + 1).getMBB() != Loop)
      InitVal = Phi.getOperand(i).getReg();
    else
      LoopVal = Phi.getOperand(i).getReg();

  assert(InitVal != 0 && LoopVal != 0 && "Unexpected Phi structure.");
}

/// Return the Phi register value that comes the loop block.
/// \param [in] Phi
/// \param [in] LoopBB
/// \return register
/// \note MachinePipelinerより移植
static unsigned getLoopPhiReg(const MachineInstr &Phi, const MachineBasicBlock *LoopBB) {
  for (unsigned i = 1, e = Phi.getNumOperands(); i != e; i += 2)
    if (Phi.getOperand(i + 1).getMBB() == LoopBB)
      return Phi.getOperand(i).getReg();
  return 0;
}

/// 指定命令の差分を計算する
/// \param [in] MI
/// \param [out] Delta
/// \retval true 計算成功
/// \retval false 計算失敗
/// \note MachinePipelinerより移植
static bool computeDelta(const MachineInstr &MI, unsigned &Delta) {
  const MachineOperand *BaseOp;
  int64_t Offset;
  bool OffsetIsScalable;
  if (!SWPipeliner::TII->getMemOperandWithOffset(MI, BaseOp, Offset, OffsetIsScalable, SWPipeliner::TRI))
    return false;

  // FIXME: This algorithm assumes instructions have fixed-size offsets.
  if (OffsetIsScalable)
    return false;

  if (!BaseOp->isReg())
    return false;

  Register BaseReg = BaseOp->getReg();

  // Check if there is a Phi. If so, get the definition in the loop.
  MachineInstr *BaseDef = SWPipeliner::MRI->getVRegDef(BaseReg);
  if (BaseDef && BaseDef->isPHI()) {
    BaseReg = getLoopPhiReg(*BaseDef, MI.getParent());
    BaseDef = SWPipeliner::MRI->getVRegDef(BaseReg);
  }
  if (!BaseDef)
    return false;

  int D = 0;
  if (!SWPipeliner::TII->getIncrementValue(*BaseDef, D) && D >= 0)
    return false;

  Delta = D;
  return true;
}

/// MachineInstr 同士に依存があるか確認する
/// \param [in] p 対象となる先命令
/// \param [in] q 対象となる後命令
/// \retval true 依存あり
/// \retval false 依存なし
/// \note MachinePipeliner より移植
static bool isDependence(const MachineInstr *p, const MachineInstr *q) {
  if (p->hasUnmodeledSideEffects() || q->hasUnmodeledSideEffects() ||
      p->mayRaiseFPException() || q->mayRaiseFPException() ||
      p->hasOrderedMemoryRef() || q->hasOrderedMemoryRef()) {
    return true;
  }
  if (!p->mayStore() || !q->mayLoad())
    return false;

  unsigned DeltaS, DeltaD;
  if (!computeDelta(*p, DeltaS) || !computeDelta(*q, DeltaD))
    return true;
  const MachineOperand *BaseOpS, *BaseOpD;

  int64_t OffsetS, OffsetD;
  bool OffsetSIsScalable, OffsetDIsScalable;
  if (!SWPipeliner::TII->getMemOperandWithOffset(*p, BaseOpS, OffsetS, OffsetSIsScalable,
                                                 SWPipeliner::TRI) ||
      !SWPipeliner::TII->getMemOperandWithOffset(*q, BaseOpD, OffsetD, OffsetDIsScalable,
                                    SWPipeliner::TRI))
    return true;

  assert(!OffsetSIsScalable && !OffsetDIsScalable &&
         "Expected offsets to be byte offsets");

  if (!BaseOpS->isIdenticalTo(*BaseOpD))
    return true;

  // Check that the base register is incremented by a constant value for each
  // iteration.
  MachineInstr *Def = SWPipeliner::MRI->getVRegDef(BaseOpS->getReg());
  if (!Def || !Def->isPHI())
    return true;
  unsigned InitVal = 0;
  unsigned LoopVal = 0;

  getPhiRegs(*Def, p->getParent(), InitVal, LoopVal);

  MachineInstr *LoopDef = SWPipeliner::MRI->getVRegDef(LoopVal);
  int D = 0;
  if (!LoopDef || !SWPipeliner::TII->getIncrementValue(*LoopDef, D))
    return true;

  uint64_t AccessSizeS = (*p->memoperands_begin())->getSize();
  uint64_t AccessSizeD = (*q->memoperands_begin())->getSize();

  // This is the main test, which checks the offset values and the loop
  // increment value to determine if the accesses may be loop carried.
  if (AccessSizeS == MemoryLocation::UnknownSize ||
      AccessSizeD == MemoryLocation::UnknownSize)
    return true;

  if (DeltaS != DeltaD || DeltaS < AccessSizeS || DeltaD < AccessSizeD)
    return true;

  return (OffsetS + (int64_t)AccessSizeS < OffsetD + (int64_t)AccessSizeD);
}

void SwplDdg::analysisInstDependence() {
  for (auto *p:getLoopBodyInsts()) {
    const auto *pmi=Loop->getOrgMI(p->getMI());
    if (pmi->isPHI()) continue;
    bool succ=false;
    for (auto *q:getLoopBodyInsts()) {
      if (!succ) {
        if (p==q) succ=true;
        continue;
      }
      const auto *qmi=Loop->getOrgMI(q->getMI());
      if (qmi->isPHI()) continue;
      if (isDependence(pmi, qmi)) {
        if (SWPipeliner::isDebugDdgOutput()) {
          dbgs() << "DBG(SwplDdg::analysisInstDependence):\n"
                 << " former_inst:" << *(p->getMI())
                 << " latter_inst:" << *(q->getMI())
                 << " distance:" << 0 << "\n"
                 << " delay:" << 1 << "\n";
        }

        update_distance_and_delay(*this, *p, *q, 0, 1);
      }
    }
  }
}

void SwplDdg::analysisRegDependence() {
  /// 1. Call analysisRegsFlowDependence() to analyze flow dependence by registers.
  analysisRegsFlowDependence();
  /// 2. Call analysisRegsAntiDependence() to analyze anti-dependence caused by registers.
  analysisRegsAntiDependence();
  /// 3. Call analysisRegsOutputDependence() to analyze output dependence by registers.
  analysisRegsOutputDependence();
  if (!DisableRegDep4tied) {
    /// 4. Call analysisRegsDependence_for_tieddef() to add dependency for tied-def.
    analysisRegDependence_for_tieddef();
  }
}

void SwplDdg::analysisRegDependence_for_tieddef() {

  auto findSwplReg = [](SwplRegs& regs, Register r)->SwplReg * {
    for (auto *p:regs) {
      if (p->getReg() == r) return p;
    }
    assert(0 && "register not found");
    return nullptr;
  };
  for (auto *def_inst : getLoopBodyInsts()) {

    auto *mi = def_inst->getMI();

//    dbgs() << *mi;
    auto e = mi->getNumDefs();
    for (unsigned def_ix=0; def_ix < e; def_ix++) {
      unsigned int use_ix = 0;
      if (!mi->isRegTiedToUseOperand(def_ix, &use_ix)) {
        continue;
      }
      auto mi_def_reg = mi->getOperand(def_ix).getReg();
      auto mi_use_reg = mi->getOperand(use_ix).getReg();

      auto *def_op = findSwplReg(def_inst->getDefRegs(), mi_def_reg);
      auto *use_op = findSwplReg(def_inst->getUseRegs(),mi_use_reg);
      auto *COPY = use_op->getDefInst();
      if (!COPY->isCopy()) {
        continue;
      }
      if (SWPipeliner::isDebugDdgOutput()) {
        dbgs() << "SwplDdg::analysisRegDependence_for_tieddef target:" << *mi;
      }

      for (auto *p : def_op->getUseInsts()) {
        int distance = 1;
        int delay = 1;

        if (SWPipeliner::isDebugDdgOutput()) {
          dbgs() << "DBG(SwplDdg::analysisRegsAntiDependence for tied):\n"
                 << " former_inst:" << *(p->getMI())
                 << " latter_inst:" << *(COPY->getMI())
                 << " use reg:" << printReg(def_op->getReg(), SWPipeliner::TRI)
                 << "\n"
                 << " distance:" << distance << "\n"
                 << " delay:" << delay << "\n";
        }
        update_distance_and_delay(*this, *p, *COPY, distance, delay);
      }
    }
  }
}

/// ループボディ内の全ての SwplInst (inst)に対して、
/// 参照レジスタを定義する命令(def_inst)を検索し、依存関係（def_inst → inst）
/// の依存を貼り命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。
void SwplDdg::analysisRegsFlowDependence() {
  for (auto *use_inst:getLoopBodyInsts()) {
    for (auto *reg:use_inst->getUseRegs()) {
      SwplInst *def_inst = nullptr;
      int def_indx = -1;
      reg->getDefPortInLoopBody(&def_inst, &def_indx);
      if (def_inst == nullptr) { continue; }
      assert(def_indx >= 0);
      int distance = (getLoop()->areBodyInstsOrder(def_inst, use_inst) ? 0 : 1);
      int delay = SWPipeliner::STM->computeRegFlowDependence(def_inst->getMI(), use_inst->getMI());
      if (SWPipeliner::isDebugDdgOutput()) {
        dbgs() << "DBG(SwplDdg::analysisRegsFlowDependence):\n"
               << " former_inst:" << *(def_inst->getMI())
               << " latter_inst:" << *(use_inst->getMI())
               << " use reg:" << printReg(reg->getReg(), SWPipeliner::TRI) << "\n"
               << " distance:" << distance << "\n"
               << " delay:" << delay << "\n";
      }
      update_distance_and_delay(*this, *def_inst, *use_inst, distance, delay);
    }
  }
}

/// ループボディ内の全ての SwplInst (def_inst)に対して、
/// 定義レジスタを定義する命令(inst)を検索し、依存関係（inst → def_inst）
/// の依存を貼り命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。
void SwplDdg::analysisRegsAntiDependence() {
  for (auto *def_inst:getLoopBodyInsts()) {
    for (auto *reg:def_inst->getDefRegs()) {
      // predecesserのregを取得
      SwplReg *pred_reg = reg->getPredecessor();
      if (pred_reg == nullptr) {
        continue;
      }
      if (pred_reg->isUseInstsEmpty()) { continue; }
      for(auto *use_inst:pred_reg->getUseInsts()) {
        assert(use_inst->isBodyInst());
        if (use_inst->isPrefetch()) {
          continue;
        }
        int distance = (getLoop()->areBodyInstsOrder(use_inst, def_inst) ? 0 : 1);
        int delay = 1;
        if (SWPipeliner::isDebugDdgOutput()) {
          dbgs() << "DBG(SwplDdg::analysisRegsAntiDependence):\n"
                 << " former_inst:" << *(use_inst->getMI())
                 << " latter_inst:" << *(def_inst->getMI())
                 << " use reg:" << printReg(reg->getReg(), SWPipeliner::TRI) << "\n"
                 << " distance:" << distance << "\n"
                 << " delay:" << delay << "\n";
        }
        update_distance_and_delay(*this, *use_inst, *def_inst, distance, delay);
      }
    }
  }
}

/// ループボディ内の全ての SwplInst (def_inst)に対して、
/// 定義レジスタを定義する命令(def_inst2)を検索し、依存関係（def_inst → def_inst2）
/// の依存を貼り命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。
void SwplDdg::analysisRegsOutputDependence() {
  for (auto *def_inst:getLoopBodyInsts()) {
    for (auto *reg:def_inst->getDefRegs()) {
      // predecesserのregを取得
      SwplReg *pred_reg = reg->getPredecessor();
      if (pred_reg == nullptr) {
        continue;
      }
      SwplInst *pred_def_inst = nullptr;
      int def_indx = -1;
      pred_reg->getDefPortInLoopBody(&pred_def_inst, &def_indx);
      if (pred_def_inst == nullptr) { continue; }
      assert(def_indx >= 0);
      int distance = (getLoop()->areBodyInstsOrder(pred_def_inst, def_inst) ? 0 : 1);
      int delay = 1;
      if (SWPipeliner::isDebugDdgOutput()) {
        dbgs() << "DBG(SwplDdg::analysisRegsOutputDependence):\n"
               << " former_inst:" << *(pred_def_inst->getMI())
               << " latter_inst:" << *(def_inst->getMI())
               << " use reg:" << printReg(reg->getReg(), SWPipeliner::TRI) << "\n"
               << " distance:" << distance << "\n"
               << " delay:" << delay << "\n";
      }
      update_distance_and_delay(*this, *pred_def_inst, *def_inst, distance, delay);
    }
  }
}
template <>
struct yaml::MappingTraits<SwplDdg::IOmi> {
  static void mapping(IO &io, SwplDdg::IOmi &info) {
    io.mapRequired("id", info.id);
    io.mapOptional("mi", info.mi, "");
  }
  static const bool flow = true;
};

template<>
struct yaml::MappingTraits<SwplDdg::IOddgnodeinfo> {
  static void mapping(IO& io, SwplDdg::IOddgnodeinfo& info) {
    io.mapRequired("distance", info.distance);
    io.mapRequired("delay", info.delay);
  }
  static const bool flow=true;
};
LLVM_YAML_IS_SEQUENCE_VECTOR(SwplDdg::IOddgnodeinfo)

template <>
struct yaml::MappingTraits<SwplDdg::IOddgnode> {
  static void mapping(IO &io, SwplDdg::IOddgnode &info) {
    io.mapRequired("from", info.from);
    io.mapRequired("to", info.to);
    io.mapRequired("infos", info.infos);
  }
};
LLVM_YAML_IS_SEQUENCE_VECTOR(SwplDdg::IOddgnode)

template <>
struct yaml::MappingTraits<SwplDdg::IOddg> {
  static void mapping(IO &io, SwplDdg::IOddg &info) {
    io.mapRequired("fname", info.fname);
    io.mapRequired("loopid", info.loopid);
    io.mapRequired("deps", info.ddgnodes);
  }
};
typedef std::vector<SwplDdg::IOddg> IOddgDocumentList;
LLVM_YAML_IS_DOCUMENT_LIST_VECTOR(SwplDdg::IOddg)
IOddgDocumentList yamlddgList;

/// SwplMem同士の依存関係を解析し、
/// 命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。\n
/// 真依存、逆依存、出力依存のそれぞれの依存を以下のルーチンを利用してDelayを算出する。\n
void SwplDdg::analysisMemDependence() {
  for (auto *former_mem:getLoopMems()) {
    for (auto *latter_mem:getLoopMems()) {
      if (former_mem == latter_mem) {
        continue;
      }
      if ( (former_mem->getInst())->isPrefetch()  ||
           (latter_mem->getInst())->isPrefetch())  {
        continue;
      }

      unsigned distance, delay;
      enum class DepKind {init, flow, anti, output};
      DepKind depKind = DepKind::init;

      if (former_mem->isMemDef()) {
        if (latter_mem->isMemDef()) {
          delay = SWPipeliner::STM->computeMemOutputDependence(former_mem->getInst()->getMI(), latter_mem->getInst()->getMI());
          depKind = DepKind::output;
        } else {
          delay = SWPipeliner::STM->computeMemFlowDependence(former_mem->getInst()->getMI(), latter_mem->getInst()->getMI());
          depKind = DepKind::flow;
        }
      } else {
        if (latter_mem->isMemDef()) {
          delay = SWPipeliner::STM->computeMemAntiDependence(former_mem->getInst()->getMI(), latter_mem->getInst()->getMI());
          depKind = DepKind::anti;
        } else {
          // Not dependence (input-dependence)
          continue;
        }
      }
      distance = getLoop()->getMemsMinOverlapDistance(former_mem, latter_mem);
      if (SWPipeliner::isDebugDdgOutput()) {
        auto *p="";
        switch (depKind) {
        case DepKind::flow: p="flow"; break;
        case DepKind::anti: p="anti"; break;
        case DepKind::output: p="output"; break;
        case DepKind::init:
          llvm_unreachable("Unknown Dependency Kind");
        }
        dbgs() << "DBG(SwplDdg::analysisMemDependence):" << p << "\n"
        << " former_inst:" << *(former_mem->getInst()->getMI())
        << " latter_inst:" << *(latter_mem->getInst()->getMI())
        << " distance:" << distance << "\n"
        << " delay:" << delay << "\n";
      }

      update_distance_and_delay(*this, *(former_mem->getInst()), *(latter_mem->getInst()), distance, delay);
    }
  }

}
/// Swpl_Inst 間のDistanceとDelayを更新する
/// \param [in,out] ddg     SwplDdg  処理対象のSwplDdg
/// \param [in] former_inst SwplInst 先行の命令のSwplInst 
/// \param [in] latter_inst SwplInst 後続の命令のSwplInst
/// \param [in] distance    int      命令間で依存が問題にならない範囲の回転の数      
/// \param [in] delay       int      命令間であける必要があるcycle数
static void update_distance_and_delay(SwplDdg &ddg, SwplInst &former_inst, SwplInst &latter_inst, int distance, int delay) {
  SwplInstGraph *graph = ddg.getGraph();
  SwplInstEdge *edge = graph->findEdge(former_inst, latter_inst);

  if (edge == nullptr) {
    edge = graph->createEdge(former_inst, latter_inst);
  }

  auto &distances = ddg.getDistancesFor(*edge);
  auto &delays = ddg.getDelaysFor(*edge);
  distances.push_back(distance);
  delays.push_back(delay);
}

/// 命令の依存グラフ情報の SwplInstGraph::Edges 単位でModuloDelayMap情報を収集する。
SwplInstEdge2ModuloDelay *SwplDdg::getModuloDelayMap(int ii) const {
  SwplInstEdge2ModuloDelay *map =  new SwplInstEdge2ModuloDelay();

  const SwplInstGraph &graph = getGraph();
  const SwplInstEdges &edges = graph.getEdges();

  for (auto *edge:edges) {
    int max_modulo_delay = INT_MIN;
    std::vector<unsigned> distances = getDistancesFor(*edge);
    std::vector<int> delays = getDelaysFor(*edge);

    auto distance = distances.begin();
    auto delay = delays.begin();
    auto distance_end = distances.end();
    for(  ; distance != distance_end ; ++distance, ++delay) {
      int delay_val = *delay;

      if( delay_val == 0 ) {
        // delayが0となるのは、edgeのfromの命令がPseudo命令である場合のみを想定
        assert( SWPipeliner::STM->isPseudo( *(edge->getInitial()->getMI() )) );
        delay_val=1;
      }

      int modulo_delay = delay_val - ii * (int)(*distance);

      max_modulo_delay = (max_modulo_delay > modulo_delay) ? max_modulo_delay : modulo_delay;
    }
    map->insert(std::make_pair(const_cast<SwplInstEdge*>(edge), max_modulo_delay));
  }
  return map;
}

void SwplDdg::print() const {
  dbgs() << "DBG(SwplDdg::print) SwplDdg. \n";
  dbgs() << "### SwplDdg: "<< this << "\n";

  const SwplInstGraph &graph = getGraph();
  const SwplInstEdges &edges = graph.getEdges();
  
  for (auto *edge:edges) {
    const SwplInst *leading_inst = edge->getInitial();
    const SwplInst *trailing_inst = edge->getTerminal();

    dbgs() << "### SwplEdge: " << edge << "\n";
    dbgs() << "### from: " << *leading_inst->getMI() ;
    dbgs() << "### to  : " << *trailing_inst->getMI();
    std::vector<unsigned> distances = getDistancesFor(*edge);
    std::vector<int> delays = getDelaysFor(*edge);

    auto distance = distances.begin();
    auto delay = delays.begin();
    auto distance_end = distances.end();
    for(  ; distance != distance_end ; ++distance, ++delay) {
      dbgs() << "### distance:" << *distance << " delay:" << *delay << "\n";
    }  
    dbgs() << "\n";
  }
}

void SwplDdg::importYaml() {
  StringRef fname;
  fname = getLoop()->getML()->getTopBlock()->getParent()->getName();

  if (yamlddgList.size()==0) {
    ErrorOr<std::unique_ptr<MemoryBuffer>>  Buffer = MemoryBuffer::getFile(SWPipeliner::getDDGFileName());
    std::error_code EC = Buffer.getError();
    if (EC)
      report_fatal_error("can not open yaml file", false);
    yaml::Input yin(Buffer.get()->getMemBufferRef());
    yin >> yamlddgList;
  }

  IOddg *target_yamlddg = nullptr;
  for (auto &y: yamlddgList) {
    if (y.fname != fname) continue;
    if (y.loopid != SWPipeliner::loop_number) continue;
    target_yamlddg = &y;
    break;
  }

  if (!target_yamlddg)
    return;

  llvm::DenseMap<const MachineInstr*, unsigned> mimap;
  int mi_no=0;
  for (auto *inst : getLoopBodyInsts()) {
    auto *mi = inst->getMI();
    mimap[mi] = mi_no++;
  }

  SwplInstGraph *graph = getGraph();
  SwplInstEdges &edges = graph->getEdges();
  for (auto &ddgnode : target_yamlddg->ddgnodes) {
    auto found = false;

    for (auto &d : ddgnode.infos)
      if (d.distance > 20)
        report_fatal_error("value too large, distance > 20", false);

    for (auto *edge : edges) {
      const SwplInst *leading_inst = edge->getInitial();
      const SwplInst *trailing_inst = edge->getTerminal();
      unsigned from = mimap[leading_inst->getMI()];
      unsigned to = mimap[trailing_inst->getMI()];

      if (ddgnode.from.id == from && ddgnode.to.id == to) {
        auto &distances = getDistancesFor(*edge);
        auto &delays = getDelaysFor(*edge);
        // check number of elements
        if (ddgnode.infos.size() != distances.size())
          report_fatal_error("number of pairs of distance and delay is incorrect", false);

        found = true;
        auto distance = distances.begin();
        auto distance_end = distances.end();
        auto delay = delays.begin();
        auto ddginfo = ddgnode.infos.begin();
        for(  ; distance != distance_end ; ++distance, ++delay, ++ddginfo) {
          *distance = ddginfo->distance;
          *delay = ddginfo->delay;
        }
        break;
      }
    }

    if (!found)
      report_fatal_error("from-mi or to-mi not found", false);
  }

}

void SwplDdg::exportYaml() {
  std::error_code EC;
  std::unique_ptr<raw_fd_ostream> OutStrm;
  OutStrm = std::make_unique<raw_fd_ostream>(SWPipeliner::getDDGFileName(), EC, sys::fs::OF_Append);
  if (EC)
    report_fatal_error("can not open yaml file", false);

  int mi_no = 0;
  llvm::DenseMap<const MachineInstr*, unsigned> mimap;
  *OutStrm << "# No., MI\n";
  for (auto *inst : getLoopBodyInsts()) {
    auto *mi = inst->getMI();
    mimap[mi] = mi_no;
    *OutStrm << "# " << mi_no << ":" << *mi;
    ++mi_no;
  }

  IOddg yamlddg;
  StringRef fname;
  fname = getLoop()->getML()->getTopBlock()->getParent()->getName();
  yamlddg.fname = fname;
  yamlddg.loopid = SWPipeliner::loop_number;
  std::string s;
  raw_string_ostream strstream(s);
  SwplInstGraph *graph = getGraph();
  SwplInstEdges &edges = graph->getEdges();
  for (auto *edge : edges) {
    const auto *from_mi = edge->getInitial()->getMI();
    const auto *to_mi = edge->getTerminal()->getMI();
    s.clear();
    from_mi->print(strstream, true, false, false, false);
    IOmi from{mimap[from_mi], strstream.str()};
    s.clear();
    to_mi->print(strstream, true, false, false, false);
    IOmi to{mimap[to_mi], strstream.str()};
    IOddgnode n{from, to};

    auto &distances = getDistancesFor(*edge);
    auto &delays = getDelaysFor(*edge);
    auto distance = distances.begin();
    auto distance_end = distances.end();
    auto delay = delays.begin();
    for(  ; distance != distance_end ; ++distance, ++delay) {
      IOddgnodeinfo d{*distance, *delay};
      n.infos.push_back(d);
    }

    yamlddg.ddgnodes.push_back(n);
  }

  yaml::Output yout(*OutStrm);
  yout << yamlddg;
}

static cl::opt<bool> DebugLoop("swpl-debug-loop",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DebugPrepare("swpl-debug-prepare",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> NoUseAA("swpl-no-use-aa",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableConvPrePost("swpl-disable-convert-prepost",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableRmCopy("swpl-disable-rm-copy",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableSuppressCopy("swpl-disable-suppress-copy",cl::init(false), cl::ReallyHidden);
static cl::opt<bool>
    IgnoreRegClass_SuppressCopy("swpl-ignore-class-suppress-copy",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableNormalizeTieddef("swpl-enable-normalize-tieddef",cl::init(true), cl::ReallyHidden);

// rm-copyを強化する（reg-alloc時と同じ処理にする）
static cl::opt<bool> IgnoreRegClass_RmCopy("swpl-ignore-class-rm-copy",cl::init(true), cl::ReallyHidden);
static cl::opt<bool> NoDep("swpl-nodep",cl::init(false), cl::ReallyHidden);

using BasicBlocks = std::vector<MachineBasicBlock *>;
using BasicBlocksIterator = std::vector<MachineBasicBlock *>::iterator ;
static MachineBasicBlock *getPredecessorBB(MachineBasicBlock &BB);
static void follow_single_predecessor_MBBs(MachineLoop *L, BasicBlocks *BBs);
static void construct_use(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO, SwplInsts &insts);
static void construct_mem_use(Register2SwplRegMap &rmap, SwplInst &inst, const MachineMemOperand *MMO, SwplInsts &insts,
                              SwplMems *mems, SwplMems *memsOtherBody);
static void construct_def(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO);


void SwplReg::inheritReg(SwplReg *former_reg, SwplReg *latter_reg) {
  assert (former_reg->Successor == NULL);
  assert (latter_reg->Predecessor == NULL);

  former_reg->Successor = latter_reg;
  latter_reg->Predecessor = former_reg;

  return;
}

void SwplReg::getDefPort( const SwplInst **p_def_inst, int *p_def_index) const {
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
    SwplReg Reg = (*p_def_inst)->getPhiUseRegFromIn();
    const SwplInst *inst = *p_def_inst;
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

bool SwplInst::isDefinePredicate() const {
  for (auto *reg:getDefRegs()) {
    if (reg->isPredReg()) return true;
  }
  return false;
}

bool SwplInst::isFloatingPoint() const {
  for (auto *reg:getDefRegs()) {
    if (reg->isFloatReg()) return true;
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

bool SwplInst::isPrefetch() const {
  return SWPipeliner::TII->isPrefetch(MI->getOpcode());
}
StringRef SwplInst::getName() {
  return (isPhi()) ? "PHI" : SWPipeliner::TII->getName( MI->getOpcode() );
}

StringRef SwplInst::getName() const {
  return (isPhi()) ? "PHI" : SWPipeliner::TII->getName( MI->getOpcode() );
}


SwplLoop *SwplLoop::Initialize(MachineLoop &L, const SwplScr::UseMap& LiveOutReg) {
  SwplLoop *loop = new SwplLoop(L);
  Register2SwplRegMap rmap;

  /// convertSSAtoNonSSA() を呼び出し、対象ループの非SSA化を行う。
  if (loop->convertSSAtoNonSSA(L, LiveOutReg)) return loop;

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
      if (SWPipeliner::TII->isSwplPseudoMI(MI)) { continue; }
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
  MachineBasicBlock *mbb=getML()->getTopBlock();
  int ix = 0;
  SWPipeliner::TII->findMIsForLoop(*mbb, &branch, &cmp, &addsub);
  for (auto &MI:getNewBodyMBB()->instrs()) {
    if (MI.isDebugInstr()) { continue; }
    if (&MI == branch) { continue; }
    if (&MI == cmp && &MI != addsub) { continue; }
    if (MI.isBranch()) { continue; }
    SwplInst *inst = new SwplInst(&MI, &*this);
    inst->InitializeWithDefUse(&MI, &*this, rmap, getPreviousInsts(), &(getMems()), &(getMemsOtherBody()));
    inst->inst_ix = ix;
    ix++;
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
      if (SWPipeliner::TII->isFPCR(MI->getOperand(i).getReg())) { continue; }
      UseOpMap[use_i]=i+1;
      use_i++;
      construct_use(rmap, *this, MI->getOperand(i), insts);
    }
  }

  if ((MI->mayLoad() || MI->mayStore() || MI->mayLoadOrStore()) && MI->memoperands_empty()) {
    // load/store命令でMI->memoperands_empty()の場合、MayAliasとして動作する必要がある
    if (SWPipeliner::isDebugDdgOutput()) {
      dbgs() << "DBG(SwplInst::InitializeWithDefUse): memoperands_empty mi=" << *MI;
    }
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
    if (SWPipeliner::STM->isPseudo(*MI)) { continue; }
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
  if (!SWPipeliner::TII->getMemOperandWithOffset(*former_mi, formerBaseOp, formerOffset, formerOffsetIsScalable, SWPipeliner::TRI))
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (formerOffsetIsScalable)
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (!formerBaseOp->isReg())
    return SwplDdg::UNKNOWN_MEM_DIFF;

  const MachineOperand *latterBaseOp;
  int64_t latterOffset;
  bool latterOffsetIsScalable;
  if (!SWPipeliner::TII->getMemOperandWithOffset(*latter_mi, latterBaseOp, latterOffset, latterOffsetIsScalable, SWPipeliner::TRI))
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (latterOffsetIsScalable)
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (!latterBaseOp->isReg())
    return SwplDdg::UNKNOWN_MEM_DIFF;

  if (formerBaseOp->getReg() != latterBaseOp->getReg())
    return SwplDdg::UNKNOWN_MEM_DIFF;
  int64_t offset=formerOffset-latterOffset;
  int64_t absoffset=labs(offset);
  return (absoffset > INT_MAX)? SwplDdg::UNKNOWN_MEM_DIFF:offset;
}

/// former_memとＮ回後のlatter_memが重なる可能性の否定できない最小のＮを返す。 \n
/// ただし、Ｎは、former_mem latter_mem の順に並んでいる時は０以上、逆の時は１以上とする。
unsigned SwplLoop::getMemsMinOverlapDistance(SwplMem *former_mem, SwplMem *latter_mem) {
  unsigned smallest_distance;
  int former_increment, latter_increment;

  const auto * former_mi=NewMI2OrgMI.at(former_mem->getInst()->getMI());
  const auto * latter_mi=NewMI2OrgMI.at(latter_mem->getInst()->getMI());
  const auto *memop1=former_mem->getMO();
  const auto *memop2=latter_mem->getMO();
  if (NoDep) {
    if (SWPipeliner::isDebugDdgOutput())
      dbgs() << "DBG(getMemIncrement): NoDep is true: return MAX_LOOP_DISTANCE\n"
             << " formaer_mi:" << *former_mi
             << " latter_mi:" << *latter_mi;
    return SwplMem::MAX_LOOP_DISTANCE;
  }

  if (!NoUseAA && !former_mi->mayAlias(SWPipeliner::AA, *latter_mi, false)) {
    // MachineInstr::mayAlias の結果は信頼性が低いため、さらに、メモリ間で重なる可能性があるか確認する
    if (memop1!=nullptr && memop2!=nullptr && memop1->getValue()!=nullptr && memop2->getValue()!=nullptr) {
      if (SWPipeliner::AA->isNoAlias(
              MemoryLocation::getAfter(memop1->getValue(), memop1->getAAInfo()),
              MemoryLocation::getAfter(memop2->getValue(), memop2->getAAInfo()))) {
         if (SWPipeliner::isDebugDdgOutput()) { dbgs() << " aliasAnalysis is NoAlias\n"; }
         return SwplMem::MAX_LOOP_DISTANCE;
      }
    }
  }

  // 最小の値を取得
  smallest_distance = (areMemsOrder(former_mem, latter_mem) ? 0 : 1);

  former_increment = getMemIncrement(former_mem);
  latter_increment = getMemIncrement(latter_mem);
  if (SWPipeliner::isDebugDdgOutput())
    dbgs() << "DBG(getMemIncrement): former_increment=" << former_increment << " , latter_increment=" << latter_increment << "\n";

  if (former_increment == SwplDdg::UNKNOWN_MEM_DIFF
      || latter_increment == SwplDdg::UNKNOWN_MEM_DIFF
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
  if (SWPipeliner::isDebugDdgOutput())
    dbgs() << "DBG(mems_initial_diff): initial_diff=" << initial_diff
     << ", former_size=" << former_mem->getSize()
     << ", latter_size=" << latter_mem->getSize() << "\n";

  if (initial_diff == SwplDdg::UNKNOWN_MEM_DIFF) {
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

static MachineOperand* used_reg(MachineInstr &phi) {
  Register def_r = phi.getOperand(0).getReg();
  unsigned opid;
  if (phi.getOperand(2).getMBB()==phi.getParent()) {
    opid=1;
  } else {
    opid=3;
  }
  if (phi.getOperand(opid).getSubReg()) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): own-operand has subreg!:" << phi;
    }
    return nullptr;
  }
  auto own_r = phi.getOperand(opid).getReg();

  if (!SWPipeliner::STM->isEnableRegAlloc() && !IgnoreRegClass_SuppressCopy) {
    if (def_r.isVirtual() && own_r.isVirtual()) {
      auto def_r_class = SWPipeliner::MRI->getRegClass(def_r);
      auto own_r_class = SWPipeliner::MRI->getRegClass(own_r);
      if (def_r_class->getID() != own_r_class->getID()) {
         if (DebugPrepare) {
           dbgs() << "DEBUG(used_reg): def-class is not use-class!:" << phi;
         }
         return nullptr;
      }
    } else {
      if (DebugPrepare) {
         dbgs() << "DEBUG(used_reg): def-class is not use-class!:" << phi;
      }
      return nullptr;
    }
  }

  auto *def_op = SWPipeliner::MRI->getOneDef(own_r);

  if (def_op==nullptr) {
    // 物理レジスタの場合:copy必須
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): MRI->getOneDef(own_r) is nullptr\n";
    }
    return nullptr;
  }

  if (def_op->getParent()->getParent()!=phi.getParent()) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): own_r is livein!\n";
    }
    return nullptr;

  }

  auto *defMI = def_op->getParent();
  unsigned use_tied_ix;
  bool t = defMI->isRegTiedToUseOperand(defMI->getOperandNo(def_op), &use_tied_ix);
  if (t && defMI->getOperand(use_tied_ix).getReg() == def_r) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): tied-def!\n";
    }
    return nullptr;
  }

  MachineInstr *start_instr = def_op->getParent();
  for (auto *I=start_instr->getNextNode();I;I=I->getNextNode()) {
    for ( auto &o:I->operands()) {
      if (!o.isReg()) continue;
      if (o.isDef()) continue;
      auto r=o.getReg();
      if (r.id()==own_r.id()) {
         if (DebugPrepare) {
           dbgs() << "DEBUG(used_reg): own-reg is used!\n";
         }
         return nullptr;
      }
      if (r.id()==def_r.id()) {
         if (DebugPrepare) {
           dbgs() << "DEBUG(used_reg): def-reg is used!\n";
         }
         return nullptr;
      }
    }
  }

  return def_op;
}

void SwplLoop::convertNonSSA(llvm::MachineBasicBlock *body, llvm::MachineBasicBlock *pre, const DebugLoc &dbgloc,
                             llvm::MachineBasicBlock *org, const SwplScr::UseMap &LiveOutReg) {
  std::vector<MachineInstr*> phis;

  DenseMap<MachineInstr*, Register> flow;
  DenseMap<MachineInstr*, MachineOperand*> uses;
  for (auto &phi:body->phis()) {
    phis.push_back(&phi);
    if (DisableSuppressCopy)
      uses[&phi]=nullptr;
    else
      uses[&phi]=used_reg(phi);
  }
  /// (1). Phi命令を検索し、Phi命令の単位に以下の処理を行う。
  for (auto *phi:phis) {
    unsigned opix;
    unsigned num=phi->getNumOperands();
    llvm::Register def_r=0, Reg=0, own_r=0, in_r=0;
    unsigned own_subreg=0;

    ///   (1)-1. Phiの参照レジスタのうち、自身のMBBで定義されるレジスタ(own_r)とLiveInのレジスタ(in_r)をおさえておく。
    int own_opix=0;
    def_r = phi->getOperand(0).getReg();
    for (opix=1; opix<num; opix++) {
      Reg = phi->getOperand(opix).getReg();
      opix++;
      assert(phi->getOperand(opix).isMBB());
      auto *mbb=phi->getOperand(opix).getMBB();
      if (mbb==org) {
        assert(own_r == 0);
        own_r = Reg;
        own_subreg = phi->getOperand(opix-1).getSubReg();
        own_opix = opix-1;
      } else {
        assert(in_r == 0);
        in_r = Reg;
      }
    }

    // def_rは後続するown_rで参照されるか調査する
    for (auto *t=phi->getNextNode();t;t=t->getNextNode()) {
      if (!t->isPHI()) break;
      Register t_own_r;
      if (t->getOperand(2).getMBB()==org) {
        t_own_r = t->getOperand(1).getReg();
      } else {
        t_own_r = t->getOperand(3).getReg();
      }
      if (def_r==t_own_r) {
        llvm::Register newReg = SWPipeliner::MRI->cloneVirtualRegister(def_r);
        MachineInstr *c=BuildMI(*body, body->getFirstTerminator(), dbgloc,
                                  SWPipeliner::TII->get(TargetOpcode::COPY), newReg)
                              .addReg(def_r /* def_rはsubreg指定は無い */);
        flow[t]=newReg;
        if (DebugPrepare) {
           dbgs() << "DEBUG(convertNonSSA): def_r is referenced by subsequent own_r:" << *c;
        }
        break;
      }
    }

    ///   (1)-2. Phiの定義を参照する命令を検索し2.の参照レジスタに置きかえる。 \n
    ///      defregはliveoutしているかを確認する。liveoutしていれば、PHIから生成されるCOPYの定義レジスタは新規を利用。
    const auto *org_phi=NewMI2OrgMI[phi];
    auto org_def_r=org_phi->getOperand(0).getReg();
    auto org_own_r=org_phi->getOperand(own_opix).getReg();
    bool liveout_def = LiveOutReg.count(org_def_r)!=0;
    bool liveout_own = LiveOutReg.count(org_own_r)!=0;

    if (liveout_def) {
      auto backup_def_r=def_r;
      def_r=SWPipeliner::MRI->cloneVirtualRegister(def_r);
      MachineInstr *Copy = BuildMI(*body, body->getFirstNonPHI(), dbgloc,SWPipeliner::TII->get(TargetOpcode::COPY), backup_def_r)
              .addReg(def_r);
      NewMI2OrgMI[Copy]=org_phi;
    }
    for (auto &x:OrgReg2NewReg) {
      Register r = x.second;
      if (r.id() == own_r.id()) {
        if (LiveOutReg.count(x.first)) {
           liveout_own = true;

        }
      }
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
    ///      --> phi_def = COPY in_r
    ///   body :
    ///          inst
    ///          inst ...
    ///      --> phi_def = COPY own_r
    ///````

    ///   (1)-3. preにin_rからown_rへのCopy命令を挿入する(def_r = Copy in_r)。
    MachineInstr *Copy = BuildMI(*pre, pre->getFirstTerminator(), dbgloc,SWPipeliner::TII->get(TargetOpcode::COPY), def_r)
                                .addReg(in_r);
    addCopies(Copy);

    ///   (1)-4. preにin_rからown_rへのCopy命令を挿入する(def_r = Copy own_r)。
    /// own_r/def_rの参照がない場合はCOPY生成不要
    auto *def_op = uses[phi];
    if (liveout_def || liveout_own || def_op==nullptr) {
      if (flow.count(phi)) {
        if (DebugPrepare) {
           dbgs() << "DEBUG(convertNonSSA): change own_r " << printReg(own_r, SWPipeliner::TRI)
                  << " to " << printReg(flow[phi], SWPipeliner::TRI) << "\n";
        }
        own_r = flow[phi];
      }

      MachineInstr *c=BuildMI(*body, body->getFirstTerminator(), dbgloc,
                                SWPipeliner::TII->get(TargetOpcode::COPY), def_r)
                            .addReg(own_r,0, own_subreg);
      NewMI2OrgMI[c]=org_phi;
      if (DebugPrepare) {
        if (def_op && liveout_def)
          dbgs() << "DEBUG(convertNonSSA): Generate copy: def-reg is liveout!\n";
        else if (def_op && liveout_own)
          dbgs() << "DEBUG(convertNonSSA): Generate copy: own-reg is liveout!\n";
      }
    } else {
      if (DebugPrepare) {
        dbgs() << "DEBUG(convertNonSSA): Suppress the generation of COPY: " << *phi;
      }
      def_op->setReg(def_r);
    }

    ///          OrgMI2NewMIがorgのphiとnewのphiとなっているので、new側をCopy命令に変更する。
    for (auto &itr: getOrgMI2NewMI()) {
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
static void collectDefReg(llvm::MachineInstr &orgMI, std::map<Register, Register> &org2new) {
  for (unsigned i = 0, e = orgMI.getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = orgMI.getOperand(i);
    if (!(MO.isReg())) { continue; }
    llvm::Register orgReg = MO.getReg();
    if (!(orgReg.isVirtual())) { continue; }
    if (MO.isDef()) {
      llvm::Register newReg = SWPipeliner::MRI->cloneVirtualRegister(orgReg);
      // 多重定義はないはず。
      assert(org2new.find(orgReg) == org2new.end());
      org2new[orgReg] = newReg;
    }
  }
}

void SwplLoop::renameReg() {
  for (auto &m_itr: getOrgMI2NewMI()) {
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
    MachineInstr &newMI = SWPipeliner::TII->duplicate(*newBody, newBody->getFirstTerminator(), *mi);
    // レジスタリネーミング向けに定義レジスタを複製し、orgとnewのレジスタをmapにため込む
    collectDefReg(*org, getOrgReg2NewReg());
    addOrgMI2NewMI(mi, &newMI);
  }
  // レジスタを変換する
  renameReg();
}

bool SwplLoop::convertSSAtoNonSSA(MachineLoop &L, const SwplScr::UseMap &LiveOutReg) {
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
  
  /// cloneBody() を呼び出し、MachineBasickBlockの複製とレジスタリネーミングを行う。(OrgReg2NewRegを生成する)
  cloneBody(new_bb, ob);

  auto regmap=getOrgReg2NewReg();
  for (auto &p:LiveOutReg) {
    // p.first: reg, p.second:vector<MO*>
    auto newreg=regmap[p.first];
    liveOuts.insert(newreg);
  }

  if (!DisableRmCopy)
    removeCopy(new_bb);
  /// convertPostPreIndeTo()を呼び出し、post/pre index命令を演算＋load/store命令に変換する
  if (!DisableConvPrePost)
    convertPrePostIndexInstr(new_bb);

  if (SWPipeliner::STM->isEnableRegAlloc()) {
    if (checkRestrictions(new_bb)) return true;
  }

  /// convertNonSSA() を呼び出し、非SSA化と複製したMachineBasicBlockの回収を行う。
  /// PHIからCOPYを生成する際、liveOutsにPHIで定義されるvregを追加している。
  convertNonSSA(new_bb, pre, dbgloc, ob, LiveOutReg);

  if (SWPipeliner::STM->isEnableRegAlloc() || EnableNormalizeTieddef)
    // レジスタ割付が動作するなら、tied-defにCOPY命令を追加する
    normalizeTiedDef(new_bb);

  return false;
}

bool SwplLoop::check_need_copy4TiedUseReg(const MachineBasicBlock* body, const MachineInstr* tiedInstr,
                                      Register tiedDefReg, Register tiedUseReg) const {

  // 1) tiedInstrの定義参照が同じレジスタの場合のみ不要

  if (tiedDefReg == tiedUseReg) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(SwplLoop::check_need_copy4TiedUseReg): tiedUseReg("
             << printReg(tiedUseReg, SWPipeliner::TRI) << ") eq tiedDef:" << *tiedInstr;
    }
    return false;
  }
  if (tiedInstr->getOpcode() == TargetOpcode::INSERT_SUBREG || tiedInstr->getOpcode() == TargetOpcode::EXTRACT_SUBREG ||
      tiedInstr->getOpcode() == TargetOpcode::EXTRACT_SUBREG || tiedInstr->getOpcode() == TargetOpcode::SUBREG_TO_REG) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(SwplLoop::check_need_copy4TiedUseReg): Instruction to exclude allocation of physical registers\n"
                "  mi: " << *tiedInstr;
    }
    return false;
  }
  if (DebugPrepare) {
    dbgs() << "DEBUG(SwplLoop::check_need_copy4TiedUseReg): Decide that the COPY instruction is necessary\n";
  }
  return true;
}

void SwplLoop::normalizeTiedDef(MachineBasicBlock *body) {

  // tied-def operands that require COPY
  SmallVector<std::tuple<MachineInstr*, MachineOperand*>, 20> targets;

  for (auto &MI:*body) {
    for (auto &MO:MI.defs()) {
      if (!MO.isReg()) continue;
      if (!MO.isTied()) continue;
      unsigned use_tied_index=0;
      bool t = MI.isRegTiedToUseOperand(MI.getOperandNo(&MO), &use_tied_index);
      assert(t && "isRegTiedToUseOperand");
      auto &useMO = MI.getOperand(use_tied_index);
      auto use_tied_r = useMO.getReg();

      if (check_need_copy4TiedUseReg(body, &MI, MO.getReg(), use_tied_r)) {
        targets.push_back({&MI, &useMO});
        if (DebugPrepare) {
          dbgs() << "DEBUG(SwplLoop::normalizeTiedDef): Copy is required.: " << MI;
        }
      } else {
        if (DebugPrepare) {
          dbgs() << "DEBUG(SwplLoop::normalizeTiedDef): Copy is not required.: " << MI;
        }
      }
    }
  }

  for (auto &p:targets) {
    auto *mi  = std::get<0>(p);
    auto *tied_mo = std::get<1>(p);
    auto tied_reg = tied_mo->getReg();
    auto new_reg = SWPipeliner::MRI->cloneVirtualRegister(tied_reg);
    auto dbgloc = mi->getDebugLoc();
    MachineInstr *Copy = BuildMI(*body, mi, dbgloc, SWPipeliner::TII->get(TargetOpcode::COPY), new_reg)
                             .addReg(tied_reg);
    tied_mo->setReg(new_reg);
    if (DebugPrepare) {
      dbgs() << "DEBUG(SwplLoop::normalizeTiedDef):\n  generate: " << *Copy;
      dbgs() << "  chenge op: " << *mi;
    }
  }
}

void SwplLoop::removeCopy(MachineBasicBlock *body) {
  std::vector<llvm::MachineInstr *> delete_mi;
  std::vector<llvm::MachineOperand *> target_mo;
  for (auto &mi:*body) {
    if (!mi.isCopy()) continue;
    if (DebugPrepare) {
      dbgs() << "DBG(SwplLoop::removeCopy)\n";
      dbgs() << " target mi: " << mi;
    }

    auto &op0=mi.getOperand(0);
    const auto *org_copy=NewMI2OrgMI.at(&mi);
    if (liveOuts.contains(op0.getReg())) {
      if (DebugPrepare) {
        dbgs() << " org mi: " << org_copy;
        dbgs() << " op0 is liveout!\n";
      }
      continue;
    }
    if (!mi.getOperand(1).isReg()) {
      if (DebugPrepare) {
        dbgs() << " op1 isnot reg!\n";
      }
      continue;
    }
    Register r0 = op0.getReg();
    if (r0.isPhysical()) {
      if (DebugPrepare) {
        dbgs() << " op0 isnot virtual-reg!\n";
      }
      continue;
    }
    target_mo.clear();
    bool hasSubreg = mi.getOperand(1).getSubReg() != 0;
    bool foundSubreg = false;
    bool foundTied = false;
    for (auto &u:SWPipeliner::MRI->use_operands(r0)) {
      if (hasSubreg && u.isTied()) {
        foundTied = true;
        break;
      }
      if (hasSubreg && u.getSubReg() != 0) {
        foundSubreg = true;
        break;
      }
      target_mo.push_back(&u);
    }
    if (foundTied) {
      if (DebugPrepare) {
        dbgs() << " op1 has subreg && use operand is tied!\n";
      }
      continue;
    }
    if (foundSubreg) {
      if (DebugPrepare) {
        dbgs() << " op1 has subreg!\n";
      }
      continue;
    }
    bool use_onlyphi=true;
    for (auto *op:target_mo) {
      auto *umi = op->getParent();
      if (!umi->isPHI()) use_onlyphi=false;
    }

    if (use_onlyphi ||
        SWPipeliner::TII->canRemoveCopy(*body, mi, *SWPipeliner::MRI,
                                        (SWPipeliner::STM->isEnableRegAlloc() || IgnoreRegClass_RmCopy))){

      // op1定義からop0定義までに間にop0の参照がある場合はCOPYを削除できない
      Register r1 = mi.getOperand(1).getReg();
      bool use_op0=false;
      auto *def_use = SWPipeliner::MRI->getOneDef(r1);
      if (def_use!=nullptr) {
        for (MachineInstr *t = def_use->getParent()->getNextNode(); t!=nullptr && !use_op0; t=t->getNextNode()) {
          if (t==&mi) break;
          for (auto &MO:t->uses()) {
            if (MO.isReg() && MO.getReg()==r0) {
              use_op0 = true;
              break;
            }
          }
        }
        if (use_op0) {
          if (DebugPrepare) {
            dbgs() << " op0 is referenced from the definition of op1 to the definition of op0.!\n";
          }
          continue;
        }
      }
      for (auto *op:target_mo) {
        auto *umi=op->getParent();
        if (DebugPrepare) {
          dbgs() << " before: " << *umi;
        }
        // r0を参照しているオペランドを書き換える
        auto &op1 = mi.getOperand(1);
        op->setReg(op1.getReg());
        if (hasSubreg) {
          op->setSubReg(op1.getSubReg());
        }
        op->setIsKill(false);
        if (DebugPrepare) {
          dbgs() << " after: " << *umi;
        }
      }

      // クローン前命令とクローン後命令の再紐づけ
      auto *org = NewMI2OrgMI.at(&mi);
      NewMI2OrgMI.erase(&mi);
      OrgMI2NewMI[org]=nullptr;
      if (DebugPrepare) {
        dbgs() << " removed!\n";
      }
      // MBBから削除する命令の収集
      delete_mi.push_back(&mi);
    } else {
      if (DebugPrepare) {
        dbgs() << " canRemoveCopy() is false!\n";
      }
    }
  }
  for (auto *mi:delete_mi)
    mi->eraseFromParent();

}

void SwplLoop::convertPrePostIndexInstr(MachineBasicBlock *body) {
  std::vector<llvm::MachineInstr *> delete_mi;
  for (auto &mi:*body) {
    MachineInstr *ldst=nullptr;
    MachineInstr *add=nullptr;
    if (SWPipeliner::TII->splitPrePostIndexInstr(*body, mi, &ldst, &add)){
      // クローン前命令とクローン後命令の再紐づけ
      auto *org = NewMI2OrgMI.at(&mi);
      NewMI2OrgMI.erase(&mi);
      OrgMI2NewMI[org] = ldst;
      NewMI2OrgMI[ldst] = org;
      NewMI2OrgMI[add] = org;
      if (DebugPrepare) {
        dbgs() << "DBG(SwplLoop::convertPrePostIndexInstr)\n";
        dbgs() << " before:" << mi;
        dbgs() << " after 1:" << *ldst;
        dbgs() << " after 2:" << *add;
      }

      // MBBから削除する命令の収集
      delete_mi.push_back(&mi);
    }
  }
  for (auto *mi:delete_mi)
    mi->eraseFromParent();

}

bool SwplLoop::checkRestrictions(const MachineBasicBlock *body) {
  if (SWPipeliner::isDisableRestrictionsCheck(
          SWPipeliner::SwplRestrictionsFlag::All)) return false;
  for (auto &mi:*body) {
    if (!SWPipeliner::isDisableRestrictionsCheck(
            SWPipeliner::SwplRestrictionsFlag::MultipleDef)) {
      int num=0;
      for (auto &def:mi.defs()) {
        if (def.isImplicit()) continue;
        num++;
      }
      if (num > 1) {
        SWPipeliner::makeMissedMessage_RestrictionsDetected(mi);
        return true;
      }
    }
    if (!SWPipeliner::isDisableRestrictionsCheck(
            SWPipeliner::SwplRestrictionsFlag::MultipleReg)) {
      for (auto &mo:mi.operands()) {
        if (mo.isReg()) {
          auto r = mo.getReg();
          if (r.isPhysical()) continue;
          auto r_class = SWPipeliner::MRI->getRegClass(r);
          if (SWPipeliner::TII->isMultipleReg(r_class)) {
            SWPipeliner::makeMissedMessage_RestrictionsDetected(mi);
            return true;
          }
        }
      }
    }
  }
  return false;
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

bool SwplLoop::containsLiveOutReg(Register vreg) {
  return liveOuts.contains(vreg);
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


int SwplMem::calcEachMemAddressIncrement() {
  int sum = 0;
  int old_sum = 0;
  if (SWPipeliner::isDebugDdgOutput()) {
    if (Inst->getMI()==nullptr)
      dbgs() << "DBG(calcEachMemAddressIncrement): \n";
    else
      dbgs() << "DBG(calcEachMemAddressIncrement): " << *(Inst->getMI());
  }
  for (auto *reg:getUseRegs()) {

    int increment = SWPipeliner::TII->calcEachRegIncrement(reg);
    if (SWPipeliner::isDebugDdgOutput()) {
      dbgs() << " calcEachRegIncrement(" << printReg(reg->getReg(), SWPipeliner::TRI)  << "): "
             << increment << "\n";
    }
    if (increment == SwplDdg::UNKNOWN_MEM_DIFF) {
      return increment;
    }
    old_sum = sum;
    sum += increment;

    // 計算結果の溢れチェック
    if (increment > 0) {
      if (sum < old_sum) {
        return SwplDdg::UNKNOWN_MEM_DIFF;
      }
    } else if (increment < 0) {
      if (sum > old_sum) {
        return SwplDdg::UNKNOWN_MEM_DIFF;
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
  // @todo オリジナル命令がpost/pre命令の場合は、変換後のadd命令が出力されない
  // これは、MAPがオリジナル命令とクローンした命令が1対1を前提としているため
  for (auto &itr: getOrgMI2NewMI()) {
    dbgs() << "OldMI=" << *itr.first << "\n";
    dbgs() << "NewMI=" << *itr.second << "\n";
  }
}

void SwplLoop::dumpOrgReg2NewReg() {
  for (auto &itr: getOrgReg2NewReg()) {
    dbgs() << "OldReg=" << printReg(itr.first, SWPipeliner::TRI)
           << "NewReg=" << printReg(itr.second, SWPipeliner::TRI) << "\n";
  }
}

void SwplLoop::dumpCopies() {
  for (auto *itr: getCopies()) {
    dbgs() << "MI=" << *itr << "\n";
  }
}

void SwplLoop::printRegID(llvm::Register r) {
  dbgs() << "RegID=" << printReg(r, SWPipeliner::TRI) << "\n";
}

SwplReg::SwplReg(const SwplReg &s) {
  Reg = s.Reg;
  DefInst = s.DefInst;
  DefIndex = s.DefIndex;
  Predecessor = s.Predecessor;
  Successor = s.Successor;
  EarlyClobber=s.EarlyClobber;
  units = s.units;
  rk = SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, s.Reg);
}

SwplReg::SwplReg(Register r, SwplInst &i, size_t def_index, bool earlyclober) {
  Reg = r;
  DefInst = &i;
  DefIndex = def_index;
  Predecessor = nullptr;
  Successor = nullptr;
  EarlyClobber=earlyclober;
  rk = SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, r);
  units = rk->getUnitNum();

}

void SwplReg::print() {
  dbgs() << "DBG(SwplReg::print) SwplReg. : ";
  dbgs() << "SwplReg=" << this << ": RegID=" << printReg(getReg(), SWPipeliner::TRI) << "\n";
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
      if (SWPipeliner::TII->isNonTargetMI4SWPL(MI)) { return; }
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
  if (SWPipeliner::TII->getMemOperandWithOffset(*(inst.getMI()), BaseOp, Offset, OffsetIsScalable, SWPipeliner::TRI)
      && BaseOp->isReg()) {
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