//=- SwplRegEstimate.h - Counting registers in SWPL -*- C++ -*---------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Processing related to register number calculation in SWPL.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_SWPLREGESTIMATE_H
#define LLVM_LIB_CODEGEN_SWPLREGESTIMATE_H

#include "SWPipeliner.h"
#include "SwplPlan.h"
#include "SwplScheduling.h"

namespace llvm{

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

}
#endif
