//=- SwplTargetMachine.h - Target Machine Info for SWP --*- C++ -*----=//
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
#ifndef LLVM_CODEGEN_SWPLTARGETMACHINE_H
#define LLVM_CODEGEN_SWPLTARGETMACHINE_H

#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetSchedule.h"

namespace llvm {
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
//  TargetSchedModel& SM;///< SchedModel
public:
  StmPatternId patternId=0;
  SmallVector<unsigned,4> stages;
  SmallVector<StmResourceId,4> resources;

  /// constructor
  StmPipeline(){}
  virtual ~StmPipeline() {}

  /// Output StmPipeline in display format。
  /// \param ost output steam
  virtual void print(llvm::raw_ostream& ost) const = 0;

  /// count the resources present in resources
  /// \param [in] resource Resources to be counted
  /// \return number of resource
  virtual int getNResources(StmResourceId resource) const = 0;

  /// return the resource name
  /// \param [in] resource ResourceId
  /// \return Resource name
  virtual const char* getResourceName(StmResourceId resource) const = 0;
};

class StmRegKind {
protected:
  enum RegKindID {unknown=0, IntReg=1, FloatReg=2, PredicateReg=3, CCReg=4};
  unsigned registerClassId = RegKindID::unknown;
  bool allocated = false;

public:
  /// constructor

  StmRegKind():registerClassId(RegKindID::unknown),allocated(false)  {}
  StmRegKind(const StmRegKind&s):registerClassId(s.registerClassId),allocated(s.allocated) {}
  virtual ~StmRegKind() {}

  /// \param id register class id
  /// \param isAllocated  physical register
  StmRegKind(unsigned id, bool isAllocated)
      : registerClassId(id), allocated(isAllocated) {}

  /// check Interger
  /// \retval true Interger
  /// \retval false not Integer
  virtual bool isInteger(void) const {
    return registerClassId == RegKindID::IntReg;
  }
  static bool isInteger(unsigned regclass) {
    return regclass == RegKindID::IntReg;
  }
  static unsigned getIntRegID() {
    return RegKindID::IntReg;
  }

  /// check Floating
  /// \retval true Floating
  /// \retval false not Floating
  virtual bool isFloating(void) const {
    return registerClassId == RegKindID::FloatReg;
  }
  static bool isFloating(unsigned regclass) {
    return regclass == RegKindID::FloatReg;
  }
  static unsigned getFloatRegID() {
    return RegKindID::FloatReg;
  }

  /// check Predicate
  /// \retval true Predicate
  /// \retval false not Predicate
  virtual bool isPredicate(void) const {
    return registerClassId == RegKindID::PredicateReg;
  }
  static bool isPredicate(unsigned regclass) {
    return regclass == RegKindID::PredicateReg;
  }
  static unsigned getPredicateRegID() {
    return RegKindID::PredicateReg;
  }

  /// check CC
  /// \retval true CC
  /// \retval false not CC
  virtual bool isCCRegister(void) const {
    return registerClassId == RegKindID::CCReg;
  }
  static bool isCC(unsigned regclass) {
    return regclass == RegKindID::CCReg;
  }
  static unsigned getCCRegID() {
    return RegKindID::CCReg;
  }

  /// check CC(for compatible)
  /// \retval true CC
  /// \retval false not CC
  virtual bool isIntegerCCRegister(void) const {
    return isCCRegister();
  }

  /// check Allocated
  ///
  /// \retval true allocated
  /// \retval false not allocated
  virtual bool isAllocalted(void) const { return allocated; }

  /// number of regkind
  /// \return bunber of regkind
  virtual int getKindNum(void) const { return 4; }

  virtual int getNumIntReg() const {
    return 32;
  }
  virtual int getNumFloatReg() const {
    return 32;
  }
  virtual int getNumPredicateReg() const {
    return 8;
  }
  virtual int getNumCCReg() const {
    return 1;
  }

  /// num of reg
  /// \return num of reg
  virtual int getNum(void) const {
    if (isInteger()) return getNumIntReg();
    if (isFloating()) return getNumFloatReg();
    if (isPredicate()) return getNumPredicateReg();
    if (isCCRegister()) return getNumCCReg();
    llvm_unreachable("registerClassId is unknown");
  }

  /// print StmRegKind
  /// \param [out] os
  virtual void print(llvm::raw_ostream &os) const {
    switch (registerClassId) {
    case RegKindID::IntReg:
      os << "StmRegKind:IntReg\n";
      break;
    case RegKindID::FloatReg:
      os << "StmRegKind:FloatReg\n";
      break;
    case RegKindID::PredicateReg:
      os << "StmRegKind:FloatReg\n";
      break;
    case RegKindID::CCReg:
      os << "StmRegKind:CCReg\n";
      break;
    default:
      os << "StmRegKind:unknown\n";
      break;
    }
  }

  /// same regkind?
  /// \param [in] kind2 target
  /// \retval true same
  /// \retval false not same
  virtual bool isSameKind(unsigned id) const {
    return registerClassId == id;
  }
  /// same regkind?
  /// \param [in] kind1 target1
  /// \param [in] kind2 target2
  /// \retval true same
  /// \retval false not same
  static bool isSameKind(const StmRegKind &kind1,
                         const StmRegKind &kind2) {
    return kind1.registerClassId == kind2.registerClassId;
  }
  /// same regkind?
  /// \param [in] kind1 target1
  /// \param [in] regclassid target2
  /// \retval true same
  /// \retval false not same
  static bool isSameKind(const StmRegKind &kind1, RegKindID regclassid) {
    return kind1.registerClassId == regclassid;
  }
};

/// StmPipelineリスト（引数など受け取り側で理帳する）
using StmPipelinesImpl =std::vector<const StmPipeline *>;

/// StmPipelineリスト(変数として宣言する際に利用する)
using StmPipelines =std::vector<const StmPipeline *>;

class SwplTargetMachine {
protected:
  // @todo 以下4つのポインタは、要・不要を見直す
  const MachineFunction *MF=nullptr; ///< Tmで必要な情報を取得するため、大元のMachineFunctionを記憶する。関数毎に再設定する。
  TargetSchedModel SM;         ///< SchedModel
  const MachineRegisterInfo *MRI=nullptr;    ///< RegisterInfo
  const TargetInstrInfo* TII=nullptr;


public:
  /// constructor
  SwplTargetMachine() {}
  /// destructor
  virtual ~SwplTargetMachine() {
  }

  /// Tmの初期化を行う。
  /// \details
  /// runOnFunctionが呼び出される毎にinitialize()を実行し、処理対象となるMachineFunction情報を受け渡す必要がある。
  /// \param mf 処理対象のMachineFunction
  virtual void initialize(const MachineFunction &mf) = 0;


  /// 命令フェッチステージの同時読み込み命令数を返す。
  /// \details
  /// getRealFetchBandwidth()の復帰値＋仮想Slot数(Pseudo用)を返す。
  /// \return 命令数(Slot数)
  virtual unsigned int getFetchBandwidth(void) const = 0;

  /// デコードステージの同時読み込み命令数を返す。
  /// \return 命令数(Slot数)
  virtual unsigned int getRealFetchBandwidth(void) const = 0;

  /// レジスタdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令
  /// \param [in] use 利用命令
  /// \return 定義から参照までのレイテンシ
  virtual int computeRegFlowDependence(const MachineInstr* def, const MachineInstr* use) const = 0;

  /// メモリのdef/use間のレイテンシを計算する。
  /// \param [in] def 定義命令（Store）
  /// \param [in] use 参照命令（Load）
  /// \return レイテンシ
  virtual int computeMemFlowDependence(const MachineInstr* def, const MachineInstr* use) const = 0;

  /// メモリのuse/def間のレイテンシを計算する。
  /// \param [in] def 参照命令（Load）
  /// \param [in] use 定義命令（Store）
  /// \return レイテンシ
  virtual int computeMemAntiDependence(const MachineInstr* use, const MachineInstr* def) const = 0;

  /// メモリのdef/def間のレイテンシを計算する。
  /// \param [in] def1 定義命令（Store）
  /// \param [in] def2 定義命令（Store）
  /// \return レイテンシ
  virtual int computeMemOutputDependence(const MachineInstr* def1, const MachineInstr* def2) const = 0;

  /// (1命令しか実行できない)Store命令であれば真を返す。
  /// \param [in] mi 判断対象の命令
  /// \return Storeであれば真となる
  virtual bool isIssuedOneByOne(const llvm::MachineInstr &mi) const = 0;

  /// 指定命令が利用するリソースの利用パターンをすべて返す。
  /// \param [in] mi 対象命令
  /// \return StmPipelinesを返す
  virtual const StmPipelinesImpl * getPipelines(const MachineInstr& mi) = 0;

  /// 指定した命令のリソース利用パターンの中から、指定した利用パターンを返す。
  /// \param [in] mi 対象命令
  /// \param [in] patternid 対象パターン
  /// \return StmPipelineを返す
  virtual const StmPipeline * getPipeline(const MachineInstr&mi,
                                        StmPatternId patternid) = 0;

  /// 指定命令が利用するリソースが一番少ない数を返す
  /// \param [in] opcode 対象命令
  /// \param [in] resource 数えたいリソース
  /// \return 一番少ない数
  virtual int getMinNResources(StmOpcodeId opcode, StmResourceId resource) = 0;

  /// 利用可能な資源の数を返す
  /// \return 資源数
  virtual unsigned int getNumResource(void) const = 0;

  /// ResourceIdに応じた名前を返す
  /// \param [in] resource 名前を取得したい資源
  /// \return ResourceIdに応じた名前
  virtual const char* getResourceName(StmResourceId resource) const = 0;

  /// 命令がPseudoかどうかを判断する
  /// \details 命令がSchedModelに定義されていない場合のみ、Pseudoと判断する
  /// \param [in] mi 対象命令
  /// \retval truer Psedo命令
  /// \retval false Pseudo命令ではない
  virtual bool isPseudo(const llvm::MachineInstr& mi) const = 0;
};



}

#endif
