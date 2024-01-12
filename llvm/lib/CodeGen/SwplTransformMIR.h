//=- SwplTransformMIR.h -  Transform MachineIR for SWP -*- C++ -*------------=//
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
#ifndef SWPLTRANSFORMMIR_H
#define SWPLTRANSFORMMIR_H
#include "SwplPlan.h"
#include "SwplScr.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/Register.h"
#include <unordered_map>
namespace llvm {

/// スケジューリング結果反映
class SwplTransformMIR {
private:
  using Reg2Vreg=DenseMap<const SwplReg*, std::vector<Register>*>;
  using RegMap=std::map<Register, Register>;

  /// 変換結果の移動先
  enum BLOCK {  PRO_MOVE,  PROLOGUE,  KERNEL,  EPILOGUE,  EPI_MOVE};
  /// MIR出力タイミング
  enum DumpMIRID {BEFORE=1, AFTER=2, AFTER_SSA=4, LAST=8, SLOT_BEFORE=16};


  SwplPlan &Plan; ///< 処理対象のスケジュールプラン
  SwplSlots &Slots; ///< Planの命令配置
  SwplLoop &Loop; ///< 処理対象のループ
  DebugLoc LoopLoc; ///< 対象ループのソース情報
  SwplTransformedMIRInfo TMI; ///< 変換に必要な回転数などの情報
  Reg2Vreg VRegMap;  ///< オリジナルRegisterと新Register＋Versionのマップ
  llvm::MachineFunction& MF; ///< llvm::MachineInstrを扱う際に必要なクラス

  // SSA化で利用するメンバ変数
  RegMap Org2NewReg; ///< original register->new register
  SwplScr::UseMap &LiveOutReg;


  /// KERNELの分岐言の比較に用いる制御変数のversionを決定する 
  /// \param [in] bandwidth
  /// \param [in] slot
  /// \return 制御変数のversion
  size_t  chooseCmpIteration(size_t bandwidth, size_t slot);

  /// 指定Register＋Versionに対応する新Registerを返す
  /// \param [in] orgReg オリジナルレジスタ
  /// \param [in] version 新レジスタが必要なVersion
  /// \return 新レジスタ 対応レジスタが登録されていない場合は!isValid()なレジスタを返す
  llvm::Register getVRegFromMap(const SwplReg* orgReg, unsigned version) const;

  /// <reg, version> と vreg の対応表をつくり、SwplTransformMIR::VRegMapに設定する
  /// \param [in] n_versions
  void constructVRegMap(size_t n_versions);

  /// TMI::originalDoVRegを定義している命令をLoop内からさがし,対応するSwplRegを返す
  /// \param [out] update 更新している命令に対応したSwplInstを返す
  /// \return 定義SwplReg
  const SwplReg*controlVReg2RegInLoop(const SwplInst**update);

  /// SwplPlanの情報からTransformMIRInfoを設定する
  void convertPlan2MIR();

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
  llvm::MachineInstr*createMIFromInst(const SwplInst &inst, size_t version);

  /// 指定したversionのSwplRegに対応するMIRVRegを取得する
  /// \param [in] org
  /// \param [in] version
  /// \return 取得したvreg
  llvm::Register getVReg(const SwplReg& org, size_t version );

  /// SwplTransformedMIRInfo::MIsに格納された命令を指定ブロック(KERNEL, PROLOGUE等)へ挿入する
  /// \param [in] ins
  /// \param [in] block
  void insertMIs(llvm::MachineBasicBlock&ins, BLOCK block);

  /// kernelのbodyの繰返し分岐を生成する
  /// \param [in,out] MBB
  void makeKernelIterationBranch(llvm::MachineBasicBlock&MBB);

  /// planからMIRに反映したループ情報を表示
  void outputNontunedMessage();

  /// SWPLのPROLOGUE,KERNEL,EPILOGUE,MOVE部分に入る言をVectorに用意する
  void prepareMIs();

  /// SwplRegとVRegをマップするための領域を用意する
  /// \param [in] reg
  /// \param [in] n_versions
  void prepareVRegVectorForReg(const SwplReg *reg, size_t n_versions);

  /// SwplRegとVRegをマップする
  /// \param [in] orgReg
  /// \param [in] version
  /// \param [in] newReg
  void setVReg(const SwplReg* orgReg, size_t version, llvm::Register newReg);

  /// Swpl成功メッセージ出力
  /// \param [in] n_body_inst
  void outputLoopoptMessage(int n_body_inst);

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

  /// 結果反映前のMachineInstrのprint
  /// \param [in] mi 対象のMachineInstr
  void printTransformingMI(const MachineInstr *mi);

  /// Count the number of COPY in the kernel loop
  void countKernelCOPY();
public:
  /// コストラクタ
  /// \param [in] mf 対象MachineFunction
  /// \param [in] plan スケジューリング計画
  /// \param [in] liveOutReg 対象ループから出力Busyとなるレジスタ（スケジューリング結果反映時にレジスタ修正範囲を特定するために利用）
  SwplTransformMIR(llvm::MachineFunction &mf, SwplPlan&plan, SwplScr::UseMap &liveOutReg)
  :Plan(plan),Slots(plan.getInstSlotMap()),Loop(plan.getLoop()),MF(mf),LiveOutReg(liveOutReg) {
    LoopLoc = Loop.getML()->getStartLoc();
  }

  virtual ~SwplTransformMIR() {
    for (auto &p: VRegMap) {
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

  /// 命令の並びを表現するためのクラス
  struct IOSlot {
    unsigned id; ///< 命令の番号（スケジューリング対象命令の出現順番号）
    unsigned slot; ///< 命令を置くslot番号
  };

  /// SwplPlanとYAMLの仲介で利用するクラス
  struct IOPlan {
    unsigned minimum_iteration_interval;
    unsigned iteration_interval;
    size_t n_renaming_versions;
    size_t n_iteration_copies;
    unsigned begin_slot;
    std::vector<IOSlot> Slots;
  };

};

}
#endif
