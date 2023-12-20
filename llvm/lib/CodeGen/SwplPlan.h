//=- SwplPlan.h - Classes as cheduling results in SWPL -*- C++ -*------------=//
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

#ifndef LLVM_LIB_CODEGEN_SWPLPLAN_H
#define LLVM_LIB_CODEGEN_SWPLPLAN_H

#include "SWPipeliner.h"

namespace llvm{


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
  /// \brief change SlotIndex
  /// \param [in] v amount of change
  /// \return Nothing
  void moveSlotIndex(long v) {slot_index=slot_index+v; return;}

  static SwplSlot baseSlot(unsigned iteration_interval);
  static SwplSlot construct(unsigned cycle, unsigned fetch_slot);
  static SwplSlot constructFromBlock(unsigned block, unsigned iteration_interval);
  static SwplSlot slotMax();
  static SwplSlot slotMin();
};


/// \brief スケジューリング結果となるSwplInstとSwplSlotのMap
// class SwplInstSlotHashmap : public llvm::DenseMap<SwplInst*, SwplSlot> {
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
  void dump();
};

class SwplMsResourceResult;

/// \brief スケジューリング結果を保持するクラス
/// \details transform mirへ渡す情報となる
class SwplPlan {
  const SwplLoop& loop;              ///< スケジューリング対象のループ情報
  // SwplInstSlotHashmap inst_slot_map; ///< スケジューリング結果
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
  // SwplInstSlotHashmap& getInstSlotMap() { return inst_slot_map ; } ///< getter
  // const SwplInstSlotHashmap& getInstSlotMap() const { return inst_slot_map ; } ///< getter
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
  void dumpInstTable(raw_ostream &stream);

  static bool isSufficientWithRenamingVersions(const SwplLoop& c_loop,
                                              //  const SwplInstSlotHashmap& c_inst_slot_map,
                                               const SwplSlots& c_slots,
                                               unsigned iteration_interval,
                                               unsigned n_renaming_versions);
  static SwplPlan* construct(const SwplLoop& c_loop,
                            //  SwplInstSlotHashmap& inst_slot_map,
                             SwplSlots& slots,
                             unsigned min_ii,
                             unsigned ii,
                             const SwplMsResourceResult& resource);
  static void destroy(SwplPlan* plan);
  static SwplPlan* generatePlan(SwplDdg& ddg);

private:
  static unsigned calculateResourceII(const SwplLoop& c_loop);
  static unsigned calcResourceMinIterationInterval(const SwplLoop& c_loop);
  static TryScheduleResult trySchedule(const SwplDdg& c_ddg,
                                       unsigned res_mii,
                                      //  SwplInstSlotHashmap** inst_slot_map,
                                       SwplSlots** slots,
                                       unsigned* selected_ii,
                                       unsigned* calculated_min_ii,
                                       unsigned* required_itr,
                                       SwplMsResourceResult* resource);
  static TryScheduleResult selectPlan(const SwplDdg& c_ddg,
                                      // SwplInstSlotHashmap& rslt_inst_slot_map,
                                      SwplSlots& rslt_slots,
                                      unsigned* selected_ii,
                                      unsigned* calculated_min_ii,
                                      unsigned* required_itr,
                                      SwplMsResourceResult& resource);
};

}
#endif
