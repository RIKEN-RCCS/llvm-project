//=- AArch64SwplPlan.h - Classes as cheduling results in SWPL -*- C++ -*-----=//
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
//=== Copyright FUJITSU LIMITED 2021  and FUJITSU LABORATORIES LTD. 2021   ===//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AARCH64_AARCH64SWPLPLAN_H
#define LLVM_LIB_TARGET_AARCH64_AARCH64SWPLPLAN_H

#include "AArch64.h"

#include "AArch64SWPipeliner.h"

namespace swpl{
extern  llvm::cl::opt<bool> DebugOutput;

/// Policyを表すenum
enum class SwplSchedPolicy {
                            /// ループ毎に自動選択
                            SWPL_SCHED_POLICY_AUTO,
                            /// 小さなループに適した方法を使う
                            SWPL_SCHED_POLICY_SMALL,
                            /// 大きなループに適した方法を使う
                            SWPL_SCHED_POLICY_LARGE,
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

/// SWPL機能におけるslotを表現するクラス
class SwplSlot {
  unsigned slot_index;

public:
  /// constructor
  constexpr SwplSlot(unsigned val = 0): slot_index(val) {}
  constexpr operator unsigned() const {
    return slot_index;
  }

  ///////////////
  // オブジェクトの比較にならないように

  /// Comparisons between SwplSlot objects
  bool operator==(const SwplSlot &Other) const { return slot_index == Other.slot_index; }
  bool operator!=(const SwplSlot &Other) const { return slot_index != Other.slot_index; }

  /// Comparisons against SwplSlot constants.
  bool operator==(unsigned Other) const { return slot_index == Other; }
  bool operator!=(unsigned Other) const { return slot_index != Other; }
  bool operator==(int Other) const { return slot_index == unsigned(Other); }
  bool operator!=(int Other) const { return slot_index != unsigned(Other); }

  unsigned calcBlock(unsigned iteration_interval);
  unsigned calcCycle();
  unsigned calcFetchSlot();

  static SwplSlot baseSlot(unsigned iteration_interval);
  static SwplSlot construct(unsigned cycle, unsigned fetch_slot);
  static SwplSlot constructFromBlock(unsigned block, unsigned iteration_interval);
  static SwplSlot slotMax();
  static SwplSlot slotMin();
};


/// \brief スケジューリング結果となるSwplInstとSwplSlotのMap
class SwplInstSlotHashmap : public llvm::DenseMap<SwplInst*, SwplSlot> {
public:
  size_t calcFlatScheduleBlocks(const SwplLoop& c_loop, unsigned iteration_interval);
  size_t calcPrologBlocks(const SwplLoop& loop, unsigned iteration_interval);
  size_t calcKernelBlocks(const SwplLoop& c_loop, unsigned iteration_interval);
  size_t calcNRenamingVersions(const SwplLoop& c_loop, unsigned iteration_interval);
  SwplSlot findBeginSlot(const SwplLoop& c_loop, unsigned iteration_interval);
  SwplSlot findFirstSlot(const SwplLoop& c_loop);
  SwplSlot findLastSlot(const SwplLoop& c_loop);
  unsigned getRelativeInstSlot(const SwplInst& c_inst, const SwplSlot& begin_slot) const;
  void getMaxMinSlot(SwplSlot& max_slot, SwplSlot &min_slot);
  bool isIccFreeAtBoundary(const SwplLoop& loop,  unsigned iteration_interval) const;
  unsigned calcLastUseCycleInBodyWithInheritance(const SwplReg& reg, unsigned iteration_interval) const;
  unsigned calcLastUseCycleInBody(const SwplReg& reg, unsigned iteration_interval) const;
  SwplSlot getEmptySlotInCycle( unsigned cycle, unsigned iteration_interval, bool isvirtual );
  void dump();
};


/// \brief スケジューリング結果を保持するクラス
/// \details transform lindaへ渡す情報となる
class SwplPlan {
  const SwplLoop& loop;              ///< スケジューリング対象のループ情報
  SwplInstSlotHashmap inst_slot_map; ///< スケジューリング結果
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
  SwplSchedPolicy policy;      ///< スケジューリングポリシー

public:
  SwplPlan(const SwplLoop& loop) : loop(loop) {} ///< constructor

  ////////////////////
  // getters
  const SwplLoop& getLoop() const { return loop; } ///< getter
  SwplLoop& getLoop() { return const_cast<SwplLoop&>(loop); } ///< getter
  SwplInstSlotHashmap& getInstSlotMap() { return inst_slot_map ; } ///< getter
  const SwplInstSlotHashmap& getInstSlotMap() const { return inst_slot_map ; } ///< getter
  unsigned getMinimumIterationInterval() const { return minimum_iteration_interval; } ///< getter
  unsigned getIterationInterval() const { return iteration_interval; } ///< getter
  size_t getNIterationCopies() const { return n_iteration_copies; } ///< getter
  size_t getNRenamingVersions() const { return n_renaming_versions; } ///< getter
  SwplSlot getBeginSlot() { return begin_slot; } ///< getter
  SwplSlot getEndSlot() { return end_slot; } ///< getter
  SwplSchedPolicy getPolicy() const { return policy; } ///< getter
  size_t getEpilogCycles() { return epilog_cycles; } ///< getter
  size_t getKernelCycles() { return kernel_cycles; } ///< getter
  size_t getPrologCycles() { return prolog_cycles; } ///< getter
  size_t getTotalCycles() { return total_cycles; } ///< getter

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
  void setPolicy( SwplSchedPolicy arg ) { policy = arg; } ///< setter
  void setEpilogCycles( size_t arg ) { epilog_cycles = arg; } ///< setter
  void setKernelCycles( size_t arg ) { kernel_cycles = arg; } ///< setter
  void setPrologCycles( size_t arg ) { prolog_cycles = arg; } ///< setter
  void setTotalCycles( size_t arg ) { total_cycles = arg; } ///< setter

  unsigned relativeInstSlot(const SwplInst& c_inst) const;
  int getTotalSlotCycles();
  void printInstTable();
  std::string getPolicyString() const;
  void dump(raw_ostream &stream);
  void dumpInstTable(raw_ostream &stream);

  static bool isSufficientWithRenamingVersions(const SwplLoop& c_loop,
                                               const SwplInstSlotHashmap& c_inst_slot_map,
                                               unsigned iteration_interval,
                                               unsigned n_renaming_versions);
  static SwplPlan* construct(const SwplLoop& c_loop,
                             SwplInstSlotHashmap& inst_slot_map,
                             unsigned min_ii,
                             unsigned ii,
                             SwplSchedPolicy policy);
  static void destroy(SwplPlan* plan);
  static SwplPlan* generatePlan(SwplDdg& ddg);
  static std::string getPolicyString(SwplSchedPolicy policy);

private:
  static unsigned calculateResourceII(const SwplLoop& c_loop);
  static unsigned calcResourceMinIterationInterval(const SwplLoop& c_loop);
  static TryScheduleResult trySchedule(const SwplDdg& c_ddg,
                                       unsigned res_mii,
                                       SwplSchedPolicy policy,
                                       SwplInstSlotHashmap** inst_slot_map,
                                       unsigned* selected_ii,
                                       unsigned* calculated_min_ii,
                                       unsigned* required_itr);
  static TryScheduleResult selectPlan(const SwplDdg& c_ddg,
                                      SwplSchedPolicy policy,
                                      SwplInstSlotHashmap& rslt_inst_slot_map,
                                      unsigned* selected_ii,
                                      unsigned* calculated_min_ii,
                                      SwplSchedPolicy* selected_policy,
                                      unsigned* required_itr);
};

}
#endif // LLVM_LIB_TARGET_AARCH64_AARCH64SWPLPLAN_H
