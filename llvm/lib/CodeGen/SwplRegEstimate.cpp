//=- SwplRegEstimate.cpp - Counting registers in SWPL -*- C++ -*-------------=//
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

#include "SwplRegEstimate.h"
#include "SWPipeliner.h"
#include "SwplPlan.h"
#include "llvm/CodeGen/SwplTargetMachine.h"

static llvm::cl::opt<bool> OptionDumpReg("swpl-debug-dump-estimate-reg",llvm::cl::init(false), llvm::cl::ReallyHidden);

namespace llvm{

/// \brief scheduling結果に対して指定したレジスタがいくつ必要であるかを数える処理
/// \note 必要なレジスタ数を正確に計算する事は、RAでなければできないため、
///       SWPL独自に必要となるレジスタを概算する
unsigned SwplRegEstimate::calcNumRegs(const SwplLoop& loop,
                                      const SwplSlots* slots,
                                      unsigned iteration_interval,
                                      unsigned regclassid,
                                      unsigned n_renaming_versions) {
  RenamedRegVector* vector_renamed_regs;
  unsigned n_immortal_regs, n_mortal_regs, n_interfered_regs,
    n_extended_regs, n_patten_regs;
  unsigned max_regs = 0, margin = 0;

  assert(slots != nullptr);
  assert(n_renaming_versions > 0);

  /* 見積もり対象のレジスタを収集 */
  vector_renamed_regs = new RenamedRegVector();
  collectRenamedRegs(loop, slots, iteration_interval,
                     regclassid, n_renaming_versions, vector_renamed_regs);

  /* predecessor, successor, phi をたどってつながるものは全て
   * 同一の hard reg に割り当てられる。
   * したがって、頭だけ数えなければならない。*/

  n_immortal_regs = getNumImmortalRegs (loop, regclassid);
  n_interfered_regs = getNumInterferedRegs(loop, slots,
                                           iteration_interval, regclassid,
                                           n_renaming_versions);
  n_mortal_regs = getNumMortalRegs (loop, slots,
                                    iteration_interval, regclassid);
  n_patten_regs = getNumPatternRegs(vector_renamed_regs,
                                    iteration_interval,
                                    n_renaming_versions, regclassid);
  n_extended_regs = getNumExtendedRegs (vector_renamed_regs,
                                        slots,
                                        iteration_interval, regclassid,
                                        n_renaming_versions);					      

  max_regs = std::max(max_regs, n_immortal_regs + n_mortal_regs);
  max_regs = std::max(max_regs, n_immortal_regs + n_interfered_regs);
  max_regs = std::max(max_regs, n_immortal_regs + n_extended_regs);
  max_regs = std::max(max_regs, n_immortal_regs + n_patten_regs);
  const char* regname="other";

  if (llvm::StmRegKind::isInteger(regclassid) && n_renaming_versions >= 3 ) {
    regname = "I";
    margin = 1;
  } else if( llvm::StmRegKind::isFloating(regclassid) && n_renaming_versions >= 3 ) {
    regname = "F";
    margin = 1;
  } else if( llvm::StmRegKind::isPredicate(regclassid) ) { /* versionの制限は無し */
    regname = "P";
    margin = 1;
  }

  max_regs += margin;

  if (OptionDumpReg) {
    dbgs() << "Estimated number of "
           << regname << " registers (classid:" << regclassid << ") :";
    dbgs() << " immortal " << n_immortal_regs
           << ", mortal  " << n_mortal_regs
           << ", interfered "<< n_interfered_regs
           << ", extended "<< n_extended_regs
           << ", pattern "<< n_patten_regs
           << ", margin "<< margin <<",\n"
           << " total    : "<< max_regs << "\n";
  }

  /* vector_renamed_regs, renamed_regを解放する*/
  for( auto rr : *vector_renamed_regs ) {
    delete rr;
  }
  delete vector_renamed_regs;

  return max_regs;
}

/// \brief RenamedRegの初期化
void SwplRegEstimate::initRenamedReg(SwplRegEstimate::Renamed_Reg* reg) {
  reg->reg = nullptr;
  reg->def_cycle = 0;
  reg->use_cycle = 0;
  reg->is_recurrence = false;
  return;
}

/// \brief RenamedRegへの設定
void SwplRegEstimate::setRenamedReg(SwplRegEstimate::Renamed_Reg* reg,
                                    const SwplReg& sreg,
                                    unsigned def_cycle,
                                    unsigned use_cycle,
                                    unsigned iteration_interval,
                                    bool is_recurrence) {
  assert(reg != nullptr);
  initRenamedReg(reg);
  reg->reg = &(const_cast<SwplReg&>(sreg));
  reg->def_cycle = (int)def_cycle;
  reg->use_cycle = (int)use_cycle;
  reg->is_recurrence = is_recurrence;
  return;
}

/// \brief 干渉を考慮してrenaming version後の変数が要求するレジスタを数える処理
/// \note レジスタ数見積りの下限を求めるために用いる
///
/// ```
///   - 以下では、T = MVE*IIと定義する.
///   また、RAが割り付けるレジスタの意味で、"レジスタ"
///   SwplRegの意味で、"vreg"という言葉を用いる.
///   また、RAが割付ける場合、renaming versiong後ろのそれぞれのvregは,
///   単一のレジスタに割付けられ、live rangeの途中で、二つのレジスタ間で乗り換える
///   様なことはない事を前提としている.
///     
///   - 一次補正
///   ライブレンンジL1がT/2を越えるようなvregが要求するレジスタ数の最小値を求める.
///   L1はT/2を超えるため、renaming versionされた自身のvregは
///   すべてのversionと互いに干渉するため, レジスタをMVE個だけ要求する.
///   また、他のprライブレンジL1がT/2を越えるvregも,
///   T/2以上のvreg同士は常に重なるため, レジスタを共有する事はないため、
///   要求するレジスタを単純に加算すればよい.
///   さらに、vregの空き区間(T-L1)に、同じレジスタを使用できる他のvregは、
///   T/2以上のライブレンジを持たないため、無視している.
///
///   - 二次補正
///   vregの空き区間(T-L1)区間に、埋めるられないライブレンジL2をもつ
///   vregをカウントする。以下の点が、L1のvregと異なる.
///   - L2のライブレンジをもつvregは、自身のすべてのversionとかさなるとは限らない。
///   => 要求するレジスタがMVE個でない場合があり、L2とversion数の関係で定まる.
///   - L2のライブレンジをもつ異なるレジスタ同士が、重なるとは限らない.
///   => 異なるvregの要求レジスタ数同士を単純に加算してはならない.
///
///   一次補正と二次補正の和と、getNumImmortalRegの結果の和が、
///   必要なレジスタ数の下限を与える.
/// ```
///
unsigned SwplRegEstimate::getNumInterferedRegs(const SwplLoop& loop,
                                               const SwplSlots* slots,
                                               unsigned iteration_interval,
                                               unsigned regclassid,
                                               unsigned n_renaming_versions) {
  unsigned counter = 0;
  unsigned maximum_live_range;
  int full_kernel_cycles, live_range;
  int maximum_gap;
  bool is_recurrence;

  assert(slots != nullptr);
  assert(n_renaming_versions > 0);

  full_kernel_cycles = n_renaming_versions * iteration_interval;
  maximum_gap = -1;
  maximum_live_range = 0;

  /* 一次補正のため,ライブレンジがT/2を越えるvregの重なりを数える
   * 同時に、二次補正に必要な情報を収集する
   */
  for( auto* inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_cycle;
  
    def_slot = slots->at(inst->inst_ix);
    def_cycle = def_slot.calcCycle();
    is_recurrence = inst->isRecurrence();

    for( auto reg : inst->getDefRegs() ) {
      unsigned last_use_cycle;
   
      if (isCountedReg(*reg, regclassid) == false) {
        continue;
      }
    
      if ( !(reg->getUseInsts().empty()) ) {
        last_use_cycle
          = slots->calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      }  else {
        last_use_cycle = def_cycle;
      }

      assert(last_use_cycle >= def_cycle);
      live_range = (int)(last_use_cycle-def_cycle+1);

      if(is_recurrence == true && n_renaming_versions == 2) {
        /* rucurrenceのlive rangeは、1cycle重ねて冗長に表現している.
         * n_renaming_versions == 2の場合に、閾値T/2を超えるかどうかに影響をあたえる.
         * ひとつのSwplRegで隙間なく一つのレジスタをを占有できる.
         * 但し、最適化が動くとその限りではない。
         * countとしては、+1するように特別扱いする.
         */
        counter += reg->getRegSize();
      } else if(live_range*2 > full_kernel_cycles) {
        /* T/2をこえるvregのうち、非生存区間の最大長 */
        if(maximum_gap < full_kernel_cycles - live_range) {
          maximum_gap = full_kernel_cycles - live_range;
          assert(maximum_gap >= 0);
        }
        /* 一次補正 */
        counter += n_renaming_versions * reg->getRegSize();
      } else {
        /* T/2をこえないregisterのうち、最大のライブレンジ */
        if(maximum_live_range < (last_use_cycle - def_cycle + 1)) {
          maximum_live_range = (last_use_cycle - def_cycle + 1);
        }
      }
    }
  }

  /**
   * T/2を越えないライブレンンジL2をもつレジスタのうち、
   * T/2を越えるレジスタの隙間(T-L1)にいれられないレジスタをカウントする.
   * 具体的には,versioningされたレジスタを表現するために必要なレジスタ数(total_color)を計算する
   * 各々のversion(0,1,2,...,n_renaming_version-1)に、
   * 0,1,2,...,total_color-1を順番に塗っていき,
   * 最終versionと,それとlive rangeが重なるversionのレジスタの色が異なればよい.
   *
   * \note 本実装では、以下に対応できていない
   *  - modulo cycleまでは考慮できていない
   *  - この条件に該当するレジスタ同士の干渉は考えていない
   *  - tuple型を考慮できていない
   */
  if(maximum_gap != -1 && maximum_gap < (signed)maximum_live_range) {
    int total_color, last_color;

    /* liverangeがのびているstage数だけ少なくとも、レジスタが必要 */
    int stage = (maximum_live_range + iteration_interval -1)/iteration_interval;

    for(total_color = stage; total_color <= (signed)n_renaming_versions; ++total_color) {
      /* 最終versionのcolor */
      last_color = (n_renaming_versions-1)%total_color;
      /* 最終versionのcolorが、0-(stage-1)のcolorなら重なる */
      if(last_color < (stage - 1)%total_color ) {
        continue;
      }
    }
    counter += total_color;
  }
  return counter;
}

/// \brief Processing to count unrenamed registers
/// \note unrenamed registers is livein register or defined by φ, and following successor leads to itself
unsigned SwplRegEstimate::getNumImmortalRegs(const SwplLoop& loop, unsigned regclassid ) {
  unsigned counter;
  llvm::SmallSet<int, 20> regset;

  counter = 0;
  for( auto inst : loop.getBodyInsts() ) {
    for( auto reg : inst->getUseRegs() ) {
      if ( reg->isRegNull() ) {
        continue;
      }
      if( !(reg->isSameKind(regclassid)) ) {
        continue;
      }
      if ( reg->isStack() ) {
        continue;
      }
      auto *def = reg->getDefInst();
      if (def!=nullptr && !def->isInLoop()) {
        if (regset.contains(reg->getReg())) continue;
        regset.insert(reg->getReg());
        counter += reg->getRegSize();
      }
    }
  }

  for( auto inst : loop.getPhiInsts() ) {
    SwplReg *reg, *successor;
    reg = &(inst->getDefRegs(0));
    assert ( !(reg->isRegNull()) );
    if ( !reg->isSameKind(regclassid) ) {
      continue;
    }
  
    for (successor = reg->getSuccessor();
         successor != nullptr;
         successor = successor->getSuccessor()) {
      if (successor == reg) {
        if (regset.contains(reg->getReg())) continue;
        regset.insert(reg->getReg());
        counter += reg->getRegSize();
        break;
      }
    }
  }
  return counter;
}

/// \brief renamingされるレジスタを数える
/// \note rename される reg （body で定義されていて、predecessor が無いもの）。
///       それぞれ live_range を計算し、重複分もそのまま予約する。
unsigned SwplRegEstimate::getNumMortalRegs(const SwplLoop& loop, 
                                           const SwplSlots* slots,
                                           unsigned iteration_interval,
                                           unsigned regclassid) {
  unsigned max_counter;
  std::vector<int> reg_counters(iteration_interval, 0);

  for( auto* inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_cycle;
  
    def_slot = slots->at(inst->inst_ix);
    def_cycle = def_slot.calcCycle();

    for( auto reg : inst->getDefRegs() ) {
      unsigned last_use_cycle;
   
      if (isCountedReg(*reg, regclassid) == false) {
        continue;
      }
    
      if ( reg->isUsed() ) {
        last_use_cycle = slots->calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      }  else {
        last_use_cycle = def_cycle;
      }
      incrementCounters (reg->getRegSize(),
                         &reg_counters, iteration_interval,
                         def_cycle, last_use_cycle);
    }
  }

  /* branch 用の icc をカウントする。
   * icc が iteration_interval のブロックを跨って使用されているかを、
   * チェックする場所が他にない。*/
  if( llvm::StmRegKind::isCC( regclassid ) ) {
    incrementCounters (1, &reg_counters, iteration_interval,
                        0, 0);
  }

  max_counter = findMaxCounter(&reg_counters, iteration_interval);
  return max_counter;
}

/// \brief regclassid毎にレジスタ見積りの数え上げを行う
/// \note 従来の見積もりと同様であり、SwplSlotをcycleに換算しているため、
///       過剰な重なりを生成する場合がある.結果、spillが出ない安全サイドに倒している.
///       Slot単位でも見積もりが可能であり、より正確になるはずあるが、
///       暫定的にcycle単位で処理をおこなう.
void SwplRegEstimate::collectRenamedRegs(const SwplLoop& loop, 
                                         const SwplSlots* slots,
                                         unsigned iteration_interval,
                                         unsigned regclassid,
                                         unsigned n_renaming_versions,
                                         RenamedRegVector* vector_renamed_regs) {
#ifndef NDEBUG
  // assertでのみ使用
  unsigned full_kernel_cycles;
  full_kernel_cycles = n_renaming_versions * iteration_interval;
#endif

  for( auto* inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_cycle;
    bool is_recurrence;
  
    assert( inst->isPhi()==false);
  
    def_slot = slots->at(inst->inst_ix);
    def_cycle = def_slot.calcCycle();

    is_recurrence = inst->isRecurrence();

    for( auto reg : inst->getDefRegs() ) {
      SwplRegEstimate::Renamed_Reg *renamed_reg;
      unsigned last_use_cycle;
   
      if (isCountedReg(*reg, regclassid) == false) {
        continue;
      }
    
      if ( !(reg->getUseInsts().empty()) ) {
        last_use_cycle
          = slots->calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      }  else {
        last_use_cycle = def_cycle;
      }
      assert(last_use_cycle >= def_cycle);
      assert(last_use_cycle - def_cycle + 1 <= full_kernel_cycles);

      renamed_reg = new SwplRegEstimate::Renamed_Reg();
      setRenamedReg(renamed_reg, *reg, def_cycle,
                    last_use_cycle, iteration_interval,
                    is_recurrence);

      vector_renamed_regs->push_back(renamed_reg);
    }
  }
  return;
}

/// \brief reg2をiteration_intervalずつずらして、cycleの位置を正規化する
void SwplRegEstimate::normalizeDefUseCycle(SwplRegEstimate::Renamed_Reg *reg1,
                                           SwplRegEstimate::Renamed_Reg *reg2,
                                           int *def_cycle_reg1, int *use_cycle_reg1,
                                           int *def_cycle_reg2, int *use_cycle_reg2,
                                           unsigned iteration_interval) {
  int base_cycle;

  base_cycle = reg1->def_cycle - 1;
			    
  *def_cycle_reg1 = 1; /* reg1->def_cycle - base_cycle */
  *use_cycle_reg1 = reg1->use_cycle - base_cycle;
  *def_cycle_reg2 = reg2->def_cycle - base_cycle;
  *use_cycle_reg2 = reg2->use_cycle - base_cycle;

  /* def_cycle_reg2をdef_cycle_reg1と同じversionにする正規化処理 */
  if(*def_cycle_reg2 < *def_cycle_reg1) {
    while(*def_cycle_reg2 < *def_cycle_reg1) {
      *def_cycle_reg2 += iteration_interval;
      *use_cycle_reg2 += iteration_interval;
    }
  } else if (*def_cycle_reg2 >= *def_cycle_reg1 + (signed)iteration_interval) {
    while(*def_cycle_reg2 >= *def_cycle_reg1 + (signed)iteration_interval) {
      *def_cycle_reg2 -= iteration_interval;
      *use_cycle_reg2 -= iteration_interval;
    }
  }
  assert(*def_cycle_reg1 <= *def_cycle_reg2);
  assert(*def_cycle_reg2 <= (signed)iteration_interval);
  return;
}
				      
/// \brief reg1のuse後方の空領域に、reg2を配置できるかどうかを判定し、
///        可能ならreg1->use_cycleからの最小のoffset値を返す. 不可能なら0を返却する.
/// \retval reg2を配置可能なreg1->use_cycleからの最小のoffset値
unsigned SwplRegEstimate::getOffsetAllocationForward(SwplRegEstimate::Renamed_Reg *reg1,
                                                     SwplRegEstimate::Renamed_Reg *reg2,
                                                     unsigned iteration_interval,
                                                     unsigned n_renaming_versions) {
  int version;
  int dead_cycle_reg1, live_cycle_reg2;
  int full_kernel_cycles;
  int offset_value = 0;
  int def_cycle_reg1, use_cycle_reg1, def_cycle_reg2, use_cycle_reg2;

  full_kernel_cycles = n_renaming_versions * iteration_interval;

  /* def_cycle_reg1 <= def_cycle_reg2をみたすように正規化 */
  normalizeDefUseCycle(reg1, reg2,
                       &def_cycle_reg1, &use_cycle_reg1,
                       &def_cycle_reg2, &use_cycle_reg2,
                       iteration_interval);

  assert(use_cycle_reg1 <= full_kernel_cycles &&
         use_cycle_reg2 <= full_kernel_cycles + (signed)iteration_interval);

  dead_cycle_reg1 = full_kernel_cycles - (use_cycle_reg1 - def_cycle_reg1 + 1);
  live_cycle_reg2 = use_cycle_reg2 - def_cycle_reg2 + 1;

  if(dead_cycle_reg1 < live_cycle_reg2) {
    /* どのようにずらしても隙間に入らないため */
    return offset_value;
  }

  /* reg2のcycleをiiずつversion回だけずらして、重なりを判定 */
  for(version=0; version < (signed)n_renaming_versions; ++version) {
    int renamed_def_cycle_reg2, renamed_use_cycle_reg2;
    renamed_def_cycle_reg2 = def_cycle_reg2 + version * iteration_interval;
    renamed_use_cycle_reg2 = use_cycle_reg2 + version * iteration_interval;

    /* reg1がdeadである領域に,reg2がおさまっていれば良い */
    if (use_cycle_reg1 < renamed_def_cycle_reg2 &&
        renamed_use_cycle_reg2 <= full_kernel_cycles) {
      offset_value = renamed_def_cycle_reg2 - use_cycle_reg1;
      assert(offset_value >= 1);
      assert(offset_value <= (signed)iteration_interval);
      /* 条件に最初に適合するoffsetが最小のoffsetのためreturn */
      return offset_value;
    }
  }
  return offset_value;
}

/// \brief reg1の空領域に、reg2を配置できるかどうかを
///        modulo cycleとrenamingを考慮して判定し、
///        可能ならreg1->use_cycleからの最小のoffset値を返す
///        不可能なら0を返却する.
/// \retval reg2を配置可能なreg1->use_cycleからの最小のoffset値
unsigned SwplRegEstimate::getOffsetAllocationBackward(SwplRegEstimate::Renamed_Reg *reg1,
                                                      SwplRegEstimate::Renamed_Reg *reg2,
                                                      unsigned iteration_interval,
                                                      unsigned n_renaming_versions) {
  unsigned version;
  int dead_cycle_reg1, live_cycle_reg2;
  int full_kernel_cycles;
  int def_cycle_reg1, use_cycle_reg1, def_cycle_reg2, use_cycle_reg2;
  int offset_value = 0;

  full_kernel_cycles = n_renaming_versions * iteration_interval;  

  /* def_cycle_reg2 <= def_cycle_reg1をみたすように正規化 */
  normalizeDefUseCycle(reg2, reg1,
                       &def_cycle_reg2, &use_cycle_reg2,
                       &def_cycle_reg1, &use_cycle_reg1,
                       iteration_interval);

  dead_cycle_reg1 = full_kernel_cycles - (use_cycle_reg1 - def_cycle_reg1 + 1);
  live_cycle_reg2 = use_cycle_reg2 - def_cycle_reg2 + 1;

  if(dead_cycle_reg1 < live_cycle_reg2) {
    /* どのようにずらしても隙間に入らないため */
    return offset_value;
  }

  /* reg2のcycleをiiずつversion回だけ前にずらして、重なりを判定 */
  for(version=0; version < n_renaming_versions; ++version) {
    int renamed_def_cycle_reg2, renamed_use_cycle_reg2;
    renamed_def_cycle_reg2 = def_cycle_reg2 - version * iteration_interval;
    renamed_use_cycle_reg2 = use_cycle_reg2 - version * iteration_interval;

    /* reg1がdeadである領域に,reg2がおさまっていれば良い */
    if (use_cycle_reg1 - full_kernel_cycles + 1 <= renamed_def_cycle_reg2 &&
        renamed_use_cycle_reg2  < def_cycle_reg1) {
      offset_value = def_cycle_reg1 - renamed_use_cycle_reg2;
      assert(offset_value >= 1);
      assert(offset_value <= (signed)iteration_interval);
      /* 条件に最初に適合するoffsetが最小のoffsetのためreturn */
      return offset_value;
    }
  }
  return offset_value;
}

/// \brief renamed_regのlive rangeを前方に延す処理
/// \note 詳細はextendRenamedRegLivesForwardを参照
void SwplRegEstimate::extendRenamedRegLivesBackward(RenamedRegVector* vector_renamed_regs,
                                                    unsigned iteration_interval,
                                                    unsigned n_renaming_versions) {
  int const init_use_offset = iteration_interval + 1;
  int full_kernel_cycles = n_renaming_versions * iteration_interval;

  for( auto reg1 : *vector_renamed_regs ) {
    int min_use_offset;
    min_use_offset = init_use_offset;

    if (reg1->is_recurrence && n_renaming_versions == 2) { continue; }
  
    /* reg2を配置するすきまがないため */
    if (reg1->use_cycle - reg1->def_cycle + 1 == full_kernel_cycles) { continue; }
    
    for( auto reg2 : *vector_renamed_regs ) {
      /* 自身との重なりをみるため、indx,indx2は同じ場合もチェックする */

      /* 空き領域に配置可能か */
      int use_offset = getOffsetAllocationBackward(reg1, reg2, iteration_interval,
                                                   n_renaming_versions);
      if(use_offset != 0) {
        /* reg1->use_cycleより大きくかつ, 最もreg1->use_cycleに近い値を取得 */
        if(min_use_offset > use_offset) {
          min_use_offset = use_offset;
          if(min_use_offset == 1) {
            /* 最小のoffsetであるため */
            break;
          }
        }
      }
    }
  
    if(min_use_offset == 1) {
      /* use直後で、別のrenamed_regで使うことが可能なためuseは延ばせない. */
      continue;
    } else{
      int old_live_range, new_live_range;
      old_live_range = reg1->use_cycle - reg1->def_cycle + 1;
    
      if(min_use_offset == init_use_offset) {
        /* forwardの場合と同様に延す */
        if( old_live_range * n_renaming_versions > full_kernel_cycles *(n_renaming_versions/2) ) {
          new_live_range = full_kernel_cycles;
        } else {
          new_live_range = ((old_live_range + iteration_interval - 1)/iteration_interval)*iteration_interval;
        }
      } else {
        new_live_range = old_live_range + min_use_offset - 1;
      }
      reg1->def_cycle = reg1->use_cycle - (new_live_range - 1);
    
    }
  }
  return;
}

/// \brief live rangeの空き領域において, 他のrenamed_regを
///        配置できないcycleをみつけ、live rangeを延ばす
///
/// ```
///   例として、ii=3,n_renaming_versions=3の場合を考える.
///   reg1は、6-9cycleが空いており、reg2(version=0),
///   reg3(version=2)を埋めることが可能である.
///   したがって、RAでreg1とreg2またはreg1とreg2は同じレジスタを
///   割り付けることが可能である.
///   ここで、reg1の6cycle目は、reg2,reg3のいずれにおいても
///   使用されることがないため、reg1のuse_cycleを後ろへ延ばす.
///   reg2の9cycle目も同様に、use_cycleを延す.
///   互いにすべての配置を比較して影響を与えない範囲に限定して、
///   use_cycleを延すため、regの順序は問わない.
///
///   1<------->9
///   reg1 *** **- ---  def:1, use:5
///   reg2 --- --- **-  def:7, use:8
///   reg3 -** --- ---  def:2, use:3
///   122 110 110 => 342 
///
///   1<------->9
///   reg1 *** *** ---  def:1, use:6
///   reg2 --- --- ***  def:7, use:8
///   reg3 -** --- ---  def:2, use:3
///   122 111 111 => 344
///
///   さらに、def側にlive rangeを延すことを考えると
///   reg3の1cycle目を延すことが可能である。
///   後ろ側へは、extendRenamedRegLivesBackwardで実施する.
///
///   1<------->9
///   reg1 *** *** ---  def:1, use:6
///   reg2 --- --- ***  def:7, use:8
///   reg3 *** --- ---  def:2, use:3
///   222 111 111 => 444
/// ```
/// 
void SwplRegEstimate::extendRenamedRegLivesForward(RenamedRegVector* vector_renamed_regs,
                                                   unsigned iteration_interval,
                                                   unsigned n_renaming_versions) {
  int const init_def_offset = iteration_interval + 1;
  int full_kernel_cycles = n_renaming_versions * iteration_interval;

  for( auto reg1 : *vector_renamed_regs ) {
    int min_def_offset;
    min_def_offset = init_def_offset;

    if (reg1->is_recurrence && n_renaming_versions == 2) { continue; }
  
    /* reg2を配置するすきまがないため */
    if (reg1->use_cycle - reg1->def_cycle + 1 == full_kernel_cycles) { continue; }
    
    for( auto reg2 : *vector_renamed_regs ) {
      /* 自身との重なりをみるため、indx,indx2は同じ場合もチェックする */

      /* 空き領域に配置可能か */
      int def_offset = getOffsetAllocationForward(reg1, reg2, iteration_interval,
                                                  n_renaming_versions);
      if(def_offset != 0) {
        /* reg1->use_cycleより大きくかつ, 最もreg1->use_cycleに近い値を取得 */
        if(min_def_offset > def_offset) {
          min_def_offset = def_offset;
          if(min_def_offset == 1) {
            /* 最小のoffsetであるため */
            break;
          }
        }
      }
    }
  
    if(min_def_offset == 1) {
      /* use直後で、別のrenamed_regで使うことが可能なためuseは延ばせない. */
      continue;
    } else{
      int old_live_range, new_live_range;
      old_live_range = reg1->use_cycle - reg1->def_cycle + 1;
    
      if(min_def_offset == init_def_offset) {
        /* 自身も含めて他のrenamed_regが隙間に配置不可能であるため、live rangeを延す.
         * iteration_intervalを超えてlive rangeを延すと、reg1 == reg2の場合に、
         * 新しいversionとの重なりが生成されてしまう.
         */
        if( old_live_range * n_renaming_versions > full_kernel_cycles *(n_renaming_versions/2) ) {
          /* reg1==reg2の場合に,他のversionと既に重なっている場合,限界まで延す */
          new_live_range = full_kernel_cycles;
        } else {
          /* 自身との過剰な重なりを作らないために、iteration_intervalを超えない範囲まで延す */
          new_live_range = ((old_live_range + iteration_interval - 1)/iteration_interval)*iteration_interval;
        }
      } else {
        new_live_range = old_live_range + min_def_offset - 1;
      }
      reg1->use_cycle = reg1->def_cycle + new_live_range - 1;
    
    }
  }
  return;
}

/// \brief reg_counterのでliveなcycleをincrementする
/// \note  begin_cycle, end_cycle は両方カウントされる。
///        cycle のどこで使われるか判らないから。
void SwplRegEstimate::incrementCounters(int reg_size, std::vector<int>* reg_counters,
                                        unsigned iteration_interval,
                                        unsigned begin_cycle, unsigned end_cycle) {
  unsigned cycle;
  for (cycle = begin_cycle; cycle <= end_cycle; ++cycle) {
    int counter;
    unsigned modulo_cycle;

    modulo_cycle = cycle % iteration_interval;
    counter = (*reg_counters)[modulo_cycle];
    (*reg_counters)[modulo_cycle] = (counter + reg_size);
  }
  return;
}

/// \brief 各cycle毎のbusy数の最大値を見積もり結果として取得する
unsigned SwplRegEstimate::getEstimateResult(RenamedRegVector* vector_renamed_regs,
                                            std::vector<int>* reg_counters,
                                            unsigned iteration_interval) {
  unsigned indx;
  unsigned max_counter, counter;

  for( auto rreg : *vector_renamed_regs ) {
    incrementCounters (rreg->reg->getRegSize(),
                        reg_counters,
                        iteration_interval,
                        rreg->def_cycle, rreg->use_cycle);
  }

  max_counter = 0;
  for (indx=0; indx < iteration_interval; ++indx) {
    counter = (*reg_counters)[indx];
    if(max_counter < counter) { max_counter = counter; }
  }
  return max_counter;

}

/// \brief live_range_mask以上のlive rangeをもつレジスタの数を求める
unsigned SwplRegEstimate::countLivesWithMask(RenamedRegVector* vector_renamed_regs,
                                             unsigned live_range_mask) {
  unsigned count_lives = 0;

  /* 最大、最小のcycleを収集 */
  for( auto rreg : *vector_renamed_regs ) {
    unsigned live_range;
    live_range = rreg->use_cycle - rreg->def_cycle + 1;
    if(live_range >= live_range_mask) {
      count_lives += rreg->reg->getRegSize();
    }
  }

  return count_lives;
}

/// \brief live_range_mask以上のlive rangeをもつレジスタの最大重なり数を求める
unsigned SwplRegEstimate::countOverlapsWithMask(RenamedRegVector* vector_renamed_regs,
                                                unsigned live_range_mask,
                                                unsigned n_renaming_versions) {
  unsigned max_cycle, min_cycle;
  unsigned pattern_max_counter;

  min_cycle = SwplSlot::slotMax().calcCycle();
  max_cycle = SwplSlot::slotMin().calcCycle();

  /* 最大、最小のcycleを収集 */
  for( auto rreg : *vector_renamed_regs ) {
    unsigned live_range;
    live_range = rreg->use_cycle - rreg->def_cycle + 1;
    if(live_range <= live_range_mask || rreg->is_recurrence == true) { continue; }
    if((signed)max_cycle < rreg->use_cycle) { max_cycle = rreg->use_cycle; }
    if((signed)min_cycle > rreg->def_cycle) { min_cycle = rreg->def_cycle; }
  }

  if (max_cycle <= min_cycle) {
    /* 該当するregが無いため中断*/
    return 0;
  }

  /* 各renamed_regのcycle毎の生存数を積み上げる*/
  std::vector<int> tmp_reg_counters( (max_cycle - min_cycle + 1), 0 ); /* cycle毎の生存数 */
  for( auto rreg : *vector_renamed_regs ) {
    int live, live_range;
    live_range = rreg->use_cycle - rreg->def_cycle + 1;
  
    if(live_range <= (signed)live_range_mask || rreg->is_recurrence == true) { continue; }
  
    for (live=rreg->def_cycle - min_cycle; live < live_range; ++ live) {
      tmp_reg_counters[live] = (tmp_reg_counters[live] + 1);
    }
  }

  pattern_max_counter = 0;      
  for( auto counter : tmp_reg_counters ) {
    if((signed)pattern_max_counter < counter) { pattern_max_counter = counter; }
  }

  return pattern_max_counter;
}

/// \brief iiに対するlive rangeの重なりから、必要レジスタ数をパターンマッチ
/// \note 必要レジスタ数はheuristicな条件で決めている
/// \note OoOが豊富な場合、SWPLの重なりを緩くしても問題ないため、
///       大きなMVEに対して過剰に見積もる事は許容される
unsigned SwplRegEstimate::getNumPatternRegs (RenamedRegVector* vector_renamed_regs,
                                             unsigned iteration_interval,
                                             unsigned n_renaming_versions, unsigned regclassid) {
  unsigned live_range_mask;
  unsigned live_overlaps;
  unsigned pattern_max_counter = 0;

  /* live_range > ii/2のケース
   * ii/2を越える場合に、SwplRegはレジスタを共有できないため、
   * renamed_regの全versionに1つのレジスタを割り当てることを前提に, レジスタ数が予測できる.
   * ただし、SwplRegが複数のレジスタを割り当てられる場合があり、
   * その場合に対しては、過剰な見積もりになっている.
   * 例えば、2/3*iiのrenamed_regが3つで、互いに1/3*iiずれている場合、
   * 2つのレジスタで割り付ける事ができる.
   * \note 効果があるケースが見つかっていないため不要と思われる
   */
  {
    live_range_mask = (iteration_interval+1)/2;
    pattern_max_counter = countLivesWithMask (vector_renamed_regs, live_range_mask);
  }

  /* live_range > iiのケース
   * 自身の隣接するversionと重なるため、renamed_regに対して複数レジスタが必要になる
   * ただし、別のrenamed_regと共有する事が可能であるため、2で割る
   */
  {
    live_range_mask = iteration_interval;
    live_overlaps = countOverlapsWithMask (vector_renamed_regs, live_range_mask,
                                           n_renaming_versions);

    /* 証明できていないが、傾向はあっている思われる*/
    if(n_renaming_versions%2 == 1) {
      /* 奇数の場合、一つのrenamed_regに対し少なくとも3個が必要であり、
       * 互いに共有できる場合があるため.
       */
      pattern_max_counter = std::max(pattern_max_counter,
                                     3 * ((live_overlaps+1)/2));
    } else {
      /* 奇数の場合、一つのrenamed_regに対し少なくとも2個が必要*/
      pattern_max_counter = std::max(pattern_max_counter,
                                     2 * ((live_overlaps+1)/2));
    }

    /* MVE==5の場合は、spillがおきやすい傾向にあるため多めに見積る */
    if( llvm::StmRegKind::isPredicate(regclassid) ) {
      if (n_renaming_versions == 5) {
        pattern_max_counter = std::max(pattern_max_counter,
                                       3 * live_overlaps);
      }
    }
  }

  /* live_range > (2*ii) のケース
   * MVEが4以下の場合は, pattern以外の見積もりで十分と判断した
   */
  {
    if(n_renaming_versions >= 5) {
      live_range_mask = iteration_interval * 2;
      live_overlaps = countOverlapsWithMask (vector_renamed_regs, live_range_mask,
                                             n_renaming_versions);
      switch (n_renaming_versions) {
      case 5:
        /* 前後2version重なり、全体が重なりあうため、5個レジスタが必要 
         * live_overlapsが少数の場合は、妥当な見積もりと思われる
         */
        pattern_max_counter = std::max(pattern_max_counter,
                                       5 * live_overlaps );
        break;
      default:
        // \note MVE=5以外の場合、明確な数は不明
        break;
      }
    }
  }

  /* tmp_reg_countersは、呼出し元で解放 */
  return pattern_max_counter;
}

/// \brief 干渉と割付けを意識してlive rangeを伸ばしながら見積もりを実施する
/// \note getNumMortalRegsよりかならず大きい結果を返却する
unsigned SwplRegEstimate::getNumExtendedRegs (RenamedRegVector* vector_renamed_regs,
                                              const SwplSlots* slots,
                                              unsigned iteration_interval,
                                              unsigned regclassid,
                                              unsigned n_renaming_versions) {
  std::vector<int> reg_counters( iteration_interval, 0 );
  int max_counter;

  /* 見積もりのためlive rangeの延長 */
  extendRenamedRegLivesForward(vector_renamed_regs,
                               iteration_interval, n_renaming_versions);
  extendRenamedRegLivesBackward(vector_renamed_regs,
                                iteration_interval, n_renaming_versions);
  max_counter = getEstimateResult(vector_renamed_regs,
                                  &reg_counters, iteration_interval);
  return max_counter;
}

/// \brief SwplRegが見積り時の対象となるかどうか
/// \note getNumMortalRegs、getNumInterferedRegsで用いる
bool SwplRegEstimate::isCountedReg(const SwplReg& reg, unsigned regclassid) {
  if ( reg.isRegNull() ) {
    return false;
  }
  if( !reg.isSameKind(regclassid) ) {
    return false;
  }
  if (reg.getPredecessor() != nullptr) {
    /* 既に割り当てられた reg を使うだけなので、計算しない。*/
    return false;
  }
  if ( reg.isStack() ) {
    return false;
  }
  return true;
}

/// \brief int_Vector内の最大の値を取得する
int SwplRegEstimate::findMaxCounter(std::vector<int>* reg_counters, unsigned iteration_interval) {
  int max_counter;
  unsigned modulo_cycle;

  /* Find max */
  max_counter = 0;
  for (modulo_cycle = 0; modulo_cycle < iteration_interval; ++modulo_cycle) {
    int counter;
    counter = (*reg_counters)[modulo_cycle];
    max_counter = std::max(max_counter, counter);
  }
  return max_counter;
}

}
