//=- SWPipeliner.h - Machine Software Pipeliner Pass -*- c++ -*--------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Machine Software Pipeliner Pass definitions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_SWPIPELINER_H
#define LLVM_LIB_CODEGEN_SWPIPELINER_H

#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "llvm/CodeGen/SwplTargetMachine.h"
#include "llvm/InitializePasses.h"
#include <llvm/ADT/SmallSet.h>
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/Register.h"

namespace llvm {

/// forward declaration of class
class SwplScr;
class SwplLoop;
class SwplInst;
class SwplReg;
class SwplMem;
class SwplInstEdge;
class SwplInstGraph;
class SwplDdg;
class SwplTransformMIR;
class SwplRegAllocInfo;
class SwplRegAllocInfoTbl;
class SwplExcKernelRegInfoTbl;

// Alias declaration of usage container
using SwplInsts = std::vector<SwplInst *>;
using SwplRegs = std::vector<SwplReg *>;
using SwplMems = std::vector<SwplMem *>;
using SwplInstList = std::list<SwplInst *>; //May be useless in vector!
using SwplInstEdges = std::vector<SwplInstEdge *>;
using SwplInst2InstsMap = llvm::DenseMap<SwplInst *, SwplInsts>;
using SwplInst2InstEdgesMap = llvm::DenseMap<SwplInst *, SwplInstEdges>;
using Register2SwplRegMap = llvm::DenseMap<Register, SwplReg *>;
using SwplInstEdge2Distances = llvm::DenseMap<SwplInstEdge *, std::vector<unsigned>>;
using SwplInstEdge2Delays = llvm::DenseMap<SwplInstEdge *, std::vector<int>>;
using SwplInstEdge2ModuloDelay = std::map<SwplInstEdge *, int>;
using SwplInstEdge2LsDelay = std::map<SwplInstEdge *, int>;
using SwplInsts_iterator = SwplInsts::iterator;
using SwplRegs_iterator = SwplRegs::iterator;
using SwplMems_iterator = SwplMems::iterator;
using SwplInstList_iterator = SwplInstList::iterator;

/**
 * Returns whether or not the specified loop is a candidate for SWP application from the options and Pragma.
 * @param L Specify target Loop information
 * @param ignoreMetadataOfRemainder true Ignore remainder loop metadata
 * @retval true Candidate for SWP application
 * @retval false SWP not applie
 */
bool enableSWP(const Loop*, bool ignoreMetadataOfRemainder);

/**
 * Returns whether or not the specified loop is a candidate for LS application from the options.
 * @retval true Candidate for LS application
 * @retval false LS not applie
 */
bool enableLS();

/**
 * Returns from Pragma whether the specified loop is memory-independent.
 * @param L Specify target Loop information
 * @retval true pipeline_nodep is specified
 * @retval false pipeline_nodep is not specified
 */
bool enableNodep(const Loop *L);

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

  /// removeMBB from phi in fromMBB
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


/// \class SwplLoop
/// \brief Manage instruction information in loops
class SwplLoop {
  MachineLoop *ML;               ///< IR showing a loop
  SwplInsts PreviousInsts;             ///< collection of preheader instructions
  SwplInsts PhiInsts;                  ///< collection Phi instructions
  SwplInsts BodyInsts;                 ///< collection of loop body's instructions
  SwplMems Mems;                       ///< collection of SwplMem in loop body
  llvm::DenseMap<SwplMem *, int> MemIncrementMap;  ///< Map of SwplMem and Increment value
  MachineBasicBlock *NewBodyMBB; ///< MBB that duplicates the MBB of the body of the target loop and makes it non-SSA
  llvm::DenseMap<const MachineInstr *, MachineInstr *> OrgMI2NewMI;                   ///< Map of the original MI of the body of the target loop and the MI of the copy destination
  llvm::DenseMap<const MachineInstr *, const MachineInstr *> NewMI2OrgMI;                   ///< Map of the MI to which the body of the target loop is copied and the original MI
  std::map<Register, Register> OrgReg2NewReg;                ///< Map of the original Register and copy destination Register of the body of the target loop
  std::vector<MachineInstr *>    Copies;                       ///< When non-SSA, an instruction is generated in PreHeader to copy Phi's Livein register to the register used in Loop. \n
                                       ///collection of this Copy instructions
  SwplRegs Regs;                       ///< Manage SwplReg generated within the target loop \note Used when freeing memory
  SwplMems MemsOtherBody;              ///< Manage SwplMem generated outside of Body \note Used when freeing memory
  SmallSet<Register, 30> liveOuts; ///< A collection to check whether the virtual registers that appear after applying SWPL are LiveOut

public:
  SwplLoop(){}
  SwplLoop(MachineLoop &l) {
    ML = &l;
  }

  /// destructor
  ~SwplLoop() {
    destroy();
  };

  // getter
  const MachineLoop *getML() const { return ML; }
  MachineLoop *getML() { return ML; }
  SwplInsts &getPreviousInsts() { return PreviousInsts; }
  const SwplInsts &getPreviousInsts() const { return PreviousInsts; }
  SwplInsts &getPhiInsts() { return PhiInsts; }
  const SwplInsts &getPhiInsts() const { return PhiInsts; }
  SwplInsts &getBodyInsts() { return BodyInsts; }
  const SwplInsts &getBodyInsts() const { return BodyInsts; }
  SwplMems &getMems() { return Mems; }
  llvm::DenseMap<SwplMem *, int> &getMemIncrementMap() { return MemIncrementMap; };
  /// Return SwplLoop::NewBodyMBB
  MachineBasicBlock *getNewBodyMBB() { return NewBodyMBB; }
  /// Set SwplLoop::NewBodyMBB
  void setNewBodyMBB(MachineBasicBlock *MBB) { NewBodyMBB = MBB; }
  /// Return preMBB of SwplLoop::NewBodyMBB
  MachineBasicBlock *getNewBodyPreMBB(MachineBasicBlock *bodyMBB) {
    assert(bodyMBB->pred_size() == 1);
    MachineBasicBlock *preMBB;
    for (auto *MBB:bodyMBB->predecessors()) {
      preMBB = MBB;
    }
    return preMBB;
  }
  /// Delete SwplLoop::NewBodyMBB
  void deleteNewBodyMBB();
  /// Return SwplLoop::OrgMI2NewMI
  llvm::DenseMap<const MachineInstr *, MachineInstr *> &getOrgMI2NewMI() { return OrgMI2NewMI; };
  /// Return SwplLoop::NewMI2OrgMI
  const MachineInstr *getOrgMI(const MachineInstr *newMI) { return NewMI2OrgMI.at(newMI); };
  /// Return SwplLoop::OrgReg2NewReg
  std::map<Register, Register> &getOrgReg2NewReg() { return OrgReg2NewReg; };
  /// Return SwplLoop::Copies
  std::vector<MachineInstr *> &getCopies() { return Copies; };
  /// Return SwplLoop::Regs
  SwplRegs &getRegs() { return Regs; };
  /// Return MemsOtherBody
  SwplMems &getMemsOtherBody() { return MemsOtherBody; };

  // iterator
  SwplInsts_iterator previous_insts_begin() { return PreviousInsts.begin(); }
  SwplInsts_iterator previous_insts_end() { return PreviousInsts.end(); }
  SwplInsts_iterator bodyinsts_begin() { return BodyInsts.begin(); }
  SwplInsts_iterator bodyinsts_end() { return BodyInsts.end(); }
  SwplMems_iterator mems_begin() { return Mems.begin(); }
  SwplMems_iterator mems_end() { return Mems.end(); }


  /// SwplLoop::PreviousInsts の要素数取得
  size_t getSizePreviousInsts() { return PreviousInsts.size(); }
  size_t getSizePreviousInsts() const { return PreviousInsts.size(); }
  /// SwplLoop::PreviousInsts の要素取得
  SwplInst &getPreviousInst(size_t index) { return *PreviousInsts[index]; }
  const SwplInst &getPreviousInst(size_t index) const { return *PreviousInsts[index]; }
  /// SwplLoop::PreviousInsts に要素を追加する
  void addPreviousInsts(SwplInst *inst) { getPreviousInsts().push_back(inst); }

  /// SwplLoop::PhiInsts の要素数取得
  size_t getSizePhiInsts() { return PhiInsts.size(); }
  size_t getSizePhiInsts() const { return PhiInsts.size(); }
  /// SwplLoop::PhiInsts の要素取得
  SwplInst &getPhiInst(size_t index) { return *PhiInsts[index]; }
  SwplInst &getPhiInst(size_t index) const { return *PhiInsts[index]; }
  /// SwplLoop::PhiInsts に要素を追加する
  void addPhiInsts(SwplInst *inst) { getPhiInsts().push_back(inst); }

  /// \brief SwplLoop::PhiInsts にPhi命令を再収集する
  void recollectPhiInsts();

  /// SwplLoop::BodyInsts への追加
  /// \param [in] inst 追加する SwplInst
  void append_bodyinsts(SwplInst *inst) { BodyInsts.push_back(inst); }

  /// SwplLoop::BodyInsts の要素数取得
  size_t getSizeBodyInsts() const { return BodyInsts.size(); }
  /// SwplLoop::BodyInsts のうち、実命令の命令数を取得
  size_t getSizeBodyRealInsts() const;
  /// SwplLoop::BodyInsts の要素取得
  SwplInst &getBodyInst(size_t index) { return *BodyInsts[index]; }
  const SwplInst &getBodyInst(size_t index) const { return *BodyInsts[index]; }
  /// SwplLoop::BodyInsts に要素を追加する
  void addBodyInsts(SwplInst *inst) { getBodyInsts().push_back(inst); }
  /// SwplLoop::BodyInsts から引数のiteratorの要素を削除し、次のiteratorを返す
  SwplInsts_iterator eraseBodyInsts(SwplInsts_iterator itr) { return getBodyInsts().erase(itr); }

  /// SwplLoop::Mems の要素数取得
  size_t getSizeMems() { return Mems.size(); }
  size_t getSizeMems() const { return Mems.size(); }
  /// SwplLoop::Mems の指定要素取得
  SwplMem &getMemElement(size_t index) { return *Mems[index]; }

  /// SwplLoop を生成する
  /// \param[in]  L MachineLoop
  /// \param [in] LiveOutReg 対象ループの出口BusyRegister（非SSA化で特別処理を行うために利用する）
  /// \return ループ内の命令情報
  static SwplLoop *Initialize(MachineLoop &L, const SwplScr::UseMap& LiveOutReg);

  /// SwplLoop::MemIncrementMap に要素を追加する
  void addMemIncrementMap(SwplMem *mem, int increment) { MemIncrementMap[mem] = increment; }

  /// SwplMem から対応するincrement情報を取得する
  int getMemIncrement(SwplMem *mem) { return MemIncrementMap[mem]; }

  /// SwplLoop::BodyInsts において、第一引数のinstの後に第二引数のinstが現れる場合にtrueを返す
  /// \param[in]  first SwplInst
  /// \param[in]  seccond SwplInst
  bool areBodyInstsOrder(SwplInst *first, SwplInst *seccond);

  /// ２つのメモリアクセスが重なる最初のイタレーション距離を返す
  /// \param[in] former_mem SwplMem
  /// \param[in] latter_mem SwplMem
  /// \return former_memとlatter_memが重なる可能性のある最小の回転数
  unsigned getMemsMinOverlapDistance(SwplMem *former_mem, SwplMem *latter_mem);

  bool findBasicInductionVariable(SwplTransformedMIRInfo &TMI) const;

  /// SwplLoop::OrgMI2NewMI および SwplLoop::NewMI2OrgMI に要素を追加する
  /// \param [in] orgMI オリジナルMI
  /// \param [in] newMI 新しいMI
  void addOrgMI2NewMI(const MachineInstr *orgMI, MachineInstr *newMI) {
    OrgMI2NewMI[orgMI] = newMI;
    NewMI2OrgMI[newMI]=orgMI;
  }
  /// SwplLoop::OrgReg2NewReg に要素を追加する
  /// \param [in] orgReg オリジナルRegister
  /// \param [in] newReg 追加するRegister
  void addOrgReg2NewReg(Register orgReg, Register newReg) { OrgReg2NewReg[orgReg] = newReg; };
  /// SwplLoop::Copies に要素を追加する
  void addCopies(MachineInstr *copy) { getCopies().push_back(copy); liveOuts.insert(copy->getOperand(0).getReg());}
  /// SwplLoop::Regs に要素を追加する
  void addRegs(SwplReg *reg) { getRegs().push_back(reg); }

  /// SwplLoop::OrgMI2NewMI の中身をdumpする
  void dumpOrgMI2NewMI();
  /// SwplLoop::OrgReg2NewReg の中身をdumpする
  void dumpOrgReg2NewReg();
  /// SwplLoop::Copies の中身をdumpする
  void dumpCopies();

  /// メモリ開放するときのためのRegの情報 SwplReg を収集する
  void collectRegs();


  /// Clone後の仮想レジスタがLiveOutしているかを確認する
  bool containsLiveOutReg(Register vreg);


private:
  /// SwplLoop::PreviousInsts の作成
  /// \param [in,out] rmap Register と SwplReg のマップ
  void makePreviousInsts(Register2SwplRegMap &rmap);
  /// SwplLoop::PhiInsts の作成
  /// \param [in,out] rmap Register と SwplReg のマップ
  void makePhiInsts(Register2SwplRegMap &rmap);
  /// SwplLoop::BodyInsts の作成
  /// \param [in,out] rmap Register と SwplReg のマップ
  void makeBodyInsts(Register2SwplRegMap &rmap);
  /// SwplLoop::PhiInsts の作成
  /// \param [in,out] rmap Register と SwplReg のマップ
  void reconstructPhi(Register2SwplRegMap &rmap);
  /// SwplLoop::MemIncrementMap を生成する
  void makeMemIncrementMap();
  /// SwplLoop をダンプする
  void print(void) ;

  /// ２つの引数の SwplMem が SwplLoop::Mems においてfirst,secondの順番に現れるかどうか
  /// \param[in] first SwplMem
  /// \param[in] second SwplMem
  bool areMemsOrder(SwplMem *first, SwplMem *second);

  /// 対象ループのbodyのBasicBlockを非SSA化する
  /// \param[in]  L MachineLoop
  /// \param[in]  LiveOutReg UseReg
  /// \retval true 変換中問題が発生
  /// \retval false 問題なく変換した
  bool convertSSAtoNonSSA(MachineLoop &L, const SwplScr::UseMap &LiveOutReg);

  /// MachineBasicBlock の複製を行う
  /// \param[in]  newBody 複製先のMachineBasicBlock
  /// \param[in]  oldBody 複製元のMachineBasicBlock
  void cloneBody(MachineBasicBlock*newBody, MachineBasicBlock*oldBody);

  /// Phi命令を検索し、非SSA形式に命令例を変換する
  /// \param[in,out]  body ループボディの MachineBasicBlock
  /// \param[in,out]  pre preheaderの MachineBasicBlock
  /// \param[in]  dbglod DebugLoc
  /// \param[in]  org オリジナルの MachineBasicBlock
  /// \param[in]  LiveOutReg UseMap
  void convertNonSSA(MachineBasicBlock *body, MachineBasicBlock *pre, const DebugLoc &dbgloc,
                     MachineBasicBlock *org, const SwplScr::UseMap &LiveOutReg);

  /// pre/post index命令を検索し演算命令＋load/store命令に変換する
  /// \param[in,out]  body ループボディの MachineBasicBlock
  void convertPrePostIndexInstr(MachineBasicBlock *body);

  /// 指定MBBに制限となるMIがあるか確認する
  /// \param[in]  body ループボディの MachineBasicBlock
  /// \retval true 制限が見つかった
  /// \retval false 制限は見つからなかった
  bool checkRestrictions(const MachineBasicBlock *body);

  /// 対象ループのbodyの無駄なCopyを削除する
  /// \param[in,out]  body ループボディの MachineBasicBlock
  void removeCopy(MachineBasicBlock *body);

  /// SwplLoop::OrgMI2NewMI の命令列を走査し、SwplLoop::OrgReg2NewReg に登録されているレジスタへのリネーミングを行う
  void renameReg(void);

  /// tied-defオペランドに対して定義前参照がある場合はCOPYを生成する
  /// \param[in,out]  body ループボディの MachineBasicBlock
  void normalizeTiedDef(MachineBasicBlock* body);

  /// tied-defオペランドは定義前参照があるか確認する
  /// \param[in]  body ループボディの MachineBasicBlock
  /// \param[in]  tiedInstr tied-defが存在する命令。検索の起点となる
  /// \param[in]  tiedDefReg tied-def属性定義レジスタ
  /// \param[in]  tiedUseReg tied-def属性参照レジスタ
  bool check_need_copy4TiedUseReg(const MachineBasicBlock* body, const MachineInstr* tiedInstr,
                              Register tiedDefReg, Register tiedUseReg) const;

  /// Register クラスからレジスタIDを表示する
  void printRegID(Register r);

  /// SwplLoop の解放
  void destroy();

};

/// \class SwplInst
/// \brief 命令情報を管理する
/// \note SwplReg の解放は SwplLoop で一括して行う
class SwplInst {
  MachineInstr *MI;     ///< 命令を示す中間表現
  SwplLoop *Loop;             ///< 本命令が属するループのSwplLoop
  SwplRegs UseRegs;           ///< 本命令が参照するレジスタのVector
  SwplRegs DefRegs;           ///< 本命令が定義するレジスタのVector
  std::map<int,int> DefOpMap; ///< defsとMachineOperand位置のMap
  std::map<int,int> UseOpMap; ///< usesとMachineOperand位置のMap

public:
  SwplInst() {};
  /// MI や SwplLoop はnullがあり得るのでアドレス渡し
  SwplInst(MachineInstr *i, SwplLoop *l) {
    MI = i;
    Loop = l;
  }

  // getter
  const MachineInstr *getMI() const { return MI; }
  MachineInstr *getMI()  { return MI; }
  SwplLoop *getLoop() { return Loop; }
  // 以下のメンバは、持ちまわる必要がないかもしれない
  const SwplRegs &getUseRegs() const { return UseRegs; }
  const SwplRegs &getDefRegs() const { return DefRegs; }
  SwplRegs &getUseRegs() { return UseRegs; }
  SwplRegs &getDefRegs() { return DefRegs; }

  SwplRegs_iterator useregs_begin() { return UseRegs.begin(); }
  SwplRegs_iterator useregs_end() { return UseRegs.end(); }
  SwplRegs_iterator defregs_begin() { return DefRegs.begin(); }
  SwplRegs_iterator defregs_end() { return DefRegs.end(); }

  /// SwplInst の初期化
  /// \param [in]  MI MachineInstr
  /// \param [in]  loop 対象Loop
  /// \param [in,out]  rmap Register と SwplReg の対応マップ
  /// \param [in,out]  insts MIより前に出現した命令の SwplInst を格納したvector
  /// \param [in,out]  mems MIより前に出現した SwplMem を格納したvector
  /// \param [in,out]  memsOtherBody SwplMems
  void InitializeWithDefUse(MachineInstr *MI, SwplLoop *loop, Register2SwplRegMap &rmap,
                            SwplInsts &insts, SwplMems *mems, SwplMems *memsOtherBody);

  /// ループ内の命令かどうかの判定
  bool isInLoop() const { return (Loop != NULL); }

  /// 生成済みかどうかの判定
  bool isCreate() const { return (!isInLoop() && MI == NULL); }

  /// ループボディ内の命令かどうかの判定
  bool isBodyInst() const { return (isInLoop() && MI != NULL); }

  /// SwplInst::UseRegs へaddする
  void addUseRegs(SwplReg *reg) { getUseRegs().push_back(reg); }

  /// SwplInst::UseRegs の要素数取得
  size_t getSizeUseRegs() const { return UseRegs.size(); }
  /// SwplInst::UseRegs の要素取得
  SwplReg &getUseRegs(size_t index) { return *UseRegs[index]; }
  const SwplReg &getUseRegs(size_t index) const { return *UseRegs[index]; }

  /// SwplInst::DefRegs へaddする
  void addDefRegs(SwplReg *reg) { getDefRegs().push_back(reg); }

  /// SwplInst::DefRegs の要素数取得
  size_t getSizeDefRegs() { return DefRegs.size(); }
  size_t getSizeDefRegs() const { return DefRegs.size(); }
  /// SwplInst::DefRegs の要素取得
  SwplReg &getDefRegs(size_t index) { return *DefRegs[index]; }
  SwplReg &getDefRegs(size_t index) const { return *DefRegs[index]; }

  /// SwplInst の比較
  bool isEqual(SwplInst *inst) { return this == inst; }

  /// SwplInst をダンプする
  void print(void);

  /// Phi命令かどうかの判定
  bool isPhi() const { return (isInLoop() && getMI() == nullptr); }

  /// Determining whether it is a copy instruction
  bool isCopy() const { return (isInLoop() && getMI() != nullptr && getMI()->isCopy()); }

  /// 指定PHIの SwplInst::UseReg のうち、Loop内で更新しているレジスタを持つ SwplReg を返す
  /// \return Registers being updated within Loop
  const SwplReg &getPhiUseRegFromIn(void) const;

  /// \brief 自身の命令の定義レジスタをPhiをまたいで自身で参照するかどうかを判定する
  /// \retval true  refer to definition registers
  /// \retval false Do not refer to definition registers
  bool isRecurrence() const;

  /// Push the registers used by the relevant SwplInst to SwplLoop::Regs
  /// \param[in] target SwplLoop
  void pushAllRegs(SwplLoop *loop);

  /// Freeing SwplInst
  void destroy();

public:
  /// Determining whether it is a Predicate register
  bool isDefinePredicate() const;
  /// Determining whether it is a floating register
  bool isFloatingPoint() const;
  bool isLoad() const { return getMI()->mayLoad(); }
  bool isStore() const { return getMI()->mayStore(); }
  bool isPrefetch() const;

  StringRef getName();
  StringRef getName() const;
  int getDefOperandIx(int id) const {
    int ix=DefOpMap.at(id)-1;
    assert(ix>=0);
    return ix;
  }
  int getUseOperandIx(int id) const {
    int ix = UseOpMap.at(id) - 1;
    assert(ix >= 0);
    return ix;
  }
  /// Remember index of SwplInst
  unsigned inst_ix = UINT_MAX;
};

/// \class SwplReg
/// \brief レジスタ情報を管理する
class SwplReg {
  Register Reg=0;      ///< レジスタ
  SwplInst *DefInst;         ///< 本レジスタを定義(Def)している命令
  size_t DefIndex;           ///< 定義している命令のインデックス
  SwplInstList UseInsts;     ///< レジスタを参照(Use)している命令群 \note SwplInstのSet
  SwplReg *Predecessor;      ///< 先任者
  SwplReg *Successor;        ///< 後任者
  bool EarlyClobber=false;   ///< early-clobber属性
  StmRegKind *rk=nullptr;
  unsigned units=1;          ///< レジスタを構成するユニット数.defaultは１

public:
  SwplReg() {};
  SwplReg(const SwplReg &s);
  SwplReg(Register r, SwplInst &i, size_t def_index, bool earlyclober=false);
  virtual ~SwplReg() {if (rk) delete rk;}

  // getter
  SwplInst *getDefInst() { return DefInst; }
  Register const getReg() { return Reg; }
  Register getReg() const { return Reg; }
  const SwplInst *getDefInst() const { return DefInst; }
  size_t getDefIndex() const { return DefIndex; }
  SwplInstList &getUseInsts() { return UseInsts; }
  const SwplInstList &getUseInsts() const { return UseInsts; }
  const SwplReg *getPredecessor() const { return Predecessor; }
  const SwplReg *getSuccessor() const { return Successor; }
  SwplReg *getPredecessor() { return Predecessor; }
  SwplReg *getSuccessor() { return Successor; }
  bool isEarlyClobber() const {return EarlyClobber;}

  /// SwplReg::DefInst, SwplReg::DefIndex の取得
  void getDefPort( SwplInst **p_def_inst, int *p_def_index);
  /// SwplReg::DefInst, SwplReg::DefIndex の取得
  void getDefPort( const SwplInst **p_def_inst, int *p_def_index) const;
  /// ループ内の SwplReg::DefInst, SwplReg::DefIndex の取得
  void getDefPortInLoopBody( SwplInst **p_def_inst, int *p_def_index);

  /// SwplReg::Reg がNULLかどうかの判定
  bool isRegNull() { return (!Reg.isValid()); }
  bool isRegNull() const { return (!Reg.isValid()); }
  /// SwplReg::UseInsts が空か判定
  bool isUseInstsEmpty() { return UseInsts.empty(); }

  /// 参照する命令があるかどうかを返す
  bool isUsed() const { return !(UseInsts.empty()); }

  /// 参照する命令がBody内にあるかどうかを返す
  bool isUsedInBody() const {
    for (auto *inst:getUseInsts()) {
      if (inst->isBodyInst()) { return true; }
    }
    return false;
  }
  /// SwplReg の継承
  /// \param [in,out] former_reg
  /// \param [in,out] latter_reg
  void inheritReg( SwplReg *former_reg, SwplReg *latter_reg);
  /// SwplReg の比較
  /// \attention 内包している Register が同じかどうかは見ていない。
  bool isEqual(SwplReg *reg) { return (this == reg) ? true : false; }

  /// SwplReg::UseInsts の追加
  void addUseInsts(SwplInst *inst) { getUseInsts().push_back(inst); }

  /// SwplReg をdumpする
  void print(void);


  SwplInstList_iterator useinsts_begin() { return UseInsts.begin(); }
  SwplInstList_iterator useinsts_end() { return UseInsts.end(); }

  void printDefInst(void);


  bool isSameKind(unsigned id) const {
    return rk->isSameKind(id);
  }
  bool isIntegerCCRegister() const {
    return rk->isIntegerCCRegister();
  }
  bool isPredReg() const {
    return rk->isPredicate();
  }
  bool isFloatReg() const {
    return rk->isFloating();
  }
  bool isIntReg() const {
    return rk->isInteger();
  }

  /// Stack-pointerを扱うレジスタかどうかを判定する
  bool isStack() const { return (Register::isStackSlot(Reg)); }

  /// レジスタを構成するユニット数を返す
  unsigned getRegSize() const {return units;}

  /// レジスタを構成するユニット数を設定する
  void setRegSize(unsigned n) {units=n;}
};

/// \class SwplMem
/// \brief メモリアクセスを表現する
class SwplMem {
  const MachineMemOperand *MO;   ///< メモリアクセスを示すオペランド情報
  SwplInst *Inst;                ///< 本メモリアクセスする命令を示す SwplInst
  SwplRegs UseRegs;              ///< 本メモリアクセスを構成する SwplReg のVector
  size_t Size;                   ///< メモリアクセスするサイズ

public:
  SwplMem() {};
  SwplMem(const MachineMemOperand *op, SwplInst &i, size_t s) {
    MO = op;
    Inst = &i;
    Size = s;
  }

  // getter
  const MachineMemOperand *getMO() const { return MO; }
  const SwplInst *getInst() const { return Inst; }
  SwplInst *getInst() { return Inst; }
  SwplRegs &getUseRegs() { return UseRegs; }
  const SwplRegs &getUseRegs() const { return UseRegs; }
  size_t getSize() { return Size; }

  SwplRegs_iterator useregs_begin() { return UseRegs.begin(); }
  SwplRegs_iterator useregs_end() { return UseRegs.end(); }

  /// SwplMem::UseRegs の要素数取得
  size_t getSizeUseRegs() { return UseRegs.size(); }
  /// SwplMem::UseReg の要素取得
  SwplReg &getUseReg(size_t index) { return *UseRegs[index]; }
  /// SwplMem::UseRegs への追加
  void addUseReg(Register reg, Register2SwplRegMap rmap) { UseRegs.push_back(rmap[reg]);}
  /// メモリアクセスの増分値を求める
  int calcEachMemAddressIncrement();

  /// SwplMem がメモリを定義するかどうか
  /// \retval true 定義する
  /// \retval false 定義しない
  bool isMemDef() { return Inst->getMI()->mayStore(); }

  /// デバッグ用情報出力
  void printInst(void);

  const static int MAX_LOOP_DISTANCE=20; ///< Max possible iteration-parallelism.
};

/// \class SwplInstEdge
/// \brief 命令間のエッジを表現する
class SwplInstEdge {
  const SwplInst *InitialVertex;  ///< エッジを構成する始点のノード
  const SwplInst *TerminalVertex; ///< エッジを構成する終点のノード

public:
  // constructor
  SwplInstEdge() {};
  SwplInstEdge(const SwplInst &ini, const SwplInst &term) {
    InitialVertex = &ini;
    TerminalVertex = &term;
  }
  // getter
  const SwplInst *getInitial() const { return InitialVertex; }
  const SwplInst *getTerminal() const { return TerminalVertex; }
};

/// \class SwplInstGraph
/// \brief 命令間の依存グラフを表現する
class SwplInstGraph {
  SwplInsts Vertices;                  ///< ノード（命令）のVector
  SwplInstEdges Edges;                 ///< 命令間のエッジのVector
  SwplInst2InstsMap SuccessorsMap;     ///< 各ノードとsuccessorの集合を管理するMap
  SwplInst2InstsMap PredecessorsMap;   ///< 各ノードとpredecessorの集合を管理するMap
  SwplInst2InstEdgesMap GoingEdgesMap; ///< 各ノードとエッジの集合を管理するMap
  SwplInsts emptyInsts; ///< SwplInstGraph::SuccessorsMap , SwplInstGraph::PredecessorsMap のキー無し時返却用の空 SwplInsts

public:
  // constructor
  SwplInstGraph() {
    emptyInsts.clear();
  };

  // destructor
  ~SwplInstGraph() {
    for(auto *edge:Edges) { delete edge; }
  };
  
  /// SwplInstGraph::SuccessorsMap から対象のSuccessorsを返す
  /// \param[in] &inst SwplInst
  /// \return SwplInsts Successors
  const SwplInsts &getSuccessors(SwplInst &inst) const {
    if( SuccessorsMap.find(&inst) == SuccessorsMap.end() ) {
      return emptyInsts;
    }
    return SuccessorsMap.at(&inst);
  }

  /// SwplInstGraph::PredecessorsMap から対象のPredecessorsを返す
  /// \param[in] &inst SwplInst
  /// \return SwplInsts Predecessors
  const SwplInsts &getPredecessors(SwplInst &inst) const {
    if( PredecessorsMap.find(&inst) == PredecessorsMap.end() ) {
      return emptyInsts;
    }
    return PredecessorsMap.at(&inst);
  }

  const SwplInsts &getVertices() const { return Vertices; }
  SwplInstEdges &getEdges() { return Edges; }
  const SwplInstEdges &getEdges() const { return Edges; }

  /// Vertices に SwpInsts のiteratorが指す SwplInst を追加する
  /// \param[in] &inst SwplInst
  void appendVertices(SwplInst &inst) { Vertices.push_back(&inst); }

  /// GoingEdgesMap にinitial->terminalのエッジが登録されていた場合はその SwplInstEdge を返却する
  /// \param[in] &initial SwplInst
  /// \param[in] &terminal SwplInst
  /// \return SwplInstEdge
  /// \note 見つからない場合はnullptrを返却する
  SwplInstEdge *findEdge(SwplInst &initial, SwplInst &terminal) {
    SwplInstEdges &edges = GoingEdgesMap[&initial];
    for(auto *edge:edges) {
      if (&terminal == edge->getTerminal()) {
        return edge;
      }
    }
    return nullptr;
  }
  SwplInstEdge *findEdge(const SwplInst &initial, const SwplInst &terminal) const {
    const SwplInstEdges &edges = GoingEdgesMap.at(&(const_cast<SwplInst &>(initial)));
    for(auto *edge:edges) {
      if (&(const_cast<SwplInst&>(terminal)) == edge->getTerminal()) {
        return edge;
      }
    }
    return nullptr;
  }

  /// initial->terminalの SwplInstEdge を生成し、返却する
  /// \details 
  /// また、SwplInstGraph::SuccessorsMap , SwplInstGraph::PredecessorsMap , 
  /// SwplInstGraph::GoingEdgesMap へinitialとterminalの関係をpushする。
  SwplInstEdge *createEdge(SwplInst &initial, SwplInst &terminal) {
    SwplInstEdge *edge = new SwplInstEdge(initial, terminal);
    Edges.push_back(edge);
    SuccessorsMap[&initial].push_back(&terminal);
    PredecessorsMap[&terminal].push_back(&initial);
    GoingEdgesMap[&initial].push_back(&*edge);
    return edge;
  }
};

/// \class SwplDdg
/// \brief ループ内の命令の依存情報を管理する
class SwplDdg {
  SwplInstGraph *Graph;                ///< 命令間の依存グラフを表現するclass
  SwplLoop *Loop;                      ///< ループ内の命令情報を管理するclass
  SwplInstEdge2Distances DistanceMap;  ///< SwplInst 間のDistanceマップ
  SwplInstEdge2Delays DelaysMap;       ///< SwplInst 間のDelayマップ

public:
  static const int UNKNOWN_MEM_DIFF = INT_MIN;        /* 0 の正反対 */

  SwplDdg() {};
  SwplDdg(SwplLoop &l) {
    Graph = new SwplInstGraph();
    Loop = &l;
  }

  /// destructor. Use destroy() to also delete the Graph area.
  virtual ~SwplDdg() {
  }

  /// destroy ddg object
  static void destroy(SwplDdg* ddg) {
    delete ddg->Graph;
    delete ddg;
  }
  
  /// SwplLoop::BodyInsts のiterator-beginを返す
  SwplInsts_iterator loop_bodyinsts_begin() { return Loop->bodyinsts_begin(); }
  /// SwplLoop::BodyInsts のiterator-endを返す
  SwplInsts_iterator loop_bodyinsts_end() { return Loop->bodyinsts_end(); }
  /// SwplLoop::Mems のiterator-beginを返す
  SwplMems_iterator loop_mems_begin() { return Loop->mems_begin(); }
  /// SwplLoop::Mems のiterator-endを返す
  SwplMems_iterator loop_mems_end() { return Loop->mems_end(); }

  // getter
  SwplLoop *getLoop() { return Loop; }
  const SwplLoop &getLoop() const { return *Loop; }
  SwplInstGraph *getGraph() { return Graph; }
  const SwplInstGraph& getGraph() const { return *Graph; }
  SwplInstEdge2Distances &getDistanceMap() { return DistanceMap; }
  SwplInstEdge2Delays &getDelaysMap() { return DelaysMap; }

  /// 引数の SwplInstEdge をkeyに SwplDdg::DistanceMap のvalueを取り出す
  const std::vector<unsigned> &getDistancesFor(SwplInstEdge &edge) const { return DistanceMap.at(&edge); }
  /// 引数の SwplInstEdge をkeyに SwplDdg::DistanceMap のvalueを取り出す
  /// 該当するkeyが存在しない場合は、対応する空のvalue(std::vector<unsigned>)を生成し、これを返す
  std::vector<unsigned> &getDistancesFor(SwplInstEdge &edge) { return DistanceMap[&edge]; }
  /// 引数の SwplInstEdge をkeyに SwplDdg::DelaysMap のvalueを取り出す
  const std::vector<int> &getDelaysFor(SwplInstEdge &edge) const { return DelaysMap.at(&edge); }
  /// 引数の SwplInstEdge をkeyに SwplDdg::DelaysMap のvalueを取り出す
  /// \note 該当するkeyが存在しない場合は、対応する空のvalue(std::vector<int>)を生成し、これを返す
  std::vector<int> &getDelaysFor(SwplInstEdge &edge) { return DelaysMap[&edge]; }

  /// SwplLoop::BodyInsts の命令数を返す
  size_t getLoopBody_ninsts() const { return Loop->getSizeBodyInsts(); }
  /// SwplLoop::BodyInsts を取得する
  SwplInsts &getLoopBodyInsts() { return getLoop()->getBodyInsts(); }

  /// SwplLoop::Mems を取得する
  SwplMems &getLoopMems() { return getLoop()->getMems(); }

  /// SwplDdg を生成し SwplLoop の命令の依存情報を設定し復帰する
  /// \param[in,out]  loop SwplLoop
  /// \return     ループ内の命令の依存情報
  static SwplDdg *Initialize(SwplLoop &loop,bool Nodep);

  /// iiを考慮したDelayのMapを生成する
  /// \param[in] ii スケジューリング時のiteration intervalの値
  /// \return 生成したModuloDelayMapのポインタ
  /// \attention 生成したmapの領域は利用者が開放する
  SwplInstEdge2ModuloDelay *getModuloDelayMap(int ii) const;

  /// デバッグ用情報出力
  void print(void) const;

  struct IOmi {
    unsigned id;
    std::string mi;
  };
  struct IOddgnodeinfo {
    unsigned distance;
    int delay;
  };
  struct IOddgnode {
    IOmi from;
    IOmi to;
    std::vector<IOddgnodeinfo> infos;
  };
  struct IOddg {
    std::string fname;
    unsigned loopid;
    std::vector<IOddgnode> ddgnodes;
  };

  /// Import yaml of DDG
  void importYaml();

private:
  /// 命令単位に SwplInstGraph を生成する
  void generateInstGraph() {
    /// SwplLoop::BodyInsts から命令単位に SwplInstGraph を用意する
    for (auto *inst:getLoopBodyInsts()) {
      getGraph()->appendVertices(*inst);
    }
  }

  /// Analyzing register dependencies
  void analysisRegDependence();
  /// Analyze flow dependence by registers
  void analysisRegsFlowDependence();
  /// Analyzing anti-dependencies due to registers
  void analysisRegsAntiDependence();
  /// Analyze output dependence by registers
  void analysisRegsOutputDependence();
  /// Analyzing register dependencies for tied-def
  void analysisRegDependence_for_tieddef();
  /// Analyze memory dependencies
  void analysisMemDependence();
  /// Analyze instruction dependencies
  void analysisInstDependence();
  /// Export yaml of DDG
  void exportYaml ();

};

/// \class LsDdg
/// \brief Manage instruction dependency information for local scheduling
class LsDdg {
  SwplInstGraph *Graph;                
  SwplLoop *Loop;                      
  SwplInstEdge2LsDelay DelaysMap;  ///< Delay map between SwplInst

public:
  LsDdg() {};
  LsDdg(SwplLoop &l) {
    Graph = new SwplInstGraph();
    Loop = &l;
  }

  /// destroy ddg object
  static void destroy(LsDdg* ddg) {
    delete ddg->Graph;
    delete ddg;
  }

  SwplInstGraph *getGraph() { return Graph; }
  const SwplInstGraph &getGraph() const { return *Graph; }
  SwplInsts &getLoopBodyInsts() { return Loop->getBodyInsts(); }
  int getDelay(SwplInstEdge &edge) const { return DelaysMap.at(&edge); }
  void setDelay(SwplInstEdge &edge, int delay) { DelaysMap[&edge] = delay; }

  /// Convert DDG for SWPL to DDG for LS
  /// \param[in]  swplddg SwplDdg
  /// \return DDG for LS
  static LsDdg *convertDdgForLS(SwplDdg *swplddg);

  /// Output for debugging
  void print(void) const;

private:
  /// Add loop body instructions to the graph as nodes
  void generateInstGraph() {
    for (auto *inst:getLoopBodyInsts()) {
      getGraph()->appendVertices(*inst);
    }
  }

  /// Add Edge to Graph
  /// If an edge already exists,
  /// it is updated if the DELAY is greater than the existing edge.
  /// \param[in]  former_inst former SwplInst
  /// \param[in]  latter_inst latter SwplInst
  /// \param[in]  delay delay
  void addEdge(SwplInst &former_inst, SwplInst &latter_inst, const int delay);

  /// Add a SwplDdg edge with distance:0
  /// \param[in]  swplddg SwplDdg
  void addEdgeNoDistance(SwplDdg *swplddg);
  
  /// Add register antidependence edges
  /// \param[in]  insts SwplInsts
  void addEdgeRegsAntiDependences(SwplInsts &insts);
};

/// SWPL pass
struct SWPipeliner : public MachineFunctionPass {
public:
  static char ID;               ///< PassのID
  static MachineOptimizationRemarkEmitter *ORE;
  static const TargetInstrInfo *TII;
  static const TargetRegisterInfo *TRI;
  static MachineRegisterInfo *MRI;
  static SwplTargetMachine *STM;
  static AliasAnalysis *AA;
  static std::string Reason;
  static SwplLoop *currentLoop;
  static unsigned min_ii_for_retry;
  static unsigned loop_number;

  /// the number of concurrent read instructions in the instruction fetch stage.
  static unsigned FetchBandwidth;

  /// the number of concurrent read instructions in the decode stage.
  static unsigned RealFetchBandwidth;

  /// Result of specifying limit suppression option
  enum class SwplRestrictionsFlag {None, MultipleReg, MultipleDef, All};

  /// Target determination result
  enum class TargetInfo {SWP_Target, LS1_Target, LS2_Target, LS3_Target, SWP_LS_NO_Target};

  /// Inquiry for specifying restriction suppression option
  static bool isDisableRestrictionsCheck(SwplRestrictionsFlag f);

  MachineFunction *MF = nullptr;
  const MachineLoopInfo *MLI = nullptr;

  /// -swpl-debug指定されているか
  static bool isDebugOutput();
  /// -swpl-debug-ddg指定されているか
  static bool isDebugDdgOutput();
  /// Acquiring the specified value of the -swpl-minii option
  static unsigned nOptionMinIIBase();
  /// Acquiring the specified value of the -swpl-maxii option
  static unsigned nOptionMaxIIBase();

  static bool isExportDDG();
  static bool isImportDDG();
  static StringRef getDDGFileName();

  /**
   * \brief SWPipelinerのコンストラクタ
   */
  SWPipeliner() : MachineFunctionPass(ID) {
    initializeSWPipelinerPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &mf) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<MachineLoopInfo>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<MachineOptimizationRemarkEmitterPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  static void remarkMissed(const char *msg, MachineFunction &mf);
  bool doInitialization (Module &m) override;
  bool doFinalization (Module &) override;

  /**
   * \brief Pass名を取得する
   *
   * -debug-passオプションを使用した際などに表示されるPassの
   * 名前を指定する。
   */
  StringRef getPassName() const override {
    return "Software Pipeliner";
  }

  /// remark-missedメッセージを出力する
  static void remarkMissed(const char *msg, MachineLoop &L);
  /// Output of nodep remarks messages
  static void remarkAnalysis(const char *msg, MachineLoop &L, const char *Name);

  /// 制限を含んだループを検出した際のSWPL非対象とするメッセージ
  /// param target 制限を含んだMI
  static void
  makeMissedMessage_RestrictionsDetected(const MachineInstr &target);

private:
  /**
   * \brief scheduleLoop
   *        Perform Swpl optimization.
   *        ・Target loop determination
   *        ・Data extraction
   *        ・scheduling
   *        ・Scheduling results reflected
   *
   * \param[in] L Target MachineLoop
   * \retval true  Swpl optimization was applied.
   * \retval false Swpl optimization was not applied.
   */
  bool scheduleLoop(MachineLoop &L);

  /**
   * \brief shouldOptimize
   *        Determine the Swpl optimization instructions for the target loop.
   *
   * \param[in] L Target MachineLoop
   * \retval true  Swpl optimization target instruction
   * \retval false Swpl optimization target instruction or optimization suppression instruction
   */
  bool shouldOptimize(const Loop *BBLoop);

  /**
   * \brief isTargetLoops
   *        Target loop determination
   */
  TargetInfo isTargetLoops(MachineLoop &L, const Loop *BBLoop);

  bool isTooManyNumOfInstruction(const MachineLoop &L) const;

  bool isNonMostInnerLoopMBB(const MachineLoop &L) const;

  bool isNonScheduleInstr(const MachineLoop &L) const;

  bool isNonNormalizeLoop(const MachineLoop &L) const;

  /**
   * \brief outputRemarkMissed
   *        Output of messages not subject to scheduling      
   *
   * \param[in] is_swpl Specify output of messages not covered by SWPL
   * \param[in] is_ls Specify output of messages not covered by LS
   * \param[in] L Target MachineLoop
   */
  void outputRemarkMissed(bool is_swpl, bool is_ls, const MachineLoop &L) const;

  /**
   * \brief software_pipeliner
   *        Perform Swpl optimization.
   *        ・Data extraction
   *        ・scheduling
   *        ・Scheduling results reflected
   *
   * \param[in] L Target MachineLoop
   * \param[in] BBLoop Target BasicBlock
   * \retval true  SWPL applicable
   * \retval false SWPL not applicable
   */
  bool software_pipeliner(MachineLoop &L, const Loop *BBLoop);

  bool localScheduler1(const MachineLoop &L);

  bool localScheduler2(const MachineLoop &L);

  bool localScheduler3(const MachineLoop &L);
};

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






} //end namespace llvm
#endif
