//=- AArch64DataDependenceAnalysis.h - SWPL DDG -*- c++ -*-------------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// 
/// AArch64 Data Dependency analysis Generation (SwplDdg)
//
//===----------------------------------------------------------------------===//
//=== Copyright FUJITSU LIMITED 2021  and FUJITSU LABORATORIES LTD. 2021   ===//
//===----------------------------------------------------------------------===//

#include "AArch64SWPipeliner.h"

bool isPrefetch(const swpl::SwplInst &inst);
