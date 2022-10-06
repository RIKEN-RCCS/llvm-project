//=- AArch64SwplTransformMIR.h -  Transform MachineIR for SWP -*- C++ -*-----=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Transform MachineIR for SWP.
//
//===----------------------------------------------------------------------===//
#ifndef AARCH64TRANSFORMLINDA_H
#define AARCH64TRANSFORMLINDA_H
#include "AArch64SwplScr.h"
#include "AArch64SwplPlan.h"
#include "Utils/AArch64BaseInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/Register.h"
#include <unordered_map>
namespace swpl {

// 元の型名: Swpl_Reg_Linda_Prg_Vector_Map
using Reg2Vreg=llvm::DenseMap<const swpl::SwplReg*, std::vector<llvm::Register>*>;

// 元の型名: Linda_Gen_Swpl_Inst_Hashmap
using MI2SwplInst=std::unordered_map<const llvm::MachineInstr*, swpl::SwplInst*>;

// 元の型名: Swpl_Inst_Swpl_Slot_Hashmap
using SwplInst2Slot=std::unordered_map<const SwplInst*, unsigned>;

// 元の型名: Linda_Gen_Vector
using MIList=std::vector<llvm::MachineInstr*>;

using RegMap=std::map<llvm::Register,llvm::Register>;

/// スケジューリング結果反映
class SwplTransformMIR {
private:
  /// 変換結果の移動先
  enum BLOCK {  PRO_MOVE,  PROLOGUE,  KERNEL,  EPILOGUE,  EPI_MOVE};
  /// MIR出力タイミング
  enum DumpMIRID {BEFORE=1, AFTER=2, AFTER_SSA=4, LAST=8};


  swpl::SwplPlan &Plan; ///< 処理対象のスケジュールプラン
  swpl::SwplInstSlotHashmap &InstSlotMap; ///< Planの命令配置
  swpl::SwplLoop &Loop; ///< 処理対象のループ
  swpl::TransformedMIRInfo TMI; ///< 変換に必要な回転数などの情報
  Reg2Vreg PrgMap;  ///< オリジナルRegisterと新Register＋Versionのマップ
  llvm::MachineFunction& MF; ///< llvm::MachineInstrを扱う際に必要なクラス

  // SSA化で利用するメンバ変数
  RegMap Org2NewReg; ///< original register->new register
  UseMap &LiveOutReg;


  /// KERNELの分岐言の比較に用いる制御変数のversionを決定する 
  /// \param [in] bandwidth
  /// \param [in] slot
  /// \return 制御変数のversion
  size_t  chooseCmpIteration(size_t bandwidth, size_t slot);

  /// 指定Register＋Versionに対応する新Registerを返す
  /// \param [in] orgReg オリジナルレジスタ
  /// \param [in] version 新レジスタが必要なVersion
  /// \return 新レジスタ 対応レジスタが登録されていない場合は!isValid()なレジスタを返す
  llvm::Register getPrgFromMap (const swpl::SwplReg* orgReg, unsigned version) const;

  /// <reg, version> と prg の対応表をつくり、SwplTransformMIR::prgMapに設定する
  /// \param [in] n_versions
  void constructPrgMap(size_t n_versions);

  /// TLI::originalDoPrgを定義している命令をLoop内からさがし,対応するSwplRegを返す
  /// \param [out] update 更新している命令に対応したSwplInstを返す
  /// \return 定義SwplReg
  const swpl::SwplReg* controlPrg2RegInLoop(const SwplInst**update);

  /// SwplPlanの情報からTransformLindaInfoを設定する
  void convertPlan2Linda();

  /// kernelのcopyの順番に対応するversionへの変換量を取得する
  /// \details
  ///  original versionが最後のcopyする言になるようずらす
  ///
  ///  条件                                 | 処理
  ///  -------------------------------------|----------------------------------
  ///  (n_versions -1)                      | original versionの定義
  ///  - (n_copies - 1)                     | 定義がEPILOGUEの最後になるようずらす
  ///  (n_copies / n_versions) * n_versions | 値の正数化
  /// \param [in] n_versions
  /// \param [in] n_copies
  /// \return
  int shiftConvertIteration2Version(int n_versions, int n_copies) const;

  /// SwplInstからMachineInstrを生成する
  /// \param [in] inst
  /// \param [in] version
  /// \return 生成したMachineInstr
  llvm::MachineInstr* createGenFromInst(const swpl::SwplInst &inst, size_t version);

  /// 指定したversionのSwplRegに対応するLindaPrgを取得する
  /// \param [in] org
  /// \param [in] version
  /// \return 取得したprg
  llvm::Register getPrg(const swpl::SwplReg& org, size_t version );

  /// Transformed_Linda_Info::gensに格納された命令を指定ブロック(KERNEL, PROLOGUE等)へ挿入する
  /// \param [in] ins
  /// \param [in] block
  void insertGens(llvm::MachineBasicBlock&ins, BLOCK block);

  /// kernelのbodyの繰返し分岐を生成する
  /// \param [in,out] MBB
  void makeKernelIterationBranch(llvm::MachineBasicBlock&MBB);

  /// planからLindaに反映したループ情報を表示
  void outputNontunedMessage();

  /// SWPLのPROLOGUE,KERNEL,EPILOGUE,MOVE部分に入る言をVectorに用意する
  void prepareGens();

  /// SwplRegとPRGをマップするための領域を用意する
  /// \param [in] reg
  /// \param [in] n_versions
  void preparePrgVectorForReg(const swpl::SwplReg *reg, size_t n_versions);

  /// SwplRegとPRGをマップする
  /// \param [in] orgReg
  /// \param [in] version
  /// \param [in] newReg
  void setPrg(const swpl::SwplReg* orgReg, size_t version, llvm::Register newReg);

  /// Swpl成功メッセージ出力
  /// \param [in] n_body_inst
  /// \param [in] policy
  /// \note Swpl_setOptInfo_forSSDEをoutputLoopoptMessageに名称変更。
  /// 元outputLoopoptMessageは利用削除。
  void outputLoopoptMessage(int n_body_inst, swpl::SwplSchedPolicy policy);

  /// SWPL後の新しいKERNEL部分を構成する
  void transformKernel();

  /// SSA形式への変更前にPHI命令等を整える
  void postTransformKernel();

  /// 非SSA形式で生成された命令列をSSA形式に変換する
  void convert2SSA();

  /// EPILOGの定義レジスタを書き換える
  void convertEpilog2SSA();

  /// Kernelの定義レジスタを書き換える
  void convertKernel2SSA();

  /// PROLOGの定義レジスタを書き換える
  void convertProlog2SSA();

  /// 指定MBBのレジスタ書き換え
  /// \param [out] mbb
  /// \param [out] regmap
  void replaceDefReg(MachineBasicBlock &mbb, std::map<Register,Register>&regmap);

  /// 指定MBBのレジスタ書き換え
  /// \param [out] mbb
  /// \param [in] regmap
  void replaceUseReg(std::set<MachineBasicBlock*> &mbbs, const std::map<Register,Register>&regmap);

  /// 指定命令のスロット位置を返す
  /// \param [in] inst
  /// \return slot-begin_slot
  unsigned relativeInstSlot(const SwplInst*inst) const;

  ///  対象関数を標準エラーに出力する
  /// \param [in] id Dumpタイミング
  void dumpMIR(DumpMIRID id) const;

public:
  /// コストラクタ
  /// \param [in] mf 対象MachineFunction
  /// \param [in] plan スケジューリング計画
  /// \param [in] liveOutReg 対象ループから出力Busyとなるレジスタ（スケジューリング結果反映時にレジスタ修正範囲を特定するために利用）
  SwplTransformMIR(llvm::MachineFunction&mf, swpl::SwplPlan&plan, UseMap&liveOutReg)
  :Plan(plan),InstSlotMap(plan.getInstSlotMap()),Loop(plan.getLoop()),MF(mf),LiveOutReg(liveOutReg) {}

  virtual ~SwplTransformMIR() {
    for (auto &p:PrgMap) {
      delete p.second;
      p.second=nullptr;
    }
  }
  /// Planに従い、対象ループをSWPL化する
  /// \retval true MIRを更新した
  /// \retval false MIRを更新しなかった
  bool transformMIR();

  /// SwplPlan情報をファイルに出力する
  void exportPlan();

  /// ファイルを入力し、SwplPlanへ設定する
  void importPlan();

};

}
#endif
