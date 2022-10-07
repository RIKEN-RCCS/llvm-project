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
#include "AArch64A64FXResourceInfo.h"
#define STMTEST

namespace swpl {

/// Reource識別ID
typedef int StmResourceId;

/// StmPipelineのパターンを識別するID
typedef unsigned StmPatternId;

/// MachineInstr::Opcodeを示すID
typedef unsigned StmOpcodeId;

/// 命令が使う資源を表現する。
/// 例：LDNP EAG* / EAG*
/// \dot "簡略図"
/// digraph g {
///  graph [ rankdir = "LR" fontsize=10];
///  node [ fontsize = "10" shape = "record" ];
///  edge [];
///  pipe1  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGA|EAGA}"];
///  pipe2  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGA|EAGB}"];
///  pipe3  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGB|EAGA}"];
///  pipe4  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGB|EAGB}"];
/// }
// \enddot
class StmPipeline {
  llvm::TargetSchedModel& SM;///< SchedModel
public:
  // 命令に対して生成した順番に付加されるID
  StmPatternId patternId=0;
  llvm::SmallVector<unsigned,4> stages;
  llvm::SmallVector<StmResourceId,4> resources;

  /// constructor
  StmPipeline(llvm::TargetSchedModel&sm):SM(sm){}

  /// StmPipelineをダンプする。
  /// \param ost 出力先
  void print(llvm::raw_ostream& ost) const;

  /// resourcesに存在する資源を数える
  /// \param [in] resource カウントしてほしい資源
  /// \return リソース数
  int getNResources(StmResourceId resource) const;

  /// ResourceIdに応じた名前を返す
  /// \param [in] resource 名前を取得したい資源
  /// \return ResourceIdに応じた名前
  static const char* getResourceName(StmResourceId resource);
};

/// StmPipelineリスト（引数など受け取り側で理帳する）
using StmPipelinesImpl =std::vector<const StmPipeline *>;

/// StmPipelineリスト(変数として宣言する際に利用する)
using StmPipelines =std::vector<const StmPipeline *>;

/// SWPL向けにRegisterの種別を判断する。
class StmRegKind {
  unsigned registerClassId=0; ///< TargetRegisterClassのID
  bool allocated=false;
  const llvm::MachineRegisterInfo &MRI;    ///< RegisterInfo

public:
  /// constructor
  /// \param id register class id
  /// \param isPReg 割当済か
  StmRegKind(unsigned id, bool isPReg, const llvm::MachineRegisterInfo&mri):registerClassId(id),allocated(isPReg),MRI(mri){}

  /// Intergerレジスタかどうかを確認する
  /// \retval true Intergerレジスタ
  /// \retval false Integerレジスタ以外
  bool isInteger(void) {
    return registerClassId==llvm::AArch64::GPR64RegClassID;
  }

  /// Floatingレジスタかどうかを確認する
  /// \retval true Floatingレジスタ
  /// \retval false Floatingレジスタ以外
  bool isFloating(void) {
    return registerClassId==llvm::AArch64::FPR64RegClassID;
  }

  /// Predicateレジスタかどうかを確認する
  /// \retval true Predicateレジスタ
  /// \retval false Predicateレジスタ以外
  bool isPredicate(void) {
    return registerClassId==llvm::AArch64::PPRRegClassID;
  }

  /// CCレジスタかどうかを確認する
  /// \retval true CCレジスタ
  /// \retval false CCレジスタ以外
  bool isCCRegister(void) {
    return registerClassId==llvm::AArch64::CCRRegClassID;
  }

  /// CCレジスタかどうかを確認する(互換性を保つため用意している)
  /// \retval true CCレジスタ
  /// \retval false CCレジスタ以外
  bool isIntegerCCRegister(void) {
    return registerClassId==llvm::AArch64::CCRRegClassID;
  }

  /// Allocate済レジスタかどうかを確認する
  ///
  /// \retval true 割当済
  /// \retval false 未割り当て
  bool isAllocalted(void) {
    return allocated;
  }

  /// レジスタの種類数を返す
  /// \return 種類数
  static int getKindNum(void){
    return 4;
  }

  /// 同種のレジスタ数を返す
  /// \return レジスタ数数
  int getNum(void){
    switch (registerClassId) {
    case llvm::AArch64::GPR64RegClassID:
      return llvm::AArch64::GPR64RegClass.getNumRegs();
    case llvm::AArch64::FPR64RegClassID:
      return llvm::AArch64::FPR64RegClass.getNumRegs();
    case llvm::AArch64::PPRRegClassID:
      return llvm::AArch64::PPR_3bRegClass.getNumRegs();
    case llvm::AArch64::CCRRegClassID:
      return llvm::AArch64::CCRRegClass.getNumRegs();
    }
    llvm_unreachable("registerClassId is 0");
  }

  /// StmRegKindの内容を出力する
  /// \param [out] os 出力先
  void print(llvm::raw_ostream& os) {
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
    os << "StmRegKind:" << MRI.getTargetRegisterInfo()->getRegClassName(r) << "\n";
  }

  /// 同一のレジスタ種別であるか
  /// \param [in] kind1 比較対象1
  /// \param [in] kind2 比較対象2
  /// \retval true 同一
  /// \retval false　同一ではない
  static bool isSameKind( const StmRegKind & kind1, const StmRegKind & kind2 ) {
    return kind1.registerClassId == kind2.registerClassId;
  }
  /// 同一のレジスタ種別であるか
  /// \param [in] kind1 比較対象1
  /// \param [in] regclassid 比較対象2のレジスタクラスID
  /// \retval true 同一
  /// \retval false 同一ではない
  static bool isSameKind( const StmRegKind & kind1, unsigned regclassid ) {
    return kind1.registerClassId == regclassid;
  }

};

/// SchedModelを利用してターゲット情報を取得し、SWPL機能に提供する
class Stm {
protected:
  const llvm::MachineFunction *MF=nullptr; ///< Tmで必要な情報を取得するため、大元のMachineFunctionを記憶する。関数毎に再設定する。
  llvm::TargetSchedModel SM;         ///< SchedModel
  const llvm::MachineRegisterInfo *MRI=nullptr;    ///< RegisterInfo
  const llvm::AArch64A64FXResInfo *ResInfo=nullptr;
  const llvm::TargetInstrInfo* TII;
  llvm::DenseMap<StmResourceId, int> tmNumSameKindResources;  ///< 資源種別ごとの数
  llvm::DenseMap<StmOpcodeId, StmPipelinesImpl * > stmPipelines; ///< Opcodeが利用する資源
  /// StmPipelineを生成する
  /// \param [in] mp 対象となる命令
  /// \return 生成したStmPipeline
  StmPipelinesImpl *generateStmPipelines(const llvm::MachineInstr&mp);
  unsigned numResource=0; ///< 資源数（資源種別数ではない）

  /// 指定命令の資源情報が取得できるかどうかをチェックする
  /// \param [in] mi 調査対象の命令
  /// \retval true 資源情報を取得可能
  /// \retval false 資源情報を取得できない
  bool isImplimented(const llvm::MachineInstr&mi) const;

public:
  /// constructor
  Stm() {}
  /// destructor
  virtual ~Stm() {
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
  void initialize(const llvm::MachineFunction &mf);


  /// 命令フェッチステージの同時読み込み命令数を返す。
  /// \details
  /// getRealFetchBandwidth()の復帰値＋仮想Slot数(Pseudo用)を返す。
  /// \return 命令数(Slot数)
  unsigned int getFetchBandwidth(void) const;

  /// デコードステージの同時読み込み命令数を返す。
  /// \return 命令数(Slot数)
  unsigned int getRealFetchBandwidth(void) const;

  /// レジスタdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令
  /// \param [in] use 利用命令
  /// \return 定義から参照までのレイテンシ
  int computeRegFlowDependence(const llvm::MachineInstr* def, const llvm::MachineInstr* use) const;

  /// メモリのdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令（Store）
  /// \param [in] use 参照命令（Load）
  /// \return レイテンシ
  int computeMemFlowDependence(const llvm::MachineInstr* def, const llvm::MachineInstr* use) const;

  /// メモリのuse/def間のレイテンシを計算する。
  /// \param [in] def 参照命令（Load）
  /// \param [in] use 定義命令（Store）
  /// \return レイテンシ
  int computeMemAntiDependence(const llvm::MachineInstr* use, const llvm::MachineInstr* def) const;

  /// メモリのdef/def間のレイテンシを計算する。
  /// \param [in] def1 定義命令（Store）
  /// \param [in] def2 定義命令（Store）
  /// \return レイテンシ
  int computeMemOutputDependence(const llvm::MachineInstr* def1, const llvm::MachineInstr* def2) const;

  /// (1命令しか実行できない)Store命令であれば真を返す。
  /// \param [in] mi 判断対象の命令
  /// \return Storeであれば真となる
  bool isIssuedOneByOne(const llvm::MachineInstr &mi) const {
    return mi.mayStore();
  }

  /// 指定命令が利用するリソースの利用パターンをすべて返す。
  /// \param [in] mi 対象命令
  /// \return StmPipelinesを返す
  const StmPipelinesImpl * getPipelines(const llvm::MachineInstr& mi);

  /// 指定した命令のリソース利用パターンの中から、指定した利用パターンを返す。
  /// \param [in] mi 対象命令
  /// \param [in] patternid 対象パターン
  /// \return StmPipelineを返す
  const StmPipeline * getPipeline(const llvm::MachineInstr&mi,
                                StmPatternId patternid);

  /// 指定命令が利用するリソースが一番少ない数を返す
  /// \param [in] opcode 対象命令
  /// \param [in] resource 数えたいリソース
  /// \return 一番少ない数
  int getMinNResources(StmOpcodeId opcode, StmResourceId resource);

  /// 利用可能な資源の数を返す
  /// \return 資源数
  unsigned int getNumResource(void) const;

  /// Slotの数を返す
  /// \return Slot数
  int getNumSlot(void) const;

  /// 指定リソースの数を返す
  /// \param [in] resource 調査したいリソース
  /// \return リソース数
  int getNumSameKindResources(StmResourceId resource);

  /// 指定レジスタの種別を返す
  /// \param [in] reg
  /// \return レジスタの種別を表すオブジェクト
  StmRegKind getRegKind(llvm::Register reg) const;

  /// 命令がPseudoかどうかを判断する
  /// \details 命令がSchedModelに定義されていない場合のみ、Pseudoと判断する
  /// \param [in] mi 対象命令
  /// \retval truer Psedo命令
  /// \retval false Pseudo命令ではない
  bool isPseudo(const llvm::MachineInstr& mi) const;
};
#ifdef STMTEST
/// StmTestからStmをテストするための派生クラス
class StmX4StmTest : public Stm {
public:
  void init(llvm::MachineFunction&mf, bool first, int TestID);
  const llvm::TargetSchedModel& getSchedModel() {return SM;}
  const llvm::MachineRegisterInfo *getMachineRegisterInfo() {return MRI;}
};

/// Tmの動作テストをおこなうクラス
class StmTest {
  swpl::StmX4StmTest STM;
  int TestID;
public:
  /// constructor
  StmTest(int id):TestID(id) {}

  /// 動作テスト実行
  void run(llvm::MachineFunction&mf);
};
#endif

}
#endif
