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
#include <set>
#include <unordered_set>


namespace llvm{

using SwplInstIntMap = llvm::DenseMap<const SwplInst*, int>;
using SwplInstPrioque = std::map<int, const SwplInst*>;
using SwplInstSet = std::set<const SwplInst*>;

/// \brief Slot to place instructions.
class SwplSlot {
public:
  unsigned slot_index;

  /// constructor
  constexpr SwplSlot(unsigned val = 0): slot_index(val) {}
  constexpr operator unsigned() const {
    return slot_index;
  }

  ///////////////
  // Avoid comparing objects.

  /// Comparisons between SwplSlot objects
  bool operator==(const SwplSlot &Other) const { return slot_index == Other.slot_index; }
  bool operator!=(const SwplSlot &Other) const { return slot_index != Other.slot_index; }

  /// Comparisons against SwplSlot constants.
  bool operator==(unsigned Other) const { return slot_index == Other; }
  bool operator!=(unsigned Other) const { return slot_index != Other; }
  bool operator==(int Other) const { return slot_index == unsigned(Other); }
  bool operator!=(int Other) const { return slot_index != unsigned(Other); }

  bool operator<(const SwplSlot &Other) const { return slot_index < Other.slot_index; }

  unsigned calcBlock(unsigned iteration_interval);
  unsigned calcCycle();
  unsigned calcFetchSlot();

  /// \brief change SlotIndex.
  /// \param [in] v amount of change.
  void moveSlotIndex(long v) {slot_index=slot_index+v; return;}

  static SwplSlot baseSlot(unsigned iteration_interval);
  static SwplSlot construct(unsigned cycle, unsigned fetch_slot);
  static SwplSlot constructFromBlock(unsigned block, unsigned iteration_interval);
  static SwplSlot slotMax();
  static SwplSlot slotMin();

  static const unsigned UNCONFIGURED_SLOT = 0;
};
/// \brief Dynamic array of SwplSlots that result in scheduling
class SwplSlots : public std::vector<SwplSlot> {
public:
  size_t calcFlatScheduleBlocks(const SwplLoop& c_loop, unsigned iteration_interval);
  size_t calcPrologBlocks(const SwplLoop& loop, unsigned iteration_interval);
  size_t calcKernelBlocks(const SwplLoop& c_loop, unsigned iteration_interval);
  size_t calcNRenamingVersions(const SwplLoop& c_loop, unsigned iteration_interval) const;
  SwplSlot findBeginSlot(const SwplLoop& c_loop, unsigned iteration_interval);
  SwplSlot findFirstSlot(const SwplLoop& c_loop);
  SwplSlot findLastSlot(const SwplLoop& c_loop);
  unsigned getRelativeInstSlot(const SwplInst& c_inst, const SwplSlot& begin_slot) const;
  void getMaxMinSlot(SwplSlot& max_slot, SwplSlot &min_slot);
  bool isIccFreeAtBoundary(const SwplLoop& loop,  unsigned iteration_interval) const;
  unsigned calcLastUseCycleInBodyWithInheritance(const SwplReg& reg, unsigned iteration_interval) const;
  unsigned calcLastUseCycleInBody(const SwplReg& reg, unsigned iteration_interval) const;
  SwplSlot getEmptySlotInCycle( unsigned cycle, unsigned iteration_interval, bool isvirtual );

  /// \brief Returns free slot in cycle (for LS)
  /// \param [in] cycle Cycle to search for.
  /// \param [in] isvirtual true if searching for an empty slot for a virtual instruction.
  /// \return slot in the cycle where no instruction is placed.
  SwplSlot getLsEmptySlotInCycle( unsigned cycle, bool isvirtual );
  size_t size() const {
    size_t s = 0;
    for (auto& t : *this) {
      if (t) s++;
    }
    return s;
  }
  SwplSlot& at(unsigned ix) {
    static SwplSlot UNCONFIGURED_Slot;
    if(ix==UINT_MAX) return UNCONFIGURED_Slot;
    return vector::at(ix);
  }
  const SwplSlot& at(unsigned ix) const {
    static SwplSlot UNCONFIGURED_Slot;
    if(ix==UINT_MAX) return UNCONFIGURED_Slot;
    return vector::at(ix);
  }

  /// \brief Returns instruction idx in slot order.
  llvm::SmallVector<unsigned, 32> getInstIdxInSlotOrder();

  void dump(const SwplLoop& c_loop);
};

/// \brief represents MRT
/// \details represents MRT
///          MRT can handle cases where ResourceID cannot be treated as an element number, so
///          represented as a map vector of ResourceID and SwplInst*.
///          The image is as below.(ID=ResourceID, instn=SwplInst*)
///          cycle1: { ID1:inst1, ID2:inst2,            ... } ↑
///          cycle2: { ID1:inst1,            ID3:inst3, ... } ｜
///          cycle3: {            ID2:inst1, ID3:inst4, ... } II
///          cycle4: {            ID2:inst4,            ... } ｜
///          cycle5: { ID1:inst4,                       ... } ↓
///
class SwplMrt {
  unsigned iteration_interval; ///< II (if LS then 0)
  unsigned size; ///< size of MRT (use LS only)
  std::vector<llvm::DenseMap<StmResourceId, const SwplInst*>*> table; //< Mrt

public:
  SwplMrt(unsigned ii) : iteration_interval(ii) {} ///< constructor

  /// \brief Reserve resources used by instructions.
  /// \details Record the resources used by the instruction in Mrt.
  ///          Instructions that do not use resources, such as pseudo-instructions,
  ///          are not reserved using Mrt.
  /// \param[in] cycle cycle to place instructions.
  /// \param[in] inst
  /// \param[in] pipeline Resource information used by inst.
  /// \note If you don't know which resource usage patterns, there's no way to fill Mrt.
  ///       Even if you add the resource usage pattern as an argument,
  ///       this function will still ask for resource information,
  ///       so we decided to get the resource information as an argument.
  /// \note The argument pipeline is TmPipeline information according to the resource usage pattern.
  void reserveResourcesForInst(unsigned cycle, const SwplInst& inst, const StmPipeline & pipeline );

  /// \brief Find instructions that are problematic for placement
  /// \details If the second argument instruction is placed in the first argument slot,
  ///          returns a set of scheduled instructions that will interfere with shedule.
  /// \param[in] cycle The cycle in which the instruction is to be placed.
  /// \param[in] inst instruction to be placed.
  /// \param[in] pipeline information used by the pipeline instruction.
  /// \return Set of instruction that causes trouble.
  ///
  /// \note The area of the pointer to be returned must be released by the caller.
  /// \note Since it is impossible to check without knowing which resource usage pattern,
  ///       we decided to get resource information as an argument.
  /// \note The argument inst is redundant because it looks for placed instructions that
  ///       conflict with the argument's resource pattern.
  ///       (Currently has no use)
  /// \note The logic is almost the same as reserveResourcesForInst. Rather than making a
  ///       reservation, if it has already been reserved, that Inst will be recorded.
  SwplInstSet* findBlockingInsts(unsigned cycle, const SwplInst& inst, const StmPipeline & pipeline );
  bool isOpenForInst(unsigned cycle, const SwplInst& inst, const StmPipeline & pipeline);
  void cancelResourcesForInst(const SwplInst& inst);

  /// \brief Display scheduling results in MRT framework
  ///
  /// ```
  ///   Take the following loop as an example
  ///   DO I=1,1024
  ///   A(I) =  A(I) + B(I)
  ///   ENDDO
  ///
  /// For example, if you translate with -Kfast,nosimd,nounroll -W0,-zswpl=dump-mrt,
  /// the MRT result will be displayed as shown below.
  /// The vertical axis represents cycles, and the horizontal axis represents instruction
  /// issue slots, which are issued in order starting from the top left instruction.
  /// The number in parentheses of the instruction indicates which rotation
  /// of the original loop the instruction was used.
  /// From this, we can see that the fadd of A(4) + B(4) is issued in the same cycle as
  /// the store of A(1).
  ///
  ///   0  0  0  0  ffload (6)  0           ffload (6)  add    (6)
  ///   0  0  0  0  ffstore(1)  ffadd  (4)  add    (6)  sub    (1)
  /// ```
  /// \param [in] slots scheduling result.
  /// \param [in] stream output stream.
  void dump(const SwplSlots& slots, raw_ostream &stream);
  void dump();

  static SwplMrt* construct(unsigned iteration_interval);

  /// \brief Prepare MRT for LS.
  /// \details Prepare MRT from the argument size and return the SwplMrt pointer.
  /// \param[in] size_input Size of MRT to generate.
  /// \return SwplMrt pointer.
  static SwplMrt* constructForLs(unsigned size_input);
  void setSize(unsigned s) { size = s; }
  unsigned getSize() { return size; }
  static void destroy(SwplMrt* mrt);

private:
  unsigned getMaxOpcodeNameLength();
  void printInstMI(raw_ostream &stream,
                    const SwplInst* inst, unsigned width);

  /// \brief Output the rotation speed of inst.
  /// \param [in] stream output stream.
  /// \param [in] c_slots scheduling result.
  /// \param [in] inst Target instruction.
  /// \param [in] ii initiation interval.
  /// \details  Output the rotation speed of Inst in '(n)' format.
  void printInstRotation(raw_ostream &stream,
                         const SwplSlots& c_slots,
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
  size_t getLoopBody_ninsts() const { return loop.getSizeBodyInsts(); }
  const SwplLoop& getLoop() const { return loop; }

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
  SwplSlots* slots;                        ///< Arrangement of slot numbers
  SwplSlots* last_slots;                   ///< Record the state of the previous placement last_slot
  SwplMrt* mrt;                            ///< MRT。{resourceID, SwplInst}のmapのVector

public:
  SwplTrialState(const SwplModuloDdg& c_modulo_ddg) : modulo_ddg(c_modulo_ddg) {} ///< constructor

  SwplSlots* getInstSlotMap() { return slots; } ///< getter
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
  SwplSlots* slots;              ///< スケジューリング結果を保持する動的配列
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
  static SwplSlots* execute(const SwplDdg &ddg,
                      const SwplLoop &loop,
                      unsigned ii,
                      SwplSlots *slots,
                      raw_ostream &stream );
  static void dumpSSMoveinfo(raw_ostream &stream, const SwplSSMoveinfo &v);
  static bool adjustSlot(SwplSlots& slots, SwplSSMoveinfo &v);
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
  void collectPostInsts(const SwplSSEdge &e, SwplSSMoveinfo &v);
};

/// \brief Information on the nodes(SwplInst*) group that will be the circulating part
class SwplSSCyclicInfo {
  std::vector<std::vector<const SwplInst*> *> cyclic_node_list;
public:
  SwplSSCyclicInfo(const SwplDdg& ddg, const SwplLoop& loop);
  virtual ~SwplSSCyclicInfo() {
    for (auto *root: cyclic_node_list){
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
              SwplSlots& slots);
  virtual ~SwplSSEdges() {
    for (auto *v : Edges) {
      delete v;
    }
  }
  void dump(raw_ostream &stream, StringRef fname) const;
  void updateInCyclic(const SwplSSCyclicInfo &);
  void updateCycles(const SwplSSMoveinfo &v);

};

/// \brief Estimate and maintain number of registers
class SwplSSNumRegisters {
  unsigned ireg; ///< Estimated number of integer registers
  unsigned freg; ///< Estimated number of floating point registers
  unsigned preg; ///< Estimated number of predicate registers
public:
  SwplSSNumRegisters(const SwplLoop &loop,
                     unsigned ii,
                     const SwplSlots *slots);
  bool operator<(const SwplSSNumRegisters& nr) const {
    return (!(ireg == nr.ireg && freg == nr.freg  && preg == nr.preg) &&
            (ireg <= nr.ireg && freg <= nr.freg  && preg <= nr.preg));
  }
  void dump(raw_ostream &stream) const;
  void print(raw_ostream &stream) const;
};

/// スケジューリング結果の状態を表すenum
enum class TryScheduleResult {
                              /// スケジュール成功
                              TRY_SCHEDULE_SUCCESS,
                              /// スケジュール失敗
                              TRY_SCHEDULE_FAIL,
                              /// ループ回転数が少ないためスケジュール失敗
                              TRY_SCHEDULE_FEW_ITER,
};


#define SWPL_ILLEGAL_SLOT (SwplSlot)0


class SwplMsResourceResult;

/// \brief スケジューリング結果を保持するクラス
/// \details transform mirへ渡す情報となる
class SwplPlan {
  const SwplLoop& loop;              ///< スケジューリング対象のループ情報
  SwplSlots slots; ///< スケジューリング結果
  unsigned minimum_iteration_interval;   ///< min II。スケジューリング試行を開始したII
  unsigned iteration_interval; ///< スケジューリング結果のII
  size_t n_iteration_copies;   ///< prolog+kernelのブロック数。ブロックはII単位となる。
  size_t n_renaming_versions;  ///< kernelがMVEによりコピーされる数
  SwplSlot begin_slot;         ///< 命令が配置されたblockの先頭スロット番号
  SwplSlot end_slot;           ///< 命令が配置されたblockの最後スロット番号
  size_t total_cycles;         ///< prolog,kernal,epilogの総サイクル数
  size_t prolog_cycles;        ///< prolog部分の（MVE展開後の）サイクル数
  size_t kernel_cycles;        ///< kernal部分のサイクル数
  size_t epilog_cycles;        ///< epilog部分のサイクル数
  unsigned num_necessary_ireg; ///< スケジューリング結果から算出した必要な整数レジスタ
  unsigned num_necessary_freg; ///< スケジューリング結果から算出した必要な浮動小数点数レジスタ
  unsigned num_necessary_preg; ///< スケジューリング結果から算出した必要なプレディケートレジスタ
  unsigned num_max_ireg;       ///< 整数レジスタの最大数(ローカルオプションによる調整込み)
  unsigned num_max_freg;       ///< 浮動小数点数レジスタの最大数(ローカルオプションによる調整込み)
  unsigned num_max_preg;       ///< プレディケートレジスタの最大数(ローカルオプションによる調整込み)

public:
  SwplPlan(const SwplLoop& loop) : loop(loop) {} ///< constructor

  ////////////////////
  // getters
  const SwplLoop& getLoop() const { return loop; } ///< getter
  SwplLoop& getLoop() { return const_cast<SwplLoop&>(loop); } ///< getter
  SwplSlots& getInstSlotMap() { return slots ; } ///< getter
  const SwplSlots& getInstSlotMap() const { return slots ; } ///< getter
  unsigned getMinimumIterationInterval() const { return minimum_iteration_interval; } ///< getter
  unsigned getIterationInterval() const { return iteration_interval; } ///< getter
  size_t getNIterationCopies() const { return n_iteration_copies; } ///< getter
  size_t getNRenamingVersions() const { return n_renaming_versions; } ///< getter
  SwplSlot getBeginSlot() const { return begin_slot; } ///< getter
  SwplSlot getEndSlot() const { return end_slot; } ///< getter
  size_t getEpilogCycles() const { return epilog_cycles; } ///< getter
  size_t getKernelCycles() const { return kernel_cycles; } ///< getter
  size_t getPrologCycles() const { return prolog_cycles; } ///< getter
  size_t getTotalCycles() const { return total_cycles; } ///< getter
  unsigned getNecessaryIreg() const { return num_necessary_ireg; } ///< getter
  unsigned getNecessaryFreg() const { return num_necessary_freg; } ///< getter
  unsigned getNecessaryPreg() const { return num_necessary_preg; } ///< getter
  unsigned getMaxIreg() const { return num_max_ireg; } ///< getter
  unsigned getMaxFreg() const { return num_max_freg; } ///< getter
  unsigned getMaxPreg() const { return num_max_preg; } ///< getter
  static bool existsPragma;    ///< Is II specified in pragma?

  ////////////////////
  // setters
  void setMinimumIterationInterval( unsigned arg ) { minimum_iteration_interval = arg; } ///< setter
  void setIterationInterval( unsigned arg ) { iteration_interval = arg; } ///< setter
  void setNIterationCopies( size_t arg ) { n_iteration_copies = arg; } ///< setter
  void setNRenamingVersions( size_t arg ) { n_renaming_versions = arg; } ///< setter
  void setBeginSlot( SwplSlot arg ) { begin_slot = arg; } ///< setter
  void setBeginSlot( unsigned arg ) { begin_slot = arg; } ///< setter
  void setEndSlot( SwplSlot arg ) { end_slot = arg; } ///< setter
  void setEndSlot( unsigned arg ) { end_slot = arg; } ///< setter
  void setEpilogCycles( size_t arg ) { epilog_cycles = arg; } ///< setter
  void setKernelCycles( size_t arg ) { kernel_cycles = arg; } ///< setter
  void setPrologCycles( size_t arg ) { prolog_cycles = arg; } ///< setter
  void setTotalCycles( size_t arg ) { total_cycles = arg; } ///< setter

  unsigned relativeInstSlot(const SwplInst& c_inst) const;
  int getTotalSlotCycles();
  void printInstTable();
  void dump(raw_ostream &stream);
  void dumpForLS(raw_ostream &stream); ///< dump for LS

  /// \brief Dump the instruction placement state of SwplPlan.
  /// \details Output MachineInstr* and Opcode name for each instruction.
  /// \param [in] stream output stream.
  void dumpInstTable(raw_ostream &stream);

  static bool isSufficientWithRenamingVersions(const SwplLoop& c_loop,
                                               const SwplSlots& c_slots,
                                               unsigned iteration_interval,
                                               unsigned n_renaming_versions);
  static SwplPlan* construct(const SwplLoop& c_loop,
                             SwplSlots& slots,
                             unsigned min_ii,
                             unsigned ii,
                             const SwplMsResourceResult& resource);
  static void destroy(SwplPlan* plan);
  static SwplPlan* generatePlan(SwplDdg& ddg);

  /// \brief Generate a plan with ListScheduler.
  /// \param [in] ddg Dependency information for list scheduling.
  /// \param [in] LiveOutReg liveout regs.
  /// \return ListScheduling results.
  static SwplPlan* generateLsPlan(const LsDdg& ddg, const SwplScr::UseMap &LiveOutReg);

private:
  static unsigned calculateResourceII(const SwplLoop& c_loop);
  static unsigned calcResourceMinIterationInterval(const SwplLoop& c_loop);
  static TryScheduleResult trySchedule(const SwplDdg& c_ddg,
                                       unsigned res_mii,
                                       SwplSlots** slots,
                                       unsigned* selected_ii,
                                       unsigned* calculated_min_ii,
                                       unsigned* required_itr,
                                       SwplMsResourceResult* resource);
  static TryScheduleResult selectPlan(const SwplDdg& c_ddg,
                                      SwplSlots& rslt_slots,
                                      unsigned* selected_ii,
                                      unsigned* calculated_min_ii,
                                      unsigned* required_itr,
                                      SwplMsResourceResult& resource);
};

/// \brief 指定したレジスタがいくつ必要であるかを数える処理群
class SwplRegEstimate {
public:
  /// \brief レジスタ見積り時に、def,useのサイクル数を変更するための一時的なレジスタ表現
  class Renamed_Reg {
  public:
    SwplReg* reg;
    int def_cycle;
    int use_cycle;
    bool is_recurrence;
  };

  using RenamedRegVector = std::vector<SwplRegEstimate::Renamed_Reg*>;

  static unsigned calcNumRegs(const SwplLoop& loop,
                              const SwplSlots* slots,
                              unsigned iteration_interval,
                              unsigned regclassid,
                              unsigned n_renaming_versions);
private:
  static void initRenamedReg(Renamed_Reg* reg);
  static void setRenamedReg(Renamed_Reg* reg,
                            const SwplReg& sreg,
                            unsigned def_cycle,
                            unsigned use_cycle,
                            unsigned iteration_interval,
                            bool is_recurrence);
  static unsigned getNumInterferedRegs(const SwplLoop& loop,
                                       const SwplSlots* slots,
                                       unsigned iteration_interval,
                                       unsigned regclassid,
                                       unsigned n_renaming_versions);
  static unsigned getNumImmortalRegs (const SwplLoop& loop, unsigned regclassid );
  static unsigned getNumMortalRegs (const SwplLoop& loop,
                                    const SwplSlots* slots,
                                    unsigned iteration_interval,
                                    unsigned regclassid);
  static void collectRenamedRegs(const SwplLoop& loop,
                                 const SwplSlots* slots,
                                 unsigned iteration_interval,
                                 unsigned regclassid,
                                 unsigned n_renaming_versions,
                                 RenamedRegVector* vector_renamed_regs);
  static void normalizeDefUseCycle(Renamed_Reg *reg1,
                                   Renamed_Reg *reg2,
                                   int *def_cycle_reg1, int *use_cycle_reg1,
                                   int *def_cycle_reg2, int *use_cycle_reg2,
                                   unsigned iteration_interval);
  static unsigned getOffsetAllocationForward(Renamed_Reg *reg1,
                                             Renamed_Reg *reg2,
                                             unsigned iteration_interval,
                                             unsigned n_renaming_versions);
  static unsigned getOffsetAllocationBackward(Renamed_Reg *reg1,
                                              Renamed_Reg *reg2,
                                              unsigned iteration_interval,
                                              unsigned n_renaming_versions);
  static void extendRenamedRegLivesBackward(RenamedRegVector* vector_renamed_regs,
                                            unsigned iteration_interval,
                                            unsigned n_renaming_versions);
  static void extendRenamedRegLivesForward(RenamedRegVector* vector_renamed_regs,
                                           unsigned iteration_interval,
                                           unsigned n_renaming_versions);
  static void incrementCounters(int reg_size, std::vector<int>* reg_counters,
                                unsigned iteration_interval,
                                unsigned begin_cycle, unsigned end_cycle);
  static unsigned getEstimateResult(RenamedRegVector* vector_renamed_regs,
                                    std::vector<int>* reg_counters,
                                    unsigned iteration_interval);
  static unsigned countLivesWithMask(RenamedRegVector* vector_renamed_regs,
                                     unsigned live_range_mask);
  static unsigned countOverlapsWithMask(RenamedRegVector* vector_renamed_regs,
                                        unsigned live_range_mask,
                                        unsigned n_renaming_versions);
  static unsigned getNumPatternRegs (RenamedRegVector* vector_renamed_regs,
                                     unsigned iteration_interval,
                                     unsigned n_renaming_versions, unsigned regclassid);
  static unsigned getNumExtendedRegs (RenamedRegVector* vector_renamed_regs,
                                      const SwplSlots* slots,
                                      unsigned iteration_interval,
                                      unsigned regclassid,
                                      unsigned n_renaming_versions);
  static bool isCountedReg(const SwplReg& reg, unsigned regclassid);
  static int findMaxCounter(std::vector<int>* reg_counters, unsigned iteration_interval);
};

/// \brief SwplのPlanを選択するための、ループ回転数に関するルーチン群
class SwplCalclIterations {
public:

  static bool preCheckIterationCount(const SwplPlanSpec & spec, unsigned int *required_itr);
  static bool checkIterationCountVariable(const SwplPlanSpec & spec, const SwplMsResult & ms);
  static bool checkIterationCountConstant(const SwplPlanSpec & spec, const SwplMsResult & ms);
};

/// \brief Class that handles list scheduling.
class LSListScheduling {
public:
  const LsDdg& lsddg; ///< ddg for LS.
  const SwplScr::UseMap &LiveOutRegs; ///< liveout regs.
  SwplSlots* slots;   ///< Collection of scheduled slots.
  SwplMrt* lsmrt;     ///< MRT for LS.
  llvm::MapVector<const SwplInst*, SwplInstEdges> Ready;  ///< MapVector for store instructions in the order of scheduling.
  SwplSlot earliest_slot; ///< First slot when scheduling.
  unsigned min_cycle;     ///< First cycle when scheduling.
  llvm::SmallVector<unsigned, 32> estimateFregCounter; //< Vreg counter for Freg. Update by calcVregs() with scheduled slots.
  llvm::SmallVector<unsigned, 32> estimateIregCounter; //< Vreg counter for Ireg. Update by calcVregs() with scheduled slots.
  llvm::SmallVector<unsigned, 32> estimatePregCounter; //< Vreg counter for Preg. Update by calcVregs() with scheduled slots.

public:
  LSListScheduling(const LsDdg& lsddg_input,
                   const SwplScr::UseMap &liveOutReg) : lsddg(lsddg_input), LiveOutRegs(liveOutReg) {
    unsigned min_slot_index = SwplSlot::slotMin().slot_index;
    unsigned fb = SWPipeliner::FetchBandwidth;
    // Find the earliest available slot.
    earliest_slot = SwplSlot(min_slot_index+(fb - (min_slot_index % fb)));
    min_cycle = earliest_slot.calcCycle();
    slots = new SwplSlots();
    estimateIregCounter.clear();
    estimateFregCounter.clear();
    estimatePregCounter.clear();
  } ///< constructor
  virtual ~LSListScheduling() {
    delete lsmrt;
  }

  /// \brief Perform scheduling.
  /// \return Returns the results of scheduling. (SwplSlots)
  SwplSlots* getScheduleResult();

  /// \brief Returns the first slot of scheduling.
  /// \return First slot when scheduling.
  SwplSlot getEarliestSlot() { return earliest_slot; }

  /// \brief aggregate num_necessary_ireg by estimateIregCounter.
  unsigned getNumNecessaryIreg() {
    unsigned num_necessary_ireg=0;
    for (unsigned n: estimateIregCounter) num_necessary_ireg = std::max(num_necessary_ireg, n);
    return num_necessary_ireg;
  }
  /// \brief aggregate num_necessary_freg by estimateFregCounter.
  unsigned getNumNecessaryFreg() {
    unsigned num_necessary_freg=0;
    for (unsigned n: estimateFregCounter) num_necessary_freg = std::max(num_necessary_freg, n);
    return num_necessary_freg;
  }
  /// \brief aggregate num_necessary_preg by estimatePregCounter.
  unsigned getNumNecessaryPreg() {
    unsigned num_necessary_preg=0;
    for (unsigned n: estimatePregCounter) num_necessary_preg = std::max(num_necessary_preg, n);
    return num_necessary_preg;
  }

private:
  /// \brief Get the slot number to place the instruction.
  /// \param [in] inst instruction to place.
  /// \param [in] edges vector of edges between instructions.
  /// \return placeable slot.
  SwplSlot getPlacementSlot(const SwplInst* inst, SwplInstEdges &edges);

  /// \brief Decide the order in which instructions are placed.
  /// \details The order is topological order.
  /// \param [in/out] READY Vector of edges when there is a placement instruction and a preceding instruction.
  void setPriorityOrder(llvm::MapVector<const SwplInst*, SwplInstEdges> &READY);

  /// \brief Create MRT for ListScheduling.
  /// \note Create MRT by adding the latencies of all instructions.
  /// \return MRT
  SwplMrt* createLsMrt();

  /// \brief Get resource patterns that can be placed in a specified cycle.
  /// \param [in] inst Instructions to be placed.
  /// \param [in] cycle Cycle to check if it can be placed.
  /// \return resource pattern that can be placed.
  ///         If there is nothing that can be placed, return nullptr.
  const StmPipeline* findPlacablePipeline(const SwplInst* inst, unsigned cycle);

  /// \brief Estimate the number of virtual registers from scheduling results.
  /// \details Update estimate[I|F|P]regCounter with the estimation result.
  void calcVregs();

  /// \brief Update estimate[I|F|P]regCounter corresponding to the specified register type.
  /// \param [in] r Register to be estimated.
  /// \param [in] ormore Element of the counter to update(or more).
  /// \param [in] orless Element of the counter to update(or less).
  void updateRegisterCounter(Register r, int ormore, int orless);
};

}
#endif
