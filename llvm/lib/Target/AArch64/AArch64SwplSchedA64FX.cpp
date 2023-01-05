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
#include "MCTargetDesc/AArch64AddressingModes.h"
#include "../../CodeGen/SWPipeliner.h"
using namespace llvm;
using P_=AArch64SwplSchedA64FX::PortKind;

#define LATENCY_NA 1

unsigned AArch64SwplSchedA64FX::VectorLength;

/// INT_OP
static StmPipeline RES_INT_OP_001_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_001_02 = {{0}, {P_::EXB}};
static StmPipeline RES_INT_OP_001_03 = {{0}, {P_::EAGA}};
static StmPipeline RES_INT_OP_001_04 = {{0}, {P_::EAGB}};
static StmPipeline RES_INT_OP_002_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_002_02 = {{0}, {P_::EXB}};
static StmPipeline RES_INT_OP_003_01 = {
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
  {P_::FLA, P_::FLB, P_::EXA, P_::EXB, P_::EAGA, P_::EAGB, P_::PRX,  P_::BR,
    P_::FLA, P_::FLB, P_::EXA, P_::EXB, P_::EAGA, P_::EAGB, P_::PRX,  P_::BR}};
static StmPipeline RES_INT_OP_004_01 = {
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2},
  {P_::FLA, P_::FLB, P_::EXA, P_::EXB, P_::EAGA, P_::EAGB, P_::PRX, P_::BR,
    P_::FLA, P_::FLB, P_::EXA, P_::EXB, P_::EAGA, P_::EAGB, P_::PRX, P_::BR,
    P_::FLA, P_::FLB, P_::EXA, P_::EXB, P_::EAGA, P_::EAGB, P_::PRX, P_::BR}};
static StmPipeline RES_INT_OP_005_01 = {{0}, {P_::EAGA}};
static StmPipeline RES_INT_OP_005_02 = {{0}, {P_::EAGB}};
static StmPipeline RES_INT_OP_006_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_006_02 = {{0}, {P_::EXB}};
static StmPipeline RES_INT_OP_007_01 = {{0}, {P_::EXA}};
static StmPipeline RES_INT_OP_008_01 = {{0, 5}, {P_::EXA, P_::EXA}};

/// INT_ST
static StmPipeline RES_INT_ST_001_01 = {{0, 0, 0, 0}, {P_::EAGA, P_::EXA, P_::LSU1, P_::LSU2}};
static StmPipeline RES_INT_ST_001_02 = {{0, 0, 0, 0}, {P_::EAGB, P_::EXA, P_::LSU1, P_::LSU2}};

/// SIMDFP_SVE_OP
static StmPipeline RES_SIMDFP_SVE_OP_001_01 = {{0}, {P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_001_02 = {{0}, {P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_002_01 = {{0, 4}, {P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_002_02 = {{0, 4}, {P_::FLB, P_::FLB_C}};
static StmPipeline RES_SIMDFP_SVE_OP_003_01 = {{0}, {P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_004_01 = {{0, 6}, {P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_005_01 = {{0, 4, 8}, {P_::EXA, P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_006_01 = {{0, 6}, {P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_006_02 = {{0, 6}, {P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_007_01 = {{0, 4}, {P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_008_01 = {{0, 1, 1, 5}, {P_::FLA, P_::FLA_C, P_::FLA, P_::FLA_C}};
static StmPipeline RES_SIMDFP_SVE_OP_009_01 = {
  {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_010_01 = {
  {0,  1,  7,  9,  13, 18, 19, 25, 27, 31, 36, 37,  43,  45,  49, 54,
    55, 61, 63, 67, 72, 73, 79, 81, 85, 90, 99, 108, 117, 126, 135},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_010_02 = {
  {0,  0,  6,  9,  12, 18, 18, 24, 27, 30, 36, 36,  42,  45,  48, 54,
    54, 60, 63, 66, 72, 72, 78, 81, 84, 90, 99, 108, 117, 126, 135},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA, P_::FLB,
    P_::FLA, P_::FLB, P_::FLB, P_::FLB, P_::FLB, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_011_01 = {{0, 4}, {P_::EXA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_012_01 = {{0, 1, 9}, {P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_012_02 = {{0, 1, 9}, {P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_012_03 = {{0, 0, 9}, {P_::FLB, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_012_04 = {{0, 0, 9}, {P_::FLB, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_013_01 = {
  {0, 1, 7, 9, 13, 18, 27},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_013_02 = {
  {0, 0, 6, 9, 12, 18, 27},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_014_01 = {
  {0, 1, 7, 9, 13, 18, 19, 25, 27, 31, 36, 37, 45, 54, 63},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_014_02 = {
  {0, 0, 6, 9, 12, 18, 18, 24, 27, 30, 36, 36, 45, 54, 63},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLA,
    P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_015_01 = {
  {0, 1, 7},
  {P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_015_02 = {
  {0, 1, 7},
  {P_::FLA, P_::FLA, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_016_01 = {
  {0, 4, 10, 10, 19}, 
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_016_02 = {
  {0, 4, 10, 10, 19}, 
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_017_01 = {
  {0, 4, 10, 10, 16, 19, 28},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLB, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_017_02 = {
  {0, 4, 10, 10, 16, 19, 28},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLB}};
static StmPipeline RES_SIMDFP_SVE_OP_018_01 = {
  {0, 4, 10, 10, 16, 19, 22, 28, 37},
  {P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA, P_::FLA,
    P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_OP_018_02 = {
  {0, 4, 10, 10, 16, 19, 22, 28, 37},
  {P_::FLB, P_::FLA, P_::FLA, P_::FLB, P_::FLA, P_::FLB, P_::FLA, P_::FLB,
    P_::FLB}};

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

static StmPipeline RES_SIMDFP_SVE_LD_004_01 = {
  {0, 0, 4, 8, 8, 9, 9},
  {P_::EXA, P_::LSU1, P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_004_02 = {
  {0, 0, 4, 8, 8, 9, 9},
  {P_::EXA, P_::LSU2, P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};

static StmPipeline RES_SIMDFP_SVE_LD_005_01 = {
  {0, 0, 4, 4, 5, 5},
  {P_::FLA, P_::LSU1, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_005_02 = {
  {0, 0, 4, 4, 5, 5},
  {P_::FLA, P_::LSU2, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB}};

static StmPipeline RES_SIMDFP_SVE_LD_006_01 = {
  {0, 0, 1, 2, 8},
  {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_02 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EAGB, P_::LSU1, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_03 = {
  {0, 0, 1, 2, 8},
  {P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_04 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EAGA, P_::LSU1, P_::EAGA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_05 = {
  {0, 0, 1, 2, 8},
  {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_06 = {
  {0, 0, 0, 1, 8},
  {P_::EAGA, P_::EAGB, P_::LSU2, P_::EAGB_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_07 = {
  {0, 0, 1, 2, 8},
  {P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGB_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_006_08 = {
  {0, 0, 0, 1, 8},
  {P_::EAGB, P_::EAGA, P_::LSU2, P_::EAGA_C, P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_007_01 = {
  {0, 0, 0, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_007_02 = {
  {0, 0, 0, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_007_03 = {
  {0, 0, 0, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU2, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_007_04 = {
  {0, 0, 0, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU2, P_::FLA_C,P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_008_01 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::EAGA, P_::EAGA_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_02 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU1, P_::EAGA_C, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_03 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU1, P_::EAGB_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_04 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::EAGB, P_::EAGB_C, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_05 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGA, P_::FLA, P_::LSU2, P_::EAGA, P_::EAGA_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_06 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU2, P_::EAGA_C, P_::FLA_C, P_::EAGB_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_07 = {
  {0, 0, 0, 0, 1, 6, 8},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU2, P_::EAGB_C, P_::FLA_C, P_::EAGA_C}};
static StmPipeline RES_SIMDFP_SVE_LD_008_08 = {
  {0, 0, 0, 1, 2, 6, 8},
  {P_::EAGB, P_::FLA, P_::LSU2, P_::EAGB, P_::EAGB_C, P_::FLA_C, P_::EAGB_C}};

static StmPipeline RES_SIMDFP_SVE_LD_009_01 = {
  {0, 0, 0},
  {P_::EAGB, P_::EAGA, P_::LSU1}};
static StmPipeline RES_SIMDFP_SVE_LD_009_02 = {
  {0, 0, 0},
  {P_::EAGA, P_::EAGB, P_::LSU1}}; 
static StmPipeline RES_SIMDFP_SVE_LD_009_03 = {
  {0, 0, 0},
  {P_::EAGB, P_::EAGA, P_::LSU2}};
static StmPipeline RES_SIMDFP_SVE_LD_009_04 = {
  {0, 0, 0},
  {P_::EAGA, P_::EAGB, P_::LSU2}};

static StmPipeline RES_SIMDFP_SVE_LD_010_01 = {
  {0, 0, 1, 1},
  {P_::EAGA, P_::LSU1, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_010_02 = {
  {0, 0, 1, 1},
  {P_::EAGA, P_::LSU1, P_::EAGB, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_010_03 = {
  {0, 0, 1, 1},
  {P_::EAGB, P_::LSU1, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_010_04 = {
  {0, 0, 1, 1},
  {P_::EAGB, P_::LSU1, P_::EAGB, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_010_05 = {
  {0, 0, 1, 1},
  {P_::EAGA, P_::LSU2, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_010_06 = {
  {0, 0, 1, 1},
  {P_::EAGA, P_::LSU2, P_::EAGB, P_::EAGA}};
static StmPipeline RES_SIMDFP_SVE_LD_010_07 = {
  {0, 0, 1, 1},
  {P_::EAGB, P_::LSU2, P_::EAGA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_LD_010_08 = {
  {0, 0, 1, 1},
  {P_::EAGB, P_::LSU2, P_::EAGB, P_::EAGA}};

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

static StmPipeline RES_SIMDFP_SVE_ST_003_01 = {
  {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 9, 10, 10, 11, 11},
  {P_::EXA, P_::LSU1, P_::LSU2, P_::EXA, P_::EXA, P_::EXA, P_::FLA, P_::FLA,
    P_::FLA, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB, P_::EAGA, P_::EAGB,
    P_::EAGA, P_::EAGB}};

static StmPipeline RES_SIMDFP_SVE_ST_004_01 = {
  {0, 0, 0, 0, 1, 1},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGA, P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_ST_004_02 = {
  {0, 0, 0, 0, 0, 1},
  {P_::EAGA, P_::FLA, P_::EAGB, P_::LSU1, P_::LSU2,  P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_ST_004_03 = {
  {0, 0, 0, 0, 0, 1},
  {P_::EAGB, P_::FLA, P_::EAGA, P_::LSU1, P_::LSU2,  P_::FLA}};
static StmPipeline RES_SIMDFP_SVE_ST_004_04 = {
  {0, 0, 0, 0, 1, 1},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGB, P_::FLA}};

static StmPipeline RES_SIMDFP_SVE_ST_005_01 = {
  {0, 0, 0, 0, 1, 1, 1},
  {P_::EAGA, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGA, P_::FLA, P_::EAGB}};
static StmPipeline RES_SIMDFP_SVE_ST_005_02 = {
  {0, 0, 0, 0, 1, 1, 1},
  {P_::EAGB, P_::FLA, P_::LSU1, P_::LSU2, P_::EAGA, P_::FLA, P_::EAGB}};

/// PREDICATE_OP
static StmPipeline RES_PREDICATE_OP_001_01 = {{0}, {P_::PRX}};
static StmPipeline RES_PREDICATE_OP_002_01 = {{0, 0, 4}, {P_::PRX, P_::FLA, P_::FLA_C}};

/// 利用資源IDと利用資源情報のmap
std::map<AArch64SwplSchedA64FX::ResourceID, AArch64SwplSchedA64FX::SchedResource>
  AArch64SwplSchedA64FX::ResInfo={
  {MI_INT_OP_001,  /// Pipeline:EX* | EAG*  Latency:1
    {{&RES_INT_OP_001_01, &RES_INT_OP_001_02, &RES_INT_OP_001_03, &RES_INT_OP_001_04},
    1}},
  {MI_INT_OP_002,  /// Pipeline:EX*  Latency:1
    {{&RES_INT_OP_002_01, &RES_INT_OP_002_02}, 1}},
  {MI_INT_OP_003,  /// Pipeline:(EXA + EXA) | (EXB + EXB)  Latency:1+1  Blocking:P
    {{&RES_INT_OP_003_01}, 2}},
  {MI_INT_OP_004,  /// Pipeline:(EXA + EXA) | (EXB + EXB)  Latency:2+1  Blocking:P
    {{&RES_INT_OP_004_01}, 3}},
  {MI_INT_OP_005,  /// Pipeline:EAG*  Latency:NA
    {{&RES_INT_OP_005_01, &RES_INT_OP_005_02}, LATENCY_NA}},
  {MI_INT_OP_006,  /// Pipeline:EX*  Latency:2
    {{&RES_INT_OP_006_01, &RES_INT_OP_006_02}, 2}},
  {MI_INT_OP_007,  /// Pipeline:EXA  Latency:5
    {{&RES_INT_OP_007_01}, 5}},
  {MI_INT_OP_008,  /// Pipeline:EXA / EXA  Latency:5 + [1]1
    {{&RES_INT_OP_008_01}, 6}},
  {MI_INT_ST_001,  /// Pipeline:EAG*, EXA  Latency:NA, NA
    {{&RES_INT_ST_001_01, &RES_INT_ST_001_02}, LATENCY_NA}},
  {MI_SIMDFP_SVE_OP_001,  /// Pipeline:FL*  Latency:9
    {{&RES_SIMDFP_SVE_OP_001_01, &RES_SIMDFP_SVE_OP_001_02}, 9}},
  {MI_SIMDFP_SVE_OP_002,  /// Pipeline:FL*  Latency:4
    {{&RES_SIMDFP_SVE_OP_002_01, &RES_SIMDFP_SVE_OP_002_02}, 4}},
  {MI_SIMDFP_SVE_OP_003,  /// Pipeline:FLA   Latency:9
    {{&RES_SIMDFP_SVE_OP_003_01}, 9}},
  {MI_SIMDFP_SVE_OP_004,  /// Pipeline:FLA   Latency:6
    {{&RES_SIMDFP_SVE_OP_004_01}, 6}},
  {MI_SIMDFP_SVE_OP_005,  /// Pipeline:EXA + NULL + FLA  Latency:1+3+4
    {{&RES_SIMDFP_SVE_OP_005_01}, 8}},
  {MI_SIMDFP_SVE_OP_006,  /// Pipeline:FLA / FL*  Latency:6 / [1]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_006_01, &RES_SIMDFP_SVE_OP_006_02}, 15, true}},
  {MI_SIMDFP_SVE_OP_007,  /// Pipeline:FLA  Latency:4
    {{&RES_SIMDFP_SVE_OP_007_01}, 4}},
  {MI_SIMDFP_SVE_OP_008,  /// Pipeline:FLA+FLA  Latency:1+4
    {{&RES_SIMDFP_SVE_OP_008_01}, 5}},
  {MI_SIMDFP_SVE_OP_009,  /// Pipeline:FLA  Latency:43  Blocking:E
    {{&RES_SIMDFP_SVE_OP_009_01}, 43}},
  {MI_SIMDFP_SVE_OP_010,  /// Pipeline:FL* / FLA / (FL* / FLA) x 14 / FL*  Latency:9 / 6 / ([1,2]9 / [2]6) x 14 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_010_01, &RES_SIMDFP_SVE_OP_010_02}, 144, true}},
  {MI_SIMDFP_SVE_OP_011,  /// Pipeline:EXA + NULL + FLA  Latency:1+3+9
    {{&RES_SIMDFP_SVE_OP_011_01}, 13}},
  {MI_SIMDFP_SVE_OP_012,  /// Pipeline:FL* / FLA / FL*  Latency:9 / 6 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_012_01, &RES_SIMDFP_SVE_OP_012_02, 
      &RES_SIMDFP_SVE_OP_012_03, &RES_SIMDFP_SVE_OP_012_04}, 18, true}},
  {MI_SIMDFP_SVE_OP_013,  /// Pipeline:FL* / FLA / (FL* / FLA) x 2 / FL*  Latency:9 / 6 / ([1,2]9 / [2]6) x 2 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_013_01, &RES_SIMDFP_SVE_OP_013_02}, 36, true}},
  {MI_SIMDFP_SVE_OP_014,  /// Pipeline:FL* / FLA / (FL* / FLA) x 6 / FL*  Latency:9 / 6 / ([1,2]9 / [2]6) x 6 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_014_01, &RES_SIMDFP_SVE_OP_014_02}, 72, true}},
  {MI_SIMDFP_SVE_OP_015,  /// Pipeline:FLA / FLA / FL*  Latency:6 / 6 / [1,2]9  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_015_01, &RES_SIMDFP_SVE_OP_015_02}, 16, true}},
  {MI_SIMDFP_SVE_OP_016,  /// Pipeline:FL* / (FLA / FL*) x 2  Latency:4 / ([1]6 / [1,2]9) x 2  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_016_01, &RES_SIMDFP_SVE_OP_016_02}, true}},
  {MI_SIMDFP_SVE_OP_017,  /// Pipeline:FL* / (FLA / FL*) x 3  Latency:4 / ([1]6 / [1,2]9) x 3  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_017_01, &RES_SIMDFP_SVE_OP_017_02}, true}},
  {MI_SIMDFP_SVE_OP_018,  /// Pipeline:FL* / (FLA / FL*) x 4  Latency:4 / ([1]6 / [1,2]9) x 4  Seq-decode:true
    {{&RES_SIMDFP_SVE_OP_018_01, &RES_SIMDFP_SVE_OP_018_02}, true}},
  {MI_SIMDFP_SVE_LD_001,  /// Pipeline:EAG*, FLA  Latency:11
    {{&RES_SIMDFP_SVE_LD_001_01, &RES_SIMDFP_SVE_LD_001_02,
      &RES_SIMDFP_SVE_LD_001_03, &RES_SIMDFP_SVE_LD_001_04},
    11}},
  {MI_SIMDFP_SVE_LD_002,  /// Pipeline:EAG*, FLA / EX*| EAG*  Latency:8/1
    {{&RES_SIMDFP_SVE_LD_002_01, &RES_SIMDFP_SVE_LD_002_02,
      &RES_SIMDFP_SVE_LD_002_03, &RES_SIMDFP_SVE_LD_002_04,
      &RES_SIMDFP_SVE_LD_002_05, &RES_SIMDFP_SVE_LD_002_06,
      &RES_SIMDFP_SVE_LD_002_07, &RES_SIMDFP_SVE_LD_002_08,
      &RES_SIMDFP_SVE_LD_002_09, &RES_SIMDFP_SVE_LD_002_10,
      &RES_SIMDFP_SVE_LD_002_11, &RES_SIMDFP_SVE_LD_002_12,
      &RES_SIMDFP_SVE_LD_002_13, &RES_SIMDFP_SVE_LD_002_14,
      &RES_SIMDFP_SVE_LD_002_15, &RES_SIMDFP_SVE_LD_002_16},
    8}},
  {MI_SIMDFP_SVE_LD_003,  /// Pipeline:EAG*  Latency:8
    {{&RES_SIMDFP_SVE_LD_003_01, &RES_SIMDFP_SVE_LD_003_02,
      &RES_SIMDFP_SVE_LD_003_03, &RES_SIMDFP_SVE_LD_003_04},
    8}},
  {MI_SIMDFP_SVE_LD_004,  /// Pipeline:EXA + NULL + FLA +Pipe((EAGA & EAGB), 2)  Latency:1+3+4+Pipe(11, 2)
    {{&RES_SIMDFP_SVE_LD_004_01, &RES_SIMDFP_SVE_LD_004_02},
    20}},
  {MI_SIMDFP_SVE_LD_005,  /// Pipeline:FLA + Pipe((EAGA & EAGB), 2)  Latency:4+Pipe(11, 2)
    {{&RES_SIMDFP_SVE_LD_005_01, &RES_SIMDFP_SVE_LD_005_02},
    16}},
  {MI_SIMDFP_SVE_LD_006,  /// Pipeline:EAG* / EAG*  Latency:8 / 6  
    {{&RES_SIMDFP_SVE_LD_006_01, &RES_SIMDFP_SVE_LD_006_02,
      &RES_SIMDFP_SVE_LD_006_03, &RES_SIMDFP_SVE_LD_006_04,
      &RES_SIMDFP_SVE_LD_006_05, &RES_SIMDFP_SVE_LD_006_06,
      &RES_SIMDFP_SVE_LD_006_07, &RES_SIMDFP_SVE_LD_006_08},
    8}},
  {MI_SIMDFP_SVE_LD_007,  /// Pipeline:EAG* /FLA  Latency:8 / 1  Seq-decode:true
    {{&RES_SIMDFP_SVE_LD_007_01, &RES_SIMDFP_SVE_LD_007_02,
      &RES_SIMDFP_SVE_LD_007_03, &RES_SIMDFP_SVE_LD_007_04},
    8, true}},
  {MI_SIMDFP_SVE_LD_008,  /// Pipeline:EAG* /FLA / EAG*  Latency:8 / 6 / 1  Seq-decode:true
    {{&RES_SIMDFP_SVE_LD_008_01, &RES_SIMDFP_SVE_LD_008_02,
      &RES_SIMDFP_SVE_LD_008_03, &RES_SIMDFP_SVE_LD_008_04,
      &RES_SIMDFP_SVE_LD_008_05, &RES_SIMDFP_SVE_LD_008_06,
      &RES_SIMDFP_SVE_LD_008_07, &RES_SIMDFP_SVE_LD_008_08},
    8, true}},
  {MI_SIMDFP_SVE_LD_009,  /// Pipeline:EAG* /EAG*  Latency:11 / 11
    {{&RES_SIMDFP_SVE_LD_009_01, &RES_SIMDFP_SVE_LD_009_02,
      &RES_SIMDFP_SVE_LD_009_03, &RES_SIMDFP_SVE_LD_009_04},
    11}},
  {MI_SIMDFP_SVE_LD_010,  /// Pipeline:EAG* /EAG*/ EAG*  Latency:1 / [1]11 / [2]11
    {{&RES_SIMDFP_SVE_LD_010_01, &RES_SIMDFP_SVE_LD_010_02,
      &RES_SIMDFP_SVE_LD_010_03, &RES_SIMDFP_SVE_LD_010_04,
      &RES_SIMDFP_SVE_LD_010_05, &RES_SIMDFP_SVE_LD_010_06,
      &RES_SIMDFP_SVE_LD_010_07, &RES_SIMDFP_SVE_LD_010_08},
    12}},
  {MI_SIMDFP_SVE_ST_001,  /// Pipeline:EAG*, FLA  Latency:NA,NA
    {{&RES_SIMDFP_SVE_ST_001_01, &RES_SIMDFP_SVE_ST_001_02}, LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_002,  /// Pipeline:EAG*, FLA / EX*| EAG*  Latency:NA,NA / 1
    {{&RES_SIMDFP_SVE_ST_002_01, &RES_SIMDFP_SVE_ST_002_02,
      &RES_SIMDFP_SVE_ST_002_03, &RES_SIMDFP_SVE_ST_002_04,
      &RES_SIMDFP_SVE_ST_002_05, &RES_SIMDFP_SVE_ST_002_06},
    LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_003,  /// Pipeline:(EXA + NULL + FLA + Pipe((EAGA & EAGB), 2) / EXA + NULL + FLA) x 2  Latency:(1+3+4+Pipe(NA, 2) / 1+3+NA) x 2
    {{&RES_SIMDFP_SVE_ST_003_01}, 11 + LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_004,  /// Pipeline:EAG*, FLA / EAG*, FLA  Latency:NA, NA / NA, NA
    {{&RES_SIMDFP_SVE_ST_004_01, &RES_SIMDFP_SVE_ST_004_02,
      &RES_SIMDFP_SVE_ST_004_03, &RES_SIMDFP_SVE_ST_004_04},
      1 + LATENCY_NA}},
  {MI_SIMDFP_SVE_ST_005,  /// Pipeline:EAG* / EAG*, FLA / EAG*, FLA Latency:1 / NA, NA / NA, NA
    {{&RES_SIMDFP_SVE_ST_005_01, &RES_SIMDFP_SVE_ST_005_02},
      1 + LATENCY_NA}},
  {MI_PREDICATE_OP_001,  /// Pipeline:PRX  Latency:3
    {{&RES_PREDICATE_OP_001_01}, 1}}
  };

/// 命令と利用資源IDのmap
std::map<unsigned int, AArch64SwplSchedA64FX::ResourceID> AArch64SwplSchedA64FX::MIOpcodeInfo{
  // Base
  {AArch64::ADDSXri, MI_INT_OP_002},
  {AArch64::ADDWri, MI_INT_OP_001},
  {AArch64::ADDWrr, MI_INT_OP_001},
  {AArch64::ADDXri, MI_INT_OP_001},
  {AArch64::ADDXrr, MI_INT_OP_001},
  {AArch64::ANDSWri, MI_INT_OP_002},
  {AArch64::ANDXri, MI_INT_OP_001},
  {AArch64::CSELWr, MI_INT_OP_002},
  {AArch64::CSELXr, MI_INT_OP_002},
  {AArch64::CSINCWr, MI_INT_OP_002},
  {AArch64::MSUBWrrr, MI_INT_OP_008},
  {AArch64::ORRWri, MI_INT_OP_001},
  {AArch64::ORRWrr, MI_INT_OP_001},
  {AArch64::PRFMui, MI_INT_OP_005},
  {AArch64::PRFUMi, MI_INT_OP_005},
  {AArch64::SUBSXri, MI_INT_OP_002},
  {AArch64::SUBSXrr, MI_INT_OP_002},
  {AArch64::SUBWri, MI_INT_OP_001},
  {AArch64::SUBXri, MI_INT_OP_001},
  {AArch64::SUBXrr, MI_INT_OP_001},

  {AArch64::STRWroX, MI_INT_ST_001},
  {AArch64::STRWui, MI_INT_ST_001},
  {AArch64::STRXroX, MI_INT_ST_001},
  {AArch64::STRXui, MI_INT_ST_001},
  
  // SIMD&FP
  {AArch64::DUPi32, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUPv2i64lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::EXTv16i8, MI_SIMDFP_SVE_OP_004},
  {AArch64::FABSDr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FABSSr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FADDDrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDSrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDv2f64, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADDv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FCMPDri, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCMPDrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCMPSri, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCMPSrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCSELDrrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCSELSrrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FCVTDSr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FCVTSDr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FDIVDrr, MI_SIMDFP_SVE_OP_009},
  {AArch64::FMADDDrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMADDSrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMAXNMDrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMAXNMSrr, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMLAv2f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLAv2f64, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLAv2i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLAv2i64_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMLAv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMSUBDrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMSUBSrrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULDrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULSrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULv1i32_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMULv1i64_indexed, MI_SIMDFP_SVE_OP_006},
  {AArch64::FMULv2f64, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMULv4f32, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUBDrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUBSrr, MI_SIMDFP_SVE_OP_001},
  {AArch64::INSvi32lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::INSvi64lane, MI_SIMDFP_SVE_OP_004},
  {AArch64::SCVTFUWDri, MI_SIMDFP_SVE_OP_011},
  {AArch64::SCVTFUWSri, MI_SIMDFP_SVE_OP_011},
  {AArch64::ZIP1v2i64, MI_SIMDFP_SVE_OP_004},

  {AArch64::LD1i32, MI_SIMDFP_SVE_LD_007},
  {AArch64::LD1i32_POST, MI_SIMDFP_SVE_LD_008},
  {AArch64::LD1i64, MI_SIMDFP_SVE_LD_007},
  {AArch64::LD1i64_POST, MI_SIMDFP_SVE_LD_008},
  {AArch64::LD1Rv2d, MI_SIMDFP_SVE_LD_003},
  {AArch64::LD1Rv2d_POST, MI_SIMDFP_SVE_LD_006},
  {AArch64::LD1Rv2s, MI_SIMDFP_SVE_LD_003},
  {AArch64::LD1Rv2s_POST, MI_SIMDFP_SVE_LD_006},
  {AArch64::LDRDpost, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRDpre, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRDui, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRDroX, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDRQroX, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDRQui, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSpre, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSroX, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDRSpost, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDRSui, MI_SIMDFP_SVE_LD_002},
  {AArch64::LDURDi, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDURQi, MI_SIMDFP_SVE_LD_003},
  {AArch64::LDURSi, MI_SIMDFP_SVE_LD_003},
  
  {AArch64::ST1i32, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST2Twov2d, MI_SIMDFP_SVE_ST_004},
  {AArch64::ST2Twov2d_POST, MI_SIMDFP_SVE_ST_005},
  {AArch64::ST2Twov4s, MI_SIMDFP_SVE_ST_004},
  {AArch64::ST2Twov4s_POST, MI_SIMDFP_SVE_ST_005},
  {AArch64::STRDpost, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRDroX, MI_SIMDFP_SVE_ST_001},
  {AArch64::STRDui, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRQroX, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRQui, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRSpost, MI_SIMDFP_SVE_ST_002},
  {AArch64::STRSroX, MI_SIMDFP_SVE_ST_001},
  {AArch64::STRSui, MI_SIMDFP_SVE_ST_002},
  {AArch64::STURDi, MI_SIMDFP_SVE_ST_001},
  {AArch64::STURQi, MI_SIMDFP_SVE_ST_001},
  {AArch64::STURSi, MI_SIMDFP_SVE_ST_001},
  
  // SVE
  {AArch64::ADD_ZI_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADD_ZI_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADD_ZZZ_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADD_ZZZ_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::ADR_LSL_ZZZ_D_2, MI_SIMDFP_SVE_OP_008},
  {AArch64::ADR_LSL_ZZZ_D_3, MI_SIMDFP_SVE_OP_008},
  {AArch64::AND_ZI, MI_SIMDFP_SVE_OP_007},
  {AArch64::BIC_PPzPP, MI_SIMDFP_SVE_OP_001},
  {AArch64::CMPHI_PPzZZ_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::CPY_ZPmV_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUP_ZR_D, MI_SIMDFP_SVE_OP_005},
  {AArch64::DUP_ZZI_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::DUP_ZZI_S, MI_SIMDFP_SVE_OP_004},
  {AArch64::EOR_PPzPP, MI_PREDICATE_OP_001},
  {AArch64::EXT_ZZI, MI_SIMDFP_SVE_OP_004},
  {AArch64::FABS_ZPmZ_UNDEF_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::FABS_ZPmZ_UNDEF_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::FADD_ZPZI_UNDEF_D, MI_SIMDFP_SVE_OP_003},
  {AArch64::FADD_ZPZI_UNDEF_S, MI_SIMDFP_SVE_OP_003},
  {AArch64::FADD_ZPZZ_UNDEF_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADD_ZPZZ_UNDEF_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADD_ZZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FADD_ZZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FCMGE_PPzZZ_D, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMGE_PPzZZ_S, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMGT_PPzZ0_D, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMGT_PPzZ0_S, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMGT_PPzZZ_D, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMGT_PPzZZ_S, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMLE_PPzZ0_D, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMLE_PPzZ0_S, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMLT_PPzZ0_D, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMLT_PPzZ0_S, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMNE_PPzZ0_D, MI_SIMDFP_SVE_OP_007},
  {AArch64::FCMNE_PPzZ0_S, MI_SIMDFP_SVE_OP_007},
  {AArch64::FMAXNM_ZPZZ_UNDEF_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMAXNM_ZPZZ_UNDEF_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::FMLA_ZPZZZ_UNDEF_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLA_ZPZZZ_UNDEF_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLS_ZPZZZ_UNDEF_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMLS_ZPZZZ_UNDEF_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZPZI_UNDEF_D, MI_SIMDFP_SVE_OP_003},
  {AArch64::FMUL_ZPZI_UNDEF_S, MI_SIMDFP_SVE_OP_003},
  {AArch64::FMUL_ZPZZ_UNDEF_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FMUL_ZZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FNMLS_ZPZZZ_UNDEF_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FNMLS_ZPZZZ_UNDEF_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUB_ZZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::FSUB_ZZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::LSL_ZZI_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::LSR_ZZI_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::LSR_ZZI_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::MLS_ZPmZZ_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::MLS_ZPmZZ_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::MUL_ZI_D, MI_SIMDFP_SVE_OP_003},
  {AArch64::MUL_ZI_S, MI_SIMDFP_SVE_OP_003},
  {AArch64::REV_ZZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::REV_ZZ_S, MI_SIMDFP_SVE_OP_004},
  {AArch64::SCVTF_ZPmZ_StoD_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::SCVTF_ZPmZ_StoS_UNDEF, MI_SIMDFP_SVE_OP_001},
  {AArch64::SEL_PPPP, MI_PREDICATE_OP_001},
  {AArch64::SEL_ZPZZ_D, MI_SIMDFP_SVE_OP_002},
  {AArch64::SEL_ZPZZ_S, MI_SIMDFP_SVE_OP_002},
  {AArch64::SPLICE_ZPZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::SPLICE_ZPZ_S, MI_SIMDFP_SVE_OP_004},
  {AArch64::UMULH_ZPZZ_UNDEF_D, MI_SIMDFP_SVE_OP_001},
  {AArch64::UMULH_ZPZZ_UNDEF_S, MI_SIMDFP_SVE_OP_001},
  {AArch64::UUNPKHI_ZZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::UUNPKLO_ZZ_D, MI_SIMDFP_SVE_OP_004},
  {AArch64::UZP1_ZZZ_S, MI_SIMDFP_SVE_OP_004},
  
  {AArch64::GLD1D, MI_SIMDFP_SVE_LD_004},
  {AArch64::GLD1D_IMM, MI_SIMDFP_SVE_LD_005},
  {AArch64::GLD1W_D, MI_SIMDFP_SVE_LD_004},
  {AArch64::GLD1W_D_IMM, MI_SIMDFP_SVE_LD_005},
  {AArch64::LD1B, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1D, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1D_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W_D, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W_D_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD1W_IMM, MI_SIMDFP_SVE_LD_001},
  {AArch64::LD2D, MI_SIMDFP_SVE_LD_010},
  {AArch64::LD2D_IMM, MI_SIMDFP_SVE_LD_009},
  {AArch64::LD2W, MI_SIMDFP_SVE_LD_010},
  {AArch64::LD2W_IMM, MI_SIMDFP_SVE_LD_009},
  
  {AArch64::SST1D_SCALED, MI_SIMDFP_SVE_ST_003},
  {AArch64::SST1W_D_SCALED, MI_SIMDFP_SVE_ST_003},
  {AArch64::ST1B, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1D, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1D_IMM, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W_D, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST1W_IMM, MI_SIMDFP_SVE_ST_001},
  {AArch64::ST2D, MI_SIMDFP_SVE_ST_005},
  {AArch64::ST2D_IMM, MI_SIMDFP_SVE_ST_004},
  {AArch64::ST2W, MI_SIMDFP_SVE_ST_005},
  {AArch64::ST2W_IMM, MI_SIMDFP_SVE_ST_004},
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

bool AArch64SwplSchedA64FX::isSeqDecode(ResourceID id) const {
  return ResInfo.at(id).seqdecode;
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchRes(
  const MachineInstr &mi) {

  unsigned Opcode = mi.getOpcode();
  auto it = MIOpcodeInfo.find(Opcode);
  if (it != MIOpcodeInfo.end()) {
    return it->second;
  }

  // COPY命令はレジスタ種別で利用する資源が異なるため
  // レジスタ種別ごとに代表的なレイテンシ・演算器で定義する
  if (Opcode == AArch64::COPY) {
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

  // ADDXrs/SUBXrs命令の判断
  if (Opcode == AArch64::ADDXrs || Opcode == AArch64::SUBXrs){
    return AArch64SwplSchedA64FX::searchResADDSUBShiftReg(mi);
  }

  // ADDXrx命令の判断
  if (Opcode == AArch64::ADDXrx){
    return AArch64SwplSchedA64FX::searchResADDExtendReg(mi);
  }

  // MADD/UMADDL命令の判断
  if (Opcode == AArch64::MADDWrrr || Opcode == AArch64::MADDXrrr ||
      Opcode == AArch64::UMADDLrrr){
    return AArch64SwplSchedA64FX::searchResMADD(mi);
  }

  // ORRWrs/ORRXrs命令の判断
  if (Opcode == AArch64::ORRWrs || Opcode == AArch64::ORRXrs){
    return AArch64SwplSchedA64FX::searchResORRShiftReg(mi);
  }

  // SBFM/UBFM命令の判断
  if (Opcode == AArch64::SBFMXri || Opcode == AArch64::SBFMWri ||
      Opcode == AArch64::UBFMXri || Opcode == AArch64::UBFMWri){
    return AArch64SwplSchedA64FX::searchResSBFM(mi);
  }

  // FADDA命令の判断
  if (Opcode == AArch64::FADDA_VPZ_D){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_012;
    case 256:
      return MI_SIMDFP_SVE_OP_013;
    default:
      return MI_SIMDFP_SVE_OP_014;
    }
  }
  if (Opcode == AArch64::FADDA_VPZ_S){
    switch (AArch64SwplSchedA64FX::VectorLength) {
    case 128:
      return MI_SIMDFP_SVE_OP_013;
    case 256:
      return MI_SIMDFP_SVE_OP_014;
    default:
      return MI_SIMDFP_SVE_OP_010;
    }
  }
  return AArch64SwplSchedA64FX::MI_NA;
}


AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResADDSUBShiftReg(const MachineInstr &mi) {
  // 参考：AArch64InstrInfo.cpp AArch64InstrInfo::isFalkorShiftExtFast

  // Immが存在しない場合、シフト量：0
  if (mi.getNumOperands() < 4)
    return MI_INT_OP_001;

  unsigned Imm = mi.getOperand(3).getImm();
  unsigned ShiftVal = AArch64_AM::getShiftValue(Imm);
  if (ShiftVal == 0)
    return MI_INT_OP_001;
  else if (ShiftVal <= 4 && AArch64_AM::getShiftType(Imm) == AArch64_AM::LSL)
    return MI_INT_OP_003;
  else
    return MI_INT_OP_004;
}


AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResADDExtendReg(const MachineInstr &mi) {
  // 参考：AArch64InstrInfo.cpp AArch64InstrInfo::isFalkorShiftExtFast

  unsigned Opcode = mi.getOpcode();
  // Immが存在しない場合、LSL、拡張後のシフト量：0
  if (mi.getNumOperands() < 4){
    if (Opcode == AArch64::ADDWrx)
      return MI_INT_OP_002;
    else
      return MI_INT_OP_003;
  }
  
  unsigned Imm = mi.getOperand(3).getImm();
  
  if (Opcode == AArch64::ADDWrx){ /// 32bit(sf = 0)
    switch (AArch64_AM::getArithExtendType(Imm)){
    default:
      return MI_INT_OP_003;
    case AArch64_AM::UXTW:
    case AArch64_AM::UXTX:
    case AArch64_AM::SXTW:
    case AArch64_AM::SXTX:
      return MI_INT_OP_002;
    }
  } else {
    switch (AArch64_AM::getArithExtendType(Imm)){
    default:
      return MI_INT_OP_003;
    case AArch64_AM::UXTX:
    case AArch64_AM::SXTX:
      return MI_INT_OP_002;
    }
  }
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResMADD(const MachineInstr &mi) {
  unsigned Opcode = mi.getOpcode();
  Register r = mi.getOperand(3).getReg();
  
  // MADD:32bit Wa==WZRの場合、MULと等価
  if (Opcode == AArch64::MADDWrrr || r == AArch64::WZR){
    return MI_INT_OP_007;
  }
  // MADD:64bit Xa==XZRの場合、MULと等価
  if (Opcode == AArch64::MADDXrrr || r == AArch64::XZR){
    return MI_INT_OP_007;
  }
  // UMADDL Xa==XZRの場合、UMULLと等価
  if (Opcode == AArch64::UMADDLrrr || r == AArch64::XZR){
    return MI_INT_OP_007;
  }

  return MI_INT_OP_008;
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResORRShiftReg(const MachineInstr &mi) {
  // 参考: AArch64InstPrinter.cpp AArch64InstPrinter::printInst
  // AArch64::ORRWrs and AArch64::ORRXrs with WZR/XZR reg
  // and zero immediate operands used as an alias for mov instruction.

  // Immが存在しない場合、LSL、拡張後のシフト量：0
  if (mi.getNumOperands() < 4){
      return MI_INT_OP_001;
  }

  unsigned Imm = mi.getOperand(3).getImm();
  unsigned ShiftVal = AArch64_AM::getShiftValue(Imm);

  if (ShiftVal == 0) {
    return MI_INT_OP_001;
  } else {
    return MI_INT_OP_004;
  }
}

AArch64SwplSchedA64FX::ResourceID AArch64SwplSchedA64FX::searchResSBFM(const MachineInstr &mi) {
  // 参考: AArch64InstPrinter.cpp AArch64InstPrinter::printInst
  unsigned Opcode = mi.getOpcode();

  const MachineOperand Op2 = mi.getOperand(2);
  const MachineOperand Op3 = mi.getOperand(3);

  bool IsSigned = (Opcode == AArch64::SBFMXri || Opcode == AArch64::SBFMWri);
  bool Is64Bit = (Opcode == AArch64::SBFMXri || Opcode == AArch64::UBFMXri);
  if (Op2.isImm() && Op2.getImm() == 0 && Op3.isImm()) {
    switch (Op3.getImm()) {
    default:
      break;
    case 7:
      if (IsSigned || (!Is64Bit)){
        // SXTB/UXTB
        return MI_INT_OP_002;
      }
      break;
    case 15:
      if (IsSigned || (!Is64Bit)){
        // SXTH/UXTH
        return MI_INT_OP_002;
      }
      break;
    case 31:
      // *xtw is only valid for signed 64-bit operations.
      if (Is64Bit && IsSigned){
        // SXTW
        return MI_INT_OP_002;
      }
      break;
    }
  }

  // All immediate shifts are aliases, implemented using the Bitfield
  // instruction. In all cases the immediate shift amount shift must be in
  // the range 0 to (reg.size -1).
  if (Op2.isImm() && Op3.isImm()) {
    int shift = 0;
    int64_t immr = Op2.getImm();
    int64_t imms = Op3.getImm();
    if (Opcode == AArch64::UBFMWri && imms != 0x1F && ((imms + 1) == immr)) {
      // LSL
      shift = 31 - imms;
      if (1 <= shift && shift <= 4) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::UBFMXri && imms != 0x3f &&
                ((imms + 1 == immr))) {
      // LSL
      shift = 63 - imms;
      if (1 <= shift && shift <= 4) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::UBFMWri && imms == 0x1f) {
      // LSR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::UBFMXri && imms == 0x3f) {
      // LSR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::SBFMWri && imms == 0x1f) {
      // ASR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    } else if (Opcode == AArch64::SBFMXri && imms == 0x3f) {
      // ASR
      shift = immr;
      if (shift == 0) {
        return MI_INT_OP_002;
      } else {
        return MI_INT_OP_006;
      }
    }
  }

  // SBFIZ/UBFIZ or SBFX/UBFX
  return MI_INT_OP_004;
}
