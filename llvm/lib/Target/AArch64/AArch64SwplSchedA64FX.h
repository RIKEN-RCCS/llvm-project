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

#include "AArch64SwplTargetMachine.h"
#include "llvm/CodeGen/SwplTargetMachine.h"

namespace llvm {

struct AArch64SwplSchedA64FX{
  static unsigned VectorLength;
  /// 命令の種類
  enum InstKind {
    INT_OP = 0x1000,
    INT_LD = 0x2000,
    INT_ST = 0x3000,
    SIMDFP_SVE_OP = 0x4000,
    SIMDFP_SVE_LD = 0x5000,
    SIMDFP_SVE_ST = 0x6000,
    SVE_CMP_INST = 0x7000,
    PREDICATE_OP = 0x8000,
    PREDICATE_LD = 0x9000,
    PREDICATE_ST = 0xA000
  };

  /// 資源
  enum PortKind {
    FLA=1, FLB=2, EXA=3, EXB=4, EAGA=5, EAGB=6, PRX=7, BR=8,
    LSU1=9, LSU2=10,
    FLA_C=11, FLB_C=12, EXA_C=13, EXB_C=14, EAGA_C=15, EAGB_C=16,
    FLA_E=17, EXA_E=18, EXB_E=19, END 
  };


  /// 利用資源ID
  enum ResourceID {
    MI_NA,
    MI_INT_OP_001 = INT_OP + 1,
    MI_INT_OP_002 = INT_OP + 2,
    MI_INT_OP_003 = INT_OP + 3,
    MI_INT_OP_004 = INT_OP + 4,
    MI_INT_OP_005 = INT_OP + 5,
    MI_INT_OP_006 = INT_OP + 6,
    MI_INT_OP_007 = INT_OP + 7,
    MI_INT_OP_008 = INT_OP + 8,
    MI_INT_LD_001 = INT_LD + 1,
    MI_INT_LD_002 = INT_LD + 2,
    MI_INT_ST_001 = INT_ST + 1,
    MI_INT_ST_002 = INT_ST + 2,
    MI_SIMDFP_SVE_OP_001 = SIMDFP_SVE_OP + 1,
    MI_SIMDFP_SVE_OP_002 = SIMDFP_SVE_OP + 2,
    MI_SIMDFP_SVE_OP_003 = SIMDFP_SVE_OP + 3,
    MI_SIMDFP_SVE_OP_004 = SIMDFP_SVE_OP + 4,
    MI_SIMDFP_SVE_OP_005 = SIMDFP_SVE_OP + 5,
    MI_SIMDFP_SVE_OP_006 = SIMDFP_SVE_OP + 6,
    MI_SIMDFP_SVE_OP_007 = SIMDFP_SVE_OP + 7,
    MI_SIMDFP_SVE_OP_008 = SIMDFP_SVE_OP + 8,
    MI_SIMDFP_SVE_OP_009 = SIMDFP_SVE_OP + 9,
    MI_SIMDFP_SVE_OP_010 = SIMDFP_SVE_OP + 10,
    MI_SIMDFP_SVE_OP_011 = SIMDFP_SVE_OP + 11,
    MI_SIMDFP_SVE_OP_012 = SIMDFP_SVE_OP + 12,
    MI_SIMDFP_SVE_OP_013 = SIMDFP_SVE_OP + 13,
    MI_SIMDFP_SVE_OP_014 = SIMDFP_SVE_OP + 14,
    MI_SIMDFP_SVE_OP_015 = SIMDFP_SVE_OP + 15,
    MI_SIMDFP_SVE_OP_016 = SIMDFP_SVE_OP + 16,
    MI_SIMDFP_SVE_OP_017 = SIMDFP_SVE_OP + 17,
    MI_SIMDFP_SVE_OP_018 = SIMDFP_SVE_OP + 18,
    MI_SIMDFP_SVE_OP_019 = SIMDFP_SVE_OP + 19,
    MI_SIMDFP_SVE_OP_020 = SIMDFP_SVE_OP + 20,
    MI_SIMDFP_SVE_OP_021 = SIMDFP_SVE_OP + 21,
    MI_SIMDFP_SVE_OP_022 = SIMDFP_SVE_OP + 22,
    MI_SIMDFP_SVE_OP_023 = SIMDFP_SVE_OP + 23,
    MI_SIMDFP_SVE_OP_024 = SIMDFP_SVE_OP + 24,
    MI_SIMDFP_SVE_OP_025 = SIMDFP_SVE_OP + 25,
    MI_SIMDFP_SVE_OP_026 = SIMDFP_SVE_OP + 26,
    MI_SIMDFP_SVE_OP_027 = SIMDFP_SVE_OP + 27,
    MI_SIMDFP_SVE_LD_001 = SIMDFP_SVE_LD + 1,
    MI_SIMDFP_SVE_LD_002 = SIMDFP_SVE_LD + 2,
    MI_SIMDFP_SVE_LD_003 = SIMDFP_SVE_LD + 3,
    MI_SIMDFP_SVE_LD_004 = SIMDFP_SVE_LD + 4,
    MI_SIMDFP_SVE_LD_005 = SIMDFP_SVE_LD + 5,
    MI_SIMDFP_SVE_LD_006 = SIMDFP_SVE_LD + 6,
    MI_SIMDFP_SVE_LD_007 = SIMDFP_SVE_LD + 7,
    MI_SIMDFP_SVE_LD_008 = SIMDFP_SVE_LD + 8,
    MI_SIMDFP_SVE_LD_009 = SIMDFP_SVE_LD + 9,
    MI_SIMDFP_SVE_LD_010 = SIMDFP_SVE_LD + 10,
    MI_SIMDFP_SVE_LD_011 = SIMDFP_SVE_LD + 11,
    MI_SIMDFP_SVE_LD_012 = SIMDFP_SVE_LD + 12,
    MI_SIMDFP_SVE_LD_013 = SIMDFP_SVE_LD + 13,
    MI_SIMDFP_SVE_LD_014 = SIMDFP_SVE_LD + 14,
    MI_SIMDFP_SVE_LD_015 = SIMDFP_SVE_LD + 15,
    MI_SIMDFP_SVE_LD_016 = SIMDFP_SVE_LD + 16,
    MI_SIMDFP_SVE_ST_001 = SIMDFP_SVE_ST + 1,
    MI_SIMDFP_SVE_ST_002 = SIMDFP_SVE_ST + 2,
    MI_SIMDFP_SVE_ST_003 = SIMDFP_SVE_ST + 3,
    MI_SIMDFP_SVE_ST_004 = SIMDFP_SVE_ST + 4,
    MI_SIMDFP_SVE_ST_005 = SIMDFP_SVE_ST + 5,
    MI_SIMDFP_SVE_ST_006 = SIMDFP_SVE_ST + 6,
    MI_SVE_CMP_INST_001 = SVE_CMP_INST + 1,
    MI_SVE_CMP_INST_002 = SVE_CMP_INST + 2,
    MI_PREDICATE_OP_001 = PREDICATE_OP + 1,
    MI_PREDICATE_OP_002 = PREDICATE_OP + 2,
  };

  /// 利用資源情報（Pipeline情報群 + レイテンシ）
  struct SchedResource {
    StmPipelines pipelines;
    int latency;
    bool seqdecode = false;
  };

  static std::map<ResourceID, SchedResource> ResInfo; ///< 資源情報を参照するための紐づけ
  static llvm::DenseMap<unsigned int, ResourceID> MIOpcodeInfo; ///< MIと資源情報の紐づけ

  /**
   * \brief 利用資源IDを返す。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  ResourceID getRes(const MachineInstr &mi) const;

  /**
   * \brief 利用資源IDに対応するStmPipelinesを返す。
   * \param [in] id 利用資源ID
   * \return StmPipelines
   */
  const StmPipelinesImpl *getPipelines(ResourceID id) const;

  /**
   * \brief 利用資源IDに対応するレイテンシを返す。
   * \param [in] id 利用資源ID
   * \return レイテンシ
   */
  unsigned getLatency(ResourceID id) const;

  /**
   * \brief 利用資源IDに対応するSequential decodeか否かの情報を返す。
   * \param [in] id 利用資源ID
   * \retval true Sequential decode
   * \retval false Sequential decodeではない
   */
  bool isSeqDecode(ResourceID id) const;

  /**
   * \brief (実命令に変換されずに)消えるPseudo命令か否かの情報を返す
   * \param [in] mi 対象命令
   * \retval true 消えるPseudo命令
   * \retval false 消えるPseudo命令ではない
   */
  bool isPseudo(const MachineInstr &mi) const;

  /**
   * 利用資源IDを調べる。
   * 利用資源IDが定義されていない場合はNA(0)を返す。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  static ResourceID searchRes(const MachineInstr &mi);

  /**
   * ADDXrs,SUBXrsの利用資源IDを調べる。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  static ResourceID searchResADDSUBShiftReg(const MachineInstr &mi);

  /**
   * ADDXrxの利用資源IDを調べる。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  static ResourceID searchResADDExtendReg(const MachineInstr &mi);

  /**
   * MADD,UMADDLの利用資源IDを調べる。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  static ResourceID searchResMADD(const MachineInstr &mi);

  /**
   * ORRWrs, ORRXrsの利用資源IDを調べる。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  static ResourceID searchResORRShiftReg(const MachineInstr &mi);

  /**
   * SBFM, UBFMの利用資源IDを調べる。
   * 利用資源IDが定義されていない場合はNA(0)を返す。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  static ResourceID searchResSBFM(const MachineInstr &mi);

};
}

#endif

