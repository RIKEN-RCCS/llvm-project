//=- AArch64SwplSchedA64FX.h - SchedModel for SWP --*- C++ -*----=//
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
#ifndef TARGET_AARCH64_AARCH64SWPLSCHEDA64FX
#define TARGET_AARCH64_AARCH64SWPLSCHEDA64FX

#include "llvm/CodeGen/SwplTargetMachine.h"

namespace llvm {
struct AArch64SwplSchedA64FX{
    enum ResourceID {
        INT_OP_001,
        INT_OP_002
    };

    ResourceID getRes(const MachineInstr &mi) const;
    const StmPipelinesImpl *getPipelines(ResourceID id) const;
    unsigned getLatency(ResourceID id) const;
    bool isPseudo(const MachineInstr &mi) const;
};
}

#endif

