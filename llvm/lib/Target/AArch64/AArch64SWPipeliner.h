//=- AArch64SWPipeliner.h - Machine Software Pipeliner Pass -*- c++ -*-------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 Machine Software Pipeliner Pass definitions.
//
//===----------------------------------------------------------------------===//
//=== Copyright FUJITSU LIMITED 2021  and FUJITSU LABORATORIES LTD. 2021   ===//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AARCH64_AARCH64SWPIPELINER_H
#define LLVM_LIB_TARGET_AARCH64_AARCH64SWPIPELINER_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "AArch64.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "AArch64Tm.h"
#include "AArch64Linda.h"

using namespace llvm;

namespace swpl {
// 利用コンテナのエイリアス宣言
using SwplInsts = std::vector<class SwplInst *>;
using SwplRegs = std::vector<class SwplReg *>;
using SwplMems = std::vector<class SwplMem *>;
using SwplInstList = std::list<class SwplInst *>; //vectorではダメなのか
using SwplInstEdges = std::vector<class SwplInstEdge *>;
using SwplInst2InstsMap = std::map<class SwplInst *, SwplInsts>;
using SwplInst2InstEdgesMap = std::map<class SwplInst *, SwplInstEdges>;
using Register2SwplRegMap = std::map<llvm::Register, class SwplReg *>;
using SwplInstEdge2Distances = std::map<SwplInstEdge *, std::vector<unsigned>>;
using SwplInstEdge2Delays = std::map<SwplInstEdge *, std::vector<int>>;
using SwplInstEdge2ModuloDelay = std::map<SwplInstEdge *, int>;

using SwplInsts_iterator = SwplInsts::iterator;
using SwplRegs_iterator = SwplRegs::iterator;
using SwplMems_iterator = SwplMems::iterator;
using SwplInstList_iterator = SwplInstList::iterator;

using MIMap = std::map<const llvm::MachineInstr *, llvm::MachineInstr *>;
using MICMap = std::map<const llvm::MachineInstr *, const llvm::MachineInstr *>;
using RegMap = std::map<llvm::Register, llvm::Register>;
using MIs = std::vector<llvm::MachineInstr *>;

/// classの前方宣言
class SwplLoop;
class SwplInst;
class SwplReg;
class SwplMem;
class SwplInstEdge;
class SwplInstGraph;
class SwplDdg;

/// swpl機能のデバッグ出力（Trad::Swpl_option_messageに対応）
extern  llvm::cl::opt<bool> DebugOutput;
extern  llvm::MachineOptimizationRemarkEmitter *ORE;
extern const llvm::TargetInstrInfo *TII;
extern const llvm::TargetRegisterInfo *TRI;
extern llvm::MachineRegisterInfo *MRI;
extern Tm TM;
extern AliasAnalysis *AA;

/// \class SwplLoop
/// \brief ループ内の命令情報を管理するclass
class SwplLoop {
  llvm::MachineLoop *ML;               ///< ループを示す中間表現
  SwplInsts PreviousInsts;             ///< preheaderの命令の集合
  SwplInsts PhiInsts;                  ///< Phi命令の集合
  SwplInsts BodyInsts;                 ///< ループボディの命令の集合
  SwplMems Mems;                       ///< ループボディ内の SwplMem の集合
  std::map<SwplMem *, int> MemIncrementMap;  ///< SwplMem と増分値のマップ
  llvm::MachineBasicBlock *NewBodyMBB; ///< 対象ループのbodyのMBBを複製し非SSA化したMBB
  MIMap OrgMI2NewMI;                   ///< 対象ループのbodyのオリジナルのMIと複製先のMIのMap
  MICMap NewMI2OrgMI;                   ///< 対象ループのbodyの複製先のMIとオリジナルのMIのMap
  RegMap OrgReg2NewReg;                ///< 対象ループのbodyのオリジナルのRegisterと複製先のRegisterのMap
  MIs    Copies;                       ///< 非SSA化の際に、PhiのLiveinレジスタをLoop内で使用するレジスタへCopyする命令をPreHeaderに生成している \n
                                       ///このCopy命令の集合
  SwplRegs Regs;                       ///< 対象ループ内で生成した SwplReg を管理する \note メモリ解放時に利用する
  SwplMems MemsOtherBody;              ///< Body以外で生成された SwplMem を管理する \note メモリ解放時に利用する

public:
  SwplLoop() {};
  SwplLoop(llvm::MachineLoop &l) {
    ML = &l;
  }

  /// destructor
  ~SwplLoop() {
    destroy();
  };

  // getter
  const llvm::MachineLoop *getML() const { return ML; }
  llvm::MachineLoop *getML() { return ML; }
  SwplInsts &getPreviousInsts() { return PreviousInsts; }
  const SwplInsts &getPreviousInsts() const { return PreviousInsts; }
  SwplInsts &getPhiInsts() { return PhiInsts; }
  const SwplInsts &getPhiInsts() const { return PhiInsts; }
  SwplInsts &getBodyInsts() { return BodyInsts; }
  const SwplInsts &getBodyInsts() const { return BodyInsts; }
  SwplMems &getMems() { return Mems; }
  std::map<class SwplMem *, int> &getMemIncrementMap() { return MemIncrementMap; };
  /// SwplLoop::NewBodyMBB を返す
  llvm::MachineBasicBlock *getNewBodyMBB() { return NewBodyMBB; }
  /// SwplLoop::NewBodyMBB を設定する
  void setNewBodyMBB(llvm::MachineBasicBlock *MBB) { NewBodyMBB = MBB; }
  /// SwplLoop::NewBodyMBB のpreMBBを返す
  llvm::MachineBasicBlock *getNewBodyPreMBB(MachineBasicBlock *bodyMBB) {
    assert(bodyMBB->pred_size() == 1);
    llvm::MachineBasicBlock *preMBB;
    for (auto *MBB:bodyMBB->predecessors()) {
      preMBB = MBB;
    }
    return preMBB;
  }
  /// SwplLoop::NewBodyMBB を削除する
  void deleteNewBodyMBB();
  /// SwplLoop::OrgMI2NewMI を返す
  MIMap &getOrgMI2NewMI() { return OrgMI2NewMI; };
  /// SwplLoop::NewMI2OrgMI を返す
  const llvm::MachineInstr *getOrgMI(const MachineInstr *newMI) { return NewMI2OrgMI.at(newMI); };
  /// SwplLoop::OrgReg2NewReg を返す
  RegMap &getOrgReg2NewReg() { return OrgReg2NewReg; };
  /// SwplLoop::Copies を返す
  MIs &getCopies() { return Copies; };
  /// SwplLoop::Regs を返す
  SwplRegs &getRegs() { return Regs; };
  /// MemsOtherBody を返す
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
  size_t getSizeBodyInsts() { return BodyInsts.size(); }
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
  static SwplLoop *Initialize(MachineLoop &L, const UseMap& LiveOutReg);

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

  bool findBasicInductionVariable(TransformedLindaInfo &TLI) const;
  
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
  void addOrgReg2NewReg(llvm::Register orgReg, llvm::Register newReg) { OrgReg2NewReg[orgReg] = newReg; };
  /// SwplLoop::Copies に要素を追加する
  void addCopies(MachineInstr *copy) { getCopies().push_back(copy); }
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
  
private:
  /// SwplLoop::PreviousInsts の作成
  /// \param [in,out] rmap llvm::Register と SwplReg のマップ
  void makePreviousInsts(Register2SwplRegMap &rmap);
  /// SwplLoop::PhiInsts の作成
  /// \param [in,out] rmap llvm::Register と SwplReg のマップ
  void makePhiInsts(Register2SwplRegMap &rmap);
  /// SwplLoop::BodyInsts の作成
  /// \param [in,out] rmap llvm::Register と SwplReg のマップ
  void makeBodyInsts(Register2SwplRegMap &rmap);
  /// SwplLoop::PhiInsts の作成
  /// \param [in,out] rmap llvm::Register と SwplReg のマップ
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
  void convertSSAtoNonSSA(MachineLoop &L, const UseMap &LiveOutReg);
  /// llvm::MachineBasicBlock の複製を行う
  /// \param[in]  newBody 複製先のMachineBasicBlock
  /// \param[in]  oldBody 複製元のMachineBasicBlock
  void cloneBody(llvm::MachineBasicBlock*newBody, llvm::MachineBasicBlock*oldBody);
  /// Phi命令を検索し、非SSA形式に命令例を変換する
  /// \param[in,out]  body ループボディの llvm::MachineBasicBlock
  /// \param[in,out]  pre preheaderの llvm::MachineBasicBlock
  /// \param[in]  dbglod DebugLoc
  /// \param[in]  org オリジナルの llvm::MachineBasicBlock
  /// \param[in]  LiveOutReg UseMap
  void convertNonSSA(llvm::MachineBasicBlock *body, llvm::MachineBasicBlock *pre, const DebugLoc &dbgloc,
                     llvm::MachineBasicBlock *org, const UseMap &LiveOutReg);
  /// SwplLoop::OrgMI2NewMI の命令列を走査し、SwplLoop::OrgReg2NewReg に登録されているレジスタへのリネーミングを行う
  void renameReg(void);
  /// llvm::Register クラスからレジスタIDを表示する
  void printRegID(llvm::Register r);

  /// SwplLoop の解放
  void destroy();
  
};

/// \class SwplInst
/// \brief 命令情報を管理するclass
/// \note SwplReg の解放は SwplLoop で一括して行う  
class SwplInst {
  llvm::MachineInstr *MI;     ///< 命令を示す中間表現
  SwplLoop *Loop;             ///< 本命令が属するループのSwplLoop
  SwplRegs UseRegs;           ///< 本命令が参照するレジスタのVector
  SwplRegs DefRegs;           ///< 本命令が定義するレジスタのVector
  std::map<int,int> DefOpMap; ///< defsとMachineOperand位置のMap
  std::map<int,int> UseOpMap; ///< usesとMachineOperand位置のMap

public:
  SwplInst() {};
  /// MI や SwplLoop はnullがあり得るのでアドレス渡し
  SwplInst(llvm::MachineInstr *i, class SwplLoop *l) {
    MI = i;
    Loop = l;
  }

  // getter
  const llvm::MachineInstr *getMI() const { return MI; }
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
  /// \param [in]  MI llvm::MachineInstr
  /// \param [in]  loop 対象Loop
  /// \param [in,out]  rmap llvm::Register と SwplReg の対応マップ
  /// \param [in,out]  insts MIより前に出現した命令の SwplInst を格納したvector
  /// \param [in,out]  mems MIより前に出現した SwplMem を格納したvector
  /// \param [in,out]  memsOtherBody SwplMems
  void InitializeWithDefUse(llvm::MachineInstr *MI, SwplLoop *loop, Register2SwplRegMap &rmap,
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

  /// Copy命令かどうかの判定
  bool isCopy() const { return (isInLoop() && getMI() != nullptr && getMI()->isCopy()); }

  /// ADDXrr命令かどうかの判定
  bool isADDXrr() const { return (isInLoop() && getMI() != nullptr && getMI()->getOpcode()==AArch64::ADDXrr); }

  /// 指定PHIの SwplInst::UseReg のうち、Loop内で更新しているレジスタを持つ SwplReg を返す
  /// \return Loop内で更新しているレジスタ
  const SwplReg &getPhiUseRegFromIn(void) const;

  /// \brief 自身の命令の定義レジスタをPhiをまたいで自身で参照するかどうかを判定する
  /// \retval true  定義レジスタを参照する
  /// \retval false 定義レジスタを参照しない
  bool isRecurrence() const;

  /// 当該 SwplInst が使用するレジスタを SwplLoop::Regs にpushする
  /// \param[in] 対象の SwplLoop
  void pushAllRegs(SwplLoop *loop);
  
  /// SwplInst の解放
  void destroy();
  
public:
  /// Predicateレジスタかどうかの判定
  bool isDefinePredicate() const;
  /// Floatingレジスタかどうかの判定
  bool isFloatingPoint() const;
  bool isLoad() const { return getMI()->mayLoad(); }
  bool isStore() const { return getMI()->mayStore(); }
  bool isPrefetch() const {
    switch(MI->getOpcode()) {
      /// AArch64 prefetch
    case AArch64::PRFMui:
      /// SVE prefetch
    case AArch64::PRFB_PRI:
    case AArch64::PRFH_PRI:
    case AArch64::PRFW_PRI:
    case AArch64::PRFD_PRI:
    case AArch64::PRFB_PRR:
    case AArch64::PRFH_PRR:
    case AArch64::PRFW_PRR:
    case AArch64::PRFD_PRR:
    case AArch64::PRFB_D_SCALED:
    case AArch64::PRFH_D_SCALED:
    case AArch64::PRFW_D_SCALED:
    case AArch64::PRFD_D_SCALED:
    case AArch64::PRFB_S_PZI:
    case AArch64::PRFH_S_PZI:
    case AArch64::PRFW_S_PZI:
    case AArch64::PRFD_S_PZI:
    case AArch64::PRFB_D_PZI:
    case AArch64::PRFH_D_PZI:
    case AArch64::PRFW_D_PZI:
    case AArch64::PRFD_D_PZI:
      return true;
    default:
      return false;
    }
  }

  llvm::StringRef getName() {
    return (isPhi()) ? "PHI" : TII->getName( MI->getOpcode() );
  }
  llvm::StringRef getName() const {
    return (isPhi()) ? "PHI" : TII->getName( MI->getOpcode() );
  }
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
};

/// \class SwplReg
/// \brief レジスタ情報を管理するclass
class SwplReg {
  llvm::Register Reg=0;      ///< レジスタ
  SwplInst *DefInst;         ///< 本レジスタを定義(Def)している命令
  size_t DefIndex;           ///< 定義している命令のインデックス
  SwplInstList UseInsts;     ///< レジスタを参照(Use)している命令群 \note SwplInstのSet
  SwplReg *Predecessor;      ///< 先任者
  SwplReg *Successor;        ///< 後任者
  bool EarlyClobber=false;   ///< early-clobber属性

public:
  SwplReg() {};
  SwplReg(llvm::Register r, SwplInst &i, size_t def_index, bool earlyclober=false) {
    Reg = r;
    DefInst = &i;
    DefIndex = def_index;
    Predecessor = nullptr;
    Successor = nullptr;
    EarlyClobber=earlyclober;
  }
  // getter
  llvm::Register const getReg() { return Reg; }
  llvm::Register getReg() const { return Reg; }
  SwplInst *getDefInst() { return DefInst; }
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
  void getDefPort( SwplInst **p_def_inst, int *p_def_index) const;
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
  /// \attention 内包している llvm::Register が同じかどうかは見ていない。
  bool isEqual(SwplReg *reg) { return (this == reg) ? true : false; }

  /// SwplReg::UseInsts の追加
  void addUseInsts(SwplInst *inst) { getUseInsts().push_back(inst); }

  /// SwplReg をdumpする
  void print(void);

  /// 増分値を求める
  /// \return 増分値
  int calcEachRegIncrement();

  SwplInstList_iterator useinsts_begin() { return UseInsts.begin(); }
  SwplInstList_iterator useinsts_end() { return UseInsts.end(); }

  void printDefInst(void);

  TmRegKind getRegKind() const {
    return TM.getRegKind(Reg);
  }

  /// Stack-pointerを扱うレジスタかどうかを判定する
  /// \note tradはframe-pointerを扱うレジスタも特別視しているが
  /// llvmでは区別していないため、Stack-pointerを扱うかどうかのみを判定する。
  bool isStack() const { return (llvm::Register::isStackSlot(Reg)); }

  /// レジスタの数を返す
  int getRegSize() const;
};

/// \class SwplMem
/// \brief メモリアクセスを表現するclass
class SwplMem {
  const llvm::MachineMemOperand *MO;   ///< メモリアクセスを示すオペランド情報
  SwplInst *Inst;                ///< 本メモリアクセスする命令を示す SwplInst
  SwplRegs UseRegs;              ///< 本メモリアクセスを構成する SwplReg のVector
  size_t Size;                   ///< メモリアクセスするサイズ

public:
  SwplMem() {};
  SwplMem(const llvm::MachineMemOperand *op, SwplInst &i, size_t s) {
    MO = op;
    Inst = &i;
    Size = s;
  }

  // getter
  const llvm::MachineMemOperand *getMO() const { return MO; }
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
  void addUseReg(llvm::Register reg, Register2SwplRegMap rmap) { UseRegs.push_back(rmap[reg]);}
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
/// \brief 命令間のエッジを表現するclass
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
/// \brief 命令間の依存グラフを表現するclass
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
/// \brief ループ内の命令の依存情報を管理するclass
class SwplDdg {
  SwplInstGraph *Graph;                ///< 命令間の依存グラフを表現するclass
  SwplLoop *Loop;                      ///< ループ内の命令情報を管理するclass
  SwplInstEdge2Distances DistanceMap;  ///< SwplInst 間のDistanceマップ
  SwplInstEdge2Delays DelaysMap;       ///< SwplInst 間のDelayマップ

public:
  SwplDdg() {};
  SwplDdg(SwplLoop &l) {
    Graph = new SwplInstGraph();
    Loop = &l;
  }

  /// destructor
  ~SwplDdg() {
    delete Graph;
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
  static SwplDdg *Initialize(SwplLoop &loop);

  /// iiを考慮したDelayのMapを生成する
  /// \param[in] ii スケジューリング時のiteration intervalの値
  /// \return 生成したModuloDelayMapのポインタ
  /// \attention 生成したmapの領域は利用者が開放する
  SwplInstEdge2ModuloDelay *getModuloDelayMap(int ii) const;

  /// デバッグ用情報出力
  void print(void) const;
  
private:
  /// 命令単位に SwplInstGraph を生成する
  void generateInstGraph() {
    /// SwplLoop::BodyInsts から命令単位に SwplInstGraph を用意する
    for (auto *inst:getLoopBodyInsts()) {
      getGraph()->appendVertices(*inst);
    }
  }

  /// レジスタによる依存を解析する
  void analysisRegDependence();
  /// レジスタによる真依存を解析する
  void analysisRegsFlowDependence();
  /// レジスタによる逆依存を解析する
  void analysisRegsAntiDependence();
  /// レジスタによる出力依存を解析する
  void analysisRegsOutputDependence();
  /// メモリによる依存を解析する
  void analysisMemDependence();
  /// 命令による依存を解析する
  void analysisInstDependence();

};
//  extern SwplLoop loop;
  
} // end namespace swpl
#endif
