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
using P_=AArch64SwplSchedA64FX::PortKind;

/// INT_OP
static StmPipeline RES_INT_OP_001_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_001_02 = {{0}, {P_::EXB}};
static StmPipeline RES_INT_OP_001_03 = {{0}, {P_::EAGA}};
static StmPipeline RES_INT_OP_001_04 = {{0}, {P_::EAGB}};
static StmPipeline RES_INT_OP_002_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_002_02 = {{0}, {P_::EXB}};

/// SIMDFP_SVE_OP
static StmPipeline RES_SIMDFP_SVE_OP_001_01 = {{0}, {P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_001_02 = {{0}, {P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_002_01 = {{0}, {P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_002_02 = {{0}, {P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_003_01 = {{0}, {P_::FLA}};

/// SIMDFP_SVE_LD
static StmPipeline RES_SIMDFP_SVE_LD_001_01 = {
  {0, 0},
  {P_::EAGA, P_::LSU1}};
static StmPipeline RES_SIMDFP_SVE_LD_001_02 = {
  {0, 0},
  {P_::EAGB, P_::LSU1}};
static StmPipeline RES_SIMDFP_SVE_LD_001_03 = {
  {0, 0},
  {P_::EAGA, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_LD_001_04 = {
  {0, 0},
  {P_::EAGB, P_::LSU2}};

static StmPipeline RES_SIMDFP_SVE_LD_002_01 = {
    {0, 0, 0, 1, 8},
    {P_::EAGA, P_::EXA, P_::LSU1, P_::EXA_C,P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_02 = {
    {0, 0, 0, 1, 8},
    {P_::EAGA, P_::EXB, P_::LSU1, P_::EXB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_03 = {
    {0, 0, 1, 2, 8},
    {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_04 = {
    {0, 0, 0, 1, 8},
    {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_05 = {
    {0, 0, 0, 1, 8},
    {P_::EAGB, P_::EXA, P_::LSU1, P_::EXA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_06 = {
    {0, 0, 0, 1, 8},
    {P_::EAGB, P_::EXB, P_::LSU1, P_::EXB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_07 = {
    {0, 0, 0, 1, 8},
    {P_::EAGB, P_::EAGA, P_::LSU1, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_08 = {
    {0, 0, 1, 2, 8},
    {P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_09 = {
    {0, 0, 0, 1, 8},
    {P_::EAGA, P_::EXA, P_::LSU2, P_::EXA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_10 = {
    {0, 0, 0, 1, 8},
    {P_::EAGA, P_::EXB, P_::LSU2, P_::EXB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_11 = {
    {0, 0, 1, 2, 8},
    {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_12 = {
    {0, 0, 0, 1, 8},
    {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_13 = {
    {0, 0, 0, 1, 8},
    {P_::EAGB, P_::EXA, P_::LSU2, P_::EXA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_14 = {
    {0, 0, 0, 1, 8},
    {P_::EAGB, P_::EXB, P_::LSU2, P_::EXB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_15 = {
    {0, 0, 0, 1, 8},
    {P_::EAGB, P_::EAGA, P_::LSU2, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_002_16 = {
    {0, 0, 1, 2, 8},
    {P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_003_01 = {
    {0, 0, 8}, {P_::EAGA, P_::LSU1, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_003_02 = {
    {0, 0, 8}, {P_::EAGB, P_::LSU1, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_003_03 = {
    {0, 0, 8}, {P_::EAGA, P_::LSU2, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_003_04 = {
    {0, 0, 8}, {P_::EAGB, P_::LSU2, P_::EAGB_C}};

/// SIMDFP_SVE_ST
static StmPipeline RES_SIMDFP_SVE_ST_001_01 = {
    {0, 0, 0, 0},
    {P_::EAGA, P_::FLA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_001_02 = {
    {0, 0, 0, 0},
    {P_::EAGB, P_::FLA, P_::LSU1, P_::LSU2}};

static StmPipeline RES_SIMDFP_SVE_ST_002_01 = {
    {0, 0, 0, 0, 0},
    {P_::EAGA, P_::FLA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_02 = {
    {0, 0, 0, 0, 0},
    {P_::EAGA, P_::FLA, P_::EXB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_03 = {
    {0, 0, 0, 0, 0},
    {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_04 = {
    {0, 0, 0, 0, 0},
    {P_::EAGB, P_::FLA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_05 = {
    {0, 0, 0, 0, 0},
    {P_::EAGB, P_::FLA, P_::EXB, P_::LSU1, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_ST_002_06 = {
    {0, 0, 0, 0, 0},
    {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU1, P_::LSU2}};

/// PREDICATE_OP
static StmPipeline RES_PREDICATE_OP_001_01 = {{0}, {P_::PRX}};

/// 利用資源IDと利用資源情報のmap
std::map<AArch64SwplSchedA64FX::ResourceID, AArch64SwplSchedA64FX::SchedResource>
    AArch64SwplSchedA64FX::ResInfo={
    {MI_INT_OP_001,  /// EX* | EAG*
      {{&RES_INT_OP_001_01, &RES_INT_OP_001_02, &RES_INT_OP_001_03, &RES_INT_OP_001_04},
      1}},
    {MI_INT_OP_002,  /// EX*
      {{&RES_INT_OP_002_01, &RES_INT_OP_002_02}, 1}},
    {MI_SIMDFP_SVE_OP_001,  /// FL*
      {{&RES_SIMDFP_SVE_OP_001_01, &RES_SIMDFP_SVE_OP_001_02}, 9}},
    {MI_SIMDFP_SVE_OP_002,  /// FL*
      {{&RES_SIMDFP_SVE_OP_002_01, &RES_SIMDFP_SVE_OP_002_02}, 4}},
    {MI_SIMDFP_SVE_OP_003,  /// FLA
      {{&RES_SIMDFP_SVE_OP_003_01}, 9}},
    {MI_SIMDFP_SVE_LD_001,  /// EAG*, FLA
      {{&RES_SIMDFP_SVE_LD_001_01, &RES_SIMDFP_SVE_LD_001_02,
        &RES_SIMDFP_SVE_LD_001_03, &RES_SIMDFP_SVE_LD_001_04},
      11}},
    {MI_SIMDFP_SVE_LD_002,  /// EAG*, FLA / EX*| EAG*
      {{&RES_SIMDFP_SVE_LD_002_01, &RES_SIMDFP_SVE_LD_002_02,
        &RES_SIMDFP_SVE_LD_002_03, &RES_SIMDFP_SVE_LD_002_04,
        &RES_SIMDFP_SVE_LD_002_05, &RES_SIMDFP_SVE_LD_002_06,
        &RES_SIMDFP_SVE_LD_002_07, &RES_SIMDFP_SVE_LD_002_08,
        &RES_SIMDFP_SVE_LD_002_09, &RES_SIMDFP_SVE_LD_002_10,
        &RES_SIMDFP_SVE_LD_002_11, &RES_SIMDFP_SVE_LD_002_12,
        &RES_SIMDFP_SVE_LD_002_13, &RES_SIMDFP_SVE_LD_002_14,
        &RES_SIMDFP_SVE_LD_002_15, &RES_SIMDFP_SVE_LD_002_16},
      8}},
    {MI_SIMDFP_SVE_LD_003,  /// EAG*
      {{&RES_SIMDFP_SVE_LD_003_01, &RES_SIMDFP_SVE_LD_003_02,
        &RES_SIMDFP_SVE_LD_003_03, &RES_SIMDFP_SVE_LD_003_04},
      8}},
    {MI_SIMDFP_SVE_ST_001,  /// EAG*, FLA
      {{&RES_SIMDFP_SVE_ST_001_01, &RES_SIMDFP_SVE_ST_001_02}, 1}},
    {MI_SIMDFP_SVE_ST_002,  /// EAG*, FLA / EX*| EAG*
      {{&RES_SIMDFP_SVE_ST_002_01, &RES_SIMDFP_SVE_ST_002_02,
        &RES_SIMDFP_SVE_ST_002_03, &RES_SIMDFP_SVE_ST_002_04,
        &RES_SIMDFP_SVE_ST_002_05, &RES_SIMDFP_SVE_ST_002_06},
      1}},
    {MI_PREDICATE_OP_001,  /// PRX
      {{&RES_PREDICATE_OP_001_01}, 1}}
  };

/// 命令と利用資源IDのmap
std::map<unsigned int, AArch64SwplSchedA64FX::ResourceID> AArch64SwplSchedA64FX::MIOpcodeInfo{
  // Base
  {AArch64::ADDXri, MI_INT_OP_001},
  {AArch64::ADDXrr, MI_INT_OP_001},
  {AArch64::SUBSXri, MI_INT_OP_001},
  {AArch64::SUBSXrr, MI_INT_OP_002},
  {AArch64::SUBXri, MI_INT_OP_001},
  // SIMD&FP
  {AArch64::FADDDrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDSrr, MI_SIMDFP_SVE_OP_001},

  {AArch64::LDURDi, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDURSi, MI_SIMDFP_SVE_LD_003},

  {AArch64::STRDpost, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRSpost, MI_SIMDFP_SVE_ST_002},
  {AArch64::STURDi, MI_SIMDFP_SVE_ST_001},
  {AArch64::STURSi, MI_SIMDFP_SVE_ST_001},
  // SVE
  {AArch64::FADD_ZPZI_UNDEF_D, MI_SIMDFP_SVE_OP_003},
  {AArch64::FADD_ZPZI_UNDEF_S, MI_SIMDFP_SVE_OP_003},

  {AArch64::LD1D, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W, MI_SIMDFP_SVE_LD_001},
  {AArch64::LDRDpost, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSpost, MI_SIMDFP_SVE_LD_002},

  {AArch64::ST1D, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W, MI_SIMDFP_SVE_ST_001},
};

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::getRes(
  const MachineInstr &mi) const {
  ResourceID id = AArch64SwplSchedA64FX::searchRes(mi);
  return id;
}

const StmPipelinesImpl *AArch64SwplSchedA64FX::getPipelines(
  ResourceID id) const {
  return &ResInfo.at(id).pipelines;
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
  return ResInfo.at(id).latency;
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
