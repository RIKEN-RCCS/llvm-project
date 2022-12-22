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
  unsigned VectorLength;
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
    FLA_C=11, FLB_C=12, EXA_C=13, EXB_C=14, EAGA_C=15, EAGB_C=16
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
    MI_SIMDFP_SVE_OP_001 = SIMDFP_SVE_OP + 1,
    MI_SIMDFP_SVE_OP_002 = SIMDFP_SVE_OP + 2,
    MI_SIMDFP_SVE_OP_003 = SIMDFP_SVE_OP + 3,
    MI_SIMDFP_SVE_OP_004 = SIMDFP_SVE_OP + 4,
    MI_SIMDFP_SVE_LD_001 = SIMDFP_SVE_LD + 1,
    MI_SIMDFP_SVE_LD_002 = SIMDFP_SVE_LD + 2,
    MI_SIMDFP_SVE_LD_003 = SIMDFP_SVE_LD + 3,
    MI_SIMDFP_SVE_ST_001 = SIMDFP_SVE_ST + 1,
    MI_SIMDFP_SVE_ST_002 = SIMDFP_SVE_ST + 2,
    MI_PREDICATE_OP_001 = PREDICATE_OP + 1
  };

  /// 利用資源情報（Pipeline情報群 + レイテンシ）
  struct SchedResource {
    StmPipelines pipelines;
    int latency;
  };

  static std::map<ResourceID, SchedResource> ResInfo; ///< 資源情報を参照するための紐づけ
  static std::map<unsigned int, ResourceID> MIOpcodeInfo; ///< MIと資源情報の紐づけ

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
   * \brief 利用資源IDに対応するレイテンシを返す。
   * \param [in] id 利用資源ID
   * \return レイテンシ
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
   * SBFM, UBFMの利用資源IDを調べる。
   * 利用資源IDが定義されていない場合はNA(0)を返す。
   * \param [in] mi 対象命令
   * \return 利用資源ID
   */
  static ResourceID searchResSBFM(const MachineInstr &mi);

};
}

#endif

