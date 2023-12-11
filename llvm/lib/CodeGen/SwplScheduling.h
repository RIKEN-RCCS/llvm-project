//=- SwplScheduling.h - Scheduling process in SWPL -*- C++ -*----------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Classes that processing scheduling in SWPL.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_SWPLSCHEDULING_H
#define LLVM_LIB_CODEGEN_SWPLSCHEDULING_H

#include "SWPipeliner.h"
#include "SwplPlan.h"
#include <set>
#include <unordered_set>


namespace llvm{

using SwplInstIntMap = llvm::DenseMap<const SwplInst*, int>;
using SwplInstPrioque = std::map<int, const SwplInst*>;
using SwplInstSet = std::set<const SwplInst*>;


/// \brief MRTを表現
/// \details MRTを表現する。
///          MRTは、ResourceIDが要素番号として扱えない場合でも対応可能とするため、
///          ResourceIDとSwplInst*のmapのvectorで表現する。
///          以下のようなイメージである。(ID=ResourceID, instn=SwplInst*)
///          cycle1: { ID1:inst1, ID2:inst2,            ... } ↑
///          cycle2: { ID1:inst1,            ID3:inst3, ... } ｜
///          cycle3: {            ID2:inst1, ID3:inst4, ... } II
///          cycle4: {            ID2:inst4,            ... } ｜
///          cycle5: { ID1:inst4,                       ... } ↓
///
class SwplMrt {
  unsigned iteration_interval; ///< II
  std::vector<std::map<StmResourceId, const SwplInst*>*> table; //< Mrt

public:
  SwplMrt(unsigned ii) : iteration_interval(ii) {} ///< constructor

  void reserveResourcesForInst(unsigned cycle, const SwplInst& inst, const StmPipeline & pipeline );
  SwplInstSet* findBlockingInsts(unsigned cycle, const SwplInst& inst, const StmPipeline & pipeline );
  bool isOpenForInst(unsigned cycle, const SwplInst& inst, const StmPipeline & pipeline);
  void cancelResourcesForInst(const SwplInst& inst);
  void dump(const SwplInstSlotHashmap& inst_slot_map, raw_ostream &stream);
  void dump();

  static SwplMrt* construct(unsigned iteration_interval);
  static void destroy(SwplMrt* mrt);

private:
  unsigned getMaxOpcodeNameLength();
  void printInstMI(raw_ostream &stream,
                    const SwplInst* inst, unsigned width);
  void printInstRotation(raw_ostream &stream,
                         const SwplInstSlotHashmap& inst_slot_map,
                         const SwplInst* inst, unsigned ii);
};


/// \brief スケジューリング機能において依存関係を保持する
class SwplModuloDdg {
  const SwplInstGraph& graph; ///< 依存グラフ
  const SwplLoop& loop;       ///< loop情報
  unsigned iterator_interval; ///< initiation interval
  SwplInstEdge2ModuloDelay* modulo_delay_map; ///< module ddg

public:
  SwplModuloDdg(const SwplInstGraph& c_graph,
                const SwplLoop& c_loop) : graph(c_graph),loop(c_loop) {} ///< constructor

  unsigned getIterationInterval() const { return iterator_interval; } ///< getter

  SwplInstIntMap* computePriorities() const;
  int getDelay(const SwplInst& c_former_isnt, const SwplInst& c_latter_inst) const;
  llvm::DebugLoc getLoopStartLoc() const;
  const SwplInsts& getSuccessors(const SwplInst& c_inst) const;
  const SwplInsts& getPredecessors(const SwplInst& c_inst) const;
  void dump();

  static SwplModuloDdg* construct(const SwplDdg& c_ddg, unsigned iterator_interval);
  static void destroy(SwplModuloDdg* modulo_ddg);
};


/// \brief スケジューリング情報および結果を保持する
class SwplTrialState {
  /// \brief instが配置できるslot, inst, 配置可能となったpipeline,の組を保持するクラス
  class SlotInstPipeline {
  public:
    SwplSlot slot;          ///< instが配置できるslot
    SwplInst *inst;         ///< 命令
    StmPipeline * pipeline; ///< 配置可能となったpipeline

    SlotInstPipeline(SwplSlot s, SwplInst* i, StmPipeline * p) : slot(s), inst(i), pipeline(p) {} ///< constructor
  };
  const SwplModuloDdg & modulo_ddg;        ///< モジュロスケジューリング用Ddg
  unsigned iteration_interval;             ///< Iteration Interval（Initiation Interval）
  SwplInstIntMap* priorities;              ///< SwplInstと優先度(priority)の組のMap
  SwplInstPrioque* inst_queue;             ///< 未配置命令のキュー
  SwplInstSlotHashmap* inst_slot_map;      ///< 配置したSwplInstとSlot番号の組のHashmap
  SwplInstSlotHashmap* inst_last_slot_map; ///< 前回配置の状態を記録するinst_slot_mao
  SwplMrt* mrt;                            ///< MRT。{resourceID, SwplInst}のmapのVector

public:
  SwplTrialState(const SwplModuloDdg& c_modulo_ddg) : modulo_ddg(c_modulo_ddg) {} ///< constructor

  SwplInstSlotHashmap* getInstSlotMap() { return inst_slot_map; } ///< getter
  SwplMrt* getMrt() { return mrt; } ///< getter

  bool tryNext();
  bool isCompleted();
  void dump(raw_ostream &stream);

  static SwplTrialState* construct(const SwplModuloDdg& c_modulo_ddg);
  static void destroy(SwplTrialState* state);

private:
  const SwplInst& getInstToBeScheduled() const;
  unsigned calculateLatestCycle(const SwplInst& inst);
  void unsetResourceConstrainedInsts(SlotInstPipeline& SIP);
  void unsetDependenceConstrainedInsts(SlotInstPipeline& SIP);
  void setInst(SlotInstPipeline& SIP);
  void unsetInst(const SwplInst& inst);
  int instPriority(const SwplInst* inst);
  llvm::DebugLoc getLoopStartLoc();
  SlotInstPipeline chooseSlot(unsigned begin_cycle, unsigned end_cycle, const SwplInst& inst);
  void scheduleInst(SlotInstPipeline& SIP);
  unsigned getNewScheduleCycle(const SwplInst& inst);
  SlotInstPipeline latestValidSlot(const SwplInst& inst, unsigned cycle);
};


/// \brief スケジューリング対象およびスケジューリングを制御するための情報を保持
class SwplPlanSpec {
  /// \brief Iterative Modulo Scheduling で使用するスケジュールする II に依存しない情報
  class ImsBaseInfo {
  public:
    unsigned budget; ///< iterative_scheduleの繰返し上限
  };

public:
  static const int ASSUMED_ITERATIONS_MAX = 32767;
  static const int ASSUMED_ITERATIONS_NONE = -1;

  const SwplDdg& ddg;          ///< 依存情報
  const SwplLoop& loop;        ///< ループ情報
  unsigned n_insts;            ///< スケジューリング対象の命令数
  unsigned itr_count;          ///< 回転数に関する条件
  bool is_itr_count_constant;  ///< 元ループの回転数が既知か
  unsigned original_cycle;     ///< 元ループの見積もりcycle
  unsigned int pre_expand_num; ///< SWPL入力時点で、ループ展開された数。現在、常に1
  int assumed_iterations;      ///< ループ回転数の見積り値。現在、常に-1（見積りがない）
  unsigned res_mii;            ///< Resource Min II
  unsigned min_ii;             ///< initiative/iterative intervalの下限
  unsigned max_ii;             ///< initiative/iterative intervalの上限
  unsigned unable_max_ii;      ///< schedulingを失敗した最大のii
  unsigned n_fillable_float_invariants; ///< 浮動小数点のループ不変の仮想レジのうち、フィルしても ResII を増加させない最大数
  ImsBaseInfo ims_base_info;

  SwplPlanSpec(const SwplDdg& c_ddg) : ddg(c_ddg),loop(c_ddg.getLoop()) {} ///< constructor

  bool init(unsigned res_mii, bool &existsPragma);
  void countLoadStore(unsigned *num_load, unsigned *num_store, unsigned *num_float) const;

private:
  static bool isIterationCountConstant(const SwplLoop& c_loop, unsigned* iteration_count);
  static unsigned getBudget(unsigned n_insts);
  static unsigned int getInitiationInterval(const SwplLoop& loop, bool& exists);
};


/// \brief schedulingに対するresourceの過不足情報を保持する
class SwplMsResourceResult {
  bool is_resource_sufficient=false; ///< レジスタ資源が十分か
  unsigned num_necessary_ireg=0; ///< スケジューリング結果から算出した必要な整数レジスタ
  unsigned num_necessary_freg=0; ///< スケジューリング結果から算出した必要な浮動小数点数レジスタ
  unsigned num_necessary_preg=0; ///< スケジューリング結果から算出した必要なプレディケートレジスタ
  unsigned num_max_ireg=0;       ///< 整数レジスタの最大数(ローカルオプションによる調整込み)
  unsigned num_max_freg=0;       ///< 浮動小数点数レジスタの最大数(ローカルオプションによる調整込み)
  unsigned num_max_preg=0;       ///< プレディケートレジスタの最大数(ローカルオプションによる調整込み)

  unsigned loose_margin_ireg = 3; ///< moderate判定用
  unsigned loose_margin_freg = 3; ///< moderate判定用
  unsigned loose_margin_preg = 1; ///< moderate判定用

public:
  void setSufficientWithArg(bool sufficient) { is_resource_sufficient = sufficient; return; } ///< setter

  bool isSufficient() const { return is_resource_sufficient; } ///< getter
  unsigned getNecessaryIreg() const { return num_necessary_ireg; } ///< getter
  unsigned getNecessaryFreg() const { return num_necessary_freg; } ///< getter
  unsigned getNecessaryPreg() const { return num_necessary_preg; } ///< getter
  unsigned getMaxIreg() const { return num_max_ireg; } ///< getter
  unsigned getMaxFreg() const { return num_max_freg; } ///< getter
  unsigned getMaxPreg() const { return num_max_preg; } ///< getter

  void init();
  bool isIregSufficient();
  bool isFregSufficient();
  bool isPregSufficient();
  void setNecessaryIreg(unsigned ireg);
  void setNecessaryFreg(unsigned freg);
  void setNecessaryPreg(unsigned preg);
  unsigned getNumInsufficientIreg();
  unsigned getNumInsufficientFreg();
  unsigned getNumInsufficientPreg();
  bool isModerate();

private:
  void setSufficient();
};


/// \brief iiとscheduling結果をまとめて扱う
class SwplMsResult {
public:
  /// 処理状態Policyを表すenum
  enum class ProcState {
                        ESTIMATE,           ///< estimate
                        RECOLLECT_EFFECTIVE,///< recollectEffective
                        RECOLLECT_MODERATE, ///< recollectModerate
                        BINARY_SEARCH,      ///< binary search
                        SIMPLY_SEARCH,      ///< simply search
  };
  SwplInstSlotHashmap* inst_slot_map; ///< スケジューリング結果を保持するHashmap
  SwplMsResourceResult resource; ///< resourceの過不足情報
  unsigned ii;                   ///< initiation interval
  unsigned tried_n_insts;        ///< スケジューリングされた命令数
  unsigned n_insufficient_iregs; ///< 不足するireg数
  unsigned n_insufficient_fregs; ///< 不足するfreg数
  unsigned n_insufficient_pregs; ///< 不足するpreg数
  unsigned required_itr_count;   ///< 必要な回転数
  unsigned required_mve;         ///< 必要なMVE展開数
  bool is_reg_sufficient;        ///< レジスタ資源が足りているか
  bool is_itr_sufficient;        ///< 回転数が足りているか
  bool is_mve_appropriate;       ///< MVE展開数が上限以内か
  double evaluation;             ///< スケジューリング結果の評価値
  ProcState proc_state;          ///< MsResultの処理状態（ex. binary_search中、など）

  bool constructInstSlotMapAtSpecifiedII(SwplPlanSpec spec);
  void checkInstSlotMap(const SwplPlanSpec & spec, bool limit_reg=true); // limit_reg is true at default.
  bool isModerate();
  bool isEffective();

  static SwplMsResult * constructInit(unsigned ii, ProcState procstate);
  static SwplMsResult * calculateMsResult(SwplPlanSpec spec);

private:
  void checkHardResource(const SwplPlanSpec & spec, bool limit_reg);
  SwplMsResourceResult isHardRegsSufficient(const SwplPlanSpec & spec);
  void evaluateSpillingSchedule(const SwplPlanSpec & spec, unsigned kernel_blocks);
  void checkIterationCount(const SwplPlanSpec & spec);
  void checkMve(const SwplPlanSpec & spec);
  unsigned getIncII(SwplPlanSpec & spec, unsigned prev_tried_n_insts);
  void outputDebugMessageForSchedulingResult(SwplPlanSpec spec);
  void outputGiveupMessageForEstimate(SwplPlanSpec & spec);
  static SwplMsResult * calculateMsResultByBinarySearch(SwplPlanSpec spec);
  static SwplMsResult * calculateMsResultAtSpecifiedII(SwplPlanSpec spec, unsigned ii, ProcState procstate);
  static SwplMsResult * calculateMsResultSimply(SwplPlanSpec spec);
  static bool collectCandidate(SwplPlanSpec & spec,
                               std::unordered_set<SwplMsResult *>& ms_result_candidate,
                               bool watch_regs,
                               ProcState procstate);
  static bool collectModerateCandidate(
      SwplPlanSpec & spec,
                                       std::unordered_set<SwplMsResult *>& ms_result_candidate,
                                       ProcState procstate);
  static bool recollectModerateCandidateWithExII(
      SwplPlanSpec & spec,
                                                 std::unordered_set<SwplMsResult *>& ms_result_candidate,
      SwplMsResult & ms_result);
  static bool recollectCandidateWithExII(
      SwplPlanSpec & spec,
                                         std::unordered_set<SwplMsResult *>& ms_result_candidate);
  static bool isRegReducible(SwplPlanSpec & spec, std::unordered_set<SwplMsResult *>& ms_result_candidate);
  static void getBinarySearchRange(const SwplPlanSpec & spec,
                                   std::unordered_set<SwplMsResult *>& ms_result_candidate,
                                   unsigned *able_min_ii, unsigned *unable_max_ii );
  static SwplMsResult * getEffectiveSchedule(const SwplPlanSpec & spec,
                                        std::unordered_set<SwplMsResult *>& ms_result_candidate);
  static bool isAnyScheduleItrSufficient(std::unordered_set<SwplMsResult *>& ms_result_candidate);
  static SwplMsResult * getModerateSchedule(std::unordered_set<SwplMsResult *>& ms_result_candidate);
};

class SwplSSEdge;
class SwplSSEdges;
using SwplSSMoveinfo = llvm::DenseMap<const SwplInst*, long>;

/// \brief StageScheduling proccessing
class SwplSSProc {
public:
  static bool execute(const SwplDdg &ddg,
                      const SwplLoop &loop,
                      unsigned ii,
                      SwplInstSlotHashmap *inst_slot_map,
                      raw_ostream &stream );
  static void dumpSSMoveinfo(raw_ostream &stream, const SwplSSMoveinfo &v);
  static bool SwplSSAdjustSlot(SwplInstSlotHashmap& ism, SwplSSMoveinfo &v);
};

class SwplSSACProc {
  const SwplDdg &ddg;
  unsigned II;
  SwplSSEdges &ssedges;
public:
  SwplSSACProc(const SwplDdg &d,
               unsigned ii,
               SwplSSEdges &e) : ddg(d), II(ii), ssedges(e) {}
  static bool execute(const SwplDdg &d,
                      unsigned ii,
                      SwplSSEdges &e,
                      SwplSSMoveinfo &v,
                      raw_ostream &stream );
private:
  unsigned getTargetEdges(llvm::SmallVector<SwplSSEdge*, 8> &v);
  void collectPreInsts(const SwplSSEdge &e, SwplSSMoveinfo &v);
  void collectPostInsts(const SwplSSEdge &e, SwplSSMoveinfo &v);
};

/// \brief Information on the nodes(SwplInst*) group that will be the circulating part
class SwplSSCyclicInfo {
  std::vector<std::vector<const SwplInst*> *> cyclic_node_list;
public:
  SwplSSCyclicInfo(const SwplDdg& ddg, const SwplLoop& loop);
  virtual ~SwplSSCyclicInfo() {
    for (auto root: cyclic_node_list){
      delete root;
    }
  }
  bool isInCyclic(const SwplInst *ini, const SwplInst *term) const;
  void dump(raw_ostream &stream) const;

};

/// \brief Edge info
class SwplSSEdge {
  int delay;
  unsigned distance;
  unsigned II;
  bool inCyclic=false;
public:
  const SwplInst *InitialVertex;  ///< Edge Preceding Instruction
  unsigned InitialCycle;
  const SwplInst *TerminalVertex; ///< Edge successor instruction
  unsigned TerminalCycle;
  long numSkipFactor;
  SwplSSEdge(const SwplInst *ini, unsigned inicycle,
             const SwplInst *term, unsigned termcycle,
             int delay_in, unsigned distance_in,
             unsigned ii);
  void setInCyclic(bool val) {
    inCyclic = val;
    return;
  }
  bool isCyclic() {
    return inCyclic;
  }
  void dump(raw_ostream &stream, StringRef fname) const;
};

/// \brief manege Edge of SWPLed
class SwplSSEdges {
  unsigned II;
public:
  std::vector<SwplSSEdge *> Edges;
  SwplSSEdges(const SwplDdg &ddg,
              const SwplLoop &loop,
              unsigned ii,
              SwplInstSlotHashmap& inst_slot_map);
  virtual ~SwplSSEdges() {
    for (auto *v : Edges) {
      delete v;
    }
  }
  void dump(raw_ostream &stream, StringRef fname) const;
  void updateInCyclic(const SwplSSCyclicInfo &);
  void updateCycles(const SwplSSMoveinfo &v);

};

}
#endif
