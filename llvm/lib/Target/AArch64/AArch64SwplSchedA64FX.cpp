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
#include "../../CodeGen/SWPipeliner.h"
using namespace llvm;

/// INT_OP
StmPipeline RES_INT_OP_001 = {{0}, {A64FXRes::P_EXA}};
StmPipeline RES_INT_OP_002 = {{0}, {A64FXRes::P_EXB}};
StmPipeline RES_INT_OP_003 = {{0}, {A64FXRes::P_EAGA}};
StmPipeline RES_INT_OP_004 = {{0}, {A64FXRes::P_EAGB}};

/// SIMDFP_SVE_OP
StmPipeline RES_SIMDFP_SVE_OP_001 = {{0}, {A64FXRes::P_FLA}};
StmPipeline RES_SIMDFP_SVE_OP_002 = {{0}, {A64FXRes::P_FLB}};

/// SIMDFP_SVE_LD
StmPipeline RES_SIMDFP_SVE_LD_001 = {
  {0, 0},
  {A64FXRes::P_EAGA, A64FXRes::P_LSU1}};
StmPipeline RES_SIMDFP_SVE_LD_002 = {
  {0, 0},
  {A64FXRes::P_EAGB, A64FXRes::P_LSU1}};
StmPipeline RES_SIMDFP_SVE_LD_003 = {
  {0, 0},
  {A64FXRes::P_EAGA, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_LD_004 = {
  {0, 0},
  {A64FXRes::P_EAGB, A64FXRes::P_LSU2}};

StmPipeline RES_SIMDFP_SVE_LD_005 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_EXA_C,A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_006 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_EXB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_007 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_EAGA, A64FXRes::P_EAGA_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_008 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_EAGB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_009 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_EXA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_010 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_EXB_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_011 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_EAGA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_012 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_EAGB, A64FXRes::P_EAGB_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_013 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXA, A64FXRes::P_LSU2, A64FXRes::P_EXA_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_014 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXB, A64FXRes::P_LSU2, A64FXRes::P_EXB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_015 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_LSU2, A64FXRes::P_EAGA, A64FXRes::P_EAGA_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_016 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EAGB, A64FXRes::P_LSU2, A64FXRes::P_EAGB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_017 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXA, A64FXRes::P_LSU2, A64FXRes::P_EXA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_018 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXB, A64FXRes::P_LSU2, A64FXRes::P_EXB_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_019 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EAGA, A64FXRes::P_LSU2, A64FXRes::P_EAGA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_020 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_LSU2, A64FXRes::P_EAGB, A64FXRes::P_EAGB_C, A64FXRes::P_EAGB_C}};

StmPipeline RES_SIMDFP_SVE_LD_021 = {
    {0, 0, 8}, {A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_022 = {
    {0, 0, 8}, {A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_023 = {
    {0, 0, 8}, {A64FXRes::P_EAGA, A64FXRes::P_LSU2, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_024 = {
    {0, 0, 8}, {A64FXRes::P_EAGB, A64FXRes::P_LSU2, A64FXRes::P_EAGB_C}};

/// SIMDFP_SVE_ST
StmPipeline RES_SIMDFP_SVE_ST_001 = {
    {0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_002 = {
    {0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};

StmPipeline RES_SIMDFP_SVE_ST_003 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_004 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_005 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_006 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_007 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_008 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};

/// PREDICATE_OP
StmPipeline RES_PREDICATE_OP_001 = {{0}, {A64FXRes::P_PRX}};

/// 利用資源IDと利用資源情報のmap
static std::map<AArch64SwplSchedA64FX::ResourceID,
                AArch64SwplSchedA64FX::SchedResource>
  ResInfo{
    {AArch64SwplSchedA64FX::MI_INT_OP_001,  /// EX* | EAG*
      {{&RES_INT_OP_001, &RES_INT_OP_002, &RES_INT_OP_003, &RES_INT_OP_004},
      1}},
    {AArch64SwplSchedA64FX::MI_INT_OP_002,  /// EX*
      {{&RES_INT_OP_001, &RES_INT_OP_002}, 1}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_001,  /// FL*
      {{&RES_SIMDFP_SVE_OP_001, &RES_SIMDFP_SVE_OP_002}, 9}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_002,  /// FL*
      {{&RES_SIMDFP_SVE_OP_001, &RES_SIMDFP_SVE_OP_002}, 4}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_003,  /// FLA
      {{&RES_SIMDFP_SVE_OP_001}, 9}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_001,  /// EAG*, FLA
      {{&RES_SIMDFP_SVE_LD_001, &RES_SIMDFP_SVE_LD_002,
        &RES_SIMDFP_SVE_LD_003, &RES_SIMDFP_SVE_LD_004},
      11}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_002,  /// EAG*, FLA / EX*| EAG*
      {{&RES_SIMDFP_SVE_LD_005, &RES_SIMDFP_SVE_LD_006,
        &RES_SIMDFP_SVE_LD_007, &RES_SIMDFP_SVE_LD_008,
        &RES_SIMDFP_SVE_LD_009, &RES_SIMDFP_SVE_LD_010,
        &RES_SIMDFP_SVE_LD_011, &RES_SIMDFP_SVE_LD_012,
        &RES_SIMDFP_SVE_LD_013, &RES_SIMDFP_SVE_LD_014,
        &RES_SIMDFP_SVE_LD_015, &RES_SIMDFP_SVE_LD_016,
        &RES_SIMDFP_SVE_LD_017, &RES_SIMDFP_SVE_LD_018,
        &RES_SIMDFP_SVE_LD_019, &RES_SIMDFP_SVE_LD_020},
      8}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_003,  /// EAG*
      {{&RES_SIMDFP_SVE_LD_021, &RES_SIMDFP_SVE_LD_022,
        &RES_SIMDFP_SVE_LD_023, &RES_SIMDFP_SVE_LD_024},
      8}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_001,  /// EAG*, FLA
      {{&RES_SIMDFP_SVE_ST_001, &RES_SIMDFP_SVE_ST_002}, 1}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_002,  /// EAG*, FLA / EX*| EAG*
      {{&RES_SIMDFP_SVE_ST_003, &RES_SIMDFP_SVE_ST_004,
        &RES_SIMDFP_SVE_ST_005, &RES_SIMDFP_SVE_ST_006,
        &RES_SIMDFP_SVE_ST_007, &RES_SIMDFP_SVE_ST_008},
      1}},
    {AArch64SwplSchedA64FX::MI_PREDICATE_OP_001,  /// PRX
      {{&RES_PREDICATE_OP_001}, 1}}
  };

/// 命令と利用資源IDのmap
static std::map<unsigned int, AArch64SwplSchedA64FX::ResourceID> MIOpcodeInfo{
  // Base
  {AArch64::ADDXri, AArch64SwplSchedA64FX::MI_INT_OP_001},
  {AArch64::ADDXrr, AArch64SwplSchedA64FX::MI_INT_OP_001},
  {AArch64::SUBSXri, AArch64SwplSchedA64FX::MI_INT_OP_001},
  {AArch64::SUBSXrr, AArch64SwplSchedA64FX::MI_INT_OP_002},
  {AArch64::SUBXri, AArch64SwplSchedA64FX::MI_INT_OP_001},
  // SIMD&FP
  {AArch64::FADDDrr, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDSrr, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_001},

  {AArch64::LDURDi, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_003},
  {AArch64::LDURSi, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_003},

  {AArch64::STRDpost, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_002},
  {AArch64::STRSpost, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_002},
  {AArch64::STURDi, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_001},
  {AArch64::STURSi, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_001},
  // SVE
  {AArch64::FADD_ZPZI_UNDEF_D, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_003},
  {AArch64::FADD_ZPZI_UNDEF_S, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_003},

  {AArch64::LD1D, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_001},
  {AArch64::LDRDpost, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSpost, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_002},

  {AArch64::ST1D, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W, AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_001},
};

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::getRes(
  const MachineInstr &mi) const {
  ResourceID id = AArch64SwplSchedA64FX::searchRes(mi);
  return id;
}

const StmPipelinesImpl *AArch64SwplSchedA64FX::getPipelines(
  ResourceID id) const {
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

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchRes(
  const MachineInstr &mi) {
  auto it = MIOpcodeInfo.find(mi.getOpcode());
  if (it != MIOpcodeInfo.end()) {
    return it->second;
  }

  // COPY命令はレジスタ種別で利用する資源が異なるため
  // レジスタ種別ごとに代表的なレイテンシ・演算器で定義する
  if (mi.getOpcode() == AArch64::COPY) {
      Register r = mi.getOperand(0).getReg();
      StmRegKind *rkind = SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, r);
      if (rkind->isFloating()) {
        return MI_SIMDFP_SVE_OP_002;
      } else if (rkind->isPredicate()) {
        return MI_PREDICATE_OP_001;
      } else {
        return MI_INT_OP_001;
      }
  }
  // @todo コントロールオプションの処理
  // @todo VLの処理
  return AArch64SwplSchedA64FX::NA;
}
