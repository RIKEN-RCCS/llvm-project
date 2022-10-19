//=- AArch64SwplScr.h -  Swpl Scheduling common function -*- C++ -*----------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Swpl Scheduling common function
//
//===----------------------------------------------------------------------===//
#ifndef AARCH64SWPLSCR_H
#define AARCH64SWPLSCR_H

#include "Utils/AArch64BaseInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/Register.h"
namespace swpl {
class SwplLoop;

/// 命令列変換情報
struct TransformedMIRInfo {
  llvm::Register originalDoVReg=0; ///<  オリジナルの制御変数（updateDoVRegMIのop1）
  llvm::Register originalDoInitVar=0; ///< オリジナル制御変数の定義PHIの初期値
  llvm::Register nonSSAOriginalDoVReg =0; ///< オリジナルの制御変数に対応した、データ抽出処理で非SSAに変換したMIR
  llvm::Register nonSSAOriginalDoInitVar=0; ///< 定義PHIの初期値に対応した、データ抽出処理で非SSAに変換したMIR
  llvm::Register doVReg=0; ///<  renaming後の制御変数
  llvm::MachineInstr *updateDoVRegMI=nullptr; ///< オリジナルの制御変数の更新命令
  llvm::MachineInstr *branchDoVRegMI=nullptr; ///< オリジナルの繰り返し分岐命令
  llvm::MachineInstr *initDoVRegMI=nullptr; ///< オリジナルの制御変数の誘導変数(Phi)
  llvm::MachineInstr *branchDoVRegMIKernel=nullptr; ///< kernel loopの繰り返し分岐命令
  int iterationInterval=0; ///< ii
  int minimumIterationInterval=0; ///< min_ii
  int coefficient=0; ///< オリジナルの制御変数の更新言のop3
  int minConstant=0; ///< オリジナルの制御変数の分岐言の種類に応じた定数. もともとのループを通過した後にループ制御変数がとりうる最小の値.
  int expansion=0; ///< renaming後の制御変数の分岐言のop3.kernelの最後の分岐言の定数を表す
  size_t nVersions=0; ///< kernelに展開されるループの数を表す
  size_t nCopies=0; ///< kernel,prolog,epilogに必要なオリジナルループの回転数
  size_t requiredKernelIteration=0; ///< tune前の展開に必要な回転数
  std::vector<llvm::MachineInstr*> mis; ///< prepareMIs() で使用するmi_tableの情報
  size_t prologEndIndx=0; ///< prepareMIs() で使用するmi_tableの情報
  size_t kernelEndIndx=0; ///< prepareMIs() で使用するmi_tableの情報
  size_t epilogEndIndx=0; ///< prepareMIs() で使用するmi_tableの情報
  bool   isIterationCountConstant=false; ///< 制御変数の初期値が定数として見つかったか (ループ回転数による展開制御に関する変数)
  int doVRegInitialValue=0; ///< オリジナルの制御変数の初期値 (if isIterationCountConstant == true)
  size_t originalKernelIteration=0; ///< 展開前のkernel部分の繰返し数 (if isIterationCountConstant == true)
  size_t transformedKernelIteration=0; ///< 展開後のkernel部分の繰返し数 (if isIterationCountConstant == true)
  size_t transformedModIteration=0; ///<展開後のmod部分の繰返し数 (if isIterationCountConstant == true)
  llvm::MachineBasicBlock*OrgPreHeader=nullptr; ///< オリジナルループの入り口
  llvm::MachineBasicBlock*Check1=nullptr; ///< 回転数チェック
  llvm::MachineBasicBlock*Prolog=nullptr; ///< prolog
  llvm::MachineBasicBlock*OrgBody=nullptr; ///< kernel
  llvm::MachineBasicBlock*Epilog=nullptr; ///< epilog
  llvm::MachineBasicBlock*Check2=nullptr; ///< 余りループ必要性チェック
  llvm::MachineBasicBlock*NewPreHeader=nullptr; ///< 余りループの入り口
  llvm::MachineBasicBlock*NewBody=nullptr; ///< 余りループ
  llvm::MachineBasicBlock*NewExit=nullptr; ///< 余りループの出口
  llvm::MachineBasicBlock*OrgExit=nullptr; ///< オリジナルの出口

  void print();

  virtual ~TransformedMIRInfo() {
  }

  /// SWPL変換が必要かどうかの判定
  /// \retval true 変換必要
  /// \retval false 変換不要
  bool isNecessaryTransformMIR() const {
    return !isIterationCountConstant || transformedKernelIteration != 0;
  }

  /// SWPL kernelループ回避分岐が必要かどうかの判定
  /// \retval true 生成必要
  /// \retval false 生成不要
  bool isNecessaryBypassKernel() const {
    return !isIterationCountConstant || transformedKernelIteration == 0;
  }

  /// SWPL余りループ回避分岐が必要かどうかの判定
  /// \retval true 生成必要
  /// \retval false 生成不要
  bool isNecessaryBypassMod() const {
    return !isIterationCountConstant || transformedKernelIteration == 0;
  }

  /// SWPL余りループが必要かどうかの判定
  /// \retval true 生成必要
  /// \retval false 生成不要
  bool isNecessaryMod() const {
    return !isIterationCountConstant || transformedKernelIteration == 0 || transformedModIteration >= 1;
  }

  /// kernelの繰り返し分岐が必要か判定する
  /// \retval true 生成必要
  /// \retval false 生成不要
  bool isNecessaryKernelIterationBranch() const {
    return !isIterationCountConstant || transformedKernelIteration != 1;
  }

  /// mod繰り返し分岐が必要か判定する
  /// \retval true 生成必要
  /// \retval false 生成不要
  bool isNecessaryModIterationBranch() const {
    return !isIterationCountConstant || transformedModIteration != 1;
  }
};

using UseMap=std::map<llvm::Register, std::vector<llvm::MachineOperand*>>;

/// Loop形状を変形したり、Loopから情報を探し出す機能を提供する
class SwplScr {
private:
  llvm::MachineLoop& ML;

  /// DO制御変数の初期値を取得する
  /// \param [out] TMI
  /// \retval true 取得成功
  /// \retval false 取得失敗
  bool getDoInitialValue(swpl::TransformedMIRInfo &TMI) const;

  /// 定数オペランドを３２ビット整数に変換する
  /// \param [in] op
  /// \param [out] coefficient
  /// \retval true 変換がうまくいった
  /// \retval false 変換できなかった
  bool getCoefficient (const llvm::MachineOperand&op, int *coefficient) const;

  /// SWPLで必要な回避分岐を生成する
  /// \param [in] tmi
  /// \param [in] dbgloc
  /// \param [out] skip_kernel_from
  /// \param [out] skip_kernel_to
  /// \param [out] skip_mod_from
  /// \param [out] skip_mod_to
  void makeBypass(const TransformedMIRInfo &tmi, const llvm::DebugLoc&dbgloc,
                  llvm::MachineBasicBlock &skip_kernel_from, llvm::MachineBasicBlock &skip_kernel_to,
                  llvm::MachineBasicBlock &skip_mod_from, llvm::MachineBasicBlock &skip_mod_to);

  /// 回転数が足りない場合に、逐次ルートを通すための分岐をkernelの前に作成する
  /// \param [in] doInitVar
  /// \param [in] dbgloc
  /// \param [out] from
  /// \param [out] to
  /// \param [in] n
  void makeBypassKernel(llvm::Register doInitVar,  const llvm::DebugLoc&dbgloc, llvm::MachineBasicBlock &from, llvm::MachineBasicBlock &to, int n) const;

  /// Kernel通過後に余りループを通る必要が無い場合の回避分岐を生成する
  /// \param [in] doUpdateVar
  /// \param [in] dbgloc
  /// \param [in] CC
  /// \param [out] from
  /// \param [out] to
  void makeBypassMod(llvm::Register doUpdateVar, const llvm::DebugLoc&dbgloc, llvm::AArch64CC::CondCode CC, llvm::MachineBasicBlock &from, llvm::MachineBasicBlock &to) const;

  /// MBBを削除する（およびSuccessor、PHI、Brの更新）
  /// \param [in,out] target 削除対象
  /// \param [in,out] from 削除後のPRED
  /// \param [in,out] to 削除後のSUCC
  /// \param [in] unnecessaryMod 削除対象がCheck2の場合かつ余りループ削除の場合、真を指定する
  void removeMBB(llvm::MachineBasicBlock *target, llvm::MachineBasicBlock *from, llvm::MachineBasicBlock *to, bool unnecessaryMod=false);

  /// 回転数が１の場合、余りループの冗長な繰返し分岐を削除する
  /// \param [in] br
  void removeIterationBranch(llvm::MachineInstr *br, llvm::MachineBasicBlock *body);

  /// oldBodyからnewBodyへ、MachineBasicBlock内の全命令を移動する
  /// \note Successor/Predecessorの変更はここではしない
  /// \param [out] newBody 移動先
  /// \param [out] oldBody 移動元
  void moveBody(llvm::MachineBasicBlock*newBody, llvm::MachineBasicBlock*oldBody);

  /// fromMBBのphiからremoveMBBを取り去る
  /// \param [in,out] fromMBB
  /// \param [in] removeMBB
  void removePredFromPhi(llvm::MachineBasicBlock *fromMBB, llvm::MachineBasicBlock *removeMBB);

public:
  SwplScr(llvm::MachineLoop&ml):ML(ml){}

  /// scr に対し、イタレーションの最後で常に vreg == coefficient * nRemainedIterations + constant を満たすような vreg を探す。
  /// 見つかった場合は vreg の他に、係数や定義点を設定し、 true を返す。
  /// \param [out] TMI
  /// \retval true 誘導変数発見
  /// \retval false 誘導変数検出できず
  bool findBasicInductionVariable(TransformedMIRInfo &TMI) const;


  /// loopの制御をおこなう言を探す。
  /// \param [out] Branch
  /// \param [out] Cmp
  /// \param [out] AddSub
  ///
  /// \retval true loopを構成する各命令を特定できた
  /// \retval false loopを構成する革命例を特定できず
  bool findMIsForLoop(llvm::MachineInstr **Branch, llvm::MachineInstr **Cmp, llvm::MachineInstr **Addsub) const;

  /// SWPLの結果をMIRへ反映するために,MBBの構成を行なう.
  /// \param [in,out] tmi
  void prepareCompensationLoop(TransformedMIRInfo &tmi);

  /// SWPL処理の最後に、無駄な分岐等を削除する
  /// \param [in] tmi
  void postSSA(TransformedMIRInfo &tmi);

  /// original loopから、loop外で参照しているregister情報を収集する
  void collectLiveOut(UseMap &usemap);

};

}
#endif
