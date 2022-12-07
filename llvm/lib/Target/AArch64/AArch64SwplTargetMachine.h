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
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/CodeGen/SwplTargetMachine.h"
#include "AArch64A64FXResourceInfo.h"

namespace llvm {

class AArch64StmPipeline: public StmPipeline {
  //  TargetSchedModel& SM;///< SchedModel
public:

  /// constructor/destructor
  AArch64StmPipeline(){}
  virtual ~AArch64StmPipeline() {}

  /// StmPipelineをダンプする。
  /// \param ost 出力先
  void print(raw_ostream& ost) const override;

  /// resourcesに存在する資源を数える
  /// \param [in] resource カウントしてほしい資源
  /// \return リソース数
  int getNResources(StmResourceId resource) const override;

  /// ResourceIdに応じた名前を返す
  /// \param [in] resource 名前を取得したい資源
  /// \return ResourceIdに応じた名前
  const char* getResourceName(StmResourceId resource) const override;
};


/// SWPL向けにRegisterの種別を判断する。
class AArch64StmRegKind: public llvm::StmRegKind {
  const llvm::MachineRegisterInfo &MRI;    ///< RegisterInfo

public:
  /// constructor
  AArch64StmRegKind(const llvm::MachineRegisterInfo&mri):llvm::StmRegKind(),MRI(mri){}

  /// \param id register class id
  /// \param isPReg 割当済か
  AArch64StmRegKind(unsigned id,
                    bool isPReg,
                    const llvm::MachineRegisterInfo&mri): llvm::StmRegKind(id, isPReg),MRI(mri){}

  AArch64StmRegKind(const AArch64StmRegKind &s): AArch64StmRegKind(s.registerClassId, s.allocated, s.MRI) {}

  virtual ~AArch64StmRegKind(){}

  /// Intergerレジスタかどうかを確認する
  /// \retval true Intergerレジスタ
  /// \retval false Integerレジスタ以外
  bool isInteger(void) const override {
    return registerClassId==llvm::StmRegKind::RegKindID::IntReg;
  }

  /// Floatingレジスタかどうかを確認する
  /// \retval true Floatingレジスタ
  /// \retval false Floatingレジスタ以外
  bool isFloating(void) const  override {
    return registerClassId==llvm::StmRegKind::RegKindID::FloatReg;
  }

  /// Predicateレジスタかどうかを確認する
  /// \retval true Predicateレジスタ
  /// \retval false Predicateレジスタ以外
  bool isPredicate(void) const override {
    return registerClassId==llvm::StmRegKind::RegKindID::PredicateReg;
  }

  /// CCレジスタかどうかを確認する
  /// \retval true CCレジスタ
  /// \retval false CCレジスタ以外
  bool isCCRegister(void) const override {
    return registerClassId==llvm::StmRegKind::RegKindID::CCReg;
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
    return llvm::AArch64::GPR64RegClass.getNumRegs();
  }
  int getNumFloatReg() const override {
    return llvm::AArch64::FPR64RegClass.getNumRegs();
  }
  int getNumPredicateReg() const override {
    return llvm::AArch64::PPR_3bRegClass.getNumRegs();
  }
  int getNumCCReg() const override {
    return llvm::AArch64::CCRRegClass.getNumRegs();
  }

  bool isSameKind(unsigned id) const override {
    return registerClassId==id;
  }


  /// StmRegKindの内容を出力する
  /// \param [out] os 出力先
  void print(llvm::raw_ostream& os) const override {
    const llvm::TargetRegisterClass *r=nullptr;
    switch (registerClassId) {
    case llvm::AArch64::GPR64RegClassID:
      r=&llvm::AArch64::GPR64RegClass;
      break;
    case llvm::AArch64::FPR64RegClassID:
      r=&llvm::AArch64::FPR64RegClass;
      break;
    case llvm::AArch64::PPRRegClassID:
      r=&llvm::AArch64::PPRRegClass;
      break;
    case llvm::AArch64::CCRRegClassID:
      r=&llvm::AArch64::CCRRegClass;
      break;
    }
    os << "AArch64StmRegKind:" << MRI.getTargetRegisterInfo()->getRegClassName(r) << "\n";
  }
};

/// SchedModelを利用してターゲット情報を取得し、SWPL機能に提供する
class AArch64SwplTargetMachine: public SwplTargetMachine {
protected:
  DenseMap<StmResourceId, int> tmNumSameKindResources;  ///< 資源種別ごとの数
  DenseMap<StmOpcodeId, StmPipelinesImpl * > stmPipelines; ///< Opcodeが利用する資源
  unsigned numResource=0; ///< 資源数（資源種別数ではない）

  const AArch64A64FXResInfo *ResInfo=nullptr;
  /// StmPipelineを生成する
  /// \param [in] mp 対象となる命令
  /// \return 生成したStmPipeline
  StmPipelinesImpl *generateStmPipelines(const MachineInstr &mp);

  /// 指定命令の資源情報が取得できるかどうかをチェックする
  /// \param [in] mi 調査対象の命令
  /// \retval true 資源情報を取得可能
  /// \retval false 資源情報を取得できない
  bool isImplimented(const MachineInstr &mi) const;

public:
  /// constructor
  AArch64SwplTargetMachine() {}
  /// destructor
  virtual ~AArch64SwplTargetMachine() {
    for (auto tms: stmPipelines) {
      if (tms.getSecond()) {
        for (auto *t:*(tms.getSecond())) {
          delete const_cast<StmPipeline *>(t);
        }
        delete tms.getSecond();
      }
    }
    if (ResInfo!=nullptr) {
      delete ResInfo;
      ResInfo=nullptr;
    }

  }

  /// Tmの初期化を行う。
  /// \details
  /// runOnFunctionが呼び出される毎にinitialize()を実行し、処理対象となるMachineFunction情報を受け渡す必要がある。
  /// \param mf 処理対象のMachineFunction
  void initialize(const MachineFunction &mf) override;


  /// 命令フェッチステージの同時読み込み命令数を返す。
  /// \details
  /// getRealFetchBandwidth()の復帰値＋仮想Slot数(Pseudo用)を返す。
  /// \return 命令数(Slot数)
  unsigned int getFetchBandwidth(void) const override;

  /// デコードステージの同時読み込み命令数を返す。
  /// \return 命令数(Slot数)
  unsigned int getRealFetchBandwidth(void) const override;

  /// レジスタdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令
  /// \param [in] use 利用命令
  /// \return 定義から参照までのレイテンシ
  int computeRegFlowDependence(const llvm::MachineInstr* def, const llvm::MachineInstr* use) const override;

  /// メモリのdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令（Store）
  /// \param [in] use 参照命令（Load）
  /// \return レイテンシ
  int computeMemFlowDependence(const llvm::MachineInstr* def, const llvm::MachineInstr* use) const override;

  /// メモリのuse/def間のレイテンシを計算する。
  /// \param [in] def 参照命令（Load）
  /// \param [in] use 定義命令（Store）
  /// \return レイテンシ
  int computeMemAntiDependence(const llvm::MachineInstr* use, const llvm::MachineInstr* def) const override;

  /// メモリのdef/def間のレイテンシを計算する。
  /// \param [in] def1 定義命令（Store）
  /// \param [in] def2 定義命令（Store）
  /// \return レイテンシ
  int computeMemOutputDependence(const llvm::MachineInstr* def1, const llvm::MachineInstr* def2) const override;

  /// (1命令しか実行できない)Store命令であれば真を返す。
  /// \param [in] mi 判断対象の命令
  /// \return Storeであれば真となる
  bool isIssuedOneByOne(const llvm::MachineInstr &mi) const override {
    return mi.mayStore();
  }

  /// 指定命令が利用するリソースの利用パターンをすべて返す。
  /// \param [in] mi 対象命令
  /// \return StmPipelinesを返す
  const StmPipelinesImpl * getPipelines(const llvm::MachineInstr& mi) override;

  /// 指定した命令のリソース利用パターンの中から、指定した利用パターンを返す。
  /// \param [in] mi 対象命令
  /// \param [in] patternid 対象パターン
  /// \return StmPipelineを返す
  const StmPipeline * getPipeline(const llvm::MachineInstr&mi,
                                StmPatternId patternid) override;

  /// 指定命令が利用するリソースが一番少ない数を返す
  /// \param [in] opcode 対象命令
  /// \param [in] resource 数えたいリソース
  /// \return 一番少ない数
  int getMinNResources(StmOpcodeId opcode, StmResourceId resource) override;

  /// 利用可能な資源の数を返す
  /// \return 資源数
  unsigned int getNumResource(void) const override;

  /// ResourceIdに応じた名前を返す
  /// \param [in] resource 名前を取得したい資源
  /// \return ResourceIdに応じた名前
  const char* getResourceName(StmResourceId resource) const override;

  /// 命令がPseudoかどうかを判断する
  /// \details 命令がSchedModelに定義されていない場合のみ、Pseudoと判断する
  /// \param [in] mi 対象命令
  /// \retval truer Psedo命令
  /// \retval false Pseudo命令ではない
  bool isPseudo(const llvm::MachineInstr& mi) const override;
};


}
#endif
