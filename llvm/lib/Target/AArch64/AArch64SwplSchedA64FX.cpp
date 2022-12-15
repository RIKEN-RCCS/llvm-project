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
#include "AArch64InstrInfo.h"
using namespace llvm;

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::getRes(const MachineInstr &mi) const {
    // @todo:実装
    return INT_OP_001;
}

const StmPipelinesImpl *AArch64SwplSchedA64FX::getPipelines(ResourceID id) const {
    // @todo:実装
    return nullptr;
}

bool AArch64SwplSchedA64FX::isPseudo(const MachineInstr &mi) const {
    // アセンブラ出力時までに無くなる可能性が大きいものはPseudoとして扱う
    switch (mi.getOpcode()) {
    case AArch64::SUBREG_TO_REG:
    case AArch64::INSERT_SUBREG:
    case AArch64::REG_SEQUENCE:
      return true;
    }
    return false;
}

unsigned AArch64SwplSchedA64FX::getLatency(ResourceID id) const {
    // @todo:実装
    return 1;
}
