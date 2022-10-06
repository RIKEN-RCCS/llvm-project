//=- AArch64SwplTransformMIR.cpp -  Transform MachineIR for SWP -*- C++ -*---=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Transform MachineIR for SWP.
//
//===----------------------------------------------------------------------===//
namespace swpl {
  class SwplScr;
  class SwplPlan;
  class SwplReg;
  class SwplLoop;
  class SwplInst;
  class SwplMem;
  struct TransformedMIRInfo;
}

#include "AArch64SwplTransformMIR.h"
#include "AArch64.h"
#include "AArch64SWPipeliner.h"
#include "AArch64SwplPlan.h"
#include "AArch64SwplScr.h"
#include "AArch64SwplTargetMachine.h"
#include "AArch64TargetTransformInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace swpl;

#define DEBUG_TYPE "aarch64-swpipeliner"

static cl::opt<std::string> ImportPlan("swpl-plan-import", cl::init(""), cl::ReallyHidden);
static cl::opt<std::string> ExportPlan("swpl-plan-export", cl::init(""), cl::ReallyHidden);
static cl::opt<int> DumpMIR("swpl-debug-dump-mir",cl::init(0), cl::ReallyHidden);
static cl::opt<bool> DisableRemoveUnnecessaryBR("swpl-disable-rm-br", cl::init(false), cl::ReallyHidden);


bool SwplTransformMIR::transformMIR() {
  bool updated=false;
  size_t n_body_inst = Loop.getSizeBodyInsts();
  size_t n_body_real_inst=Loop.getSizeBodyRealInsts();
  SwplScr SCR(*(Loop.getML()));
  static bool first=true;
  if (first) {
    first=false;
    if (!ImportPlan.empty()) importPlan();
    if (!ExportPlan.empty()) exportPlan();
  }

  if (DumpMIR & (int)BEFORE) dumpMIR(BEFORE);

  /// (1) convertPlan2Linda()でSwplPlanの情報をTLIに移し変える
  convertPlan2Linda();
  if (TMI.isNecessaryTransformMIR()){
    updated=true;
    /// (2) TransformedMIRInfo::isNecessaryTransformMIR()であれば\n
    /// (2-1) SwplScr::prepareCompensationLoop()でループの外を変形する
    SCR.prepareCompensationLoop(TMI);
    /// (2-2) transformKernel()でループの中を変形する
    transformKernel();
    /// (2-3) SwplLoop::deleteNewBodyMBB() データ抽出で生成したMBBを削除する
    Loop.deleteNewBodyMBB();
    /// TLI.gensをリセットする(gensで指しているMIRは削除済のため)
    TMI.mis.clear();
    /// (2-4) postTransformKernel() Check1,Check2合流点でPHIを生成する
    postTransformKernel();
    if (DumpMIR) {
      dbgs() << "** TransformedMIRInfo begin **\n";
      TMI.print();
      dbgs() << "** TransformedMIRInfo end   **\n";
      if (DumpMIR & (int)AFTER) dumpMIR(AFTER);
    }

    /// (2-5) convert2SSA() SSA形式に変換する
    convert2SSA();
    if (DumpMIR & (int)AFTER_SSA) dumpMIR(AFTER_SSA);
    if (!DisableRemoveUnnecessaryBR) {
      /// (2-6) SwplScr::postSSA() 不要な分岐を削除する("-swpl-disable-rm-br"が指定されていなければ)
      SCR.postSSA(TMI, Loop);
      if (DumpMIR & (int)LAST) dumpMIR(LAST);
    }

    /// (2-7) outputLoopoptMessage() SWPL成功の最適化messageを出力する
    outputLoopoptMessage(n_body_real_inst, Plan.getPolicy());
  }

  if (swpl::DebugOutput) {
    /// (3) "-swpl-debug"が指定されている場合は、デバッグ情報を出力する
    if (TMI.isNecessaryTransformMIR()){
      const char *p;
      switch(Plan.getPolicy()) {
      case swpl::SwplSchedPolicy::SWPL_SCHED_POLICY_SMALL:p="S";break;
      case swpl::SwplSchedPolicy::SWPL_SCHED_POLICY_LARGE:p="L";break;
      default:p="A";break;
      }
      dbgs()  << formatv(
              "        :\n"
              "        : Loop is software pipelined. (ii={0}, kernel={1} cycles, prologue,epilogue ={2} cycles)\n"
              "        :      {3}\n"
              "        :      IPC (initial={4}, real={5}, rate={6:P})\n"
              "        :      = Instructions({7})/II({8})\n"
              "        :      Virtual inst:({9})\n"
              "        :\n",
              /* 0 */ (int)TMI.iterationInterval,
              /* 1 */ (int)(TMI.iterationInterval * TMI.nVersions),
              /* 2 */ (int)(TMI.iterationInterval * (TMI.nCopies - TMI.nVersions)),
              /* 3 */ p,
              /* 4 */ (float)n_body_real_inst / (float)TMI.minimumIterationInterval,
              /* 5 */ (float)n_body_real_inst / (float)TMI.iterationInterval,
              /* 6 */ (float)TMI.minimumIterationInterval / (float)TMI.iterationInterval,
              /* 7 */ (int)n_body_real_inst,
              /* 8 */ (int)TMI.iterationInterval,
              /* 0 */ (int)(n_body_inst - n_body_real_inst));
    } else {
      dbgs() <<
              "        :\n"
              "        : Swpl kernel is not generated because of shortage of iteration.\n"
              "        :\n";
    }
  }
  return updated;
}

/// KERNELの分岐言の比較に用いる制御変数のversionを決定する
/// \details
/// - どの制御変数を選んでも構わないが,KERNELの最後にくるものを選択する.
/// - 制御変数のversionを定義するsub言のSlotを計算して取得する.
/// - 上から数えて一つ目のsubを0番目としてあつかう.
size_t SwplTransformMIR::chooseCmpIteration(size_t bandwidth, size_t slot) {
  size_t i, initial_cycle, cycle, iteration;
  initial_cycle = slot / STM.getFetchBandwidth();
  /* EPILOGUEから逆向きに探索を始め,最初にKERNELのcycleの範囲に入るものを返却する*/
  for (i = 0; i <= TMI.nCopies - 1; ++i) {
    iteration = TMI.nCopies - 1 - i;
    cycle = initial_cycle + TMI.iterationInterval * iteration;
    if (cycle < TMI.prologEndIndx/bandwidth + (TMI.kernelEndIndx - TMI.prologEndIndx)/bandwidth) {
      return iteration;
    }
  }
  llvm_unreachable ("");
  return iteration;
}

/// SwplRegのversionからllvm::Registerへのmapを構成する.
/// \details
///   SWPLがkernelを展開する際のSwplRegに割り当てるllvm::Registerを決定する.
///   もともとのループで使用していたllvm::Registerが version == n_versions-1 に
///   対応するようにmapを生成する.
///   ただし,phi instの定義するSwplRegのみversion == 0にoriginal Registerが対応する.
/// - phi instの定義するSwplRegについて
/// ```
///     def = φ(out, use) において
///     (def, 0) = [original prg] = (use, n_versions - 1);
///     1 <= i < n_versions のとき (def, version) = (use, version - 1);
/// ```
/// - 他のSwplReg
///     他の SwplReg は全てバージョン毎に新しい llvm::Register を割り当てる。
///     ただし、同じ llvm::Register を割り当てなければならない SwplReg についての考慮も必要。
///     例えば$x0,$nzcvなどは同じoriginalなllvm::Registerを割り当てる必要がある.
/// - 割り当てるversionについて
///     llvm::Registerに関してはversion == n_version -1 において,
///     original llvm::Registerへ定義がなされるようにversionを設定する.
///       入口busyの解決の為には,prologでmoveを出す.
///       出口busyの解決の為には,epilogの最後の定義をoriginal llvm::Registerに行なうため,
///       version == n_version - 1がEPILOGの最後のversionになるよう展開する.
void SwplTransformMIR::constructPrgMap(size_t n_versions) {

  llvm::SmallSet<llvm::Register, 32> defined_prg;
  /* phi instに関して*/
  for (auto* inst: Loop.getPhiInsts()){

    auto &def_reg=inst->getDefRegs(0);
    preparePrgVectorForReg(&def_reg, n_versions);

    /* (def, 0) = [original prg] = (use, n_versions - 1);
     * def_regのversion == 0はphi instの定義の参照
     * use_regのversion == n_versions -1 はphi instで参照される
     */
    const auto &use_reg=inst->getPhiUseRegFromIn();
    preparePrgVectorForReg (&use_reg, n_versions);
    /*
     * phiのuse_reg  == original_prgを定義しているのは,
     * use_regのversion == n_versions - 1 としてsetする.
     */
    setPrg (&use_reg, n_versions - 1, use_reg.getReg());
    defined_prg.insert(use_reg.getReg());
  }

  /* bodyで定義されるprgに関してmapを構成する*/
  for (const auto* inst: Loop.getBodyInsts()) {
    for (const auto* def_reg: inst->getDefRegs()) {
      if (PrgMap.count(def_reg)==0) {
        llvm::Register original_prg = def_reg->getReg();
        preparePrgVectorForReg(def_reg, n_versions);

        /* vgatherなどでoriginal_prgはNoRegがありうる*/
        if (!original_prg.isValid()) { continue; }

        if (defined_prg.count(original_prg)==0) {
          /* original_prgへdefするversionは,上記のphi instと併せて全てsetする*/
          setPrg (def_reg, n_versions - 1, original_prg);
          defined_prg.insert(original_prg);
        }
      }
    }
  }
}

const SwplReg*
SwplTransformMIR::controlPrg2RegInLoop(const SwplInst **updateInst) {
  auto &map=Loop.getOrgMI2NewMI();
  const auto *update=map[TMI.updateDoVRegMI];
  /// データ抽出でレジスタを書き換えているため、オリジナルMIRでの誘導変数(Register)から変換後のRegisterを取得する。

  /// (1) レジスタ同士の変換はできないため、（誘導変数更新MIRである）UpdateDoPrgGenの変換後MIRを取得し、そのOP1を利用する
  TMI.nonSSAOriginalDoVReg =update->getOperand(1).getReg();
  const auto *copy=map[TMI.initDoVRegMI];
  assert(copy->getOpcode()==AArch64::COPY);
  TMI.nonSSAOriginalDoInitVar=copy->getOperand(1).getReg();

  LLVM_DEBUG(dbgs() << "updateDoVRegMI:" << *(TMI.updateDoVRegMI));
  LLVM_DEBUG(dbgs() << "map(updateDoVRegMI):" << *update);
  /// (2) 取得した誘導変数更新MIRから、対応するSwplInst探し、定義SwplRegを取得し返却する。
  for (auto I=Loop.getSizeBodyInsts(); I>0; I--) {
    const auto &inst = Loop.getBodyInst (I-1);
    if (inst.getMI()!=update) continue;
    *updateInst=&inst;
    auto &reg=inst.getDefRegs(0);
    return &reg;
  }
  llvm_unreachable ("");
  return nullptr;
}

void SwplTransformMIR::convertPlan2Linda() {
  /**
   * min_n_iterations:
   *  kernelを繰り返す為に必要なループ制御変数の値\n
   *  もともとの回転数ではなくループ制御変数に関する値である為,
   *  cmp_iterationに応じて値が変わる事に注意.
   */
  size_t min_n_iterations;
  size_t do_prg_versions;
  /**
   * cmp_iteration:
   * kernelに展開したループ制御変数の比較言の内, 最終的なkernelの比較言に採用した回転数の順番を表す.\n
   * 実行する回転数に影響はせず,分岐定数に影響する.
   * 0 <= cmp_iteration <= n_versions-1 が成立する
   */
  size_t cmp_iteration;

  SwplScr SCR(*Loop.getML());
  size_t bandwidth = STM.getFetchBandwidth();

  /// (1) SwplScr::findBasicInductionVariable():元のループの制御変数に関する情報の取得
  ///  制御変数の初期値を見つける
  if (!SCR.findBasicInductionVariable (TMI)) {
    llvm_unreachable ("LindaSr::findBasicInductionVariable() returned false");
    return;
  }

  /// (2) planによる展開数の決定
  TMI.minimumIterationInterval = Plan.getMinimumIterationInterval();
  TMI.iterationInterval = Plan.getIterationInterval();
  TMI.nVersions = Plan.getNRenamingVersions();
  TMI.nCopies   = Plan.getNIterationCopies();
  TMI.requiredKernelIteration  = TMI.nCopies;
  /// (3) 展開後の回転数の情報を更新
  if (TMI.isIterationCountConstant) {
    size_t kernel_available_iteration;
    if (TMI.originalKernelIteration >= TMI.requiredKernelIteration) {
      kernel_available_iteration
              =
          TMI.originalKernelIteration - (TMI.requiredKernelIteration - TMI.nVersions);
      TMI.transformedKernelIteration = kernel_available_iteration/ TMI.nVersions;
      TMI.transformedModIteration    =
          TMI.originalKernelIteration -
          TMI.transformedKernelIteration * TMI.nVersions
              - (TMI.nCopies - TMI.nVersions);
      assert (TMI.transformedKernelIteration >= 1);
    } else {
      TMI.transformedKernelIteration = 0;
      TMI.transformedModIteration    = TMI.originalKernelIteration;
    }
  }

  /// (4) outputNontunedMessage(): デバッグメッセージを出力
  outputNontunedMessage();

  /// (5) 各部分(prolog/kernel/epilog)のMIRの範囲を計算する
  TMI.prologEndIndx
          = (TMI.nCopies - TMI.nVersions) * TMI.iterationInterval * bandwidth;
  TMI.kernelEndIndx
          =
      TMI.nVersions * TMI.iterationInterval * bandwidth + TMI.prologEndIndx;
  TMI.epilogEndIndx
          = (TMI.nCopies - TMI.nVersions) * TMI.iterationInterval * bandwidth +
      TMI.kernelEndIndx;

  const SwplInst *update=nullptr;
  /// (6) controlPrg2RegInLoop(): 分岐用のDO制御変数を獲得
  const auto* induction_reg = controlPrg2RegInLoop (&update);

  const auto* def_inst = update;
  /// (7) chooseCmpIteration(): 分岐用のDO制御変数を更新する命令を獲得
  cmp_iteration = chooseCmpIteration (bandwidth, relativeInstSlot(def_inst));

  /// (8) kernelを繰り返すのに必要な元のループ回転数の計算
  min_n_iterations = TMI.nCopies + (TMI.nVersions - (cmp_iteration + 1)) ;
  /// (9) 分岐言と更新言を考慮して誘導変数と比較できる値へ換算
  TMI.expansion = TMI.coefficient * min_n_iterations + TMI.minConstant;

  /// (10) constructPrgMap(): SwplRegとRegisterのMapを用意
  constructPrgMap (TMI.nVersions);
  /// (11) prepareGens(): 展開用の言の生成
  prepareGens ();

  /// (12) getPrg() Do制御変数の新レジスタを取得
  do_prg_versions = (cmp_iteration +
                     shiftConvertIteration2Version(
                                         TMI.nVersions, TMI.nCopies)) %
                    TMI.nVersions ;

  TMI.doVReg = getPrg(*induction_reg, do_prg_versions);

  /**
   * @note
   * getCoefficientでcoefficientがi4max以下であるチェックはしているが、
   * (n_copies+1)*coefficientがi4maxを越える場合を考慮していない。
   * bct化されている場合、coefficientは実質的にunroll展開数を表すため、
   * i4maxを越えている事はない。
   */
  assert (TMI.coefficient > 0);
  assert (min_n_iterations >= 2);
}

llvm::MachineInstr *SwplTransformMIR::createGenFromInst(const SwplInst &inst, size_t version) {

  /* 新規言の生成 */
  /// (1) 引数：instを元に、llvm::MachineInstrを生成する
  const auto* org_gen = inst.getMI();  /* オリジナルの言 */
  assert(org_gen);
  auto *new_gen=MF.CloneMachineInstr(org_gen); /* コピー */
  LLVM_DEBUG(dbgs() << "SwplTransformMIR::createGenFromInst() begin\n");
  LLVM_DEBUG(dbgs() << "before vwesion:" << version << " new_gen:" << *new_gen << "\n");

  // 同一命令のDef/useに同じレジスタが存在する場合、異なるSwplRegが割りあたる方式に対応
  int i=-1;
  for (auto *reg:inst.getDefRegs()) {
    /// (2) def-register(virtualのみ)に割り当てるレジスタを getPrg() で取得する
    i++;
    Register r=reg->getReg();
    if (r.isPhysical()) continue;
    LLVM_DEBUG(dbgs() << "swplreg=" << reg << ":" << printReg(r, TRI) << "from(" << inst.getDefOperandIx(i) << "):" << new_gen->getOperand(inst.getDefOperandIx(i)) <<"\n");
    auto new_reg = getPrg (*reg, version);
    LLVM_DEBUG(dbgs() << "newreg=" << printReg(new_reg, TRI) << "\n" );
    if (new_reg==r) continue; // 新しいレジスタを割り当てる必要がなかったので、何もしない
    auto &mo=new_gen->getOperand(inst.getDefOperandIx(i));
    mo.setReg(new_reg);
  }
  i=-1;
  for (auto *reg:inst.getUseRegs()) {
    /// (3) use-register(virtualのみ)に割り当てるレジスタを getPrg() で取得する
    i++;
    Register r=reg->getReg();
    if (r.isPhysical()) continue;
    LLVM_DEBUG(dbgs() << "swplreg=" << reg << ":" << printReg(r, TRI) << "from(" << inst.getDefOperandIx(i) << "):" << new_gen->getOperand(inst.getDefOperandIx(i)) <<"\n");
    auto new_reg = getPrg (*reg, version);
    LLVM_DEBUG(dbgs() << "newreg=" << printReg(new_reg, TRI) << "\n" );
    if (new_reg==r) continue; // 新しいレジスタを割り当てる必要がなかったので、何もしない
    auto &mo=new_gen->getOperand(inst.getUseOperandIx(i));
    mo.setReg(new_reg);
  }
  LLVM_DEBUG(dbgs() << "after new_gen:" << *new_gen << "\n");
  LLVM_DEBUG(dbgs() << "SwplTransformMIR::createGenFromInst() end\n");
  return new_gen;
}

llvm::Register SwplTransformMIR::getPrgFromMap (const swpl::SwplReg *org, unsigned version)  const {
  // llvm::Register割当がない場合はInValidなRegisterを返す
  auto it_prgs = PrgMap.find(org);
  if (it_prgs==PrgMap.end()) return llvm::Register();
  auto prgs=it_prgs->getSecond();
  if (prgs==nullptr) return llvm::Register();
  auto reg=prgs->at(version);
  return reg;
}

llvm::Register SwplTransformMIR::getPrg(const swpl::SwplReg& org, size_t version)  {

  auto orgReg=org.getReg();

  /// (1) regにlinda_prgが未設定(llvm::Register::isValid()ではない)の場合はそのレジスタを返す。
  if (!orgReg.isValid()) {
    llvm_unreachable("original-register is invalid");
    return orgReg;
  }

  /// (2) 既に allocate されてて、元の prg を使わなければならない場合、元のprgを使う。
  /// \note allocate済レジスタ：Physicalレジスタのこと。Physicalレジスタは変更しない
  if (orgReg.isPhysical()) {
    // ここで setReg() を実行しているが、実際には不要と考えられる
    setPrg (&org, version, orgReg);
    return orgReg;
  }

  /// (3) ループ前で定義されている場合,
  /// ループ内で定義が無いため,originalのregを参照するだけである.\n
  ///そのため外で定義したllvm::Registerを返す。
  auto* def_inst=org.getDefInst();
  if (!def_inst->isInLoop()) return orgReg;

  /// (4) 既にprgMapに登録済みである場合はそれを返す。
  auto newReg = getPrgFromMap (&org, version);
  if (newReg.isValid()) {
    return newReg;
  }

  /// (7) φで定義されている場合
  if (def_inst->isPhi()) {
    if (version == 0) {
      /**
       * original_prgはversion == 0で参照される.
       * またφ命令により定義、つまり前回転の最後のversionとして定義される.
       * ==> original_prgを返す
      */
      setPrg (&org, version, orgReg);
      return orgReg;
    } else {
      /**
       * version == 0でない場合、 SwplInst::getPhiUseRegFromIn() で取得したuseRegを指定して getPrg()し、結果を返す。
       */

      const auto &useReg = def_inst->getPhiUseRegFromIn ();

      newReg = getPrg (useReg, version - 1);
      setPrg (&org, version, newReg);
      return newReg;
    }
  }

   /// (8) その他のregの場合.\n
   /// この部分入力となるdef_prgは常に新しいllvm::Registerを作成する.
   /// 主にループ内で定義前参照ではないllvm::Registerに対する定義.
   /// ただし下記の場合には注意が必要である.
   /// @attention
   ///   (1) def->ref->def->のように同一Registerを二度定義する場合
   ///   後者のdef_regは constructPrgMap() で登録済みである.
   ///   SWPLの出力結果はSSAの必要がある為,original_prgへの定義は後者のみである.
   ///   ただし,def_regがiccなどの場合はoriginal_prgへの定義となるが,
   ///   これらの考慮は上で行なっている為、この入力とはならない.\n
   ///   (2) ref->def->ref->defのように同一Registerを二度定義する場合
   ///   後者のdef_regはphi_instでuseされる為,constructPrgMapで登録済みである.
   ///   SSAのため前者のdef_regはoriginal_prgに定義してはならない.
   ///   本処理に到達するprgは前者のdefのprgのみである.

    newReg = MRI->cloneVirtualRegister(orgReg);
    setPrg (&org, version, newReg);
    return newReg;
}


void SwplTransformMIR::insertGens(llvm::MachineBasicBlock& ins,
                                  SwplTransformMIR::BLOCK block) {

  size_t start_index=0;
  size_t end_index=0;

  /// (1) 挿入位置を計算する
  switch(block) {
  case PRO_MOVE:
    start_index = TMI.epilogEndIndx;
    end_index   = TMI.mis.size();
    break;
  case PROLOGUE:
    start_index = 0;
    end_index   = TMI.prologEndIndx;
    break;
  case KERNEL:
    start_index = TMI.prologEndIndx;
    end_index   = TMI.kernelEndIndx;
    break;
  case EPILOGUE:
    start_index = TMI.kernelEndIndx;
    end_index   = TMI.epilogEndIndx;
    break;
  case EPI_MOVE:
    start_index = TMI.epilogEndIndx;
    end_index   = TMI.mis.size();
    break;
  default:
    llvm_unreachable("");
    break;
  }

  if (block==PRO_MOVE) {
    /// (2) 挿入位置がPRO_MOVEの場合、データ抽出が生成したCOPY命令をまずは挿入する
    for (auto *mi:Loop.getCopies()) {
      mi->removeFromParent();
      ins.push_back(mi);
    }
  }
  /// (3) 計算した開始slotから終了slotまでの命令を挿入位置に移動する
  for (size_t i = start_index; i < end_index; ++i) {
    auto *mi = TMI.mis[i];
    if (mi != nullptr) {
      ins.push_back(mi);
    }
  }
}

void SwplTransformMIR::makeKernelIterationBranch(MachineBasicBlock &MBB) {
  auto insertionPoint=MBB.getFirstInstrTerminator();
  assert(TMI.branchDoVRegMI->isBranch());
  const auto &debugLoc= TMI.branchDoVRegMI->getDebugLoc();

  const auto*regClass=MRI->getRegClass(TMI.doVReg);
  auto ini= TMI.doVReg;
  if (regClass->hasSubClassEq(&AArch64::GPR64RegClass)) {
    /// 条件判定（SUBSXri）で利用できないレジスタクラスの場合、COPYを生成し、利用可能レジスタクラスを定義する
    ini=MRI->createVirtualRegister(&AArch64::GPR64spRegClass);
    BuildMI(MBB, insertionPoint, debugLoc, TII->get(AArch64::COPY), ini)
            .addReg(TMI.doVReg);
  }

  /// compare(SUBSXri)生成
  // $XZR(+CC)=SUBSXri %TLI.doVReg, TLI.expansion
  BuildMI(MBB, insertionPoint, debugLoc, TII->get(AArch64::SUBSXri), AArch64::XZR)
          .addReg(ini)
          .addImm(TMI.expansion)
          .addImm(0);

  /// すでに存在するCheck2(ModLoop迂回用チェック)の比較命令の対象レジスタをTLI.nonSSAOriginalDoPrgに書き換える
  // c2 mbbの先頭命令を取得（SUBSXRiのはず）
  auto *cmp=&*(TMI.Check2->begin());
  assert(cmp->getOpcode()==AArch64::SUBSXri);
  auto &op=cmp->getOperand(1);
  assert(op.isReg());
  op.setReg(TMI.nonSSAOriginalDoVReg);

  // branchの生成
  auto CC=AArch64CC::LT;
  if (TMI.coefficient > 0) {
    CC=AArch64CC::GE;
  }
  /// Bcc命令を生成し、TLI.branchDoPrgGenKernelに記録しておく
  auto br=BuildMI(MBB, insertionPoint, debugLoc, TII->get(AArch64::Bcc))
          .addImm(CC)
          .addMBB(&MBB);
  TMI.branchDoVRegMIKernel =&*br;
}

void SwplTransformMIR::outputNontunedMessage() {

  if (swpl::DebugOutput) {
    ///  内部開発者向け：情報を出力 (-swpl-debugが指定されていた場合)
    dbgs()  << formatv(
            "        : Required iteration count in MIR input is        :   {0}"
            " (= kernel:{1} + pro/epilogue:{2} + mod:{3}) \n"
            , (int)TMI.requiredKernelIteration
            , (int)TMI.nVersions
            , (int)(TMI.nCopies - TMI.nVersions)
            , (int)(TMI.requiredKernelIteration - TMI.nCopies)
    );

    if (TMI.isIterationCountConstant) {
      dbgs()  << formatv(
              "        : Original iteration count in MIR is found        :   {0}\n"
              "        :      Non-tuned SWPL (ker exp, ker itr, mod itr) : ( {1}, {2}, {3})\n"
              , (int)(TMI.originalKernelIteration)
              , (int)(TMI.nVersions)
              , (int)(TMI.transformedKernelIteration)
              , (int)(TMI.transformedModIteration)
      );
    } else {
      dbgs()  <<
              "        : Original loop iteration is not found.\n";
    }
  }
}

void SwplTransformMIR::prepareGens() {

  unsigned iteration_interval_slot;

  TMI.mis.resize(TMI.epilogEndIndx);
  iteration_interval_slot = TMI.iterationInterval * STM.getFetchBandwidth();
  /// (1) shiftConvertIteration2Version() :命令挿入位置（コピー毎の移動値）を計算する
  int n_shift = shiftConvertIteration2Version(TMI.nVersions, TMI.nCopies);

  const MachineInstr*firstMI=nullptr;
  /* PROLOGUE,KERNEL,EPILOGUE部分 */
  for (auto *sinst: Loop.getBodyInsts()) {
    unsigned slot;
    if (firstMI==nullptr) firstMI=sinst->getMI();

    /// (2) relativeInstSlot() 命令の相対位置を計算する
    slot = relativeInstSlot(sinst);
    /// (3) TLI.nCopies分、命令を生成する
    for (size_t j = 0; j < TMI.nCopies; ++j) {
      size_t version;
      unsigned new_slot;

      /// (3-1) 命令の挿入位置を計算し、createGenFromInst() で生成した命令を挿入する
      version = (j + n_shift) % TMI.nVersions;
      new_slot = slot + iteration_interval_slot * j;
      auto *newMI = createGenFromInst (*sinst, version);
      TMI.mis[new_slot]=newMI;
    }
  }
  {
 /// (4) 入口busyを解決する為のMOVE部分を処理する

    assert(firstMI!=nullptr);
    size_t target_version;
    /* original versionからKERNELの先頭で参照されるversionへのmoveが必要*/
    target_version = (shiftConvertIteration2Version(TMI.nVersions, TMI.nCopies) - 1 +
         TMI.nVersions) %
        TMI.nVersions;

    for (auto *sphi: Loop.getPhiInsts()) {
      const SwplReg &originalSReg = sphi->getPhiUseRegFromIn ();
      Register newReg = getPrg (originalSReg, target_version);
      if (newReg == originalSReg.getReg()) {
        continue;
      }
      llvm::MachineInstr *copy=BuildMI(MF,firstMI->getDebugLoc(),TII->get(AArch64::COPY), newReg)
                                  .addReg(originalSReg.getReg());
      TMI.mis.push_back(copy);
    }
  }

  return ;
}

void SwplTransformMIR::preparePrgVectorForReg(const swpl::SwplReg *reg, size_t n_versions) {
  auto *regs=new std::vector<llvm::Register>;
  regs->resize(n_versions);
  PrgMap[reg]=regs;
}

void SwplTransformMIR::setPrg(const swpl::SwplReg *orgReg, size_t version, llvm::Register newReg) {
  auto *regs=PrgMap[orgReg];
  assert(regs!=nullptr);
  assert(regs->size()>version);
  regs->at(version)=newReg;
}

void SwplTransformMIR::outputLoopoptMessage(int n_body_inst, SwplSchedPolicy policy) {

  int ipc100=0;
  assert(TMI.iterationInterval != 0);
  if (TMI.iterationInterval != 0) {
    ipc100 = (int)((100*n_body_inst)/ TMI.iterationInterval);
    assert(0 < ipc100 && ipc100 <= 400);
    if (ipc100 < 0) {
      ipc100 = 0;
    } else if (ipc100 > 400) {
      ipc100 = 400;
    }
  }

  int mve = TMI.nVersions;
  assert(0 < mve && mve <= 255);
  if (mve < 0) {
    mve = 0;
  } else if (mve > 255) {
    mve = 255;
  }
  std::string msg=formatv("software pipelining (IPC: {0}, ITR: {1}, MVE: {2}, POL: {3})",
                          (ipc100/100.), TMI.nCopies, mve, (policy==SwplSchedPolicy::SWPL_SCHED_POLICY_SMALL?"S":"L"));

  swpl::ORE->emit([&]() {
    return MachineOptimizationRemark(DEBUG_TYPE, "SoftwarePipelined",
                                     Loop.getML()->getStartLoc(), Loop.getML()->getHeader())
            << msg;
  });
}

void SwplTransformMIR::transformKernel() {

  /// (3) body,prolog,epilogの命令を挿入

  /// (3-1) MOVE in PROLOGUE 部の作成
  insertGens (*(TMI.Prolog), PRO_MOVE);
  /// (3-2) PROLOGUE部の作成
  insertGens (*(TMI.Prolog), PROLOGUE);

  /// (3-3) KERNEL部の作成
  insertGens (*(TMI.OrgBody), KERNEL);

  /// (3-4) EPILOGUE部の作成
  insertGens (*(TMI.Epilog), EPILOGUE);

  /// (4) SWPL KERNELの繰返し分岐を生成
  makeKernelIterationBranch(*(TMI.OrgBody));
}

int SwplTransformMIR::shiftConvertIteration2Version(int n_versions, int n_copies) const {
  int n_shift;
  assert (n_copies >= n_versions);
  n_shift = (n_versions - 1) - (n_copies - 1) +  (n_copies / n_versions) * n_versions;
  assert(n_shift >= 0);

  return n_shift;
}

void SwplTransformMIR::postTransformKernel() {
  /// (1) NPにC1飛び込みのPHIを生成する
  /// (元のLoopのPHIを元に生成する)
  auto &mimap=Loop.getOrgMI2NewMI();
  auto InsertPoint1= TMI.NewPreHeader->getFirstNonPHI();
  for (auto &mi: TMI.NewBody->phis()) {
    /// (2) PHI生成
    auto *cp=mimap[&mi];
    auto op0=cp->getOperand(0).getReg();
    auto op1=cp->getOperand(1).getReg();
    auto defPhi=MRI->cloneVirtualRegister(op0);
    BuildMI(*(TMI.NewPreHeader), InsertPoint1, mi.getDebugLoc(), TII->get(AArch64::PHI), defPhi)
            .addReg(op1).addMBB(TMI.Check1).addReg(op0).addMBB(TMI.Check2);

    /// (2-1) NP側のレジスタを新規生成するPHIで定義するレジスタに切り替える
    if (mi.getOperand(2).getMBB()== TMI.NewPreHeader) {
      auto &op=mi.getOperand(1);
      op.setReg(defPhi);
    } else {
      auto &op=mi.getOperand(3);
      op.setReg(defPhi);
    }
  }
  /// (3) NEに出口BUSY用のPHIを生成する
  /// (出口BUSYは collectLiveOut で収集している)
  auto regmap=Loop.getOrgReg2NewReg();
  auto InsertPoint2= TMI.NewExit->getFirstNonPHI();
  const auto &debugLoc= TMI.NewBody->begin()->getDebugLoc();
  for (auto &p:LiveOutReg) {
    // p.first: reg, p.second:vector<MO*>
    auto newreg=regmap[p.first];
    auto defPhi=MRI->cloneVirtualRegister(newreg);
    BuildMI(*(TMI.NewExit), InsertPoint2, debugLoc, TII->get(AArch64::PHI), defPhi)
            .addReg(p.first).addMBB(TMI.NewBody).addReg(newreg).addMBB(TMI.Check2);
    for (auto *mo:p.second) {
      assert(mo->getReg()==p.first);
      mo->setReg(defPhi);
    }
  }
}

void SwplTransformMIR::replaceDefReg(MachineBasicBlock &mbb, std::map<Register,Register>&regmap) {
  std::map<Register, std::vector<llvm::MachineOperand*>> useregs;
  auto InsertPoint=mbb.getFirstNonPHI();
  bool kernel = (&mbb== TMI.OrgBody);

  for (auto &mi:mbb) {
    // 参照レジスタの情報を集める && 変更
    for (auto &op:mi.operands()) {
      if (!op.isReg()) continue;
      auto useReg=op.getReg();
      if (useReg.isPhysical() || op.isDef()) continue;
      if (kernel) {
        // 参照先行になっているか調査のため、参照情報を収集
        useregs[useReg].push_back(&op);
      }
      if (regmap.count(useReg) != 0) {
        op.setReg(regmap[useReg]);
      }
    }
    // 定義レジスタは必ず書き換える
    for (auto &op:mi.operands()) {
      if (!op.isReg() || op.getReg().isPhysical() || op.isUse()) continue;
      auto def=op.getReg();
      auto newDef=MRI->cloneVirtualRegister(def);
      if (kernel) {
        const auto &debugLoc=InsertPoint->getDebugLoc();
        if (useregs.count(def) != 0) {
          // 参照先行なのでリカレンスになっている--> PHIが必要
          auto defPhi = MRI->cloneVirtualRegister(def);
          BuildMI(mbb, InsertPoint, debugLoc, TII->get(AArch64::PHI), defPhi)
                  .addReg(def).addMBB(TMI.Prolog).addReg(newDef).addMBB(TMI.OrgBody);

          // ここまでの参照レジスタをPHI定義のレジスタに書き換える。
          for (auto *useOp:useregs[def]) {
            useOp->setReg(defPhi);
          }
        }
      }
      // 通常の定義レジスタに対する処理
      regmap[def]=newDef;
      op.setReg(newDef);
    }
  }
}

void SwplTransformMIR::replaceUseReg(std::set<MachineBasicBlock*> &mbbs, const std::map<Register,Register>&regmap) {
  // registerのdef/use情報は正しい場合、効率的にレジスタの変更がおこなえる
  std::vector<MachineOperand*> targets;
  for (auto &m:regmap) {
    /// (1) 変更すべきオペランドを集める
    for (auto &op:MRI->use_operands(m.first)) {
      auto *mi=op.getParent();
      auto *mbb=mi->getParent();
      if (mbbs.count(mbb)!=0) {
        targets.push_back(&op);
      }
    }
    /// (2) 収集したターゲットのレジスタを書き換える
    for (auto *op:targets) {
      op->setReg(m.second);
    }
    /// (3) 収集した情報をクリアし、次のターゲット収集に備える
    targets.clear();
  }
}

void SwplTransformMIR::convertEpilog2SSA() {
  std::map<Register,Register> conv;
  replaceDefReg(*(TMI.Epilog), conv);

  std::set<MachineBasicBlock*> mbbs;
  mbbs.insert(TMI.Check2);
  mbbs.insert(TMI.NewPreHeader);
  mbbs.insert(TMI.NewBody);
  mbbs.insert(TMI.NewExit);
  mbbs.insert(TMI.OrgExit);
  replaceUseReg(mbbs, conv);
}

void SwplTransformMIR::convertKernel2SSA() {
  std::map<Register,Register> conv;
  replaceDefReg(*(TMI.OrgBody), conv);

  std::set<MachineBasicBlock*> mbbs;
  mbbs.insert(TMI.Epilog);
  mbbs.insert(TMI.Check2);
  mbbs.insert(TMI.NewPreHeader);
  mbbs.insert(TMI.NewBody);
  mbbs.insert(TMI.NewExit);
  mbbs.insert(TMI.OrgExit);
  replaceUseReg(mbbs, conv);
}

void SwplTransformMIR::convertProlog2SSA() {
  std::map<Register,Register> conv;
  std::set<Register> defs;
  for (auto &mi:*(TMI.Prolog)) {
    for (auto &op:mi.operands()) {
      if (!op.isReg() || op.isDef()) continue;
      auto use=op.getReg();
      if (use.isPhysical()) continue;
      if (conv.count(use)!=0) op.setReg(conv[use]);
    }
    for (auto &op:mi.operands()) {
      if (!op.isReg() || op.isUse()) continue;
      auto def=op.getReg();
      if (def.isPhysical()) continue;
      if (defs.count(def)==0) {
        defs.insert(def);
      } else {
        auto newDef = MRI->cloneVirtualRegister(def);
        op.setReg(newDef);
        conv[def]=newDef;
      }
    }
  }

  std::set<MachineBasicBlock*> mbbs;
  mbbs.insert(TMI.OrgBody);
  mbbs.insert(TMI.Epilog);
  mbbs.insert(TMI.Check2);
  mbbs.insert(TMI.NewPreHeader);
  mbbs.insert(TMI.NewBody);
  mbbs.insert(TMI.NewExit);
  mbbs.insert(TMI.OrgExit);
  replaceUseReg(mbbs, conv);

}

void SwplTransformMIR::convert2SSA() {
  convertEpilog2SSA();
  convertKernel2SSA();
  convertProlog2SSA();
}


namespace swpl {

/// 命令の並びを表現するためのクラス
struct IOSlot {
  unsigned id; ///< 命令の番号（スケジューリング対象命令の出現順番号）
  unsigned slot; ///< 命令を置くslot番号
};

/// SwplPlanとYAMLの仲介で利用するクラス
struct IOPlan {
  unsigned minimum_iteration_interval;
  unsigned iteration_interval;
  size_t n_renaming_versions;
  size_t n_iteration_copies;
  unsigned begin_slot;
  std::vector<IOSlot> Slots;
};
}

template <>
struct llvm::yaml::MappingTraits<swpl::IOSlot> {
  static void mapping(IO &io, IOSlot &info) {
    io.mapRequired("id", info.id);
    io.mapRequired("slot", info.slot);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(swpl::IOSlot)

template <>
struct llvm::yaml::MappingTraits<swpl::IOPlan> {
  static void mapping(IO &io, IOPlan &info) {
    io.mapRequired("minimum_iteration_interval", info.minimum_iteration_interval);
    io.mapRequired("iteration_interval", info.iteration_interval);
    io.mapRequired("n_renaming_versions", info.n_renaming_versions);
    io.mapRequired("n_iteration_copies", info.n_iteration_copies);
    io.mapRequired("begin_slot", info.begin_slot);
    io.mapRequired("slots", info.Slots);
  }
};

void SwplTransformMIR::exportPlan() {
  std::error_code EC;
  /// -swpl-export-planで指定したファイルを開く
  raw_fd_ostream OutStrm(ExportPlan, EC);
  if (EC) {
    errs() << "Couldn't open " << ExportPlan << " for writing.\n";
    return;
  }
  /// SwplPlanからYAML出力のためIOPlanに情報を設定する
  IOPlan ioplan;
  ioplan.minimum_iteration_interval=Plan.getMinimumIterationInterval();
  ioplan.iteration_interval=Plan.getIterationInterval();
  ioplan.n_renaming_versions=Plan.getNRenamingVersions();
  ioplan.n_iteration_copies=Plan.getNIterationCopies();
  ioplan.begin_slot=Plan.getBeginSlot();
  unsigned  n=0;
  /// スケジューリング対象命令をコメントとして出力する
  OutStrm << "# inst-slot-map(Supplement of export-plan):\n";
  for (auto *inst:Loop.getBodyInsts()) {
    OutStrm << "# " << n << "," << *(inst->getMI());
    IOSlot ioslot={n, relativeInstSlot(inst)};
    ioplan.Slots.push_back(ioslot);
    n++;
  }
  /// IOPlanを出力する
  llvm::yaml::Output yout(OutStrm);
  yout << ioplan;
}

void SwplTransformMIR::importPlan() {
  /// -swpl-import-planで指定したファイルを開く

  ErrorOr<std::unique_ptr<MemoryBuffer>>  Buffer = MemoryBuffer::getFile(ImportPlan);
  if (std::error_code EC = Buffer.getError()) {
    errs() << "open file error:" << ImportPlan << "\n";
    return;
  }
  /// YAMLファイルからIOPlanに入力する
  swpl::IOPlan ioplan;
  llvm::yaml::Input yin(Buffer.get()->getMemBufferRef());
  yin >> ioplan;
  /// IOPlanからSwplPlanに情報を移し替える
  Plan.setMinimumIterationInterval(ioplan.minimum_iteration_interval);
  Plan.setIterationInterval(ioplan.iteration_interval);
  Plan.setNRenamingVersions(ioplan.n_renaming_versions);
  Plan.setNIterationCopies(ioplan.n_iteration_copies);
  Plan.setBeginSlot(ioplan.begin_slot);
  // 念の為MAPをリセット
  InstSlotMap.clear();
  for (auto &slot:ioplan.Slots) {
    /// \note スケジューリングが必要な命令に対し、指示が不足しているかは確認していない。
    /// また、指示で指定した命令IDが、存在範囲外かも確認していない
    InstSlotMap[&(Loop.getBodyInst(slot.id))]=slot.slot+ioplan.begin_slot;
  }
}

void SwplTransformMIR::dumpMIR(DumpMIRID id) const {
  const char*title;
  /// ターゲットループ情報を出力する
  dbgs() << "target loop:" << *(Loop.getML());
  if (id==BEFORE) {
    /// 出力タイミングがBEFOREの場合、LiveOut情報をまず出力する
    for (auto &t:LiveOutReg) {
      dbgs() << "LiveOutReg(" << printReg(t.first, TRI) << "):\n";
      for (auto *op:t.second) {
        dbgs() << "op(" << *op << "): " << *(op->getParent());
      }
    }
  }
  switch (id) {
  case BEFORE:title="BEFORE:"; break;
  case AFTER:title="AFTER:"; break;
  case AFTER_SSA:title="AFTER_SSA:"; break;
  case LAST:title="LAST:"; break;
  }
  /// 対象のMachineFunctionに属するMIRをすべて出力する
  dbgs() << title << "\n";
  for (auto &mbb:MF) {
    dbgs() << mbb;
  }
}

unsigned SwplTransformMIR::relativeInstSlot(const SwplInst *inst) const {
  return InstSlotMap[const_cast<SwplInst*>(inst)]-Plan.getBeginSlot();
}
