//=- SwplPlan.cpp - Classes as cheduling results in SWPL -*- C++ -*----------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Classes that interface with result reflection in SWPL.
//
//===----------------------------------------------------------------------===//

#include "SwplPlan.h"
#include "SWPipeliner.h"
#include "SwplCalclIterations.h"
#include "SwplRegEstimate.h"
#include "SwplScheduling.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Support/FormatVariadic.h"
#include <cmath>

using namespace llvm; // for NV
using namespace ore; // for NV

#define DEBUG_TYPE "aarch64-swpipeliner"

namespace llvm{

#define ceil_div(x, y) (((x) - 1) / (y) + 1)

bool SwplPlan::existsPragma = false;

/// \brief instが配置されたslot番号（begin_slot起点）を返す
/// \details SwplPlan.slotsに配置されたinstを探し、
///          begin_slot起点のslot番号を返す
/// \param [in] c_inst
/// \return instが配置されたslot番号（begin_slot起点）
unsigned SwplPlan::relativeInstSlot(const SwplInst& c_inst) const {
  return slots.getRelativeInstSlot(c_inst, begin_slot);
}

/// \brief 命令が配置された範囲のCycle数を返す
/// \return 命令が配置された範囲のCycle数
int SwplPlan::getTotalSlotCycles() {
  return (end_slot - begin_slot) / SWPipeliner::FetchBandwidth;
}

/// \brief SwplPlanをダンプする
/// \param [in] stream 出力stream
/// \return なし
void SwplPlan::dump(raw_ostream &stream) {
  stream << "(plan " << format("0x%p",this) <<":\n";
  stream << "  iteration_interval  = " << iteration_interval <<"\n";
  stream << "  n_iteration_copies  = " << n_iteration_copies << "\n";;
  stream << "  n_renaming_versions = " << n_renaming_versions <<"\n";
  stream << "  begin_slot = " << begin_slot << "\n";
  stream << "  end_slot   = " << end_slot <<"\n";
  stream << "  total_cycles  = " << total_cycles << "\n";
  stream << "  prolog_cycles = " << prolog_cycles << "\n";
  stream << "  kernel_cycles = " << kernel_cycles << "\n";
  stream << "  epilog_cycles = " << epilog_cycles << "\n";

  dumpInstTable(stream);
  stream << ")\n";
  return;
}

/// \brief SwplPlanの命令配置状態をダンプする
/// \detail 命令ごとにMachineInstr*とOpcode名を出力する。
/// \param [in] stream 出力stream
/// \return なし
void SwplPlan::dumpInstTable(raw_ostream &stream) {
  size_t table_size = (size_t)end_slot - (size_t)begin_slot;
  std::vector<SwplInst*> table(table_size, nullptr);

  for( auto inst : loop.getBodyInsts() ) {
    unsigned slot;
    slot = slots.getRelativeInstSlot(*inst, begin_slot);
    table[slot] = inst;
  }
  stream << "\n\t( MachineInstr* : OpcodeName )\n";

  for (unsigned i = 0; i < table_size; ++i) {
    SwplInst* inst;
    SwplSlot slot;

    slot = begin_slot + i;
    if(slot.calcFetchSlot() == 0) {
      stream << "\n";
    }
    if(i!=0 && i%(SWPipeliner::FetchBandwidth * iteration_interval) == 0 ) { // IIごとに改行
      stream << "\n";
    }

    inst = table[i];

    if (inst == nullptr) {
      stream << "\t--" << slot.calcFetchSlot() << "--\t";
    } else {
      stream << "\t(";
      stream << format("%p", inst->getMI());
      stream << ":" << inst->getName();
      stream << ")\t";
    }
  }
  stream << "\n";

  return;
}

/// \brief registerが足りているかを判定する
/// \details renaming_versionsでregisterが足りるかを判定する
/// \param [in] c_loop ループ情報
/// \param [in] slots スケジューリング結果
/// \param [in] iteration_interval iteration interval
/// \param [in] n_renaming_versions renaming versions
/// \retval true 足りる
/// \retval false 足りない
///
/// \note 固定割付けでないレジスタが、整数、浮動小数点数、プレディケートであることが前提の処理である。
/// \note CCRegは固定割付けのため処理しない。
bool SwplPlan::isSufficientWithRenamingVersions(const SwplLoop& c_loop,
                                                const SwplSlots& slots,
                                                unsigned iteration_interval,
                                                unsigned n_renaming_versions) {
  SwplMsResourceResult ms_resource_result;
  unsigned n_necessary_regs;

  ms_resource_result.init();

  n_necessary_regs = SwplRegEstimate::calcNumRegs(c_loop, &slots, iteration_interval,
                                                  llvm::StmRegKind::getIntRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryIreg(n_necessary_regs);

  n_necessary_regs = SwplRegEstimate::calcNumRegs(c_loop, &slots, iteration_interval,
                                                  llvm::StmRegKind::getFloatRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryFreg(n_necessary_regs);

  n_necessary_regs = SwplRegEstimate::calcNumRegs(c_loop, &slots, iteration_interval,
                                                  llvm::StmRegKind::getPredicateRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryPreg(n_necessary_regs);

  if ( !(slots.isIccFreeAtBoundary(c_loop, iteration_interval)) ) {
    ms_resource_result.setSufficientWithArg(false);
  }

  return ms_resource_result.isModerate();
}

/// \brief SwplPlanを取得し、スケジューリング結果からPlanの要素を設定する
/// \details SwplPlanを取得し、スケジューリング結果からSwplPlanの要素を設定する。
///          SwplPlanは、schedulingが完了した後, transform mirへ渡す情報となる。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] slots スケジューリング結果
/// \param [in] min_ii 計算されたMinII
/// \param [in] ii スケジュールで採用されたII
/// \param [in] resource スケジュールで利用する資源情報
/// \return スケジューリング結果を設定したSwplPlanを返す。
SwplPlan* SwplPlan::construct(const SwplLoop& c_loop,
                              SwplSlots& slots,
                              unsigned min_ii,
                              unsigned ii,
                              const SwplMsResourceResult& resource) {
  SwplPlan* plan = new SwplPlan(c_loop); //plan->loop =loop;
  size_t prolog_blocks, kernel_blocks;

  plan->minimum_iteration_interval = min_ii;
  plan->iteration_interval = ii;
  plan->slots = slots;

  prolog_blocks = slots.calcPrologBlocks(plan->loop, ii);
  kernel_blocks = slots.calcKernelBlocks(plan->loop, ii);
  plan->n_iteration_copies = prolog_blocks + kernel_blocks;
  plan->n_renaming_versions
    = slots.calcNRenamingVersions(plan->loop, ii);

  plan->begin_slot = slots.findBeginSlot(plan->loop, ii);
  plan->end_slot = SwplSlot::baseSlot(ii) + 1;

  plan->prolog_cycles = prolog_blocks * ii;
  plan->kernel_cycles = kernel_blocks * ii;
  plan->epilog_cycles = plan->prolog_cycles;
  plan->total_cycles = (plan->prolog_cycles
                        + plan->kernel_cycles
                        + plan->epilog_cycles);

  plan->num_necessary_freg = resource.getNecessaryFreg();
  plan->num_max_freg = resource.getMaxFreg();
  plan->num_necessary_ireg = resource.getNecessaryIreg();
  plan->num_max_ireg = resource.getMaxIreg();
  plan->num_necessary_preg = resource.getNecessaryPreg();
  plan->num_max_preg = resource.getMaxPreg();

  return plan;
}

/// \brief SwplPlanの解放
/// \param [in] plan SwplPlanオブジェクトのポインタ
/// \return なし
void SwplPlan::destroy(SwplPlan* plan) {
  delete plan;
  return;
}

/// \brief SwplDdgを元にschedulingを試行し,SwplPlanを生成する
/// \details selectPlan関数がTRY_SCHEDULE_SUCCESSで復帰した場合、
///          SwplPlan::construct関数の結果を返す。
/// \param [in] ddg データ依存情報
/// \retval schedule結果(SwplPlan)
/// \retval nullptr schedule結果が得られない場合
SwplPlan* SwplPlan::generatePlan(SwplDdg& ddg)
{
  SwplSlots slots;
  slots.resize(ddg.getLoopBody_ninsts());
  unsigned ii, min_ii, itr;
  SwplMsResourceResult resource;

  TryScheduleResult rslt =
    selectPlan(ddg,
               slots, &ii, &min_ii, &itr, resource);

  switch(rslt) {
  case TryScheduleResult::TRY_SCHEDULE_SUCCESS:
    return SwplPlan::construct( *(ddg.getLoop()), slots, min_ii, ii, resource);

  case TryScheduleResult::TRY_SCHEDULE_FAIL:
    return nullptr;

  case TryScheduleResult::TRY_SCHEDULE_FEW_ITER: {
    SWPipeliner::Reason =
        llvm::formatv("This loop is not software pipelined because the iteration count is smaller than "
                      "{0} and "
                      "the software pipelining does not improve the performance.", itr);
    return nullptr;
  }
  }
  llvm_unreachable("select_plan returned an unknown state.");
  return nullptr;
}


/// \brief StmのResource情報により制限されるmin IIを計算する
/// \details resource min IIは以下の手順で求める。
///          1. 演算器数分のカウンタを用意（0.0で初期化）
///          2. スケジューリング対象の命令ごとに以下を実施
///            - 命令が使用する資源パターンごとに以下を実施
///              - 利用する演算器のカウンタに"1/命令が使用する資源パターン数"を加算
///          3. 演算器ごとのカウンタの最大（小数点切り上げ）が
///             Resource情報により制限されるmin IIとなる
///
/// \param [in] c_loop ループを構成する命令の情報
/// \return 算出したresource_ii
///
/// \note LLVM版では非パイプライン情報は取得できない。
///       現実装では、非パイプライン命令もパイプライン命令と同様に扱って計算している
unsigned SwplPlan::calculateResourceII(const SwplLoop& c_loop) {
  int max_counter;
  unsigned numresource;

  // 資源ごとの利用カウントをresource_appearsで保持する。
  // resource_appearsは、STM->getNumResource()+1分確保し、
  // resource_appears[A64FXRes::PortKind]が資源ごとの利用カウントとなる。
  // 要素[0]はA64FXRes::PortKind::P_NULLに該当する要素とし、使用しない。
  numresource = SWPipeliner::STM->getNumResource();
  std::vector<float> resource_appears(numresource+1, 0.0);

  for (auto inst : c_loop.getBodyInsts()) {
    // 資源情報の生成と資源パターン数の取得
    const auto *pipes = SWPipeliner::STM->getPipelines( *(inst->getMI()) );
    unsigned num_pattern = pipes->size();

    for(auto pipeline : *pipes ) {
      for(unsigned i=0; i<pipeline->resources.size(); i++) {
        StmResourceId resource = pipeline->resources[i];

        // どの資源パターンが使用されるかはわからないため、
        // n_appearsには"1.0/資源利用のパターン数"の値を足しこむ
        resource_appears[resource] += (1.0 / num_pattern);
      }
    }
  }
  max_counter = 0;
  for(auto val : resource_appears ) {
    int count = std::ceil(val); // 小数点切り上げ
    max_counter = std::max( max_counter, count );
  }
  return max_counter;
}

/// \brief 資源により制限されるmin IIを計算する
/// \details 以下から求めたIIの最大を返す。
///           - 1cycleに単位のfetch slot数の制約から求めたmin II
///           - TmのResource情報により制限されるmin II
///
/// \param [in] c_loop ループを構成する命令の情報
/// \return ResII
unsigned SwplPlan::calcResourceMinIterationInterval(const SwplLoop& c_loop) {
  unsigned fetch_constrained_ii;
  size_t n_body_insts;
  unsigned res_ii;

  n_body_insts = c_loop.getSizeBodyRealInsts();
  assert (n_body_insts != 0);

  /* 1cycleにつきfetch slot数しか命令は発行できない事による制約 */
  fetch_constrained_ii = ceil_div(n_body_insts, SWPipeliner::RealFetchBandwidth );

  // メモリポート数による制約は、
  // calculateResorceIIで計算されるmemory unitの制約の方が厳しい為、
  // 考慮しない。
  // Tradコードにも「実質的には不要な処理」とコメントがある。
  res_ii = calculateResourceII(c_loop);
  return std::max(fetch_constrained_ii, res_ii);
}

/// \brief 指定のアルゴリズムでスケジュールを試行する
/// \details スケジューリング指示情報(PLAN_SPEC)を生成し、
///          calculateMsResultによるスケジューリングを依頼する。
///
/// \param [in] c_ddg 対象の DDG
/// \param [in] res_mii ddg に対して計算された ResMII
/// \param [out] slots スケジュールしたスロットの動的配列
/// \param [out] selected_ii スケジュールで採用された II
/// \param [out] calculated_min_ii 計算された MinII
/// \param [out] required_itr ソフトパイプルートを通るのに必要なイテレート数
/// \param [out] resource スケジュールで利用する資源情報
/// \retval TRY_SCHEDULE_SUCCESS スケジュール成功。全ての出力引数を利用できる。
/// \retval TRY_SCHEDULE_FAIL スケジュール失敗。全ての出力引数は未定義。
/// \retval TRY_SCHEDULE_FEW_ITER ループのイテレート数不足によりソフトパイプ適用不可。
///         required_itr のみ利用できる。
TryScheduleResult SwplPlan::trySchedule(const SwplDdg& c_ddg,
                                        unsigned res_mii,
                                        SwplSlots** slots,
                                        unsigned* selected_ii,
                                        unsigned* calculated_min_ii,
                                        unsigned* required_itr,
                                        SwplMsResourceResult* resource) {
  SwplPlanSpec spec(c_ddg);
  if( !(spec.init(res_mii, existsPragma)) ){
    // SwplPlanSpec::init()復帰値は現在trueのみ。将来的にfalseがくる可能性を考えifを残す
    return TryScheduleResult::TRY_SCHEDULE_FAIL;
  }

  *calculated_min_ii = spec.min_ii;

  if( !(SwplCalclIterations::preCheckIterationCount(const_cast<const SwplPlanSpec &>(spec), required_itr)) ) {
    return TryScheduleResult::TRY_SCHEDULE_FEW_ITER;
  }

  SwplMsResult *ms_result = SwplMsResult::calculateMsResult(spec);

  if (ms_result != nullptr && ms_result->slots != nullptr) {
    *slots = ms_result->slots;
    *selected_ii = ms_result->ii;
    *resource = ms_result->resource;
    delete ms_result;
    return TryScheduleResult::TRY_SCHEDULE_SUCCESS;
  } else {
    return TryScheduleResult::TRY_SCHEDULE_FAIL;
  }

}

/// \brief IMSでスケジューリングを行う
///
/// \param [in] c_ddg 対象の DDG
/// \param [out] rslt_slots スケジュール結果であるスロットの動的配列
/// \param [out] selected_ii スケジュールで採用された II
/// \param [out] calculated_min_ii 計算された MinII
/// \param [out] required_itr ソフトパイプルートを通るのに必要なイテレート数
/// \param [out] resource スケジュールで利用する資源情報
/// \retval TRY_SCHEDULE_SUCCESS スケジュール成功。全ての出力引数を利用できる。
/// \retval TRY_SCHEDULE_FAIL スケジュール失敗。全ての出力引数は未定義。
/// \retval TRY_SCHEDULE_FEW_ITER ループのイテレート数不足によりソフトパイプ適用不可。
///                               required_itr のみ利用できる。
TryScheduleResult SwplPlan::selectPlan(const SwplDdg& c_ddg,
                                       SwplSlots& rslt_slots,
                                       unsigned* selected_ii,
                                       unsigned* calculated_min_ii,
                                       unsigned* required_itr,
                                       SwplMsResourceResult& resource) {
  unsigned res_mii = calcResourceMinIterationInterval( c_ddg.getLoop() );

  SwplSlots* slots_tmp;
  unsigned ii_tmp, min_ii_tmp, itr;
  SwplMsResourceResult resource_tmp;

  switch(trySchedule(c_ddg,
                     res_mii,
                     &slots_tmp,
                     &ii_tmp,
                     &min_ii_tmp,
                     &itr,
                     &resource_tmp)) {
  case TryScheduleResult::TRY_SCHEDULE_SUCCESS:
    rslt_slots = *slots_tmp; // copy
    delete slots_tmp;
    *selected_ii = ii_tmp;
    *calculated_min_ii = min_ii_tmp;
    *required_itr = itr;
    resource = resource_tmp; // copy
    return TryScheduleResult::TRY_SCHEDULE_SUCCESS;
  case TryScheduleResult::TRY_SCHEDULE_FAIL:
    return TryScheduleResult::TRY_SCHEDULE_FAIL;
  case TryScheduleResult::TRY_SCHEDULE_FEW_ITER:
    *required_itr = itr;
    return TryScheduleResult::TRY_SCHEDULE_FEW_ITER;
  }
  llvm_unreachable("unknown TryScheduleResult");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief プロローグを構成するblock数を返す
/// \details プロローグを構成するblock数を返す。
///          block数は、IIで区切られたサイクルの数。ステージ数と同意。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return プロローグを構成するblock数
size_t SwplSlots::calcPrologBlocks(const SwplLoop& c_loop,
                                                 unsigned iteration_interval) {
  return (calcFlatScheduleBlocks(c_loop, iteration_interval) - 1);
}

/// \brief slotsにて、命令が配置されている最初のSlot番号を返す
/// \details first_slotsにて、命令が配置されている最初のSlot番号を返す。
/// \param [in] c_loop ループを構成する命令の情報
/// \return 検出したスロット番号
///
/// \note c_loop.BodyInstsの命令がすべて配置済みでなければならない。
SwplSlot SwplSlots::findFirstSlot(const SwplLoop& c_loop) {
  SwplSlot first_slot = SwplSlot::slotMax();

  for (auto &slot : *this) {
    if (slot == SwplSlot::UNCONFIGURED_SLOT) continue;
    first_slot = (first_slot < slot) ? first_slot : slot;
  }
  return first_slot;
}

/// \brief last_slotsにて、命令が配置されている最後のSlot番号を返す
/// \details slotsにて、命令が配置されている最後のSlot番号を返す。
/// \param [in] c_loop ループを構成する命令の情報
/// \return 検出したスロット番号
///
/// \note c_loop.BodyInstsの命令がすべて配置済みでなければならない。
SwplSlot SwplSlots::findLastSlot(const SwplLoop& c_loop) {
  SwplSlot last_slot = SwplSlot::slotMin();

  for (auto &slot : *this) {
    if (slot == SwplSlot::UNCONFIGURED_SLOT) continue;
    last_slot = (last_slot > slot) ? last_slot : slot;
  }
  return last_slot;
}

/// \brief calcFlatScheduleBlocks
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return スケジューリング結果のunroll前のブロック数
size_t SwplSlots::calcFlatScheduleBlocks(const SwplLoop& c_loop,
                                                   unsigned iteration_interval) {
  SwplSlot first_slot, last_slot;
  first_slot = findFirstSlot(c_loop);
  last_slot = findLastSlot(c_loop);
  return (last_slot.calcBlock(iteration_interval) + 1
          - first_slot.calcBlock(iteration_interval));
}

/// \brief カーネルがunrollされる数(renaming version)を返す
/// \details カーネルがunrollされる数(renaming version)を返す
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return renaming version
size_t SwplSlots::calcKernelBlocks(const SwplLoop& c_loop,
                                                 unsigned iteration_interval) {
  size_t n_renaming_versions;

  n_renaming_versions = calcNRenamingVersions(c_loop, iteration_interval);
  return n_renaming_versions;
}

/// \brief instが配置されたslot番号（begin_slot起点）を返す
/// \param [in] c_inst
/// \param [in] begin_slot
/// \return instが配置されたslot番号（begin_slot起点）
unsigned SwplSlots::getRelativeInstSlot(const SwplInst& c_inst,
                                                  const SwplSlot& begin_slot) const {
  assert(this->at(c_inst.inst_ix) != SwplSlot::UNCONFIGURED_SLOT);
  return this->at(c_inst.inst_ix) - begin_slot;
}

/// \brief registerのlive rangeとiiの関係からrenaming versionを求める
/// \details  body で定義されていて、predecessor が無い regについて live_range を計算し、
///           最大値を求める。
///           live rangeが跨るblockの数だけ、regのrenamingを行なう必要がある。
///           現状はn_renaming_versionsがkernelに重なる回転数と一致する仕組みとなっている。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return renaming versions数
size_t SwplSlots::calcNRenamingVersions(const SwplLoop& c_loop,
                                                  unsigned iteration_interval) const {
  size_t max_live_cycles;
  size_t necessary_n_renaming_versions;

  max_live_cycles = 1;
  for (auto* def_inst : c_loop.getBodyInsts()) {
    SwplSlot def_slot;
    size_t def_cycle;

    assert(this->at(def_inst->inst_ix) != SwplSlot::UNCONFIGURED_SLOT);
    def_slot = this->at(def_inst->inst_ix);
    def_cycle = def_slot.calcCycle();
    for( auto reg : def_inst->getDefRegs() ) {
      size_t live_cycles, last_use_cycle;

      if ( reg->isRegNull() ) {
        continue;
      }
      if ( reg->getPredecessor() != nullptr) {
        /* 既に割り当てられた reg を使うだけなので、計算しない。*/
        continue;
      }
      if ( !reg->isUsed() ) {
        continue;
      }
      last_use_cycle = calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      assert (def_cycle <= last_use_cycle);

      live_cycles = last_use_cycle - def_cycle + 1UL;
      assert (live_cycles >= 1);

      max_live_cycles = std::max(max_live_cycles, live_cycles);
    }
  }
  necessary_n_renaming_versions = ceil_div(max_live_cycles, iteration_interval);
  assert (necessary_n_renaming_versions >= 1);
  return necessary_n_renaming_versions;
}

/// \brief 最初のInstが配置されているblockの先頭Slotを返す
/// \details 最初のInstが配置されているblockの先頭Slotを返す。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return 最初のInstが配置されているblockの先頭Slot番号
SwplSlot SwplSlots::findBeginSlot(const SwplLoop& c_loop,
                                unsigned iteration_interval) {
  SwplSlot first_slot;
  unsigned begin_block;

  first_slot = findFirstSlot(c_loop);
  begin_block = first_slot.calcBlock(iteration_interval);
  return SwplSlot::constructFromBlock(begin_block, iteration_interval);
}

/// \brief icc が iteration_interval のブロックを跨って使用されていないかをチェックする
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \retval true ブロックを跨って使用されていない
/// \retval false ブロックを跨って使用されている
bool SwplSlots::isIccFreeAtBoundary(const SwplLoop& loop,
                                             unsigned iteration_interval) const {
  for( auto inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_block;

    if ((def_slot = this->at(inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
      report_fatal_error("inst not found in inst_slot_map.");
    }
    def_block = def_slot.calcBlock(iteration_interval);

    for( auto reg : inst->getDefRegs() ) {
      unsigned last_use_cycle, last_use_block;

      if( reg->isRegNull() ) {
        continue;
      }
      if( !reg->isIntegerCCRegister() ) {
        continue;
      }
      if( !reg->isUsed() ) {
        continue;
      }
      last_use_cycle = calcLastUseCycleInBody(*reg, iteration_interval);

      last_use_block = last_use_cycle / iteration_interval;
      assert (last_use_block >= def_block);
      if (last_use_block != def_block) { /* F1686 */
        return false;
      }
    }
  }
  return true;
}

/// \brief reg が最後に使われる cycle を返す
/// \details reg が最後に使われる cycle を返す。
///          regがsuccessorを持つ場合は遡って調べる。
/// \param [in] reg 調査対象のreg
/// \param [in] iteration_interval II
/// \return regが最後に使われる cycle
unsigned SwplSlots::calcLastUseCycleInBodyWithInheritance(const SwplReg& reg,
                                                                    unsigned iteration_interval) const {
  const SwplReg* successor_reg;

  successor_reg = reg.getSuccessor();
  if (successor_reg != nullptr) {
    return calcLastUseCycleInBodyWithInheritance (*successor_reg,
                                                  iteration_interval);
  }
  return calcLastUseCycleInBody(reg, iteration_interval);
}

/// \brief reg が最後に使われる slot を返す
/// \details phi で使われる場合は、次のイタレーションで最後に使われる slot に
///          iteration_interval を足して、比べる。
/// \param [in] reg 調査対象のreg
/// \param [in] iteration_interval II
/// \return regが最後に使われる cycle
unsigned SwplSlots::calcLastUseCycleInBody(const SwplReg& reg,
                                                     unsigned iteration_interval) const {
  unsigned last_use_cycle;
  const SwplInst* def_inst;

  /* last_use_cycle の初期値を設定。 */
  def_inst = reg.getDefInst();
  if (def_inst->isBodyInst()) {
    SwplSlot def_slot;

    if ((def_slot = this->at(def_inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
      report_fatal_error("instruction not found in InstSlotHashmap.");
    }
    last_use_cycle = def_slot.calcCycle();
  } else {
    last_use_cycle = SwplSlot::slotMin().calcCycle();
  }

  for (auto use_inst : reg.getUseInsts() ) {
    unsigned use_cycle;

    if( use_inst->isPhi()) {
      unsigned next_use_cycle;

      assert( !((reg.getDefInst())->isPhi()) );
      SwplReg& next_reg = use_inst->getDefRegs(0);
      assert ( !reg.isRegNull() );
      next_use_cycle = calcLastUseCycleInBodyWithInheritance(next_reg,
                                                             iteration_interval);
      use_cycle = next_use_cycle + iteration_interval;
    } else if( use_inst->isBodyInst() ) {
      SwplSlot use_slot;

      if ((use_slot = this->at(use_inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
        report_fatal_error("instruction not found in Slots.");
      }
      use_cycle = use_slot.calcCycle();
    } else {
      llvm_unreachable("unexpected use_inst.");
      continue;
    }
    last_use_cycle = std::max(last_use_cycle, use_cycle);
  }
  return last_use_cycle;
}

/// \brief slots中の最大最小slotを求める。
/// \param [out] max_slot 最大slot
/// \param [out] max_slot 最小slot
/// \return なし
void SwplSlots::getMaxMinSlot(SwplSlot& max_slot, SwplSlot& min_slot) {
  assert(this->size()!=0);
  SwplSlot max=0;
  SwplSlot min=SwplSlot::slotMax();

  for (auto& mp : *this) {
    if (mp == SwplSlot::UNCONFIGURED_SLOT) continue;
    if (max < mp) max=mp;
    if (min > mp) min=mp;
  }
  max_slot = max;
  min_slot = min;
  return;
}

/// \brief cycleにおける空きslotを返す
/// \param [in] cycle 検索対象のcycle
/// \param [in] iteration_interval II
/// \param [in] isvirtual 仮想命令用の空きSlotを探すのであればtrue
/// \return cycleのうち、命令が配置されていないslot
SwplSlot SwplSlots::getEmptySlotInCycle( unsigned cycle,
                                                   unsigned iteration_interval,
                                                   bool isvirtual ) {
  // InstSlotHashmapを２次元配列で表したイメージは以下のとおり。
  // Real命令と仮想命令によって、配置可能なSlotが異なる
  //
  //         |----仮想命令用slot-----|----Real命令用slot----|
  // cycle 1  slot  slot  slot  slot  slot  slot  slot  slot
  // cycle 2  slot  slot  slot  slot  slot  slot  slot  slot
  //       :  slot  slot  slot  slot  slot  slot  slot  slot
  // cycle n  slot  slot  slot  slot  slot  slot  slot  slot
  //         |<-------------------------------------------->| SWPipeliner::FetchBandwidth
  //                                 |<-------------------->| SWPipeliner::RealFetchBandwidth

  unsigned bandwidth = SWPipeliner::FetchBandwidth;
  unsigned realbandwidth = SWPipeliner::RealFetchBandwidth;

  std::vector<bool> openslot(bandwidth);
  for(unsigned i=0; i<bandwidth; i++) {
    // isvirtualがfalseの場合は、opnslotの初期値を以下のようにする。
    // ・0～(bandwidth - realbandwidth - 1)の要素をfalse
    // ・(bandwidth - realbandwidth)～(bandwidth-1)の要素をtrue
    // isvirtualがtrueの場合は逆
    openslot[i] = (i<(bandwidth - realbandwidth)) ? isvirtual : !isvirtual;
  }

  unsigned target_modulo_cycle = cycle % iteration_interval;

  for (auto& mp : *this) {
    if (mp == SwplSlot::UNCONFIGURED_SLOT) continue;
    unsigned modulo_cycle = mp.calcCycle() % iteration_interval;
    if(target_modulo_cycle == modulo_cycle) {
      openslot[mp.calcFetchSlot()] = false; // 使用済み
    }
  }
  for( unsigned idx=0; idx<bandwidth; idx++) {
    if( openslot[idx] == true ) {
      return SwplSlot::construct(cycle, 0) + idx;
    }
  }
  return SWPL_ILLEGAL_SLOT;
}

/// \brief デバッグ用ダンプ
void SwplSlots::dump(const SwplLoop& c_loop) {
  if( this->size() == 0 ) {
      dbgs() << "Nothing...\n";
      return;
  }

  SwplSlot max_slot, min_slot;
  getMaxMinSlot(max_slot, min_slot);

  unsigned max_cycle, min_cycle;
  max_cycle = max_slot.calcCycle();
  min_cycle = min_slot.calcCycle();

  max_slot = SwplSlot::construct(max_cycle, 0) + SWPipeliner::FetchBandwidth;
  min_slot = SwplSlot::construct(min_cycle, 0);

  size_t table_size = (size_t)max_slot - (size_t)min_slot;
  std::vector<SwplInst*> table(table_size, nullptr);
  int ix = 0;

  for(auto* inst : c_loop.getBodyInsts()) {
    if (this->at(ix) != SwplSlot::UNCONFIGURED_SLOT)
      table[(this->at(ix) - min_slot)] = inst;
    ix++;
  }

  for (unsigned i = 0; i < table_size; ++i) {
    SwplInst* inst;
    SwplSlot slot;

    slot = min_slot + i;
    if (slot.calcFetchSlot() == 0) {
      dbgs() << "\n\t(" << slot << ")";
    }

    inst = table[i];

    dbgs() << "\t";
    if (inst == nullptr) {
      dbgs() << "--" << slot.calcFetchSlot() << "--\t";
    } else {
      dbgs() << inst->getName() << "\t" ;
    }
  }
  dbgs() << "\n";
  return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief slotのcycleを返す
/// \details cycleに換算した値を返す
/// \return slotに該当するcycle
unsigned SwplSlot::calcCycle() {
  return slot_index / SWPipeliner::FetchBandwidth;
}

/// \brief slotのFetchSlotを返す
/// \details FetchSlot = 各cycleの何番目のslotか
/// \return FetchSlot
unsigned SwplSlot::calcFetchSlot() {
  return slot_index % SWPipeliner::FetchBandwidth;
}

/// \brief slotが何番目のblockであるかを返す
/// \param [in] iteration_interval II
/// \return 何番目のblockか
unsigned SwplSlot::calcBlock(unsigned iteration_interval)
{
  return calcCycle() / iteration_interval;
}

/// \brief base slotを返す
/// \param [in] iteration_interval II
/// \return base slot
SwplSlot SwplSlot::baseSlot(unsigned iteration_interval) {
  unsigned middle_block;
  SwplSlot middle_slot;

  middle_slot = SwplSlot::slotMax() / 2 + SwplSlot::slotMin () / 2;
  middle_block = middle_slot.calcBlock(iteration_interval);
  return SwplSlot::constructFromBlock(middle_block, iteration_interval) - 1;
}

/// \brief SwplSlotを生成する
/// \param [in] cycle cycle
/// \param [in] fetch_slot fetch slot
/// \return SwplSlot
SwplSlot SwplSlot::construct(unsigned cycle, unsigned fetch_slot) {
  if( ( SwplSlot::slotMin().calcCycle() > cycle ) ||
      ( cycle > SwplSlot::slotMax().calcCycle() ) ||
      ( fetch_slot >= SWPipeliner::FetchBandwidth )
       ) {
    if (SWPipeliner::isDebugOutput()) {
      dbgs() << "!swp-msg: Used cycle for scheduling is out of the range.\n";
    }
    return SWPL_ILLEGAL_SLOT;
  }
  return cycle * SWPipeliner::FetchBandwidth + fetch_slot;
}

/// \brief block番号からSwplSlotを生成する
/// \param [in] block block
/// \param [in] iteration_interval II
/// \return SwplSlot
SwplSlot SwplSlot::constructFromBlock(unsigned block, unsigned iteration_interval) {
  return SwplSlot::construct(block * iteration_interval, 0);
}

/// \brief slotの最大を返す
/// \return 最大slot
SwplSlot SwplSlot::slotMax() { return 1500000; }

/// \brief slotの最小を返す
/// \return 最小slot
SwplSlot SwplSlot::slotMin() { return  500000; }
}
