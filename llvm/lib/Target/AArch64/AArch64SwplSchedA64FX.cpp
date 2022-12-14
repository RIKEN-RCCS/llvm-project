//=- AArch64SwplSchedA64FX.cpp - SchedModel for SWP --*- C++ -*----=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SchedModel for SWP.
//
//===----------------------------------------------------------------------===//
#include "AArch64SwplSchedA64FX.h"
using namespace llvm;

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::getRes(const MachineInstr &mi){
    // @todo:実装
    return INT_OP_001;
}
const StmPipelinesImpl *AArch64SwplSchedA64FX::getPipelines(ResourceID id){
    // @todo:実装
    return nullptr;
}