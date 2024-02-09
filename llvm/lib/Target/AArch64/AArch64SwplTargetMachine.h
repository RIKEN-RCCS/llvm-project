//=- AArch64SwplTargetMachine.h - Target Machine Info for SWP --*- C++ -*----=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Target Machine Info for SWP.
//
//===----------------------------------------------------------------------===//
#ifndef AARCH64SWPLTM_H
#define AARCH64SWPLTM_H

#include "AArch64TargetTransformInfo.h"
#include "llvm/CodeGen/SwplTargetMachine.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "AArch64SwplTargetMachine.h"

namespace llvm {

/// SWPL向けにRegisterの種別を判断する。
class AArch64StmRegKind: public StmRegKind {
  const MachineRegisterInfo &MRI;    ///< RegisterInfo

public:
  /// constructor
  AArch64StmRegKind(const MachineRegisterInfo&mri):StmRegKind(),MRI(mri){}

  /// \param id register class id
  /// \param isPReg 割当済か
  /// \param unitNum レジスタを構成するユニット数
  /// \param mri MRI
  AArch64StmRegKind(unsigned id,
                    bool isPReg,
                    unsigned unitNum,
                    const MachineRegisterInfo&mri): StmRegKind(id,  isPReg, unitNum),MRI(mri){}

  AArch64StmRegKind(const AArch64StmRegKind &s): AArch64StmRegKind(s.registerClassId, s.allocated, s.unitNum, s.MRI) {}

  virtual ~AArch64StmRegKind(){}

  /// Intergerレジスタかどうかを確認する
  /// \retval true Intergerレジスタ
  /// \retval false Integerレジスタ以外
  bool isInteger(void) const override {
    return registerClassId==StmRegKind::RegKindID::IntReg;
  }

  /// Floatingレジスタかどうかを確認する
  /// \retval true Floatingレジスタ
  /// \retval false Floatingレジスタ以外
  bool isFloating(void) const  override {
    return registerClassId==StmRegKind::RegKindID::FloatReg;
  }

  /// Predicateレジスタかどうかを確認する
  /// \retval true Predicateレジスタ
  /// \retval false Predicateレジスタ以外
  bool isPredicate(void) const override {
    return registerClassId==StmRegKind::RegKindID::PredicateReg;
  }

  /// CCレジスタかどうかを確認する
  /// \retval true CCレジスタ
  /// \retval false CCレジスタ以外
  bool isCCRegister(void) const override {
    return registerClassId==StmRegKind::RegKindID::CCReg;
  }

  /// Allocate済レジスタかどうかを確認する
  ///
  /// \retval true 割当済
  /// \retval false 未割り当て
  bool isAllocalted(void) const override {
    return allocated;
  }

  /// レジスタの種類数を返す
  /// \return 種類数
  int getKindNum(void) const override {
    return 4;
  }

  int getNumIntReg() const override {
    return AArch64::GPR64RegClass.getNumRegs()-3;
  }
  int getNumFloatReg() const override {
    return AArch64::FPR64RegClass.getNumRegs();
  }
  int getNumPredicateReg() const override {
    return AArch64::PPR_3bRegClass.getNumRegs();
  }
  int getNumCCReg() const override {
    return AArch64::CCRRegClass.getNumRegs();
  }

  bool isSameKind(unsigned id) const override {
    return registerClassId==id;
  }


  /// StmRegKindの内容を出力する
  /// \param [out] os 出力先
  void print(raw_ostream& os) const override {
    const TargetRegisterClass *r=nullptr;
    switch (registerClassId) {
    case AArch64::GPR64RegClassID:
      r=&AArch64::GPR64RegClass;
      break;
    case AArch64::FPR64RegClassID:
      r=&AArch64::FPR64RegClass;
      break;
    case AArch64::PPRRegClassID:
      r=&AArch64::PPRRegClass;
      break;
    case AArch64::CCRRegClassID:
      r=&AArch64::CCRRegClass;
      break;
    }
    os << "AArch64StmRegKind:" << MRI.getTargetRegisterInfo()->getRegClassName(r) << "\n";
  }
};

struct AArch64SwplSchedA64FX{
  static unsigned VectorLength;
  /// Types of Instructions
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

  /// Resources
  enum PortKind {
    FLA=1, FLB=2, EXA=3, EXB=4, EAGA=5, EAGB=6, PRX=7, BR=8,
    LSU1=9, LSU2=10,
    FLA_C=11, FLB_C=12, EXA_C=13, EXB_C=14, EAGA_C=15, EAGB_C=16,
    FLA_E=17, EXA_E=18, EXB_E=19, END 
  };


  /// Usage resource ID
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

  /// Usage Resource Information（Pipeline information group + latency）
  struct SchedResource {
    StmPipelines pipelines;
    int latency;
    bool seqdecode = false;
  };

  static std::map<ResourceID, SchedResource> ResInfo; ///< Linking for referencing resource information
  static llvm::DenseMap<unsigned int, ResourceID> MIOpcodeInfo; ///< Linking MI and resource information

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

/// SchedModelを利用してターゲット情報を取得し、SWPL機能に提供する
class AArch64SwplTargetMachine: public SwplTargetMachine {
protected:
  DenseMap<StmOpcodeId, StmPipelinesImpl * > stmPipelines; ///< Opcodeが利用する資源
  unsigned numResource=0; ///< 資源数（資源種別数ではない）

  /// 指定命令の資源情報が取得できるかどうかをチェックする
  /// \param [in] mi 調査対象の命令
  /// \retval true 資源情報を取得可能
  /// \retval false 資源情報を取得できない
  bool isImplimented(const MachineInstr &mi) const;

public:
  AArch64SwplSchedA64FX SwplSched; ///< A64FX SchedModel for SWP

  /// constructor
  AArch64SwplTargetMachine() {}
  /// destructor
  virtual ~AArch64SwplTargetMachine();

  /// SwplTargetMachineの初期化を行う。
  /// \details
  /// runOnFunctionが呼び出される毎にinitialize()を実行し、処理対象となるMachineFunction情報を受け渡す必要がある。
  /// \param mf 処理対象のMachineFunction
  void initialize(const MachineFunction &mf) override;

  /// レジスタdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令
  /// \param [in] use 利用命令
  /// \return 定義から参照までのレイテンシ
  int computeRegFlowDependence(const MachineInstr* def, const MachineInstr* use) const override;

  /// メモリのdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令（Store）
  /// \param [in] use 参照命令（Load）
  /// \return レイテンシ
  int computeMemFlowDependence(const MachineInstr* def, const MachineInstr* use) const override;

  /// メモリのuse/def間のレイテンシを計算する。
  /// \param [in] def 参照命令（Load）
  /// \param [in] use 定義命令（Store）
  /// \return レイテンシ
  int computeMemAntiDependence(const MachineInstr* use, const MachineInstr* def) const override;

  /// メモリのdef/def間のレイテンシを計算する。
  /// \param [in] def1 定義命令（Store）
  /// \param [in] def2 定義命令（Store）
  /// \return レイテンシ
  int computeMemOutputDependence(const MachineInstr* def1, const MachineInstr* def2) const override;

  /// 指定命令が利用するリソースの利用パターンをすべて返す。
  /// \param [in] mi 対象命令
  /// \return StmPipelinesを返す
  const StmPipelinesImpl * getPipelines(const MachineInstr& mi) const override;


  /// 利用可能な資源の数を返す
  /// \return 資源数
  unsigned int getNumResource(void) const override;

  /// ResourceIdに応じた名前を返す
  /// \param [in] resource 名前を取得したい資源
  /// \return ResourceIdに応じた名前
  const char* getResourceName(StmResourceId resource) const override;

  /// StmPipelineをダンプする。
  /// \param ost 出力先
  /// \param pipeline 出力対象pipeline
  void print(raw_ostream &ost, const StmPipeline &pipeline) const override;

  /// 命令がPseudoかどうかを判断する
  /// \details 命令がSchedModelに定義されていない場合のみ、Pseudoと判断する
  /// \param [in] mi 対象命令
  /// \retval truer Psedo命令
  /// \retval false Pseudo命令ではない
  bool isPseudo(const MachineInstr& mi) const override;

  /// 命令から命令種のIDを取得する
  /// \param [in] mi 対象命令
  /// \return 命令種のID AArch64SwplSchedA64FX::INT_OPなど
  unsigned getInstType(const MachineInstr &mi) const override;

  /// 命令種のIDに該当する文字列を返す
  /// \param [in] insttypeid 命令種のID
  /// \return 命令種のIDに該当する文字列
  const char* getInstTypeString(unsigned insttypeid) const override;

  /// 命令種と依存レジスタによるペナルティを算出する
  /// \param [in] prod 先行命令のMI
  /// \param [in] cons 後続命令のMI
  /// \param [in] regs 先行命令と後続命令で依存するレジスタ
  /// \return 命令種と依存レジスタによるペナルティ
  unsigned calcPenaltyByInsttypeAndDependreg(const MachineInstr& prod, const MachineInstr& cons,
                                             const llvm::Register& reg) const override;

  /// SWPLでのレジスタ割り付けを無効にするか否かを判断する
  /// \retval true  レジスタ割り付けを無効にする
  /// \retval false レジスタ割り付けを無効にしない
  bool isDisableRegAlloc(void) const override;

  /// [TODO:削除予定] SWPLでのレジスタ割り付けを有効にするか否かを判断する
  /// \retval true  レジスタ割り付けを有効にする
  /// \retval false レジスタ割り付けを有効にしない
  bool isEnableRegAlloc(void) const override;

  /// Determine the output location of the COPY instruction for registers that span the MBB
  /// \retval true  Output COPY instruction to prologue and epilogue
  /// \retval false Output COPY instruction to the kernel
  bool isEnableProEpiCopy(void) const override;
};

}

#endif

