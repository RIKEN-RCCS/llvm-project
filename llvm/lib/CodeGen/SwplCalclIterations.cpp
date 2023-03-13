//=- SwplCalclIterations.cpp - check Iterations in SWPL -*- C++ -*-----------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Processing related to checking Iteration in SWPL.
//
//===----------------------------------------------------------------------===//

#include "SwplCalclIterations.h"
#include "SwplScheduling.h"


namespace llvm {

/// \brief スケジューリング結果が要する回転数が、実際の回転数（可変）を満たすかをチェックする
/// \note assumeに対応しないため、常にtrueを返却する
bool SwplCalclIterations::checkIterationCountVariable(const SwplPlanSpec & spec, const SwplMsResult & ms) {
  return true;
}

/// \brief スケジューリング結果が要する回転数が、実際の回転数（固定）を満たすかをチェックする
/// \details スケジューリング結果が要する回転数が、実際の回転数（固定）を満たすかをチェックする
///          実際の命令列上の回転数を優先する.
/// \retval true スケジューリング結果が要する回転数が、実際の回転数（固定）を満たす
/// \retval false スケジューリング結果が要する回転数が、実際の回転数（固定）を満たさない
bool SwplCalclIterations::checkIterationCountConstant(const SwplPlanSpec & spec, const SwplMsResult & ms) {
  return (ms.required_itr_count <= spec.itr_count);
}

/// \brief schedulingする前に、SWPLの最低条件を確認する
/// \param [in] spec スケジューリング指示情報
/// \param [in,out] required_itr 必要なループ回転数
/// \retval true 条件を満して、SWPL可能
/// \retval false 条件を満しておらず、SWPL不可能
/// \remark required_itrに、ソース上必要なループ回転数を設定する.
///         元ループの回転数が2回転以下の時, SWPL不可.
/// \note 2回転の時はschedulingしたいが,
///       現在の実装ではn_copies >=3 を要求する為,できていない.
///       この値(2回転) はis_iteration_count_constantでも利用している為そろえる事.
/// \note 現在、SwplPlanSpec.assumed_iterationは常にASSUMED_ITERATIONS_NONE (-1)
/// \note 現在、SwplPlanSpec.pre_expand_numは常に1
bool SwplCalclIterations::preCheckIterationCount(const SwplPlanSpec & spec, unsigned int *required_itr) {
  *required_itr = 0;

  unsigned int const minimum_n_copies = 3;
  *required_itr = spec.pre_expand_num * minimum_n_copies;

  /* 定数の場合、Pragma 指定よりも、実際のMIR上の数を優先する */
  if(spec.is_itr_count_constant) {
    if (spec.itr_count <= 2) {
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "        : SWPL is canceled for this loop whose iteration count < 3. \n";
      }
      return false;
    }
    return true; /* MIR上に定数がある場合は、見積値を参照しない. */
  } else {
    /* 見積り値 */
    int assumed_iterations = spec.assumed_iterations;
    if (assumed_iterations >= 0) {
      assert(assumed_iterations <= SwplPlanSpec::ASSUMED_ITERATIONS_MAX);
    }

    if( assumed_iterations >= 0 && *required_itr > (unsigned)assumed_iterations ) {
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "        : SWPL is canceled for this loop whose assumed iteration count < 3. \n";
      }
      return false;
    }
  }
  return true;
}

}
