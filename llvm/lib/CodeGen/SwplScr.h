//=- SwplScr.h -  Swpl Scheduling common function -*- C++ -*-----------------=//
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
#ifndef SWPLSCR_H
#define SWPLSCR_H

#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/Register.h"
namespace llvm {
class SwplLoop;
class SwplRegAllocInfo;
class SwplRegAllocInfoTbl;
class SwplExcKernelRegInfoTbl;

/// Swpl-RAで使用する、カーネルループ外のレジスタ情報の行
struct ExcKernelRegInfo {
  unsigned vreg;        ///< 仮想レジスタ
  int      num_def;     ///< 仮想レジスタの定義番号(無効値:0以下の値, 有効値:1以上の値)
  int      num_use;     ///< 仮想レジスタの参照番号(無効値:0以下の値, 有効値:1以上の値)
  unsigned total_mi;    ///< MIの総数

  /// 定義番号を更新する
  /// \param [in] def 定義番号
  void updateNumDef(int def) {
    num_def = def;
    return;
  };

  /// 参照番号を更新する
  /// \param [in] use 参照番号
  void updateNumUse(int use) {
    num_use = use;
    return;
  };
};

/// Swpl-RAで使用する、カーネルループ外のレジスタ情報の表
class SwplExcKernelRegInfoTbl {
  std::vector<ExcKernelRegInfo> ekri_tbl;

public:
  unsigned length() {
    return ekri_tbl.size();
  }

  /// ExcKernelRegInfo構造体を追加する
  /// \param [in] vreg 仮想レジスタ番号
  /// \param [in] num_def 定義番号
  /// \param [in] num_use 参照番号
  void addExcKernelRegInfo(unsigned vreg,
                           int num_def,
                           int num_use);

  /// Swpl-RAで使用する、カーネルループ外のレジスタ情報の表の内、vregに該当する行を返す
  /// \param [in] vreg 仮想レジスタ番号
  /// \retval 非nullptr 指定された仮想レジスタに該当するExcKernelRegInfoへのポインタ
  /// \retval nullptr 指定された仮想レジスタに該当するExcKernelRegInfoは存在しない
  ExcKernelRegInfo* getWithVReg(unsigned vreg);

  /// Swpl-RAで使用する、カーネルループ外のレジスタ情報の表の内、vregが参照から始まっている行の有無を返す
  /// \param [in] vreg 仮想レジスタ番号
  /// \retval true 指定された仮想レジスタが参照から始まっている行あり
  /// \retval false 指定された仮想レジスタが参照から始まっている行なし
  bool isUseFirstVRegInExcK(unsigned vreg);

  /// debug dump
  void dump();
};

/// Swpl-RAで使用する生存区間表の行
struct RegAllocInfo {
  unsigned vreg;                        ///< 割り当て済み仮想レジスタ
  unsigned preg;                        ///< 割り当て済み物理レジスタ
  int      num_def;                     ///< 仮想レジスタの定義番号(無効値:0以下の値, 有効値:1以上の値)
  int      num_use;                     ///< 仮想レジスタの参照番号(無効値:0以下の値, 有効値:1以上の値)
  int      liverange;                   ///< 生存区間(参照番号-定義番号)
  unsigned tied_vreg;                   ///< tied仮想レジスタ
  unsigned vreg_classid;                ///< 仮想レジスタのレジスタクラスID
  std::vector<MachineOperand*> vreg_mo; ///< 仮想レジスタのMachineOperand
  unsigned total_mi;                    ///< MIの総数

  /// MachineOperandを追加する
  /// \param [in,out] mo vreg_moに追加するMachineOperand
  void addMo( MachineOperand* mo ) {
    vreg_mo.push_back( mo );
    return;
  };

  /// tied仮想レジスタを更新する
  /// \param [in] reg tied仮想レジスタ
  void updateTiedVReg( unsigned reg ) {
    tied_vreg = reg;
    return;
  };

  /// 定義番号を更新する。
  /// また、更新された定義番号を使用して、LiveRangeを再計算する。
  /// \param [in] def 定義番号
  void updateNumDef( int def ) {
    num_def = def;
    liverange = calcLiveRange();
    return;
  };

  /// 定義番号を更新する。
  /// また、更新された定義番号を使用して、LiveRangeを再計算する。
  /// 物理レジスタの再利用を禁止するため、参照点は最大値となる。
  /// \param [in] def 定義番号
  void updateNumDefNoReuse(int def) {
    num_def = def;
    liverange = calcLiveRangeNoReuse();
    return;
  };

  /// 参照番号を更新する。
  /// また、更新された参照番号を使用して、LiveRangeを再計算する。
  /// \param [in] use 参照番号
  void updateNumUse( int use ) {
    num_use = use;
    liverange = calcLiveRange();
    return;
  };

  /// 参照番号を更新する。
  /// また、更新された参照番号を使用して、LiveRangeを再計算する。
  /// 物理レジスタの再利用を禁止するため、参照点は最大値となる。
  /// \param [in] use 参照番号
  void updateNumUseNoReuse(int use) {
    num_use = use;
    liverange = calcLiveRangeNoReuse();
    return;
  };

private:
  /// 定義点と参照点から生存区間を求める。
  /// 仮想レジスタごとの生存区間を求める。
  /// 定義点、参照点のどちらかが存在しない場合、計算不能として-1を返す。
  /// \retval 0以上 計算した生存区間
  /// \retval -1 計算不能
  int calcLiveRange();

  /// 定義点と参照点から生存区間を求める。
  /// 仮想レジスタごとの生存区間を求める。
  /// 定義点、参照点のどちらかが存在しない場合、計算不能として-1を返す。
  /// 定義 < 参照の場合、物理レジスタの再利用を禁止するため、
  /// 参照点を最大値にする。
  /// \retval 0以上 計算した生存区間
  /// \retval -1 計算不能
  int calcLiveRangeNoReuse();
};


/// Swpl-RAで使用する生存区間表
class SwplRegAllocInfoTbl {

  std::vector<RegAllocInfo> rai_tbl; ///< Swpl-RAで使用する生存区間表
  unsigned total_mi;                 ///< MIの総数
  int num_ireg = -1;                   ///< Integerレジスタの数
  int num_freg = -1;                   ///< Floating-pointレジスタの数
  int num_preg = -1;                   ///< Predicateレジスタの数

  /// 割り当てた最大レジスタ数を数える
  void countRegs();

  /// レジスタ割り付け情報からliverangeを求める
  /// \param [in] range liverange
  /// \param [in] r レジスタ割り付け情報
  /// \param [in] unitNum レジスタを構成するユニット数
  void setRangeReg(std::vector<int>* range, RegAllocInfo& r, unsigned unitNum);

public:
  SwplRegAllocInfoTbl(unsigned num_of_mi);
  virtual ~SwplRegAllocInfoTbl() {}

  unsigned length() {
    return rai_tbl.size();
  }

  /// RegAllocInfo構造体を追加する
  /// \param [in] vreg 仮想レジスタ番号
  /// \param [in] num_def 定義番号
  /// \param [in] num_use 参照番号
  /// \param [in] mo llvm::MachineOperand
  /// \param [in] tied_vreg tiedの場合、その相手となる仮想レジスタ
  /// \param [in] preg 物理レジスタ番号
  void addRegAllocInfo( unsigned vreg,
                       int num_def,
                       int num_use,
                       MachineOperand *mo,
                       unsigned tied_vreg,
                       unsigned preg=0);

  /// Swpl-RAで使用する生存区間表のうち、vregに該当する行を返す
  /// \param [in] vreg 仮想レジスタ番号
  /// \retval 非nullptr 指定された仮想レジスタに該当するRegAllocInfoへのポインタ
  /// \retval nullptr 指定された仮想レジスタに該当するRegAllocInfoは存在しない
  RegAllocInfo* getWithVReg( unsigned vreg );

  /// Swpl-RAで使用する生存区間表のうち、indexに該当する行を返す
  /// \param [in] idx インデックス番号
  /// \retval 非nullptr 指定されたインデックス番号に該当するRegAllocInfoへのポインタ
  /// \retval nullptr 指定されたインデックス番号に該当するRegAllocInfoは存在しない
  RegAllocInfo* getWithIdx( unsigned idx ) {
    return &(rai_tbl.at(idx));
  }

  /// レジスタが不足した場合の、再利用可能な物理レジスタ返す
  /// \param [in] rai 割り当て対象のRegAllocInfo
  /// \retval 非0 再利用可能な物理レジスタ番号
  /// \retval 0 再利用可能な物理レジスタは見つからなかった
  unsigned getReusePReg( RegAllocInfo* rai );

  /// Swpl-RAで使用する生存区間表のうち、pregに該当する行の有無を返す
  /// \param [in] preg チェック対象の物理レジスタ
  /// \retval true pregに該当する行あり
  /// \retval false pregに該当する行なし
  bool isUsePReg( unsigned preg );

  /// debug dump
  void dump();

  /// Integerレジスタに割り当てた最大レジスタ数を返す
  int countIReg();
  /// Floating-pointレジスタに割り当てた最大レジスタ数を返す
  int countFReg();
  /// Predicateレジスタに割り当てた最大レジスタ数を返す
  int countPReg();

  /// 割り当て可能なIntegerレジスタの数を返す
  int availableIRegNumber() const;
  /// 割り当て可能なFloating-pointレジスタの数を返す
  int availableFRegNumber() const;
  /// 割り当て可能なPredicateレジスタの数を返す
  int availablePRegNumber() const;

private:
  /// 生存区間表の行のLiveRangeが重なるかを判定する
  /// \param [in] reginfo1 チェック対象のRegAllocInfo
  /// \param [in] reginfo2 チェック対象のRegAllocInfo
  /// \retval true 重なる
  /// \retval false 重ならない
  bool isOverlapLiveRange( RegAllocInfo *reginfo1, RegAllocInfo *reginfo2 );

  /// 物理的に重なるレジスタかを判定する
  /// \param [in] preg1 チェック対象の物理レジスタ
  /// \param [in] preg2 チェック対象の物理レジスタ
  /// \retval true 重なる
  /// \retval false 重なる
  bool isPRegOverlap( unsigned preg1, unsigned preg2 );
};

/// 命令列変換情報
struct SwplTransformedMIRInfo {
  Register originalDoVReg=0; ///<  オリジナルの制御変数（updateDoVRegMIのop1）
  Register originalDoInitVar=0; ///< オリジナル制御変数の定義PHIの初期値
  Register nonSSAOriginalDoVReg =0; ///< オリジナルの制御変数に対応した、データ抽出処理で非SSAに変換したMIR
  Register nonSSAOriginalDoInitVar=0; ///< 定義PHIの初期値に対応した、データ抽出処理で非SSAに変換したMIR
  Register doVReg=0; ///<  renaming後の制御変数
  MachineInstr *updateDoVRegMI=nullptr; ///< オリジナルの制御変数の更新命令
  MachineInstr *branchDoVRegMI=nullptr; ///< オリジナルの繰り返し分岐命令
  MachineInstr *initDoVRegMI=nullptr; ///< オリジナルの制御変数の誘導変数(Phi)
  MachineInstr *branchDoVRegMIKernel=nullptr; ///< kernel loopの繰り返し分岐命令
  int iterationInterval=0; ///< ii
  int minimumIterationInterval=0; ///< min_ii
  int coefficient=0; ///< オリジナルの制御変数の更新言のop3
  int minConstant=0; ///< オリジナルの制御変数の分岐言の種類に応じた定数. もともとのループを通過した後にループ制御変数がとりうる最小の値.
  int expansion=0; ///< renaming後の制御変数の分岐言のop3.kernelの最後の分岐言の定数を表す
  size_t nVersions=0; ///< kernelに展開されるループの数を表す
  size_t nCopies=0; ///< kernel,prolog,epilogに必要なオリジナルループの回転数
  size_t requiredKernelIteration=0; ///< tune前の展開に必要な回転数
  std::vector<MachineInstr*> mis; ///< prepareMIs() で使用するmi_tableの情報
  std::vector<MachineInstr*> livein_copy_mis; ///< COPY instruction for livein
  std::vector<MachineInstr*> liveout_copy_mis; ///< COPY instruction for livein
  MachineInstr *prolog_liveout_mi=nullptr; ///< Instruction for prolog liveout
  MachineInstr *kernel_livein_mi=nullptr; ///< Instruction for kernel livein
  MachineInstr *kernel_liveout_mi=nullptr; ///< Instruction for kernel liveout
  MachineInstr *epilog_livein_mi=nullptr; ///< Instruction for epilog livein
  size_t prologEndIndx=0; ///< prepareMIs() で使用するmi_tableの情報
  size_t kernelEndIndx=0; ///< prepareMIs() で使用するmi_tableの情報
  size_t epilogEndIndx=0; ///< prepareMIs() で使用するmi_tableの情報
  bool   isIterationCountConstant=false; ///< 制御変数の初期値が定数として見つかったか (ループ回転数による展開制御に関する変数)
  int doVRegInitialValue=0; ///< オリジナルの制御変数の初期値 (if isIterationCountConstant == true)
  size_t originalKernelIteration=0; ///< 展開前のkernel部分の繰返し数 (if isIterationCountConstant == true)
  size_t transformedKernelIteration=0; ///< 展開後のkernel部分の繰返し数 (if isIterationCountConstant == true)
  size_t transformedModIteration=0; ///<展開後のmod部分の繰返し数 (if isIterationCountConstant == true)
  MachineBasicBlock*OrgPreHeader=nullptr; ///< オリジナルループの入り口
  MachineBasicBlock*Check1=nullptr; ///< 回転数チェック
  MachineBasicBlock*Prolog=nullptr; ///< prolog
  MachineBasicBlock*OrgBody=nullptr; ///< kernel
  MachineBasicBlock*Epilog=nullptr; ///< epilog
  MachineBasicBlock*Check2=nullptr; ///< 余りループ必要性チェック
  MachineBasicBlock*NewPreHeader=nullptr; ///< 余りループの入り口
  MachineBasicBlock*NewBody=nullptr; ///< 余りループ
  MachineBasicBlock*NewExit=nullptr; ///< 余りループの出口
  MachineBasicBlock*OrgExit=nullptr; ///< オリジナルの出口

  SwplRegAllocInfoTbl *swplRAITbl=nullptr; ///< Swpl-RAで使用する生存区間表
  SwplExcKernelRegInfoTbl *swplEKRITbl=nullptr; ///< Swpl-RAで使用するカーネルループ外のレジスタ情報の表

  void print();

  virtual ~SwplTransformedMIRInfo() {
    if (swplRAITbl!=nullptr) {
      delete swplRAITbl;
      swplRAITbl = nullptr;
    }
    if (swplEKRITbl!=nullptr) {
      delete swplEKRITbl;
      swplEKRITbl = nullptr;
    }
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


/// Loop形状を変形したり、Loopから情報を探し出す機能を提供する
class SwplScr {
private:
  MachineLoop& ML;

  /// DO制御変数の初期値を取得する
  /// \param [out] TMI
  /// \retval true 取得成功
  /// \retval false 取得失敗
  bool getDoInitialValue(SwplTransformedMIRInfo &TMI) const;

  /// 定数オペランドを３２ビット整数に変換する
  /// \param [in] op
  /// \param [out] coefficient
  /// \retval true 変換がうまくいった
  /// \retval false 変換できなかった
  bool getCoefficient (const MachineOperand&op, int *coefficient) const;

  /// SWPLで必要な回避分岐を生成する
  /// \param [in] tmi
  /// \param [in] dbgloc
  /// \param [out] skip_kernel_from
  /// \param [out] skip_kernel_to
  /// \param [out] skip_mod_from
  /// \param [out] skip_mod_to
  void makeBypass(const SwplTransformedMIRInfo &tmi, const DebugLoc&dbgloc,
                  MachineBasicBlock &skip_kernel_from, MachineBasicBlock &skip_kernel_to,
                  MachineBasicBlock &skip_mod_from, MachineBasicBlock &skip_mod_to);


  /// MBBを削除する（およびSuccessor、PHI、Brの更新）
  /// \param [in,out] target 削除対象
  /// \param [in,out] from 削除後のPRED
  /// \param [in,out] to 削除後のSUCC
  /// \param [in] unnecessaryMod 削除対象がCheck2の場合かつ余りループ削除の場合、真を指定する
  void removeMBB(MachineBasicBlock *target, MachineBasicBlock *from, MachineBasicBlock *to, bool unnecessaryMod=false);

  /// 回転数が１の場合、余りループの冗長な繰返し分岐を削除する
  /// \param [in] br
  void removeIterationBranch(MachineInstr *br, MachineBasicBlock *body);

  /// oldBodyからnewBodyへ、MachineBasicBlock内の全命令を移動する
  /// \note Successor/Predecessorの変更はここではしない
  /// \param [out] newBody 移動先
  /// \param [out] oldBody 移動元
  void moveBody(MachineBasicBlock*newBody, MachineBasicBlock*oldBody);

  /// fromMBBのphiからremoveMBBを取り去る
  /// \param [in,out] fromMBB
  /// \param [in] removeMBB
  void removePredFromPhi(MachineBasicBlock *fromMBB, MachineBasicBlock *removeMBB);

public:
  using UseMap=llvm::DenseMap<Register, std::vector<MachineOperand*>>;

  SwplScr(MachineLoop&ml):ML(ml){}

  /// scr に対し、イタレーションの最後で常に vreg == coefficient * nRemainedIterations + constant を満たすような vreg を探す。
  /// 見つかった場合は vreg の他に、係数や定義点を設定し、 true を返す。
  /// \param [out] TMI
  /// \retval true 誘導変数発見
  /// \retval false 誘導変数検出できず
  bool findBasicInductionVariable(SwplTransformedMIRInfo &TMI) const;

  /// SWPLの結果をMIRへ反映するために,MBBの構成を行なう.
  /// \param [in,out] tmi
  void prepareCompensationLoop(SwplTransformedMIRInfo &tmi);

  /// SWPL処理の最後に、無駄な分岐等を削除する
  /// \param [in] tmi
  void postSSA(SwplTransformedMIRInfo &tmi);

  /// original loopから、loop外で参照しているregister情報を収集する
  void collectLiveOut(UseMap &usemap);

};


}
#endif
