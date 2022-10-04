//=- AArch64A64FXResourceInfo.td - AArch64 A64FX ResourceInfo -*- tablegen -*=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 A64FX Resource Infomation for Instructions.
//
//===----------------------------------------------------------------------===//
//===- Copyright FUJITSU LIMITED 2021 and FUJITSU LABORATORIES LTD. 2021  -===//
//===----------------------------------------------------------------------===//

let SchedModel = A64FXModel in {

def PROC_V8_BR_00 : ProcResource<1>;
def PROC_V8_BR_01 : ProcResource<1>;
def PROC_V8_BR_02 : ProcResource<1>;
def PROC_V8_BR_03 : ProcResource<1>;
def PROC_V8_LDST_00 : ProcResource<1>;
def PROC_V8_LDST_01 : ProcResource<1>;
def PROC_V8_LD_00 : ProcResource<1>;
def PROC_V8_LD_01 : ProcResource<1>;
def PROC_V8_LD_02 : ProcResource<1>;
def PROC_V8_LD_03 : ProcResource<1>;
def PROC_V8_LD_04 : ProcResource<1>;
def PROC_V8_LD_05 : ProcResource<1>;
def PROC_V8_LD_06 : ProcResource<1>;
def PROC_V8_MISC_00 : ProcResource<1>;
def PROC_V8_MISC_01 : ProcResource<1>;
def PROC_V8_MISC_02 : ProcResource<1>;
def PROC_V8_MISC_03 : ProcResource<1>;
def PROC_V8_MISC_04 : ProcResource<1>;
def PROC_V8_MISC_05 : ProcResource<1>;
def PROC_V8_OP_00 : ProcResource<1>;
def PROC_V8_OP_01 : ProcResource<1>;
def PROC_V8_OP_02 : ProcResource<1>;
def PROC_V8_OP_03 : ProcResource<1>;
def PROC_V8_OP_04 : ProcResource<1>;
def PROC_V8_OP_05 : ProcResource<1>;
def PROC_V8_OP_06 : ProcResource<1>;
def PROC_V8_OP_07 : ProcResource<1>;
def PROC_V8_OP_08 : ProcResource<1>;
def PROC_V8_OP_09 : ProcResource<1>;
def PROC_V8_OP_10 : ProcResource<1>;
def PROC_V8_OP_11 : ProcResource<1>;
def PROC_V8_OP_12 : ProcResource<1>;
def PROC_V8_OP_13 : ProcResource<1>;
def PROC_V8_OP_14 : ProcResource<1>;
def PROC_V8_OP_15 : ProcResource<1>;
def PROC_V8_OP_16 : ProcResource<1>;
def PROC_V8_OP_17 : ProcResource<1>;
def PROC_V8_PF_00 : ProcResource<1>;
def PROC_V8_PF_01 : ProcResource<1>;
def PROC_V8_ST_00 : ProcResource<1>;
def PROC_V8_ST_01 : ProcResource<1>;
def PROC_V8_ST_02 : ProcResource<1>;
def PROC_V8_ST_03 : ProcResource<1>;
def PROC_V8_ST_04 : ProcResource<1>;
def PROC_V8_ST_05 : ProcResource<1>;
def PROC_V8_ST_06 : ProcResource<1>;
def PROC_V8_SWP_00 : ProcResource<1>;
def PROC_V8_SWP_01 : ProcResource<1>;
def PROC_V8_SWP_02 : ProcResource<1>;
def PROC_SIMDFP_LD_00 : ProcResource<1>;
def PROC_SIMDFP_LD_01 : ProcResource<1>;
def PROC_SIMDFP_LD_02 : ProcResource<1>;
def PROC_SIMDFP_LD_03 : ProcResource<1>;
def PROC_SIMDFP_LD_04 : ProcResource<1>;
def PROC_SIMDFP_LD_05 : ProcResource<1>;
def PROC_SIMDFP_LD_06 : ProcResource<1>;
def PROC_SIMDFP_LD_07 : ProcResource<1>;
def PROC_SIMDFP_LD_08 : ProcResource<1>;
def PROC_SIMDFP_LD_09 : ProcResource<1>;
def PROC_SIMDFP_LD_10 : ProcResource<1>;
def PROC_SIMDFP_LD_11 : ProcResource<1>;
def PROC_SIMDFP_LD_12 : ProcResource<1>;
def PROC_SIMDFP_LD_13 : ProcResource<1>;
def PROC_SIMDFP_LD_14 : ProcResource<1>;
def PROC_SIMDFP_LD_15 : ProcResource<1>;
def PROC_SIMDFP_LD_16 : ProcResource<1>;
def PROC_SIMDFP_LD_17 : ProcResource<1>;
def PROC_SIMDFP_LD_18 : ProcResource<1>;
def PROC_SIMDFP_LD_19 : ProcResource<1>;
def PROC_SIMDFP_LD_20 : ProcResource<1>;
def PROC_SIMDFP_LD_21 : ProcResource<1>;
def PROC_SIMDFP_LD_22 : ProcResource<1>;
def PROC_SIMDFP_LD_23 : ProcResource<1>;
def PROC_SIMDFP_LD_24 : ProcResource<1>;
def PROC_SIMDFP_LD_25 : ProcResource<1>;
def PROC_SIMDFP_LD_26 : ProcResource<1>;
def PROC_SIMDFP_OP_00 : ProcResource<1>;
def PROC_SIMDFP_OP_01 : ProcResource<1>;
def PROC_SIMDFP_OP_02 : ProcResource<1>;
def PROC_SIMDFP_OP_03 : ProcResource<1>;
def PROC_SIMDFP_OP_04 : ProcResource<1>;
def PROC_SIMDFP_OP_05 : ProcResource<1>;
def PROC_SIMDFP_OP_06 : ProcResource<1>;
def PROC_SIMDFP_OP_07 : ProcResource<1>;
def PROC_SIMDFP_OP_08 : ProcResource<1>;
def PROC_SIMDFP_OP_09 : ProcResource<1>;
def PROC_SIMDFP_OP_10 : ProcResource<1>;
def PROC_SIMDFP_OP_11 : ProcResource<1>;
def PROC_SIMDFP_OP_12 : ProcResource<1>;
def PROC_SIMDFP_OP_13 : ProcResource<1>;
def PROC_SIMDFP_OP_14 : ProcResource<1>;
def PROC_SIMDFP_OP_15 : ProcResource<1>;
def PROC_SIMDFP_OP_16 : ProcResource<1>;
def PROC_SIMDFP_OP_17 : ProcResource<1>;
def PROC_SIMDFP_OP_18 : ProcResource<1>;
def PROC_SIMDFP_OP_19 : ProcResource<1>;
def PROC_SIMDFP_OP_20 : ProcResource<1>;
def PROC_SIMDFP_OP_21 : ProcResource<1>;
def PROC_SIMDFP_OP_22 : ProcResource<1>;
def PROC_SIMDFP_OP_23 : ProcResource<1>;
def PROC_SIMDFP_OP_24 : ProcResource<1>;
def PROC_SIMDFP_OP_25 : ProcResource<1>;
def PROC_SIMDFP_OP_26 : ProcResource<1>;
def PROC_SIMDFP_OP_27 : ProcResource<1>;
def PROC_SIMDFP_OP_28 : ProcResource<1>;
def PROC_SIMDFP_OP_29 : ProcResource<1>;
def PROC_SIMDFP_OP_30 : ProcResource<1>;
def PROC_SIMDFP_OP_31 : ProcResource<1>;
def PROC_SIMDFP_OP_32 : ProcResource<1>;
def PROC_SIMDFP_OP_33 : ProcResource<1>;
def PROC_SIMDFP_OP_34 : ProcResource<1>;
def PROC_SIMDFP_OP_35 : ProcResource<1>;
def PROC_SIMDFP_OP_36 : ProcResource<1>;
def PROC_SIMDFP_OP_37 : ProcResource<1>;
def PROC_SIMDFP_OP_38 : ProcResource<1>;
def PROC_SIMDFP_OP_39 : ProcResource<1>;
def PROC_SIMDFP_OP_40 : ProcResource<1>;
def PROC_SIMDFP_OP_41 : ProcResource<1>;
def PROC_SIMDFP_OP_42 : ProcResource<1>;
def PROC_SIMDFP_OP_43 : ProcResource<1>;
def PROC_SIMDFP_OP_44 : ProcResource<1>;
def PROC_SIMDFP_OP_45 : ProcResource<1>;
def PROC_SIMDFP_OP_46 : ProcResource<1>;
def PROC_SIMDFP_OP_47 : ProcResource<1>;
def PROC_SIMDFP_OP_48 : ProcResource<1>;
def PROC_SIMDFP_OP_49 : ProcResource<1>;
def PROC_SIMDFP_OP_50 : ProcResource<1>;
def PROC_SIMDFP_OP_51 : ProcResource<1>;
def PROC_SIMDFP_OP_52 : ProcResource<1>;
def PROC_SIMDFP_OP_53 : ProcResource<1>;
def PROC_SIMDFP_OP_54 : ProcResource<1>;
def PROC_SIMDFP_OP_55 : ProcResource<1>;
def PROC_SIMDFP_OP_56 : ProcResource<1>;
def PROC_SIMDFP_OP_57 : ProcResource<1>;
def PROC_SIMDFP_ST_00 : ProcResource<1>;
def PROC_SIMDFP_ST_01 : ProcResource<1>;
def PROC_SIMDFP_ST_02 : ProcResource<1>;
def PROC_SIMDFP_ST_03 : ProcResource<1>;
def PROC_SIMDFP_ST_04 : ProcResource<1>;
def PROC_SIMDFP_ST_05 : ProcResource<1>;
def PROC_SIMDFP_ST_06 : ProcResource<1>;
def PROC_SIMDFP_ST_07 : ProcResource<1>;
def PROC_SIMDFP_ST_08 : ProcResource<1>;
def PROC_SIMDFP_ST_09 : ProcResource<1>;
def PROC_SVE_LD_00 : ProcResource<1>;
def PROC_SVE_LD_01 : ProcResource<1>;
def PROC_SVE_LD_02 : ProcResource<1>;
def PROC_SVE_LD_03 : ProcResource<1>;
def PROC_SVE_LD_04 : ProcResource<1>;
def PROC_SVE_LD_05 : ProcResource<1>;
def PROC_SVE_LD_06 : ProcResource<1>;
def PROC_SVE_LD_07 : ProcResource<1>;
def PROC_SVE_LD_08 : ProcResource<1>;
def PROC_SVE_LD_09 : ProcResource<1>;
def PROC_SVE_LD_10 : ProcResource<1>;
def PROC_SVE_LD_11 : ProcResource<1>;
def PROC_SVE_LD_12 : ProcResource<1>;
def PROC_SVE_LD_13 : ProcResource<1>;
def PROC_SVE_LD_14 : ProcResource<1>;
def PROC_SVE_LD_15 : ProcResource<1>;
def PROC_SVE_MISC_00 : ProcResource<1>;
def PROC_SVE_MISC_01 : ProcResource<1>;
def PROC_SVE_OP_00 : ProcResource<1>;
def PROC_SVE_OP_01 : ProcResource<1>;
def PROC_SVE_OP_02 : ProcResource<1>;
def PROC_SVE_OP_03 : ProcResource<1>;
def PROC_SVE_OP_05 : ProcResource<1>;
def PROC_SVE_OP_06 : ProcResource<1>;
def PROC_SVE_OP_07 : ProcResource<1>;
def PROC_SVE_OP_08 : ProcResource<1>;
def PROC_SVE_OP_09 : ProcResource<1>;
def PROC_SVE_OP_10 : ProcResource<1>;
def PROC_SVE_OP_11 : ProcResource<1>;
def PROC_SVE_OP_12 : ProcResource<1>;
def PROC_SVE_OP_13 : ProcResource<1>;
def PROC_SVE_OP_14 : ProcResource<1>;
def PROC_SVE_OP_15 : ProcResource<1>;
def PROC_SVE_OP_16 : ProcResource<1>;
def PROC_SVE_OP_17 : ProcResource<1>;
def PROC_SVE_OP_18 : ProcResource<1>;
def PROC_SVE_OP_19 : ProcResource<1>;
def PROC_SVE_OP_20 : ProcResource<1>;
def PROC_SVE_OP_21 : ProcResource<1>;
def PROC_SVE_OP_22 : ProcResource<1>;
def PROC_SVE_OP_23 : ProcResource<1>;
def PROC_SVE_OP_24 : ProcResource<1>;
def PROC_SVE_OP_25 : ProcResource<1>;
def PROC_SVE_OP_26 : ProcResource<1>;
def PROC_SVE_OP_27 : ProcResource<1>;
def PROC_SVE_OP_28 : ProcResource<1>;
def PROC_SVE_OP_29 : ProcResource<1>;
def PROC_SVE_OP_30 : ProcResource<1>;
def PROC_SVE_OP_31 : ProcResource<1>;
def PROC_SVE_OP_32 : ProcResource<1>;
def PROC_SVE_OP_33 : ProcResource<1>;
def PROC_SVE_OP_34 : ProcResource<1>;
def PROC_SVE_OP_35 : ProcResource<1>;
def PROC_SVE_OP_36 : ProcResource<1>;
def PROC_SVE_OP_37 : ProcResource<1>;
def PROC_SVE_OP_38 : ProcResource<1>;
def PROC_SVE_OP_39 : ProcResource<1>;
def PROC_SVE_OP_40 : ProcResource<1>;
def PROC_SVE_OP_41 : ProcResource<1>;
def PROC_SVE_OP_42 : ProcResource<1>;
def PROC_SVE_OP_43 : ProcResource<1>;
def PROC_SVE_OP_44 : ProcResource<1>;
def PROC_SVE_OP_45 : ProcResource<1>;
def PROC_SVE_OP_46 : ProcResource<1>;
def PROC_SVE_OP_47 : ProcResource<1>;
def PROC_SVE_OP_48 : ProcResource<1>;
def PROC_SVE_OP_49 : ProcResource<1>;
def PROC_SVE_OP_50 : ProcResource<1>;
def PROC_SVE_OP_51 : ProcResource<1>;
def PROC_SVE_OP_52 : ProcResource<1>;
def PROC_SVE_OP_53 : ProcResource<1>;
def PROC_SVE_OP_54 : ProcResource<1>;
def PROC_SVE_OP_55 : ProcResource<1>;
def PROC_SVE_OP_56 : ProcResource<1>;
def PROC_SVE_OP_57 : ProcResource<1>;
def PROC_SVE_OP_58 : ProcResource<1>;
def PROC_SVE_OP_59 : ProcResource<1>;
def PROC_SVE_PF_00 : ProcResource<1>;
def PROC_SVE_PF_01 : ProcResource<1>;
def PROC_SVE_PF_02 : ProcResource<1>;
def PROC_SVE_PF_03 : ProcResource<1>;
def PROC_SVE_PF_04 : ProcResource<1>;
def PROC_SVE_PRED_00 : ProcResource<1>;
def PROC_SVE_PRED_01 : ProcResource<1>;
def PROC_SVE_PRED_02 : ProcResource<1>;
def PROC_SVE_PRED_03 : ProcResource<1>;
def PROC_SVE_PRED_04 : ProcResource<1>;
def PROC_SVE_PRED_05 : ProcResource<1>;
def PROC_SVE_ST_00 : ProcResource<1>;
def PROC_SVE_ST_01 : ProcResource<1>;
def PROC_SVE_ST_02 : ProcResource<1>;
def PROC_SVE_ST_03 : ProcResource<1>;
def PROC_SVE_ST_04 : ProcResource<1>;
def PROC_SVE_ST_05 : ProcResource<1>;
def PROC_SVE_ST_06 : ProcResource<1>;
def PROC_SVE_ST_07 : ProcResource<1>;
def PROC_SVE_ST_08 : ProcResource<1>;
def PROC_SVE_ST_09 : ProcResource<1>;
def PROC_SVE_ST_10 : ProcResource<1>;
def PROC_SVE_ST_11 : ProcResource<1>;
def PROC_SVE_ST_12 : ProcResource<1>;
def PROC_SVE_ST_13 : ProcResource<1>;
def PROC_SVE_ST_14 : ProcResource<1>;
def PROC_SVE_ST_15 : ProcResource<1>;
def PROC_SVE_ST_16 : ProcResource<1>;


def RES_V8_BR_00 : ProcResGroup<[PROC_V8_BR_00]> {let BufferSize = -1;}
def RES_V8_BR_01 : ProcResGroup<[PROC_V8_BR_01]> {let BufferSize = -1;}
def RES_V8_BR_02 : ProcResGroup<[PROC_V8_BR_02]> {let BufferSize = -1;}
def RES_V8_BR_03 : ProcResGroup<[PROC_V8_BR_03]> {let BufferSize = -1;}
def RES_V8_LDST_00 : ProcResGroup<[PROC_V8_LDST_00]> {let BufferSize = -1;}
def RES_V8_LDST_01 : ProcResGroup<[PROC_V8_LDST_01]> {let BufferSize = -1;}
def RES_V8_LD_00 : ProcResGroup<[PROC_V8_LD_00]> {let BufferSize = -1;}
def RES_V8_LD_01 : ProcResGroup<[PROC_V8_LD_01]> {let BufferSize = -1;}
def RES_V8_LD_02 : ProcResGroup<[PROC_V8_LD_02]> {let BufferSize = -1;}
def RES_V8_LD_03 : ProcResGroup<[PROC_V8_LD_03]> {let BufferSize = -1;}
def RES_V8_LD_04 : ProcResGroup<[PROC_V8_LD_04]> {let BufferSize = -1;}
def RES_V8_LD_05 : ProcResGroup<[PROC_V8_LD_05]> {let BufferSize = -1;}
def RES_V8_LD_06 : ProcResGroup<[PROC_V8_LD_06]> {let BufferSize = -1;}
def RES_V8_MISC_00 : ProcResGroup<[PROC_V8_MISC_00]> {let BufferSize = -1;}
def RES_V8_MISC_01 : ProcResGroup<[PROC_V8_MISC_01]> {let BufferSize = -1;}
def RES_V8_MISC_02 : ProcResGroup<[PROC_V8_MISC_02]> {let BufferSize = -1;}
def RES_V8_MISC_03 : ProcResGroup<[PROC_V8_MISC_03]> {let BufferSize = -1;}
def RES_V8_MISC_04 : ProcResGroup<[PROC_V8_MISC_04]> {let BufferSize = -1;}
def RES_V8_MISC_05 : ProcResGroup<[PROC_V8_MISC_05]> {let BufferSize = -1;}
def RES_V8_OP_00 : ProcResGroup<[PROC_V8_OP_00]> {let BufferSize = -1;}
def RES_V8_OP_01 : ProcResGroup<[PROC_V8_OP_01]> {let BufferSize = -1;}
def RES_V8_OP_02 : ProcResGroup<[PROC_V8_OP_02]> {let BufferSize = -1;}
def RES_V8_OP_03 : ProcResGroup<[PROC_V8_OP_03]> {let BufferSize = -1;}
def RES_V8_OP_04 : ProcResGroup<[PROC_V8_OP_04]> {let BufferSize = -1;}
def RES_V8_OP_05 : ProcResGroup<[PROC_V8_OP_05]> {let BufferSize = -1;}
def RES_V8_OP_06 : ProcResGroup<[PROC_V8_OP_06]> {let BufferSize = -1;}
def RES_V8_OP_07 : ProcResGroup<[PROC_V8_OP_07]> {let BufferSize = -1;}
def RES_V8_OP_08 : ProcResGroup<[PROC_V8_OP_08]> {let BufferSize = -1;}
def RES_V8_OP_09 : ProcResGroup<[PROC_V8_OP_09]> {let BufferSize = -1;}
def RES_V8_OP_10 : ProcResGroup<[PROC_V8_OP_10]> {let BufferSize = -1;}
def RES_V8_OP_11 : ProcResGroup<[PROC_V8_OP_11]> {let BufferSize = -1;}
def RES_V8_OP_12 : ProcResGroup<[PROC_V8_OP_12]> {let BufferSize = -1;}
def RES_V8_OP_13 : ProcResGroup<[PROC_V8_OP_13]> {let BufferSize = -1;}
def RES_V8_OP_14 : ProcResGroup<[PROC_V8_OP_14]> {let BufferSize = -1;}
def RES_V8_OP_15 : ProcResGroup<[PROC_V8_OP_15]> {let BufferSize = -1;}
def RES_V8_OP_16 : ProcResGroup<[PROC_V8_OP_16]> {let BufferSize = -1;}
def RES_V8_OP_17 : ProcResGroup<[PROC_V8_OP_17]> {let BufferSize = -1;}
def RES_V8_PF_00 : ProcResGroup<[PROC_V8_PF_00]> {let BufferSize = -1;}
def RES_V8_PF_01 : ProcResGroup<[PROC_V8_PF_01]> {let BufferSize = -1;}
def RES_V8_ST_00 : ProcResGroup<[PROC_V8_ST_00]> {let BufferSize = -1;}
def RES_V8_ST_01 : ProcResGroup<[PROC_V8_ST_01]> {let BufferSize = -1;}
def RES_V8_ST_02 : ProcResGroup<[PROC_V8_ST_02]> {let BufferSize = -1;}
def RES_V8_ST_03 : ProcResGroup<[PROC_V8_ST_03]> {let BufferSize = -1;}
def RES_V8_ST_04 : ProcResGroup<[PROC_V8_ST_04]> {let BufferSize = -1;}
def RES_V8_ST_05 : ProcResGroup<[PROC_V8_ST_05]> {let BufferSize = -1;}
def RES_V8_ST_06 : ProcResGroup<[PROC_V8_ST_06]> {let BufferSize = -1;}
def RES_V8_SWP_00 : ProcResGroup<[PROC_V8_SWP_00]> {let BufferSize = -1;}
def RES_V8_SWP_01 : ProcResGroup<[PROC_V8_SWP_01]> {let BufferSize = -1;}
def RES_V8_SWP_02 : ProcResGroup<[PROC_V8_SWP_02]> {let BufferSize = -1;}
def RES_SIMDFP_LD_00 : ProcResGroup<[PROC_SIMDFP_LD_00]> {let BufferSize = -1;}
def RES_SIMDFP_LD_01 : ProcResGroup<[PROC_SIMDFP_LD_01]> {let BufferSize = -1;}
def RES_SIMDFP_LD_02 : ProcResGroup<[PROC_SIMDFP_LD_02]> {let BufferSize = -1;}
def RES_SIMDFP_LD_03 : ProcResGroup<[PROC_SIMDFP_LD_03]> {let BufferSize = -1;}
def RES_SIMDFP_LD_04 : ProcResGroup<[PROC_SIMDFP_LD_04]> {let BufferSize = -1;}
def RES_SIMDFP_LD_05 : ProcResGroup<[PROC_SIMDFP_LD_05]> {let BufferSize = -1;}
def RES_SIMDFP_LD_06 : ProcResGroup<[PROC_SIMDFP_LD_06]> {let BufferSize = -1;}
def RES_SIMDFP_LD_07 : ProcResGroup<[PROC_SIMDFP_LD_07]> {let BufferSize = -1;}
def RES_SIMDFP_LD_08 : ProcResGroup<[PROC_SIMDFP_LD_08]> {let BufferSize = -1;}
def RES_SIMDFP_LD_09 : ProcResGroup<[PROC_SIMDFP_LD_09]> {let BufferSize = -1;}
def RES_SIMDFP_LD_10 : ProcResGroup<[PROC_SIMDFP_LD_10]> {let BufferSize = -1;}
def RES_SIMDFP_LD_11 : ProcResGroup<[PROC_SIMDFP_LD_11]> {let BufferSize = -1;}
def RES_SIMDFP_LD_12 : ProcResGroup<[PROC_SIMDFP_LD_12]> {let BufferSize = -1;}
def RES_SIMDFP_LD_13 : ProcResGroup<[PROC_SIMDFP_LD_13]> {let BufferSize = -1;}
def RES_SIMDFP_LD_14 : ProcResGroup<[PROC_SIMDFP_LD_14]> {let BufferSize = -1;}
def RES_SIMDFP_LD_15 : ProcResGroup<[PROC_SIMDFP_LD_15]> {let BufferSize = -1;}
def RES_SIMDFP_LD_16 : ProcResGroup<[PROC_SIMDFP_LD_16]> {let BufferSize = -1;}
def RES_SIMDFP_LD_17 : ProcResGroup<[PROC_SIMDFP_LD_17]> {let BufferSize = -1;}
def RES_SIMDFP_LD_18 : ProcResGroup<[PROC_SIMDFP_LD_18]> {let BufferSize = -1;}
def RES_SIMDFP_LD_19 : ProcResGroup<[PROC_SIMDFP_LD_19]> {let BufferSize = -1;}
def RES_SIMDFP_LD_20 : ProcResGroup<[PROC_SIMDFP_LD_20]> {let BufferSize = -1;}
def RES_SIMDFP_LD_21 : ProcResGroup<[PROC_SIMDFP_LD_21]> {let BufferSize = -1;}
def RES_SIMDFP_LD_22 : ProcResGroup<[PROC_SIMDFP_LD_22]> {let BufferSize = -1;}
def RES_SIMDFP_LD_23 : ProcResGroup<[PROC_SIMDFP_LD_23]> {let BufferSize = -1;}
def RES_SIMDFP_LD_24 : ProcResGroup<[PROC_SIMDFP_LD_24]> {let BufferSize = -1;}
def RES_SIMDFP_LD_25 : ProcResGroup<[PROC_SIMDFP_LD_25]> {let BufferSize = -1;}
def RES_SIMDFP_LD_26 : ProcResGroup<[PROC_SIMDFP_LD_26]> {let BufferSize = -1;}
def RES_SIMDFP_OP_00 : ProcResGroup<[PROC_SIMDFP_OP_00]> {let BufferSize = -1;}
def RES_SIMDFP_OP_01 : ProcResGroup<[PROC_SIMDFP_OP_01]> {let BufferSize = -1;}
def RES_SIMDFP_OP_02 : ProcResGroup<[PROC_SIMDFP_OP_02]> {let BufferSize = -1;}
def RES_SIMDFP_OP_03 : ProcResGroup<[PROC_SIMDFP_OP_03]> {let BufferSize = -1;}
def RES_SIMDFP_OP_04 : ProcResGroup<[PROC_SIMDFP_OP_04]> {let BufferSize = -1;}
def RES_SIMDFP_OP_05 : ProcResGroup<[PROC_SIMDFP_OP_05]> {let BufferSize = -1;}
def RES_SIMDFP_OP_06 : ProcResGroup<[PROC_SIMDFP_OP_06]> {let BufferSize = -1;}
def RES_SIMDFP_OP_07 : ProcResGroup<[PROC_SIMDFP_OP_07]> {let BufferSize = -1;}
def RES_SIMDFP_OP_08 : ProcResGroup<[PROC_SIMDFP_OP_08]> {let BufferSize = -1;}
def RES_SIMDFP_OP_09 : ProcResGroup<[PROC_SIMDFP_OP_09]> {let BufferSize = -1;}
def RES_SIMDFP_OP_10 : ProcResGroup<[PROC_SIMDFP_OP_10]> {let BufferSize = -1;}
def RES_SIMDFP_OP_11 : ProcResGroup<[PROC_SIMDFP_OP_11]> {let BufferSize = -1;}
def RES_SIMDFP_OP_12 : ProcResGroup<[PROC_SIMDFP_OP_12]> {let BufferSize = -1;}
def RES_SIMDFP_OP_13 : ProcResGroup<[PROC_SIMDFP_OP_13]> {let BufferSize = -1;}
def RES_SIMDFP_OP_14 : ProcResGroup<[PROC_SIMDFP_OP_14]> {let BufferSize = -1;}
def RES_SIMDFP_OP_15 : ProcResGroup<[PROC_SIMDFP_OP_15]> {let BufferSize = -1;}
def RES_SIMDFP_OP_16 : ProcResGroup<[PROC_SIMDFP_OP_16]> {let BufferSize = -1;}
def RES_SIMDFP_OP_17 : ProcResGroup<[PROC_SIMDFP_OP_17]> {let BufferSize = -1;}
def RES_SIMDFP_OP_18 : ProcResGroup<[PROC_SIMDFP_OP_18]> {let BufferSize = -1;}
def RES_SIMDFP_OP_19 : ProcResGroup<[PROC_SIMDFP_OP_19]> {let BufferSize = -1;}
def RES_SIMDFP_OP_20 : ProcResGroup<[PROC_SIMDFP_OP_20]> {let BufferSize = -1;}
def RES_SIMDFP_OP_21 : ProcResGroup<[PROC_SIMDFP_OP_21]> {let BufferSize = -1;}
def RES_SIMDFP_OP_22 : ProcResGroup<[PROC_SIMDFP_OP_22]> {let BufferSize = -1;}
def RES_SIMDFP_OP_23 : ProcResGroup<[PROC_SIMDFP_OP_23]> {let BufferSize = -1;}
def RES_SIMDFP_OP_24 : ProcResGroup<[PROC_SIMDFP_OP_24]> {let BufferSize = -1;}
def RES_SIMDFP_OP_25 : ProcResGroup<[PROC_SIMDFP_OP_25]> {let BufferSize = -1;}
def RES_SIMDFP_OP_26 : ProcResGroup<[PROC_SIMDFP_OP_26]> {let BufferSize = -1;}
def RES_SIMDFP_OP_27 : ProcResGroup<[PROC_SIMDFP_OP_27]> {let BufferSize = -1;}
def RES_SIMDFP_OP_28 : ProcResGroup<[PROC_SIMDFP_OP_28]> {let BufferSize = -1;}
def RES_SIMDFP_OP_29 : ProcResGroup<[PROC_SIMDFP_OP_29]> {let BufferSize = -1;}
def RES_SIMDFP_OP_30 : ProcResGroup<[PROC_SIMDFP_OP_30]> {let BufferSize = -1;}
def RES_SIMDFP_OP_31 : ProcResGroup<[PROC_SIMDFP_OP_31]> {let BufferSize = -1;}
def RES_SIMDFP_OP_32 : ProcResGroup<[PROC_SIMDFP_OP_32]> {let BufferSize = -1;}
def RES_SIMDFP_OP_33 : ProcResGroup<[PROC_SIMDFP_OP_33]> {let BufferSize = -1;}
def RES_SIMDFP_OP_34 : ProcResGroup<[PROC_SIMDFP_OP_34]> {let BufferSize = -1;}
def RES_SIMDFP_OP_35 : ProcResGroup<[PROC_SIMDFP_OP_35]> {let BufferSize = -1;}
def RES_SIMDFP_OP_36 : ProcResGroup<[PROC_SIMDFP_OP_36]> {let BufferSize = -1;}
def RES_SIMDFP_OP_37 : ProcResGroup<[PROC_SIMDFP_OP_37]> {let BufferSize = -1;}
def RES_SIMDFP_OP_38 : ProcResGroup<[PROC_SIMDFP_OP_38]> {let BufferSize = -1;}
def RES_SIMDFP_OP_39 : ProcResGroup<[PROC_SIMDFP_OP_39]> {let BufferSize = -1;}
def RES_SIMDFP_OP_40 : ProcResGroup<[PROC_SIMDFP_OP_40]> {let BufferSize = -1;}
def RES_SIMDFP_OP_41 : ProcResGroup<[PROC_SIMDFP_OP_41]> {let BufferSize = -1;}
def RES_SIMDFP_OP_42 : ProcResGroup<[PROC_SIMDFP_OP_42]> {let BufferSize = -1;}
def RES_SIMDFP_OP_43 : ProcResGroup<[PROC_SIMDFP_OP_43]> {let BufferSize = -1;}
def RES_SIMDFP_OP_44 : ProcResGroup<[PROC_SIMDFP_OP_44]> {let BufferSize = -1;}
def RES_SIMDFP_OP_45 : ProcResGroup<[PROC_SIMDFP_OP_45]> {let BufferSize = -1;}
def RES_SIMDFP_OP_46 : ProcResGroup<[PROC_SIMDFP_OP_46]> {let BufferSize = -1;}
def RES_SIMDFP_OP_47 : ProcResGroup<[PROC_SIMDFP_OP_47]> {let BufferSize = -1;}
def RES_SIMDFP_OP_48 : ProcResGroup<[PROC_SIMDFP_OP_48]> {let BufferSize = -1;}
def RES_SIMDFP_OP_49 : ProcResGroup<[PROC_SIMDFP_OP_49]> {let BufferSize = -1;}
def RES_SIMDFP_OP_50 : ProcResGroup<[PROC_SIMDFP_OP_50]> {let BufferSize = -1;}
def RES_SIMDFP_OP_51 : ProcResGroup<[PROC_SIMDFP_OP_51]> {let BufferSize = -1;}
def RES_SIMDFP_OP_52 : ProcResGroup<[PROC_SIMDFP_OP_52]> {let BufferSize = -1;}
def RES_SIMDFP_OP_53 : ProcResGroup<[PROC_SIMDFP_OP_53]> {let BufferSize = -1;}
def RES_SIMDFP_OP_54 : ProcResGroup<[PROC_SIMDFP_OP_54]> {let BufferSize = -1;}
def RES_SIMDFP_OP_55 : ProcResGroup<[PROC_SIMDFP_OP_55]> {let BufferSize = -1;}
def RES_SIMDFP_OP_56 : ProcResGroup<[PROC_SIMDFP_OP_56]> {let BufferSize = -1;}
def RES_SIMDFP_OP_57 : ProcResGroup<[PROC_SIMDFP_OP_57]> {let BufferSize = -1;}
def RES_SIMDFP_ST_00 : ProcResGroup<[PROC_SIMDFP_ST_00]> {let BufferSize = -1;}
def RES_SIMDFP_ST_01 : ProcResGroup<[PROC_SIMDFP_ST_01]> {let BufferSize = -1;}
def RES_SIMDFP_ST_02 : ProcResGroup<[PROC_SIMDFP_ST_02]> {let BufferSize = -1;}
def RES_SIMDFP_ST_03 : ProcResGroup<[PROC_SIMDFP_ST_03]> {let BufferSize = -1;}
def RES_SIMDFP_ST_04 : ProcResGroup<[PROC_SIMDFP_ST_04]> {let BufferSize = -1;}
def RES_SIMDFP_ST_05 : ProcResGroup<[PROC_SIMDFP_ST_05]> {let BufferSize = -1;}
def RES_SIMDFP_ST_06 : ProcResGroup<[PROC_SIMDFP_ST_06]> {let BufferSize = -1;}
def RES_SIMDFP_ST_07 : ProcResGroup<[PROC_SIMDFP_ST_07]> {let BufferSize = -1;}
def RES_SIMDFP_ST_08 : ProcResGroup<[PROC_SIMDFP_ST_08]> {let BufferSize = -1;}
def RES_SIMDFP_ST_09 : ProcResGroup<[PROC_SIMDFP_ST_09]> {let BufferSize = -1;}
def RES_SVE_LD_00 : ProcResGroup<[PROC_SVE_LD_00]> {let BufferSize = -1;}
def RES_SVE_LD_01 : ProcResGroup<[PROC_SVE_LD_01]> {let BufferSize = -1;}
def RES_SVE_LD_02 : ProcResGroup<[PROC_SVE_LD_02]> {let BufferSize = -1;}
def RES_SVE_LD_03 : ProcResGroup<[PROC_SVE_LD_03]> {let BufferSize = -1;}
def RES_SVE_LD_04 : ProcResGroup<[PROC_SVE_LD_04]> {let BufferSize = -1;}
def RES_SVE_LD_05 : ProcResGroup<[PROC_SVE_LD_05]> {let BufferSize = -1;}
def RES_SVE_LD_06 : ProcResGroup<[PROC_SVE_LD_06]> {let BufferSize = -1;}
def RES_SVE_LD_07 : ProcResGroup<[PROC_SVE_LD_07]> {let BufferSize = -1;}
def RES_SVE_LD_08 : ProcResGroup<[PROC_SVE_LD_08]> {let BufferSize = -1;}
def RES_SVE_LD_09 : ProcResGroup<[PROC_SVE_LD_09]> {let BufferSize = -1;}
def RES_SVE_LD_10 : ProcResGroup<[PROC_SVE_LD_10]> {let BufferSize = -1;}
def RES_SVE_LD_11 : ProcResGroup<[PROC_SVE_LD_11]> {let BufferSize = -1;}
def RES_SVE_LD_12 : ProcResGroup<[PROC_SVE_LD_12]> {let BufferSize = -1;}
def RES_SVE_LD_13 : ProcResGroup<[PROC_SVE_LD_13]> {let BufferSize = -1;}
def RES_SVE_LD_14 : ProcResGroup<[PROC_SVE_LD_14]> {let BufferSize = -1;}
def RES_SVE_LD_15 : ProcResGroup<[PROC_SVE_LD_15]> {let BufferSize = -1;}
def RES_SVE_MISC_00 : ProcResGroup<[PROC_SVE_MISC_00]> {let BufferSize = -1;}
def RES_SVE_MISC_01 : ProcResGroup<[PROC_SVE_MISC_01]> {let BufferSize = -1;}
def RES_SVE_OP_00 : ProcResGroup<[PROC_SVE_OP_00]> {let BufferSize = -1;}
def RES_SVE_OP_01 : ProcResGroup<[PROC_SVE_OP_01]> {let BufferSize = -1;}
def RES_SVE_OP_02 : ProcResGroup<[PROC_SVE_OP_02]> {let BufferSize = -1;}
def RES_SVE_OP_03 : ProcResGroup<[PROC_SVE_OP_03]> {let BufferSize = -1;}
def RES_SVE_OP_05 : ProcResGroup<[PROC_SVE_OP_05]> {let BufferSize = -1;}
def RES_SVE_OP_06 : ProcResGroup<[PROC_SVE_OP_06]> {let BufferSize = -1;}
def RES_SVE_OP_07 : ProcResGroup<[PROC_SVE_OP_07]> {let BufferSize = -1;}
def RES_SVE_OP_08 : ProcResGroup<[PROC_SVE_OP_08]> {let BufferSize = -1;}
def RES_SVE_OP_09 : ProcResGroup<[PROC_SVE_OP_09]> {let BufferSize = -1;}
def RES_SVE_OP_10 : ProcResGroup<[PROC_SVE_OP_10]> {let BufferSize = -1;}
def RES_SVE_OP_11 : ProcResGroup<[PROC_SVE_OP_11]> {let BufferSize = -1;}
def RES_SVE_OP_12 : ProcResGroup<[PROC_SVE_OP_12]> {let BufferSize = -1;}
def RES_SVE_OP_13 : ProcResGroup<[PROC_SVE_OP_13]> {let BufferSize = -1;}
def RES_SVE_OP_14 : ProcResGroup<[PROC_SVE_OP_14]> {let BufferSize = -1;}
def RES_SVE_OP_15 : ProcResGroup<[PROC_SVE_OP_15]> {let BufferSize = -1;}
def RES_SVE_OP_16 : ProcResGroup<[PROC_SVE_OP_16]> {let BufferSize = -1;}
def RES_SVE_OP_17 : ProcResGroup<[PROC_SVE_OP_17]> {let BufferSize = -1;}
def RES_SVE_OP_18 : ProcResGroup<[PROC_SVE_OP_18]> {let BufferSize = -1;}
def RES_SVE_OP_19 : ProcResGroup<[PROC_SVE_OP_19]> {let BufferSize = -1;}
def RES_SVE_OP_20 : ProcResGroup<[PROC_SVE_OP_20]> {let BufferSize = -1;}
def RES_SVE_OP_21 : ProcResGroup<[PROC_SVE_OP_21]> {let BufferSize = -1;}
def RES_SVE_OP_22 : ProcResGroup<[PROC_SVE_OP_22]> {let BufferSize = -1;}
def RES_SVE_OP_23 : ProcResGroup<[PROC_SVE_OP_23]> {let BufferSize = -1;}
def RES_SVE_OP_24 : ProcResGroup<[PROC_SVE_OP_24]> {let BufferSize = -1;}
def RES_SVE_OP_25 : ProcResGroup<[PROC_SVE_OP_25]> {let BufferSize = -1;}
def RES_SVE_OP_26 : ProcResGroup<[PROC_SVE_OP_26]> {let BufferSize = -1;}
def RES_SVE_OP_27 : ProcResGroup<[PROC_SVE_OP_27]> {let BufferSize = -1;}
def RES_SVE_OP_28 : ProcResGroup<[PROC_SVE_OP_28]> {let BufferSize = -1;}
def RES_SVE_OP_29 : ProcResGroup<[PROC_SVE_OP_29]> {let BufferSize = -1;}
def RES_SVE_OP_30 : ProcResGroup<[PROC_SVE_OP_30]> {let BufferSize = -1;}
def RES_SVE_OP_31 : ProcResGroup<[PROC_SVE_OP_31]> {let BufferSize = -1;}
def RES_SVE_OP_32 : ProcResGroup<[PROC_SVE_OP_32]> {let BufferSize = -1;}
def RES_SVE_OP_33 : ProcResGroup<[PROC_SVE_OP_33]> {let BufferSize = -1;}
def RES_SVE_OP_34 : ProcResGroup<[PROC_SVE_OP_34]> {let BufferSize = -1;}
def RES_SVE_OP_35 : ProcResGroup<[PROC_SVE_OP_35]> {let BufferSize = -1;}
def RES_SVE_OP_36 : ProcResGroup<[PROC_SVE_OP_36]> {let BufferSize = -1;}
def RES_SVE_OP_37 : ProcResGroup<[PROC_SVE_OP_37]> {let BufferSize = -1;}
def RES_SVE_OP_38 : ProcResGroup<[PROC_SVE_OP_38]> {let BufferSize = -1;}
def RES_SVE_OP_39 : ProcResGroup<[PROC_SVE_OP_39]> {let BufferSize = -1;}
def RES_SVE_OP_40 : ProcResGroup<[PROC_SVE_OP_40]> {let BufferSize = -1;}
def RES_SVE_OP_41 : ProcResGroup<[PROC_SVE_OP_41]> {let BufferSize = -1;}
def RES_SVE_OP_42 : ProcResGroup<[PROC_SVE_OP_42]> {let BufferSize = -1;}
def RES_SVE_OP_43 : ProcResGroup<[PROC_SVE_OP_43]> {let BufferSize = -1;}
def RES_SVE_OP_44 : ProcResGroup<[PROC_SVE_OP_44]> {let BufferSize = -1;}
def RES_SVE_OP_45 : ProcResGroup<[PROC_SVE_OP_45]> {let BufferSize = -1;}
def RES_SVE_OP_46 : ProcResGroup<[PROC_SVE_OP_46]> {let BufferSize = -1;}
def RES_SVE_OP_47 : ProcResGroup<[PROC_SVE_OP_47]> {let BufferSize = -1;}
def RES_SVE_OP_48 : ProcResGroup<[PROC_SVE_OP_48]> {let BufferSize = -1;}
def RES_SVE_OP_49 : ProcResGroup<[PROC_SVE_OP_49]> {let BufferSize = -1;}
def RES_SVE_OP_50 : ProcResGroup<[PROC_SVE_OP_50]> {let BufferSize = -1;}
def RES_SVE_OP_51 : ProcResGroup<[PROC_SVE_OP_51]> {let BufferSize = -1;}
def RES_SVE_OP_52 : ProcResGroup<[PROC_SVE_OP_52]> {let BufferSize = -1;}
def RES_SVE_OP_53 : ProcResGroup<[PROC_SVE_OP_53]> {let BufferSize = -1;}
def RES_SVE_OP_54 : ProcResGroup<[PROC_SVE_OP_54]> {let BufferSize = -1;}
def RES_SVE_OP_55 : ProcResGroup<[PROC_SVE_OP_55]> {let BufferSize = -1;}
def RES_SVE_OP_56 : ProcResGroup<[PROC_SVE_OP_56]> {let BufferSize = -1;}
def RES_SVE_OP_57 : ProcResGroup<[PROC_SVE_OP_57]> {let BufferSize = -1;}
def RES_SVE_OP_58 : ProcResGroup<[PROC_SVE_OP_58]> {let BufferSize = -1;}
def RES_SVE_OP_59 : ProcResGroup<[PROC_SVE_OP_59]> {let BufferSize = -1;}
def RES_SVE_PF_00 : ProcResGroup<[PROC_SVE_PF_00]> {let BufferSize = -1;}
def RES_SVE_PF_01 : ProcResGroup<[PROC_SVE_PF_01]> {let BufferSize = -1;}
def RES_SVE_PF_02 : ProcResGroup<[PROC_SVE_PF_02]> {let BufferSize = -1;}
def RES_SVE_PF_03 : ProcResGroup<[PROC_SVE_PF_03]> {let BufferSize = -1;}
def RES_SVE_PF_04 : ProcResGroup<[PROC_SVE_PF_04]> {let BufferSize = -1;}
def RES_SVE_PRED_00 : ProcResGroup<[PROC_SVE_PRED_00]> {let BufferSize = -1;}
def RES_SVE_PRED_01 : ProcResGroup<[PROC_SVE_PRED_01]> {let BufferSize = -1;}
def RES_SVE_PRED_02 : ProcResGroup<[PROC_SVE_PRED_02]> {let BufferSize = -1;}
def RES_SVE_PRED_03 : ProcResGroup<[PROC_SVE_PRED_03]> {let BufferSize = -1;}
def RES_SVE_PRED_04 : ProcResGroup<[PROC_SVE_PRED_04]> {let BufferSize = -1;}
def RES_SVE_PRED_05 : ProcResGroup<[PROC_SVE_PRED_05]> {let BufferSize = -1;}
def RES_SVE_ST_00 : ProcResGroup<[PROC_SVE_ST_00]> {let BufferSize = -1;}
def RES_SVE_ST_01 : ProcResGroup<[PROC_SVE_ST_01]> {let BufferSize = -1;}
def RES_SVE_ST_02 : ProcResGroup<[PROC_SVE_ST_02]> {let BufferSize = -1;}
def RES_SVE_ST_03 : ProcResGroup<[PROC_SVE_ST_03]> {let BufferSize = -1;}
def RES_SVE_ST_04 : ProcResGroup<[PROC_SVE_ST_04]> {let BufferSize = -1;}
def RES_SVE_ST_05 : ProcResGroup<[PROC_SVE_ST_05]> {let BufferSize = -1;}
def RES_SVE_ST_06 : ProcResGroup<[PROC_SVE_ST_06]> {let BufferSize = -1;}
def RES_SVE_ST_07 : ProcResGroup<[PROC_SVE_ST_07]> {let BufferSize = -1;}
def RES_SVE_ST_08 : ProcResGroup<[PROC_SVE_ST_08]> {let BufferSize = -1;}
def RES_SVE_ST_09 : ProcResGroup<[PROC_SVE_ST_09]> {let BufferSize = -1;}
def RES_SVE_ST_10 : ProcResGroup<[PROC_SVE_ST_10]> {let BufferSize = -1;}
def RES_SVE_ST_11 : ProcResGroup<[PROC_SVE_ST_11]> {let BufferSize = -1;}
def RES_SVE_ST_12 : ProcResGroup<[PROC_SVE_ST_12]> {let BufferSize = -1;}
def RES_SVE_ST_13 : ProcResGroup<[PROC_SVE_ST_13]> {let BufferSize = -1;}
def RES_SVE_ST_14 : ProcResGroup<[PROC_SVE_ST_14]> {let BufferSize = -1;}
def RES_SVE_ST_15 : ProcResGroup<[PROC_SVE_ST_15]> {let BufferSize = -1;}
def RES_SVE_ST_16 : ProcResGroup<[PROC_SVE_ST_16]> {let BufferSize = -1;}


def SCHED_V8_BR_00 : SchedWriteRes<[RES_V8_BR_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_BR_01 : SchedWriteRes<[RES_V8_BR_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_BR_02 : SchedWriteRes<[RES_V8_BR_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_BR_03 : SchedWriteRes<[RES_V8_BR_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LDST_00 : SchedWriteRes<[RES_V8_LDST_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LDST_01 : SchedWriteRes<[RES_V8_LDST_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LD_00 : SchedWriteRes<[RES_V8_LD_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LD_01 : SchedWriteRes<[RES_V8_LD_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LD_02 : SchedWriteRes<[RES_V8_LD_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LD_03 : SchedWriteRes<[RES_V8_LD_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LD_04 : SchedWriteRes<[RES_V8_LD_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LD_05 : SchedWriteRes<[RES_V8_LD_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_LD_06 : SchedWriteRes<[RES_V8_LD_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_MISC_00 : SchedWriteRes<[RES_V8_MISC_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_MISC_01 : SchedWriteRes<[RES_V8_MISC_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_MISC_02 : SchedWriteRes<[RES_V8_MISC_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_MISC_03 : SchedWriteRes<[RES_V8_MISC_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_MISC_04 : SchedWriteRes<[RES_V8_MISC_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_MISC_05 : SchedWriteRes<[RES_V8_MISC_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_00 : SchedWriteRes<[RES_V8_OP_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_01 : SchedWriteRes<[RES_V8_OP_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_02 : SchedWriteRes<[RES_V8_OP_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_03 : SchedWriteRes<[RES_V8_OP_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_04 : SchedWriteRes<[RES_V8_OP_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_05 : SchedWriteRes<[RES_V8_OP_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_06 : SchedWriteRes<[RES_V8_OP_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_07 : SchedWriteRes<[RES_V8_OP_07]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_08 : SchedWriteRes<[RES_V8_OP_08]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_09 : SchedWriteRes<[RES_V8_OP_09]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_10 : SchedWriteRes<[RES_V8_OP_10]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_11 : SchedWriteRes<[RES_V8_OP_11]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_12 : SchedWriteRes<[RES_V8_OP_12]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_13 : SchedWriteRes<[RES_V8_OP_13]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_14 : SchedWriteRes<[RES_V8_OP_14]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_15 : SchedWriteRes<[RES_V8_OP_15]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_16 : SchedWriteRes<[RES_V8_OP_16]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_OP_17 : SchedWriteRes<[RES_V8_OP_17]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_PF_00 : SchedWriteRes<[RES_V8_PF_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_PF_01 : SchedWriteRes<[RES_V8_PF_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_ST_00 : SchedWriteRes<[RES_V8_ST_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_ST_01 : SchedWriteRes<[RES_V8_ST_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_ST_02 : SchedWriteRes<[RES_V8_ST_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_ST_03 : SchedWriteRes<[RES_V8_ST_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_ST_04 : SchedWriteRes<[RES_V8_ST_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_ST_05 : SchedWriteRes<[RES_V8_ST_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_ST_06 : SchedWriteRes<[RES_V8_ST_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_SWP_00 : SchedWriteRes<[RES_V8_SWP_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_SWP_01 : SchedWriteRes<[RES_V8_SWP_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_V8_SWP_02 : SchedWriteRes<[RES_V8_SWP_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_00 : SchedWriteRes<[RES_SIMDFP_LD_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_01 : SchedWriteRes<[RES_SIMDFP_LD_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_02 : SchedWriteRes<[RES_SIMDFP_LD_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_03 : SchedWriteRes<[RES_SIMDFP_LD_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_04 : SchedWriteRes<[RES_SIMDFP_LD_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_05 : SchedWriteRes<[RES_SIMDFP_LD_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_06 : SchedWriteRes<[RES_SIMDFP_LD_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_07 : SchedWriteRes<[RES_SIMDFP_LD_07]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_08 : SchedWriteRes<[RES_SIMDFP_LD_08]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_09 : SchedWriteRes<[RES_SIMDFP_LD_09]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_10 : SchedWriteRes<[RES_SIMDFP_LD_10]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_11 : SchedWriteRes<[RES_SIMDFP_LD_11]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_12 : SchedWriteRes<[RES_SIMDFP_LD_12]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_13 : SchedWriteRes<[RES_SIMDFP_LD_13]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_14 : SchedWriteRes<[RES_SIMDFP_LD_14]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_15 : SchedWriteRes<[RES_SIMDFP_LD_15]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_16 : SchedWriteRes<[RES_SIMDFP_LD_16]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_17 : SchedWriteRes<[RES_SIMDFP_LD_17]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_18 : SchedWriteRes<[RES_SIMDFP_LD_18]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_19 : SchedWriteRes<[RES_SIMDFP_LD_19]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_20 : SchedWriteRes<[RES_SIMDFP_LD_20]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_21 : SchedWriteRes<[RES_SIMDFP_LD_21]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_22 : SchedWriteRes<[RES_SIMDFP_LD_22]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_23 : SchedWriteRes<[RES_SIMDFP_LD_23]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_24 : SchedWriteRes<[RES_SIMDFP_LD_24]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_25 : SchedWriteRes<[RES_SIMDFP_LD_25]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_LD_26 : SchedWriteRes<[RES_SIMDFP_LD_26]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_00 : SchedWriteRes<[RES_SIMDFP_OP_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_01 : SchedWriteRes<[RES_SIMDFP_OP_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_02 : SchedWriteRes<[RES_SIMDFP_OP_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_03 : SchedWriteRes<[RES_SIMDFP_OP_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_04 : SchedWriteRes<[RES_SIMDFP_OP_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_05 : SchedWriteRes<[RES_SIMDFP_OP_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_06 : SchedWriteRes<[RES_SIMDFP_OP_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_07 : SchedWriteRes<[RES_SIMDFP_OP_07]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_08 : SchedWriteRes<[RES_SIMDFP_OP_08]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_09 : SchedWriteRes<[RES_SIMDFP_OP_09]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_10 : SchedWriteRes<[RES_SIMDFP_OP_10]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_11 : SchedWriteRes<[RES_SIMDFP_OP_11]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_12 : SchedWriteRes<[RES_SIMDFP_OP_12]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_13 : SchedWriteRes<[RES_SIMDFP_OP_13]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_14 : SchedWriteRes<[RES_SIMDFP_OP_14]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_15 : SchedWriteRes<[RES_SIMDFP_OP_15]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_16 : SchedWriteRes<[RES_SIMDFP_OP_16]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_17 : SchedWriteRes<[RES_SIMDFP_OP_17]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_18 : SchedWriteRes<[RES_SIMDFP_OP_18]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_19 : SchedWriteRes<[RES_SIMDFP_OP_19]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_20 : SchedWriteRes<[RES_SIMDFP_OP_20]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_21 : SchedWriteRes<[RES_SIMDFP_OP_21]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_22 : SchedWriteRes<[RES_SIMDFP_OP_22]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_23 : SchedWriteRes<[RES_SIMDFP_OP_23]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_24 : SchedWriteRes<[RES_SIMDFP_OP_24]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_25 : SchedWriteRes<[RES_SIMDFP_OP_25]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_26 : SchedWriteRes<[RES_SIMDFP_OP_26]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_27 : SchedWriteRes<[RES_SIMDFP_OP_27]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_28 : SchedWriteRes<[RES_SIMDFP_OP_28]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_29 : SchedWriteRes<[RES_SIMDFP_OP_29]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_30 : SchedWriteRes<[RES_SIMDFP_OP_30]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_31 : SchedWriteRes<[RES_SIMDFP_OP_31]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_32 : SchedWriteRes<[RES_SIMDFP_OP_32]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_33 : SchedWriteRes<[RES_SIMDFP_OP_33]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_34 : SchedWriteRes<[RES_SIMDFP_OP_34]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_35 : SchedWriteRes<[RES_SIMDFP_OP_35]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_36 : SchedWriteRes<[RES_SIMDFP_OP_36]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_37 : SchedWriteRes<[RES_SIMDFP_OP_37]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_38 : SchedWriteRes<[RES_SIMDFP_OP_38]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_39 : SchedWriteRes<[RES_SIMDFP_OP_39]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_40 : SchedWriteRes<[RES_SIMDFP_OP_40]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_41 : SchedWriteRes<[RES_SIMDFP_OP_41]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_42 : SchedWriteRes<[RES_SIMDFP_OP_42]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_43 : SchedWriteRes<[RES_SIMDFP_OP_43]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_44 : SchedWriteRes<[RES_SIMDFP_OP_44]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_45 : SchedWriteRes<[RES_SIMDFP_OP_45]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_46 : SchedWriteRes<[RES_SIMDFP_OP_46]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_47 : SchedWriteRes<[RES_SIMDFP_OP_47]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_48 : SchedWriteRes<[RES_SIMDFP_OP_48]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_49 : SchedWriteRes<[RES_SIMDFP_OP_49]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_50 : SchedWriteRes<[RES_SIMDFP_OP_50]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_51 : SchedWriteRes<[RES_SIMDFP_OP_51]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_52 : SchedWriteRes<[RES_SIMDFP_OP_52]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_53 : SchedWriteRes<[RES_SIMDFP_OP_53]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_54 : SchedWriteRes<[RES_SIMDFP_OP_54]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_55 : SchedWriteRes<[RES_SIMDFP_OP_55]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_56 : SchedWriteRes<[RES_SIMDFP_OP_56]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_OP_57 : SchedWriteRes<[RES_SIMDFP_OP_57]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_00 : SchedWriteRes<[RES_SIMDFP_ST_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_01 : SchedWriteRes<[RES_SIMDFP_ST_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_02 : SchedWriteRes<[RES_SIMDFP_ST_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_03 : SchedWriteRes<[RES_SIMDFP_ST_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_04 : SchedWriteRes<[RES_SIMDFP_ST_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_05 : SchedWriteRes<[RES_SIMDFP_ST_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_06 : SchedWriteRes<[RES_SIMDFP_ST_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_07 : SchedWriteRes<[RES_SIMDFP_ST_07]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_08 : SchedWriteRes<[RES_SIMDFP_ST_08]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SIMDFP_ST_09 : SchedWriteRes<[RES_SIMDFP_ST_09]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_00 : SchedWriteRes<[RES_SVE_LD_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_01 : SchedWriteRes<[RES_SVE_LD_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_02 : SchedWriteRes<[RES_SVE_LD_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_03 : SchedWriteRes<[RES_SVE_LD_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_04 : SchedWriteRes<[RES_SVE_LD_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_05 : SchedWriteRes<[RES_SVE_LD_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_06 : SchedWriteRes<[RES_SVE_LD_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_07 : SchedWriteRes<[RES_SVE_LD_07]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_08 : SchedWriteRes<[RES_SVE_LD_08]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_09 : SchedWriteRes<[RES_SVE_LD_09]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_10 : SchedWriteRes<[RES_SVE_LD_10]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_11 : SchedWriteRes<[RES_SVE_LD_11]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_12 : SchedWriteRes<[RES_SVE_LD_12]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_13 : SchedWriteRes<[RES_SVE_LD_13]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_14 : SchedWriteRes<[RES_SVE_LD_14]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_LD_15 : SchedWriteRes<[RES_SVE_LD_15]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_MISC_00 : SchedWriteRes<[RES_SVE_MISC_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_MISC_01 : SchedWriteRes<[RES_SVE_MISC_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_00 : SchedWriteRes<[RES_SVE_OP_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_01 : SchedWriteRes<[RES_SVE_OP_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_02 : SchedWriteRes<[RES_SVE_OP_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_03 : SchedWriteRes<[RES_SVE_OP_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_05 : SchedWriteRes<[RES_SVE_OP_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_06 : SchedWriteRes<[RES_SVE_OP_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_07 : SchedWriteRes<[RES_SVE_OP_07]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_08 : SchedWriteRes<[RES_SVE_OP_08]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_09 : SchedWriteRes<[RES_SVE_OP_09]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_10 : SchedWriteRes<[RES_SVE_OP_10]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_11 : SchedWriteRes<[RES_SVE_OP_11]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_12 : SchedWriteRes<[RES_SVE_OP_12]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_13 : SchedWriteRes<[RES_SVE_OP_13]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_14 : SchedWriteRes<[RES_SVE_OP_14]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_15 : SchedWriteRes<[RES_SVE_OP_15]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_16 : SchedWriteRes<[RES_SVE_OP_16]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_17 : SchedWriteRes<[RES_SVE_OP_17]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_18 : SchedWriteRes<[RES_SVE_OP_18]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_19 : SchedWriteRes<[RES_SVE_OP_19]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_20 : SchedWriteRes<[RES_SVE_OP_20]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_21 : SchedWriteRes<[RES_SVE_OP_21]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_22 : SchedWriteRes<[RES_SVE_OP_22]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_23 : SchedWriteRes<[RES_SVE_OP_23]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_24 : SchedWriteRes<[RES_SVE_OP_24]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_25 : SchedWriteRes<[RES_SVE_OP_25]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_26 : SchedWriteRes<[RES_SVE_OP_26]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_27 : SchedWriteRes<[RES_SVE_OP_27]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_28 : SchedWriteRes<[RES_SVE_OP_28]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_29 : SchedWriteRes<[RES_SVE_OP_29]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_30 : SchedWriteRes<[RES_SVE_OP_30]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_31 : SchedWriteRes<[RES_SVE_OP_31]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_32 : SchedWriteRes<[RES_SVE_OP_32]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_33 : SchedWriteRes<[RES_SVE_OP_33]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_34 : SchedWriteRes<[RES_SVE_OP_34]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_35 : SchedWriteRes<[RES_SVE_OP_35]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_36 : SchedWriteRes<[RES_SVE_OP_36]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_37 : SchedWriteRes<[RES_SVE_OP_37]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_38 : SchedWriteRes<[RES_SVE_OP_38]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_39 : SchedWriteRes<[RES_SVE_OP_39]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_40 : SchedWriteRes<[RES_SVE_OP_40]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_41 : SchedWriteRes<[RES_SVE_OP_41]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_42 : SchedWriteRes<[RES_SVE_OP_42]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_43 : SchedWriteRes<[RES_SVE_OP_43]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_44 : SchedWriteRes<[RES_SVE_OP_44]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_45 : SchedWriteRes<[RES_SVE_OP_45]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_46 : SchedWriteRes<[RES_SVE_OP_46]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_47 : SchedWriteRes<[RES_SVE_OP_47]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_48 : SchedWriteRes<[RES_SVE_OP_48]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_49 : SchedWriteRes<[RES_SVE_OP_49]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_50 : SchedWriteRes<[RES_SVE_OP_50]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_51 : SchedWriteRes<[RES_SVE_OP_51]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_52 : SchedWriteRes<[RES_SVE_OP_52]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_53 : SchedWriteRes<[RES_SVE_OP_53]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_54 : SchedWriteRes<[RES_SVE_OP_54]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_55 : SchedWriteRes<[RES_SVE_OP_55]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_56 : SchedWriteRes<[RES_SVE_OP_56]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_57 : SchedWriteRes<[RES_SVE_OP_57]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_58 : SchedWriteRes<[RES_SVE_OP_58]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_OP_59 : SchedWriteRes<[RES_SVE_OP_59]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PF_00 : SchedWriteRes<[RES_SVE_PF_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PF_01 : SchedWriteRes<[RES_SVE_PF_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PF_02 : SchedWriteRes<[RES_SVE_PF_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PF_03 : SchedWriteRes<[RES_SVE_PF_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PF_04 : SchedWriteRes<[RES_SVE_PF_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PRED_00 : SchedWriteRes<[RES_SVE_PRED_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PRED_01 : SchedWriteRes<[RES_SVE_PRED_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PRED_02 : SchedWriteRes<[RES_SVE_PRED_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PRED_03 : SchedWriteRes<[RES_SVE_PRED_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PRED_04 : SchedWriteRes<[RES_SVE_PRED_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_PRED_05 : SchedWriteRes<[RES_SVE_PRED_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_00 : SchedWriteRes<[RES_SVE_ST_00]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_01 : SchedWriteRes<[RES_SVE_ST_01]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_02 : SchedWriteRes<[RES_SVE_ST_02]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_03 : SchedWriteRes<[RES_SVE_ST_03]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_04 : SchedWriteRes<[RES_SVE_ST_04]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_05 : SchedWriteRes<[RES_SVE_ST_05]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_06 : SchedWriteRes<[RES_SVE_ST_06]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_07 : SchedWriteRes<[RES_SVE_ST_07]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_08 : SchedWriteRes<[RES_SVE_ST_08]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_09 : SchedWriteRes<[RES_SVE_ST_09]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_10 : SchedWriteRes<[RES_SVE_ST_10]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_11 : SchedWriteRes<[RES_SVE_ST_11]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_12 : SchedWriteRes<[RES_SVE_ST_12]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_13 : SchedWriteRes<[RES_SVE_ST_13]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_14 : SchedWriteRes<[RES_SVE_ST_14]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_15 : SchedWriteRes<[RES_SVE_ST_15]> {let Latency = 0;let ResourceCycles = [0];}
def SCHED_SVE_ST_16 : SchedWriteRes<[RES_SVE_ST_16]> {let Latency = 0;let ResourceCycles = [0];}

}
