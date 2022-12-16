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
#include "AArch64SwplTargetMachine.h"
#include "AArch64SwplSchedA64FX.h"
#include "AArch64InstrInfo.h"
using namespace llvm;

StmPipeline INT_OP_001_01 = {{0},{A64FXRes::P_EXA}};
StmPipeline INT_OP_001_02 = {{0},{A64FXRes::P_EXB}};
StmPipeline INT_OP_001_03 = {{0},{A64FXRes::P_EAGA}};
StmPipeline INT_OP_001_04 = {{0},{A64FXRes::P_EAGB}};

/// 利用資源IDと利用資源情報のmap
static std::map<AArch64SwplSchedA64FX::ResourceID,AArch64SwplSchedA64FX::SchedResource> ResInfo{
  {AArch64SwplSchedA64FX::INT_OP_001,
    {{&INT_OP_001_01,&INT_OP_001_02,&INT_OP_001_03,&INT_OP_001_04},1}
  }
};

/// 命令と利用資源IDのmap
static std::map<unsigned int,AArch64SwplSchedA64FX::ResourceID> MIOpcodeInfo{
  {AArch64::ADDXri,AArch64SwplSchedA64FX::INT_OP_001}
};

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::getRes(const MachineInstr &mi) const {
  ResourceID id = AArch64SwplSchedA64FX::searchRes(mi);
  return id;
}

const StmPipelinesImpl *AArch64SwplSchedA64FX::getPipelines(ResourceID id) const {
  return &::ResInfo[id].pipelines;
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
  return ::ResInfo[id].latency;
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchRes(const MachineInstr &mi){
  auto it = MIOpcodeInfo.find(mi.getOpcode());
  if (it != MIOpcodeInfo.end()) {
    return it->second;
  }
  // @todo コントロールオプションの処理
  // @todo VLの処理
  return AArch64SwplSchedA64FX::NA;
}

