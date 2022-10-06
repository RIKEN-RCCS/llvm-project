//=- AArch64SwplCalclIterations.h - check Iterations in SWPL -*- C++ -*------=//
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

#ifndef LLVM_LIB_TARGET_AARCH64_AARCH64SWPLCALSITERATIONS_H
#define LLVM_LIB_TARGET_AARCH64_AARCH64SWPLCALSITERATIONS_H

#include "AArch64.h"
#include "AArch64SWPipeliner.h"
#include "AArch64SwplPlan.h"
#include "AArch64SwplScheduling.h"
#include "AArch64SwplTargetMachine.h"

namespace swpl{
extern  llvm::cl::opt<bool> DebugOutput;

#define ASSUMED_ITERATIONS_MAX (32767)
#define ASSUMED_ITERATIONS_NONE (-1)
#define OCL_ITERATIONS_UNSPECIFIED (-1)

/// \brief SwplのPlanを選択するための、ループ回転数に関するルーチン群
class SwplCalclIterations {
public:
  static bool preCheckIterationCount(const PlanSpec& spec, unsigned int *required_itr);
  static bool checkIterationCountVariable(const PlanSpec& spec, const MsResult& ms);
  static bool checkIterationCountConstant(const PlanSpec& spec, const MsResult& ms);
};

}
#endif // LLVM_LIB_TARGET_AARCH64_AARCH64SWPLCALSITERATIONS_H
