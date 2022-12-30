//=- SwplCalclIterations.h - check Iterations in SWPL -*- C++ -*-------------=//
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

#ifndef LLVM_LIB_CODEGEN_SWPLCALSITERATIONS_H
#define LLVM_LIB_CODEGEN_SWPLCALSITERATIONS_H


#include "SWPipeliner.h"
#include "SwplPlan.h"
#include "SwplScheduling.h"

namespace llvm {



/// \brief SwplのPlanを選択するための、ループ回転数に関するルーチン群
class SwplCalclIterations {
public:

  static bool preCheckIterationCount(const SwplPlanSpec & spec, unsigned int *required_itr);
  static bool checkIterationCountVariable(const SwplPlanSpec & spec, const SwplMsResult & ms);
  static bool checkIterationCountConstant(const SwplPlanSpec & spec, const SwplMsResult & ms);
};

}
#endif
