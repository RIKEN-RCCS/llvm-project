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
StmPipeline RES_INT_OP_001_01 = {{0}, {A64FXRes::P_EXA}};
StmPipeline RES_INT_OP_001_02 = {{0}, {A64FXRes::P_EXB}};
StmPipeline RES_INT_OP_001_03 = {{0}, {A64FXRes::P_EAGA}};
StmPipeline RES_INT_OP_001_04 = {{0}, {A64FXRes::P_EAGB}};
StmPipeline RES_INT_OP_002_01 = {{0}, {A64FXRes::P_EXA}};
StmPipeline RES_INT_OP_002_02 = {{0}, {A64FXRes::P_EXB}};

/// SIMDFP_SVE_OP
StmPipeline RES_SIMDFP_SVE_OP_001_01 = {{0}, {A64FXRes::P_FLA}};
StmPipeline RES_SIMDFP_SVE_OP_001_02 = {{0}, {A64FXRes::P_FLB}};
StmPipeline RES_SIMDFP_SVE_OP_002_01 = {{0}, {A64FXRes::P_FLA}};
StmPipeline RES_SIMDFP_SVE_OP_002_02 = {{0}, {A64FXRes::P_FLB}};
StmPipeline RES_SIMDFP_SVE_OP_003_01 = {{0}, {A64FXRes::P_FLA}};

/// SIMDFP_SVE_LD
StmPipeline RES_SIMDFP_SVE_LD_001_01 = {
  {0, 0},
  {A64FXRes::P_EAGA, A64FXRes::P_LSU1}};
StmPipeline RES_SIMDFP_SVE_LD_001_02 = {
  {0, 0},
  {A64FXRes::P_EAGB, A64FXRes::P_LSU1}};
StmPipeline RES_SIMDFP_SVE_LD_001_03 = {
  {0, 0},
  {A64FXRes::P_EAGA, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_LD_001_04 = {
  {0, 0},
  {A64FXRes::P_EAGB, A64FXRes::P_LSU2}};

StmPipeline RES_SIMDFP_SVE_LD_002_01 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_EXA_C,A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_02 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_EXB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_03 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_EAGA, A64FXRes::P_EAGA_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_04 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_EAGB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_05 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_EXA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_06 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_EXB_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_07 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_EAGA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_08 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_EAGB, A64FXRes::P_EAGB_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_09 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXA, A64FXRes::P_LSU2, A64FXRes::P_EXA_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_10 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EXB, A64FXRes::P_LSU2, A64FXRes::P_EXB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_11 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_LSU2, A64FXRes::P_EAGA, A64FXRes::P_EAGA_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_12 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGA, A64FXRes::P_EAGB, A64FXRes::P_LSU2, A64FXRes::P_EAGB_C, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_13 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXA, A64FXRes::P_LSU2, A64FXRes::P_EXA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_14 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EXB, A64FXRes::P_LSU2, A64FXRes::P_EXB_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_15 = {
    {0, 0, 0, 1, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_EAGA, A64FXRes::P_LSU2, A64FXRes::P_EAGA_C, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_002_16 = {
    {0, 0, 1, 2, 8},
    {A64FXRes::P_EAGB, A64FXRes::P_LSU2, A64FXRes::P_EAGB, A64FXRes::P_EAGB_C, A64FXRes::P_EAGB_C}};

StmPipeline RES_SIMDFP_SVE_LD_003_01 = {
    {0, 0, 8}, {A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_003_02 = {
    {0, 0, 8}, {A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_EAGB_C}};
StmPipeline RES_SIMDFP_SVE_LD_003_03 = {
    {0, 0, 8}, {A64FXRes::P_EAGA, A64FXRes::P_LSU2, A64FXRes::P_EAGA_C}};
StmPipeline RES_SIMDFP_SVE_LD_003_04 = {
    {0, 0, 8}, {A64FXRes::P_EAGB, A64FXRes::P_LSU2, A64FXRes::P_EAGB_C}};

/// SIMDFP_SVE_ST
StmPipeline RES_SIMDFP_SVE_ST_001_01 = {
    {0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_001_02 = {
    {0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};

StmPipeline RES_SIMDFP_SVE_ST_002_01 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_002_02 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_002_03 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGA, A64FXRes::P_FLA, A64FXRes::P_EAGB, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_002_04 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_EXA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_002_05 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_EXB, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};
StmPipeline RES_SIMDFP_SVE_ST_002_06 = {
    {0, 0, 0, 0, 0},
    {A64FXRes::P_EAGB, A64FXRes::P_FLA, A64FXRes::P_EAGA, A64FXRes::P_LSU1, A64FXRes::P_LSU2}};

/// PREDICATE_OP
StmPipeline RES_PREDICATE_OP_001_01 = {{0}, {A64FXRes::P_PRX}};

/// 利用資源IDと利用資源情報のmap
static std::map<AArch64SwplSchedA64FX::ResourceID,
                AArch64SwplSchedA64FX::SchedResource>
  ResInfo{
    {AArch64SwplSchedA64FX::MI_INT_OP_001,  /// EX* | EAG*
      {{&RES_INT_OP_001_01, &RES_INT_OP_001_02, &RES_INT_OP_001_03, &RES_INT_OP_001_04},
      1}},
    {AArch64SwplSchedA64FX::MI_INT_OP_002,  /// EX*
      {{&RES_INT_OP_002_01, &RES_INT_OP_002_02}, 1}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_001,  /// FL*
      {{&RES_SIMDFP_SVE_OP_001_01, &RES_SIMDFP_SVE_OP_002_02}, 9}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_002,  /// FL*
      {{&RES_SIMDFP_SVE_OP_002_01, &RES_SIMDFP_SVE_OP_002_02}, 4}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_OP_003,  /// FLA
      {{&RES_SIMDFP_SVE_OP_003_01}, 9}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_001,  /// EAG*, FLA
      {{&RES_SIMDFP_SVE_LD_001_01, &RES_SIMDFP_SVE_LD_001_02,
        &RES_SIMDFP_SVE_LD_001_03, &RES_SIMDFP_SVE_LD_001_04},
      11}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_002,  /// EAG*, FLA / EX*| EAG*
      {{&RES_SIMDFP_SVE_LD_002_01, &RES_SIMDFP_SVE_LD_002_02,
        &RES_SIMDFP_SVE_LD_002_03, &RES_SIMDFP_SVE_LD_002_04,
        &RES_SIMDFP_SVE_LD_002_05, &RES_SIMDFP_SVE_LD_002_06,
        &RES_SIMDFP_SVE_LD_002_07, &RES_SIMDFP_SVE_LD_002_08,
        &RES_SIMDFP_SVE_LD_002_09, &RES_SIMDFP_SVE_LD_002_10,
        &RES_SIMDFP_SVE_LD_002_11, &RES_SIMDFP_SVE_LD_002_12,
        &RES_SIMDFP_SVE_LD_002_13, &RES_SIMDFP_SVE_LD_002_14,
        &RES_SIMDFP_SVE_LD_002_15, &RES_SIMDFP_SVE_LD_002_16},
      8}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_LD_003,  /// EAG*
      {{&RES_SIMDFP_SVE_LD_003_01, &RES_SIMDFP_SVE_LD_003_02,
        &RES_SIMDFP_SVE_LD_003_03, &RES_SIMDFP_SVE_LD_003_04},
      8}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_001,  /// EAG*, FLA
      {{&RES_SIMDFP_SVE_ST_001_01, &RES_SIMDFP_SVE_ST_001_02}, 1}},
    {AArch64SwplSchedA64FX::MI_SIMDFP_SVE_ST_002,  /// EAG*, FLA / EX*| EAG*
      {{&RES_SIMDFP_SVE_ST_002_01, &RES_SIMDFP_SVE_ST_002_02,
        &RES_SIMDFP_SVE_ST_002_03, &RES_SIMDFP_SVE_ST_002_04,
        &RES_SIMDFP_SVE_ST_002_05, &RES_SIMDFP_SVE_ST_002_06},
      1}},
    {AArch64SwplSchedA64FX::MI_PREDICATE_OP_001,  /// PRX
      {{&RES_PREDICATE_OP_001_01}, 1}}
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
  return &::ResInfo.at(id).pipelines;
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
  return ::ResInfo.at(id).latency;
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
