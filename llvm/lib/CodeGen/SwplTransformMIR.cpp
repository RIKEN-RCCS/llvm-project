//=- SwplTransformMIR.cpp -  Transform MachineIR for SWP -*- C++ -*----------=//
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
namespace llvm {
  class SwplScr;
  class SwplPlan;
  class SwplReg;
  class SwplLoop;
  class SwplInst;
  class SwplMem;
  struct SwplTransformedMIRInfo;
}

#include "SwplTransformMIR.h"
#include "SWPipeliner.h"
#include "SwplPlan.h"
#include "SwplScr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"

using namespace llvm;
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
  if (DumpMIR & (int)SLOT_BEFORE) dumpMIR(SLOT_BEFORE);

  /// (1) convertPlan2MIR()でSwplPlanの情報をTMIに移し変える
  convertPlan2MIR();
  if (TMI.isNecessaryTransformMIR()){
    updated=true;

    /// 物理レジスタを割り付ける
    //if (!SWPipeliner::STM->isDisableRegAlloc())
    if (SWPipeliner::STM->isEnableRegAlloc()) {
      if (!SWPipeliner::TII->SwplRegAlloc(&TMI, MF)) {
        // 割付失敗
        return false;
      }

    }

    /// (2) SwplTransformedMIRInfo::isNecessaryTransformMIR()であれば\n
    /// (2-1) SwplScr::prepareCompensationLoop()でループの外を変形する
    SCR.prepareCompensationLoop(TMI);
    /// (2-2) transformKernel()でループの中を変形する
    transformKernel();
    /// (2-3) SwplLoop::deleteNewBodyMBB() データ抽出で生成したMBBを削除する
    Loop.deleteNewBodyMBB();
    /// TMI.misをリセットする(misで指しているMIRは削除済のため)
    TMI.mis.clear();
    /// (2-4) postTransformKernel() Check1,Check2合流点でPHIを生成する
    postTransformKernel();
    if (DumpMIR) {
      dbgs() << "** SwplTransformedMIRInfo begin **\n";
      TMI.print();
      dbgs() << "** SwplTransformedMIRInfo end   **\n";
      if (DumpMIR & (int)AFTER) dumpMIR(AFTER);
    }

    /// (2-5) convert2SSA() SSA形式に変換する
    convert2SSA();
    if (DumpMIR & (int)AFTER_SSA) dumpMIR(AFTER_SSA);
    if (!DisableRemoveUnnecessaryBR) {
      /// (2-6) SwplScr::postSSA() 不要な分岐を削除する("-swpl-disable-rm-br"が指定されていなければ)
      SCR.postSSA(TMI);
      if (DumpMIR & (int)LAST) dumpMIR(LAST);
    }

    /// (2-7) Count the number of COPY in the kernel loop
    countKernelCOPY();

    /// (2-8) outputLoopoptMessage() SWPL成功の最適化messageを出力する
    outputLoopoptMessage(n_body_real_inst);
  }

  if (SWPipeliner::isDebugOutput()) {
    /// (3) "-swpl-debug"が指定されている場合は、デバッグ情報を出力する
    if (TMI.isNecessaryTransformMIR()){
      dbgs()  << formatv(
              "        :\n"
              "        : Loop is software pipelined. (ii={0}, kernel={1} cycles, prologue,epilogue ={2} cycles)\n"
              "        :      IPC (initial={3}, real={4}, rate={5:P})\n"
              "        :      = Instructions({6})/II({7})\n"
              "        :      Virtual inst:({8})\n"
              "        :\n",
              /* 0 */ (int)TMI.iterationInterval,
              /* 1 */ (int)(TMI.iterationInterval * TMI.nVersions),
              /* 2 */ (int)(TMI.iterationInterval * (TMI.nCopies - TMI.nVersions)),
              /* 3 */ (float)n_body_real_inst / (float)TMI.minimumIterationInterval,
              /* 4 */ (float)n_body_real_inst / (float)TMI.iterationInterval,
              /* 5 */ (float)TMI.minimumIterationInterval / (float)TMI.iterationInterval,
              /* 6 */ (int)n_body_real_inst,
              /* 7 */ (int)TMI.iterationInterval,
              /* 8 */ (int)(n_body_inst - n_body_real_inst));
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
  initial_cycle = slot / SWPipeliner::FetchBandwidth;
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

/// SwplRegのversionからRegisterへのmapを構成する.
/// \details
///   SWPLがkernelを展開する際のSwplRegに割り当てるRegisterを決定する.
///   もともとのループで使用していたRegisterが version == n_versions-1 に
///   対応するようにmapを生成する.
///   ただし,phi instの定義するSwplRegのみversion == 0にoriginal Registerが対応する.
/// - phi instの定義するSwplRegについて
/// ```
///     def = φ(out, use) において
///     (def, 0) = [original vreg] = (use, n_versions - 1);
///     1 <= i < n_versions のとき (def, version) = (use, version - 1);
/// ```
/// - 他のSwplReg
///     他の SwplReg は全てバージョン毎に新しい Register を割り当てる。
///     ただし、同じ Register を割り当てなければならない SwplReg についての考慮も必要。
///     例えば$x0,$nzcvなどは同じoriginalなRegisterを割り当てる必要がある.
/// - 割り当てるversionについて
///     Registerに関してはversion == n_version -1 において,
///     original Registerへ定義がなされるようにversionを設定する.
///       入口busyの解決の為には,prologでmoveを出す.
///       出口busyの解決の為には,epilogの最後の定義をoriginal Registerに行なうため,
///       version == n_version - 1がEPILOGの最後のversionになるよう展開する.
void SwplTransformMIR::constructVRegMap(size_t n_versions) {

  SmallSet<Register, 32> defined_vreg;
  /* phi instに関して*/
  for (auto* inst: Loop.getPhiInsts()){

    auto &def_reg=inst->getDefRegs(0);
    prepareVRegVectorForReg(&def_reg, n_versions);

    /* (def, 0) = [original vreg] = (use, n_versions - 1);
     * def_regのversion == 0はphi instの定義の参照
     * use_regのversion == n_versions -1 はphi instで参照される
     */
    const auto &use_reg=inst->getPhiUseRegFromIn();
    prepareVRegVectorForReg(&use_reg, n_versions);
    /*
     * phiのuse_reg  == original_vregを定義しているのは,
     * use_regのversion == n_versions - 1 としてsetする.
     */
    setVReg(&use_reg, n_versions - 1, use_reg.getReg());
    defined_vreg.insert(use_reg.getReg());
  }

  /* bodyで定義されるvregに関してmapを構成する*/
  for (const auto* inst: Loop.getBodyInsts()) {
    for (const auto* def_reg: inst->getDefRegs()) {
      if (VRegMap.count(def_reg)==0) {
        Register original_vreg = def_reg->getReg();
        prepareVRegVectorForReg(def_reg, n_versions);

        /* vgatherなどでoriginal_vregはNoRegがありうる*/
        if (!original_vreg.isValid()) { continue; }

        if (defined_vreg.count(original_vreg)==0) {
          /* original_vregへdefするversionは,上記のphi instと併せて全てsetする*/
          setVReg(def_reg, n_versions - 1, original_vreg);
          defined_vreg.insert(original_vreg);
        }
      }
    }
  }
}

const SwplReg*
SwplTransformMIR::controlVReg2RegInLoop(const SwplInst **updateInst) {
  auto &map=Loop.getOrgMI2NewMI();
  const auto *update=map[TMI.updateDoVRegMI];
  /// データ抽出でレジスタを書き換えているため、オリジナルMIRでの誘導変数(Register)から変換後のRegisterを取得する。

  /// (1) レジスタ同士の変換はできないため、（誘導変数更新MIRである）UpdateDoVRegMIの変換後MIRを取得し、そのOP1を利用する
  TMI.nonSSAOriginalDoVReg =update->getOperand(1).getReg();
  const auto *copy=map[TMI.initDoVRegMI];
  assert(copy->getOpcode()==TargetOpcode::COPY);
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

void SwplTransformMIR::convertPlan2MIR() {
  /**
   * min_n_iterations:
   *  kernelを繰り返す為に必要なループ制御変数の値\n
   *  もともとの回転数ではなくループ制御変数に関する値である為,
   *  cmp_iterationに応じて値が変わる事に注意.
   */
  size_t min_n_iterations;
  size_t do_vreg_versions;
  /**
   * cmp_iteration:
   * kernelに展開したループ制御変数の比較言の内, 最終的なkernelの比較言に採用した回転数の順番を表す.\n
   * 実行する回転数に影響はせず,分岐定数に影響する.
   * 0 <= cmp_iteration <= n_versions-1 が成立する
   */
  size_t cmp_iteration;

  SwplScr SCR(*Loop.getML());
  size_t bandwidth = SWPipeliner::FetchBandwidth;

  /// (1) SwplScr::findBasicInductionVariable():元のループの制御変数に関する情報の取得
  ///  制御変数の初期値を見つける
  if (!SCR.findBasicInductionVariable (TMI)) {
    llvm_unreachable ("SwplScr::findBasicInductionVariable() returned false");
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
  /// (6) controlVReg2RegInLoop(): 分岐用のDO制御変数を獲得
  const auto* induction_reg = controlVReg2RegInLoop(&update);

  const auto* def_inst = update;
  /// (7) chooseCmpIteration(): 分岐用のDO制御変数を更新する命令を獲得
  cmp_iteration = chooseCmpIteration (bandwidth, relativeInstSlot(def_inst));

  /// (8) kernelを繰り返すのに必要な元のループ回転数の計算
  min_n_iterations = TMI.nCopies + (TMI.nVersions - (cmp_iteration + 1)) ;
  /// (9) 分岐言と更新言を考慮して誘導変数と比較できる値へ換算
  TMI.expansion = TMI.coefficient * min_n_iterations + TMI.minConstant;

  /// (10) constructVRegMap(): SwplRegとRegisterのMapを用意
  constructVRegMap(TMI.nVersions);
  /// (11) prepareMIs(): 展開用の言の生成
  prepareMIs();

  /// (12) getVReg() Do制御変数の新レジスタを取得
  do_vreg_versions = (cmp_iteration +
                     shiftConvertIteration2Version(
                                         TMI.nVersions, TMI.nCopies)) %
                    TMI.nVersions ;

  TMI.doVReg = getVReg(*induction_reg, do_vreg_versions);

  assert (TMI.coefficient > 0);
//  assert (min_n_iterations >= 2);
}

MachineInstr *SwplTransformMIR::createMIFromInst(const SwplInst &inst, size_t version) {

  /// (1) 引数：instを元に、MachineInstrを生成する
  const auto*org_MI = inst.getMI();  /* オリジナル */
  assert(org_MI);
  auto *new_MI=MF.CloneMachineInstr(org_MI); /* コピー */
  LLVM_DEBUG(dbgs() << "SwplTransformMIR::createMIFromInst() begin\n");
  LLVM_DEBUG(dbgs() << "before vwesion:" << version << " new_MI:" << *new_MI
                    << "\n");

  // 同一命令のDef/useに同じレジスタが存在する場合、異なるSwplRegが割りあたる方式に対応
  int i=-1;
  for (auto *reg:inst.getDefRegs()) {
    /// (2) def-register(virtualのみ)に割り当てるレジスタを getVReg() で取得する
    i++;
    Register r=reg->getReg();
    if (r.isPhysical()) continue;
    LLVM_DEBUG(dbgs() << "swplreg=" << reg << ":" << printReg(r, SWPipeliner::TRI)
                      << "from(" << inst.getDefOperandIx(i) << "):" << new_MI->getOperand(inst.getDefOperandIx(i)) <<"\n");
    auto new_reg = getVReg(*reg, version);
    LLVM_DEBUG(dbgs() << "newreg=" << printReg(new_reg, SWPipeliner::TRI) << "\n" );
    if (new_reg==r) continue; // 新しいレジスタを割り当てる必要がなかったので、何もしない
    auto &mo= new_MI->getOperand(inst.getDefOperandIx(i));
    mo.setReg(new_reg);
  }
  i=-1;
  for (auto *reg:inst.getUseRegs()) {
    /// (3) use-register(virtualのみ)に割り当てるレジスタを getVReg() で取得する
    i++;
    Register r=reg->getReg();
    if (r.isPhysical()) continue;
    LLVM_DEBUG(dbgs() << "swplreg=" << reg << ":" << printReg(r, SWPipeliner::TRI)
                      << "from(" << inst.getUseOperandIx(i) << "):" << new_MI->getOperand(inst.getUseOperandIx(i)) <<"\n");
    auto new_reg = getVReg(*reg, version);
    LLVM_DEBUG(dbgs() << "newreg=" << printReg(new_reg, SWPipeliner::TRI) << "\n" );
    if (new_reg==r) continue; // 新しいレジスタを割り当てる必要がなかったので、何もしない
    auto &mo= new_MI->getOperand(inst.getUseOperandIx(i));
    mo.setReg(new_reg);
  }
  LLVM_DEBUG(dbgs() << "after new_MI:" << *new_MI << "\n");
  LLVM_DEBUG(dbgs() << "SwplTransformMIR::createMIFromInst() end\n");
  return new_MI;
}

Register SwplTransformMIR::getVRegFromMap(const SwplReg *org, unsigned version)  const {
  // Register割当がない場合はInValidなRegisterを返す
  auto it_vregs = VRegMap.find(org);
  if (it_vregs == VRegMap.end()) return Register();
  auto vregs = it_vregs->getSecond();
  if (vregs ==nullptr) return Register();
  auto reg= vregs->at(version);
  return reg;
}

Register SwplTransformMIR::getVReg(const SwplReg& org, size_t version)  {

  auto orgReg=org.getReg();

  /// (1) regにvregが未設定(Register::isValid()ではない)の場合はそのレジスタを返す。
  if (!orgReg.isValid()) {
    llvm_unreachable("original-register is invalid");
    return orgReg;
  }

  /// (2) 既に allocate されてて、元の vreg を使わなければならない場合、元のvregを使う。
  /// \note allocate済レジスタ：Physicalレジスタのこと。Physicalレジスタは変更しない
  if (orgReg.isPhysical()) {
    // ここで setReg() を実行しているが、実際には不要と考えられる
    setVReg(&org, version, orgReg);
    return orgReg;
  }

  /// (3) ループ前で定義されている場合,
  /// ループ内で定義が無いため,originalのregを参照するだけである.\n
  ///そのため外で定義したRegisterを返す。
  auto* def_inst=org.getDefInst();
  if (!def_inst->isInLoop()) return orgReg;

  /// (4) 既にvregMapに登録済みである場合はそれを返す。
  auto newReg = getVRegFromMap(&org, version);
  if (newReg.isValid()) {
    return newReg;
  }

  /// (7) φで定義されている場合
  if (def_inst->isPhi()) {
    if (version == 0) {
      /**
       * original_vregはversion == 0で参照される.
       * またφ命令により定義、つまり前回転の最後のversionとして定義される.
       * ==> original_vregを返す
      */
      setVReg(&org, version, orgReg);
      return orgReg;
    } else {
      /**
       * version == 0でない場合、 SwplInst::getPhiUseRegFromIn() で取得したuseRegを指定して getVReg()し、結果を返す。
       */

      const auto &useReg = def_inst->getPhiUseRegFromIn ();

      newReg = getVReg(useReg, version - 1);
      setVReg(&org, version, newReg);
      return newReg;
    }
  }

   /// (8) その他のregの場合.\n
   /// この部分入力となるdef_vregは常に新しいRegisterを作成する.
   /// 主にループ内で定義前参照ではないRegisterに対する定義.
   /// ただし下記の場合には注意が必要である.
   /// @attention
   ///   (1) def->ref->def->のように同一Registerを二度定義する場合
   ///   後者のdef_regは constructVRegMap() で登録済みである.
   ///   SWPLの出力結果はSSAの必要がある為,original_vregへの定義は後者のみである.
   ///   ただし,def_regがccなどの場合はoriginal_vregへの定義となるが,
   ///   これらの考慮は上で行なっている為、この入力とはならない.\n
   ///   (2) ref->def->ref->defのように同一Registerを二度定義する場合
   ///   後者のdef_regはphi_instでuseされる為,constructVRegMapで登録済みである.
   ///   SSAのため前者のdef_regはoriginal_vregに定義してはならない.
   ///   本処理に到達するvregは前者のdefのvregのみである.

    newReg = SWPipeliner::MRI->cloneVirtualRegister(orgReg);
    setVReg(&org, version, newReg);
    return newReg;
}


void SwplTransformMIR::insertMIs(MachineBasicBlock& ins,
                                  SwplTransformMIR::BLOCK block) {

  size_t start_index=0;
  size_t end_index=0;

  /// (1) Calculate insertion position
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
    /// (2) If the insertion position is PRO_MOVE, 
    /// the COPY instruction generated by data extraction is inserted first
    for (auto *mi:Loop.getCopies()) {
      mi->removeFromParent();
      ins.push_back(mi);
    }
  }

  /// (3) Insert a starting instruction
  if (SWPipeliner::STM->isEnableRegAlloc()) {
    if (SWPipeliner::STM->isEnableProEpiCopy()) {
      // Livein
      if (block == KERNEL) {
        ins.push_back(TMI.kernel_livein_mi); // Add SWPLIVEIN
      // Liveout
      } else if (block == EPILOGUE) {
        ins.push_back(TMI.epilog_livein_mi); // Add SWPLIVEIN
        for (auto *mi : TMI.liveout_copy_mis) { // Add COPY
          ins.push_back(mi);
        }
      }
    } else {
      // Livein
      if (block == KERNEL) {
        for (auto *mi : TMI.livein_copy_mis) { // Add COPY
          ins.push_back(mi);
        }
      }
    }
  }

  /// (4) Insert an instruction from the calculated start slot to the end slot
  for (size_t i = start_index; i < end_index; ++i) {
    auto *mi = TMI.mis[i];
    if (mi != nullptr) {
      ins.push_back(mi);
    }
  }

  /// (5) Insert a terminating instruction
  if (SWPipeliner::STM->isEnableRegAlloc()) {
    if (SWPipeliner::STM->isEnableProEpiCopy()) {
      // Livein
      if (block == PROLOGUE) {
        for (auto *mi : TMI.livein_copy_mis) { // Add COPY
          ins.push_back(mi);
        }
        ins.push_back(TMI.prolog_liveout_mi); // Add SWPLIVEOUT
      // Liveout
      } else if (block == KERNEL) {
        ins.push_back(TMI.kernel_liveout_mi); // Add SWPLIVEOUT
      }
    } else {
      // Liveout
      if (block == KERNEL) {
        for (auto *mi : TMI.liveout_copy_mis) { // Add COPY
          ins.push_back(mi);
        }
      }
    }
  }
}

void SwplTransformMIR::makeKernelIterationBranch(MachineBasicBlock &MBB) {

  assert(TMI.branchDoVRegMI->isBranch());
  Register preg = 0;
  if (SWPipeliner::STM->isEnableRegAlloc() && TMI.swplRAITbl != nullptr) {
    // TMI.doVReg に入っている仮想レジスタに、SwplRegAlloc()で
    // 物理レジスタが割り当てられていたら、それを使用する。
    auto rai = TMI.swplRAITbl->getWithVReg( TMI.doVReg );
    if( rai != nullptr ) {
      preg = rai->preg;
    }
  }
  const auto &debugLoc= TMI.branchDoVRegMI->getDebugLoc();
  MachineInstr *br = SWPipeliner::TII->makeKernelIterationBranch(*SWPipeliner::MRI, MBB, debugLoc, TMI.doVReg,
                                                                 TMI.expansion, TMI.coefficient, preg);

  /// すでに存在するCheck2(ModLoop迂回用チェック)の比較命令の対象レジスタをTMI.nonSSAOriginalDoVRegに書き換える
  // c2 mbbの先頭命令を取得
  auto *cmp=&*(TMI.Check2->begin());
  auto &op=cmp->getOperand(1);
  assert(op.isReg());
  op.setReg(TMI.nonSSAOriginalDoVReg);

  TMI.branchDoVRegMIKernel =&*br;
}

void SwplTransformMIR::outputNontunedMessage() {

  if (SWPipeliner::isDebugOutput()) {
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

void SwplTransformMIR::prepareMIs() {

  unsigned iteration_interval_slot;

  TMI.mis.resize(TMI.epilogEndIndx);
  iteration_interval_slot = TMI.iterationInterval * SWPipeliner::FetchBandwidth;
  /// (1) shiftConvertIteration2Version() :命令挿入位置（コピー毎の移動値）を計算する
  int n_shift = shiftConvertIteration2Version(TMI.nVersions, TMI.nCopies);

  MachineInstr*firstMI=nullptr;
  /* PROLOGUE,KERNEL,EPILOGUE部分 */
  for (auto *sinst: Loop.getBodyInsts()) {
    unsigned slot;
    if (firstMI==nullptr) firstMI=sinst->getMI();
    MachineInstr *mi=sinst->getMI();
    if (mi->mayLoadOrStore() && mi->getNumMemOperands()) {
      // G_LD/STはMMO必須のため、安易な削除ではなく、MOVolatileの設定をすることにした
      // MIR上は、MMOに"volatile"が表示される
      // mi->dropMemRefs(MF);
      for (auto *mmo:mi->memoperands()) {
        mmo->setFlags(MachineMemOperand::Flags::MOVolatile);
      }
    }

    /// (2) relativeInstSlot() 命令の相対位置を計算する
    slot = relativeInstSlot(sinst);
    /// (3) TMI.nCopies分、命令を生成する
    for (size_t j = 0; j < TMI.nCopies; ++j) {
      size_t version;
      unsigned new_slot;

      /// (3-1) 命令の挿入位置を計算し、createMIFromInst() で生成した命令を挿入する
      version = (j + n_shift) % TMI.nVersions;
      new_slot = slot + iteration_interval_slot * j;
      auto *newMI = createMIFromInst(*sinst, version);
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
      Register newReg = getVReg(originalSReg, target_version);
      if (newReg == originalSReg.getReg()) {
        continue;
      }
      MachineInstr *copy=BuildMI(MF,firstMI->getDebugLoc(),SWPipeliner::TII->get(TargetOpcode::COPY), newReg)
                                  .addReg(originalSReg.getReg());
      TMI.mis.push_back(copy);
    }
  }

  return ;
}

void SwplTransformMIR::prepareVRegVectorForReg(const SwplReg *reg, size_t n_versions) {
  auto *regs=new std::vector<Register>;
  regs->resize(n_versions);
  VRegMap[reg]=regs;
}

void SwplTransformMIR::setVReg(const SwplReg *orgReg, size_t version, Register newReg) {
  auto *regs= VRegMap[orgReg];
  assert(regs!=nullptr);
  assert(regs->size()>version);
  regs->at(version)=newReg;
}

void SwplTransformMIR::outputLoopoptMessage(int n_body_inst) {

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
  std::string msg=formatv("software pipelining ("
                          "IPC: {0}, ITR: {1}, MVE: {2}, II: {3}, Stage: {4}, "
                          "(VReg Fp: {5}/{6}, Int: {7}/{8}, Pred: {9}/{10})), "
                          "SRA(PReg Fp: {11}/{12}, Int: {13}/{14}, Pred: {15}/{16})",
                          (ipc100/100.), TMI.nCopies, mve,
                          TMI.iterationInterval,
                          Plan.getNIterationCopies() - mve + 1,
                          Plan.getNecessaryFreg(), Plan.getMaxFreg(),
                          Plan.getNecessaryIreg(), Plan.getMaxIreg(),
                          Plan.getNecessaryPreg(), Plan.getMaxPreg(),
                          (TMI.swplRAITbl==nullptr?0:TMI.swplRAITbl->countFReg()),
                          (TMI.swplRAITbl==nullptr?0:TMI.swplRAITbl->availableFRegNumber()),
                          (TMI.swplRAITbl==nullptr?0:TMI.swplRAITbl->countIReg()),
                          (TMI.swplRAITbl==nullptr?0:TMI.swplRAITbl->availableIRegNumber()),
                          (TMI.swplRAITbl==nullptr?0:TMI.swplRAITbl->countPReg()),
                          (TMI.swplRAITbl==nullptr?0:TMI.swplRAITbl->availablePRegNumber())
                          );

  SWPipeliner::Reason = "";
  SWPipeliner::ORE->emit([&]() {
    return MachineOptimizationRemark(DEBUG_TYPE, "SoftwarePipelined",
                                     LoopLoc, Loop.getML()->getHeader())
            << msg;
  });
}

void SwplTransformMIR::transformKernel() {
  /// (1) Create instructions for SWPL
  if (SWPipeliner::STM->isEnableRegAlloc() && SWPipeliner::STM->isEnableProEpiCopy()){
    SWPipeliner::TII->createSwplPseudoMIs(&TMI, MF);
  }

  /// (2) Insert body,prolog,epilog instructions

  /// (2-1) MOVE in PROLOGUE section
  insertMIs(*(TMI.Prolog), PRO_MOVE);
  /// (2-2) PROLOGUE section
  insertMIs(*(TMI.Prolog), PROLOGUE);

  /// (2-3) KERNEL section
  insertMIs(*(TMI.OrgBody), KERNEL);

  /// (2-4) EPILOGUE section
  insertMIs(*(TMI.Epilog), EPILOGUE);

  /// (3) Generate a repeating branch of SWPL KERNEL
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
    auto defPhi=SWPipeliner::MRI->cloneVirtualRegister(op0);
    BuildMI(*(TMI.NewPreHeader), InsertPoint1, mi.getDebugLoc(), SWPipeliner::TII->get(TargetOpcode::PHI), defPhi)
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
    auto defPhi=SWPipeliner::MRI->cloneVirtualRegister(newreg);
    BuildMI(*(TMI.NewExit), InsertPoint2, debugLoc, SWPipeliner::TII->get(TargetOpcode::PHI), defPhi)
            .addReg(p.first).addMBB(TMI.NewBody).addReg(newreg).addMBB(TMI.Check2);
    for (auto *mo:p.second) {
      assert(mo->getReg()==p.first);
      mo->setReg(defPhi);
    }
  }
}

void SwplTransformMIR::replaceDefReg(MachineBasicBlock &mbb, std::map<Register,Register>&regmap) {
  std::map<Register, std::vector<MachineOperand*>> useregs;
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
      auto newDef=SWPipeliner::MRI->cloneVirtualRegister(def);
      if (kernel) {
        const auto &debugLoc=InsertPoint->getDebugLoc();
        if (useregs.count(def) != 0) {
          // 参照先行なのでリカレンスになっている--> PHIが必要
          auto defPhi = SWPipeliner::MRI->cloneVirtualRegister(def);
          BuildMI(mbb, InsertPoint, debugLoc, SWPipeliner::TII->get(TargetOpcode::PHI), defPhi)
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
    for (auto &op:SWPipeliner::MRI->use_operands(m.first)) {
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
        auto newDef = SWPipeliner::MRI->cloneVirtualRegister(def);
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

template <>
struct yaml::MappingTraits<SwplTransformMIR::IOSlot> {
  static void mapping(IO &io, SwplTransformMIR::IOSlot &info) {
    io.mapRequired("id", info.id);
    io.mapRequired("slot", info.slot);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(SwplTransformMIR::IOSlot)

template <>
struct yaml::MappingTraits<SwplTransformMIR::IOPlan> {
  static void mapping(IO &io, SwplTransformMIR::IOPlan &info) {
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
  yaml::Output yout(OutStrm);
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
  IOPlan ioplan;
  yaml::Input yin(Buffer.get()->getMemBufferRef());
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
      dbgs() << "LiveOutReg(" << printReg(t.first, SWPipeliner::TRI) << "):\n";
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
  case SLOT_BEFORE:title="SLOT_BEFORE:";
    dbgs() << title << "\n";
    for (auto *inst:Loop.getBodyInsts()) {
      dbgs()  << *(inst->getMI());
    }
    return;
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

void SwplTransformMIR::printTransformingMI(const MachineInstr *mi) {
  /*
   * SWPLの結果反映中のMIは、MFから紐づいていないため、
   * mi->print(dbgs())でOpcodeがUNKNOWNで出力されてしまう。
   * 本関数は、結果反映前のMFを用いてprintするための関数である。
   */
  mi->print(dbgs(),
            /* IsStandalone= */ true,
            /* SkipOpers= */ false,
            /* SkipDebugLoc= */ false,
            /* AddNewLine= */ true,
            MF.getSubtarget().getInstrInfo() );
  return;
}

void SwplTransformMIR::countKernelCOPY() {
    unsigned count = 0;
    for (auto &mi : *(TMI.OrgBody)) {
        if (mi.isCopy()) count++;
    }

    std::string msg =
        "The number of COPY instructions in the kernel loop for software "
        "pipelining is ";

    SWPipeliner::ORE->emit([&]() {
        return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "countKernelCOPY",
                                                 LoopLoc,
                                                 Loop.getML()->getHeader())
               << msg << ore::NV("KernelCOPY", count) << ".";
    });
}
