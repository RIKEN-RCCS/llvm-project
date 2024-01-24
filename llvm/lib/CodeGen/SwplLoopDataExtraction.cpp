//=- LoopDataExtraction.cpp - SWPL LOOP -*- c++ -*---------------------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  Loop Data Extraction (SwplLoop)
//
//===----------------------------------------------------------------------===//

#include "SWPipeliner.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include <iostream>


using namespace llvm;
#define DEBUG_TYPE "swp-loop"


static cl::opt<bool> DebugLoop("swpl-debug-loop",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DebugPrepare("swpl-debug-prepare",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> NoUseAA("swpl-no-use-aa",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableConvPrePost("swpl-disable-convert-prepost",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableRmCopy("swpl-disable-rm-copy",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableSuppressCopy("swpl-disable-suppress-copy",cl::init(false), cl::ReallyHidden);
static cl::opt<bool>
    IgnoreRegClass_SuppressCopy("swpl-ignore-class-suppress-copy",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableNormalizeTieddef("swpl-enable-normalize-tieddef",cl::init(true), cl::ReallyHidden);

// rm-copyを強化する（reg-alloc時と同じ処理にする）
static cl::opt<bool> IgnoreRegClass_RmCopy("swpl-ignore-class-rm-copy",cl::init(true), cl::ReallyHidden);
static cl::opt<bool> NoDep("swpl-nodep",cl::init(false), cl::ReallyHidden);

using BasicBlocks = std::vector<MachineBasicBlock *>;
using BasicBlocksIterator = std::vector<MachineBasicBlock *>::iterator ;
static MachineBasicBlock *getPredecessorBB(MachineBasicBlock &BB);
static void follow_single_predecessor_MBBs(MachineLoop *L, BasicBlocks *BBs);
static void construct_use(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO, SwplInsts &insts);
static void construct_mem_use(Register2SwplRegMap &rmap, SwplInst &inst, const MachineMemOperand *MMO, SwplInsts &insts,
                              SwplMems *mems, SwplMems *memsOtherBody);
static void construct_def(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO);


void SwplReg::inheritReg(SwplReg *former_reg, SwplReg *latter_reg) {
  assert (former_reg->Successor == NULL);
  assert (latter_reg->Predecessor == NULL);

  former_reg->Successor = latter_reg;
  latter_reg->Predecessor = former_reg;

  return;
}

void SwplReg::getDefPort( const SwplInst **p_def_inst, int *p_def_index) const {
  *p_def_inst = DefInst;
  *p_def_index = (int)DefIndex;
}

void SwplReg::getDefPort( SwplInst **p_def_inst, int *p_def_index) {
  *p_def_inst = DefInst;
  *p_def_index = (int)DefIndex;
}

void SwplReg::getDefPortInLoopBody( SwplInst **p_def_inst, int *p_def_index) {
  getDefPort(p_def_inst, p_def_index);
  while(*p_def_inst != nullptr && (*p_def_inst)->isPhi()) {
    SwplReg Reg = (*p_def_inst)->getPhiUseRegFromIn();
    const SwplInst *inst = *p_def_inst;
    Reg.getDefPort(p_def_inst, p_def_index);
    if (*p_def_inst == inst) {
      // Body内にdef_instが存在しない 
      *p_def_inst = nullptr;
      return;
    }
  }
  if (!((*p_def_inst)->isInLoop())) {
    *p_def_inst = nullptr;
  }
}

bool SwplInst::isDefinePredicate() const {
  for (auto *reg:getDefRegs()) {
    if (reg->isPredReg()) return true;
  }
  return false;
}

bool SwplInst::isFloatingPoint() const {
  for (auto *reg:getDefRegs()) {
    if (reg->isFloatReg()) return true;
  }
  return false;
}

const SwplReg &SwplInst::getPhiUseRegFromIn() const {
  assert(isPhi());
  assert(getSizeUseRegs() == 2);
  // getUseRegs(0)が外ブロックからの場合
  // getUseRegs(1)がループブロックからの場合
  return getUseRegs(1);
}

bool SwplInst::isPrefetch() const {
  return SWPipeliner::TII->isPrefetch(MI->getOpcode());
}
StringRef SwplInst::getName() {
  return (isPhi()) ? "PHI" : SWPipeliner::TII->getName( MI->getOpcode() );
}

StringRef SwplInst::getName() const {
  return (isPhi()) ? "PHI" : SWPipeliner::TII->getName( MI->getOpcode() );
}


SwplLoop *SwplLoop::Initialize(MachineLoop &L, const SwplScr::UseMap& LiveOutReg) {
  SwplLoop *loop = new SwplLoop(L);
  Register2SwplRegMap rmap;

  /// convertSSAtoNonSSA() を呼び出し、対象ループの非SSA化を行う。
  if (loop->convertSSAtoNonSSA(L, LiveOutReg)) return loop;

  /// makePreviousInsts() を呼び出し、ループボディで使われているレジスタを定義しているループ外の命令の一覧
  /// SwplLoop::PreviousInsts を作成する。
  loop->makePreviousInsts(rmap);
  /// makePhiInsts() を呼び出し、Phi命令の情報 SwplLoop::PhiInsts を作成する。
  loop->makePhiInsts(rmap);
  /// makeBodyInsts() を呼び出し、ループボディ内の命令一覧 SwplLoop::BodyInsts を作成する。
  loop->makeBodyInsts(rmap);
  /// reconstructPhi() を呼び出し、次のイテレーションで使われるレジスタの情報をPhi命令のuse情報として追加する。
  loop->reconstructPhi(rmap);
  
  /// makeMemIncrementMap() を呼び出し、メモリとアドレス増分値のマップ SwplLoop::MemIncrementMap を作成する。
  loop->makeMemIncrementMap();

  /// collectRegs() を呼び出し、メモリ開放するときのためのRegの情報 SwplReg を収集する。
  loop->collectRegs();
  
  if (DebugLoop) {
    loop->print();
  }
  return loop;
}

/// ループボディで使われているレジスタを定義しているループ外の命令の一覧 SwplLoop::PreviousInsts を作成する。
void SwplLoop::makePreviousInsts(Register2SwplRegMap &rmap) {
  BasicBlocks BBs;

  follow_single_predecessor_MBBs(getML(), &BBs);

  for (auto *BB:BBs) {
    for (auto &MI:BB->instrs()) {
      if (MI.isDebugInstr()) { continue; }
      if (SWPipeliner::TII->isSwplPseudoMI(MI)) { continue; }
      SwplInst *inst = new SwplInst(&MI, (SwplLoop *)nullptr);
      inst->InitializeWithDefUse(&MI, (SwplLoop *)nullptr, rmap, getPreviousInsts(), (SwplMems *)nullptr, &(getMemsOtherBody()));
      addPreviousInsts(inst);
    }
  }

  // 非SSA化で生成したCopy命令の情報を収集する
  MachineBasicBlock *bodyMBB = getNewBodyMBB();
  MachineBasicBlock *preMBB = getNewBodyPreMBB(bodyMBB);
  for (auto &MI:preMBB->instrs()) {
    if (MI.isDebugInstr()) { continue; }
    SwplInst *inst = new SwplInst(&MI, (SwplLoop *)nullptr);
    inst->InitializeWithDefUse(&MI, (SwplLoop *)nullptr, rmap, getPreviousInsts(), (SwplMems *)nullptr, &(getMemsOtherBody()));
    addPreviousInsts(inst);
  }
}

/// ループ内で定義があるレジスタに対して SwplLoop::PhiInsts を生成する。
void SwplLoop::makePhiInsts(Register2SwplRegMap &rmap) {
  /// 対象は必ず NewBodyMBB 内にあるCopy命令。
  /// Copies に収集してあるので、その命令を対象にする。
  for (auto &MI:getCopies()) {
    assert(MI->getOpcode() == TargetOpcode::COPY);
    const MachineOperand &MO = MI->getOperand(0);
    assert(MO.isReg() && MO.isDef());
    llvm::Register Reg = MO.getReg();
    assert(Reg.isVirtual());

    SwplReg *swpl_reg;
    auto itr = rmap.find(Reg);
    if (itr == rmap.end()) {
      SwplInst *d_inst = new SwplInst(nullptr, nullptr);
      swpl_reg = new SwplReg(Reg, *(d_inst), 0, false);

      d_inst->addDefRegs(swpl_reg);
      addPreviousInsts(d_inst);
    } else {
      swpl_reg = itr->second;
    }
    SwplInst *phi = new SwplInst(nullptr, this);
    swpl_reg->addUseInsts(phi);
    phi->addUseRegs(swpl_reg);

    SwplReg *swpl_defreg = new SwplReg(Reg, *phi, 0, false);

    phi->addDefRegs(swpl_defreg);
    rmap[Reg] = swpl_defreg;
    addPhiInsts(phi);
  }
}


/// ループボディ内の命令一覧 SwplLoop::BodyInsts を作成する。
void SwplLoop::makeBodyInsts(Register2SwplRegMap &rmap) {
  MachineInstr*branch=nullptr;
  MachineInstr*cmp=nullptr;
  MachineInstr*addsub=nullptr;
  MachineBasicBlock *mbb=getML()->getTopBlock();
  int ix = 0;
  SWPipeliner::TII->findMIsForLoop(*mbb, &branch, &cmp, &addsub);
  for (auto &MI:getNewBodyMBB()->instrs()) {
    if (MI.isDebugInstr()) { continue; }
    if (&MI == branch) { continue; }
    if (&MI == cmp && &MI != addsub) { continue; }
    if (MI.isBranch()) { continue; }
    SwplInst *inst = new SwplInst(&MI, &*this);
    inst->InitializeWithDefUse(&MI, &*this, rmap, getPreviousInsts(), &(getMems()), &(getMemsOtherBody()));
    inst->inst_ix = ix;
    ix++;
    addBodyInsts(inst);
  }
}

/// 次のイテレーションで使われるレジスタの情報をPhi命令のuse情報として追加する。
void SwplLoop::reconstructPhi(Register2SwplRegMap &rmap) {
  for (auto *phi:getPhiInsts()) {
    for (auto *swpl_reg:phi->getDefRegs()) {
      llvm::Register Reg = swpl_reg->getReg();
      auto itr = rmap.find(Reg);
      SwplReg *tmp_swpl_reg = itr->second;
      tmp_swpl_reg->addUseInsts(phi);
      phi->addUseRegs(tmp_swpl_reg);
    }
  }
}

void SwplInst::InitializeWithDefUse(llvm::MachineInstr *MI, SwplLoop *loop, Register2SwplRegMap &rmap,
                                    SwplInsts &insts, SwplMems *mems, SwplMems *memsOtherBody) {
  if (MI == nullptr) {
    llvm_unreachable("InitializeWithDefUse: not MI");
  }
  assert(MI->getNumDefs()<10);
  llvm::MachineOperand *DefMO[10];
  int def_i = 0;
  int use_i = 0;
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    if (!MI->getOperand(i).isReg()) { continue; }
    if (MI->getOperand(i).isDef()) {
      DefOpMap[def_i]=i+1;
      // Defの設定
      DefMO[def_i] = &(MI->getOperand(i));
      def_i++;
    } else {
      if (SWPipeliner::TII->isFPCR(MI->getOperand(i).getReg())) { continue; }
      UseOpMap[use_i]=i+1;
      use_i++;
      construct_use(rmap, *this, MI->getOperand(i), insts);
    }
  }

  if ((MI->mayLoad() || MI->mayStore() || MI->mayLoadOrStore()) && MI->memoperands_empty()) {
    // load/store命令でMI->memoperands_empty()の場合、MayAliasとして動作する必要がある
    if (SWPipeliner::isDebugDdgOutput()) {
      dbgs() << "DBG(SwplInst::InitializeWithDefUse): memoperands_empty mi=" << *MI;
    }
    construct_mem_use(rmap, *this, nullptr, insts, mems, memsOtherBody);
  }

  for (MachineMemOperand *MMO : MI->memoperands()) {
    construct_mem_use(rmap, *this, MMO, insts, mems, memsOtherBody);
  }
  for (int i=0; i<def_i; ++i) {
    construct_def(rmap, *this, *DefMO[i]);
  }
}

bool SwplInst::isRecurrence() const {
  // 複数回定義される場合はfalseを返す
  if (getSizeDefRegs() != 1) { return false; }

  SwplReg def_reg = getDefRegs(0);
  SwplInst *phi = nullptr; 
  for (auto *inst:def_reg.getUseInsts()) {
    if (inst->isPhi()) {
      phi = inst;
    }
  }

  if (phi != nullptr) {
    SwplReg phi_def_reg = phi->getDefRegs(0);
    for (auto *inst:phi_def_reg.getUseInsts()) {
      if (inst == this) {
        return true;
      }
    }
  }  
  return false;
}


size_t SwplLoop::getSizeBodyRealInsts() const {
  size_t n = 0;
  for (auto *inst:getBodyInsts()) {
    const llvm::MachineInstr *MI = inst->getMI();
    if (SWPipeliner::STM->isPseudo(*MI)) { continue; }
    n++;
  }
  return n;
}

/// メモリとアドレス増分値のマップ SwplLoop::MemIncrementMap を作成する。
void SwplLoop::makeMemIncrementMap() {
  for (auto *mem:getMems()) {
    int increment = mem->calcEachMemAddressIncrement();
    addMemIncrementMap(mem, increment);
  }
}

bool SwplLoop::areBodyInstsOrder(SwplInst *first, SwplInst *second) {
  int state=0;
  for ( auto*inst:getBodyInsts()) {
    if (state==0) {
      if (inst==first) state=1;
    } else {
      if (inst==second) return true;
    }
  }
  return false;
}

bool SwplLoop::areMemsOrder(SwplMem *first, SwplMem *second) {
  assert(first != second);
  int state=0;
  for ( auto*mem:getMems()) {
    if (state==0) {
      if (mem==first) state=1;
    } else {
      if (mem==second) return true;
    }
  }
  return false;
}

/// メモリアクセス間の差を計算する
/// \param [in] former_mi MachineInstr
/// \param [in] latter_mi MachineInstr
/// \return 差
static int mems_initial_diff(const MachineInstr* former_mi, const MachineInstr *latter_mi) {
  const MachineOperand *formerBaseOp;
  int64_t formerOffset;
  bool formerOffsetIsScalable;
  if (!SWPipeliner::TII->getMemOperandWithOffset(*former_mi, formerBaseOp, formerOffset, formerOffsetIsScalable, SWPipeliner::TRI))
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (formerOffsetIsScalable)
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (!formerBaseOp->isReg())
    return SwplDdg::UNKNOWN_MEM_DIFF;

  const MachineOperand *latterBaseOp;
  int64_t latterOffset;
  bool latterOffsetIsScalable;
  if (!SWPipeliner::TII->getMemOperandWithOffset(*latter_mi, latterBaseOp, latterOffset, latterOffsetIsScalable, SWPipeliner::TRI))
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (latterOffsetIsScalable)
    return SwplDdg::UNKNOWN_MEM_DIFF;
  if (!latterBaseOp->isReg())
    return SwplDdg::UNKNOWN_MEM_DIFF;

  if (formerBaseOp->getReg() != latterBaseOp->getReg())
    return SwplDdg::UNKNOWN_MEM_DIFF;
  int64_t offset=formerOffset-latterOffset;
  int64_t absoffset=labs(offset);
  return (absoffset > INT_MAX)? SwplDdg::UNKNOWN_MEM_DIFF:offset;
}

/// former_memとＮ回後のlatter_memが重なる可能性の否定できない最小のＮを返す。 \n
/// ただし、Ｎは、former_mem latter_mem の順に並んでいる時は０以上、逆の時は１以上とする。
unsigned SwplLoop::getMemsMinOverlapDistance(SwplMem *former_mem, SwplMem *latter_mem) {
  unsigned smallest_distance;
  int former_increment, latter_increment;

  const auto * former_mi=NewMI2OrgMI.at(former_mem->getInst()->getMI());
  const auto * latter_mi=NewMI2OrgMI.at(latter_mem->getInst()->getMI());
  const auto *memop1=former_mem->getMO();
  const auto *memop2=latter_mem->getMO();
  if (NoDep) {
    if (SWPipeliner::isDebugDdgOutput())
      dbgs() << "DBG(getMemIncrement): NoDep is true: return MAX_LOOP_DISTANCE\n"
             << " formaer_mi:" << *former_mi
             << " latter_mi:" << *latter_mi;
    return SwplMem::MAX_LOOP_DISTANCE;
  }

  if (!NoUseAA && !former_mi->mayAlias(SWPipeliner::AA, *latter_mi, false)) {
    // MachineInstr::mayAlias の結果は信頼性が低いため、さらに、メモリ間で重なる可能性があるか確認する
    if (memop1!=nullptr && memop2!=nullptr && memop1->getValue()!=nullptr && memop2->getValue()!=nullptr) {
      if (SWPipeliner::AA->isNoAlias(
              MemoryLocation::getAfter(memop1->getValue(), memop1->getAAInfo()),
              MemoryLocation::getAfter(memop2->getValue(), memop2->getAAInfo()))) {
         if (SWPipeliner::isDebugDdgOutput()) { dbgs() << " aliasAnalysis is NoAlias\n"; }
         return SwplMem::MAX_LOOP_DISTANCE;
      }
    }
  }

  // 最小の値を取得
  smallest_distance = (areMemsOrder(former_mem, latter_mem) ? 0 : 1);

  former_increment = getMemIncrement(former_mem);
  latter_increment = getMemIncrement(latter_mem);
  if (SWPipeliner::isDebugDdgOutput())
    dbgs() << "DBG(getMemIncrement): former_increment=" << former_increment << " , latter_increment=" << latter_increment << "\n";

  if (former_increment == SwplDdg::UNKNOWN_MEM_DIFF
      || latter_increment == SwplDdg::UNKNOWN_MEM_DIFF
      || former_increment != latter_increment) {
    return smallest_distance;
  }
  assert(former_increment == latter_increment);

  /* 最初にループに入った時のアドレスの差を求める。
   *  i = b;          ─┐この差のこと.
   *  j = b + 4;      ─┘
   * ┌→
   * │
   * │...  = a[i] op ...
   * │
   * │...  = a[j] op ...
   * │
   * │i = i + 1
   * │j = j + 1
   * └─
   */

  int distance;
  int initial_diff = mems_initial_diff (former_mi, latter_mi);
  if (SWPipeliner::isDebugDdgOutput())
    dbgs() << "DBG(mems_initial_diff): initial_diff=" << initial_diff
     << ", former_size=" << former_mem->getSize()
     << ", latter_size=" << latter_mem->getSize() << "\n";

  if (initial_diff == SwplDdg::UNKNOWN_MEM_DIFF) {
    return smallest_distance;
  }

  /* アクセスするメモリサイズを取得し、
   */
  int former_size = former_mem->getSize();
  int latter_size = latter_mem->getSize();

  /* 両者のアドレス増分が0の場合は、初期アドレスの差とアクセスサイズを考慮して
   * 重ならなければ、何回転先の命令であっても重ならない */
  if (initial_diff != 0 && latter_increment == 0) {
    if (!((initial_diff + (int) former_size > 0) && (initial_diff - (int) latter_size < 0))) {
      return SwplMem::MAX_LOOP_DISTANCE;
    }
  }
  /* 初期アドレスの差を考慮して、何回転先に重なるかを計算する */
  for (distance = smallest_distance; distance < SwplMem::MAX_LOOP_DISTANCE; ++distance) {
    int diff;

    /* 重なるのは、
       (former_address + former_size > latter_address)
       && (former_address < latter_address + latter_size)

       former_address := former_initial_address
       latter_address := latter_initial_address + disntace * latter_increment
       initial_diff := former_initial_address - latter_initial_address
       diff := former_address - latter_address
     */

    diff = initial_diff - distance * latter_increment;
    if ((diff + (int) former_size > 0) && (diff - (int) latter_size < 0)) {
      return distance;
    }
  }
  return distance;
}

/// スケジューリング結果反映機能のMVE処理やKERNEL分離処理においてPhiを特別視したいため、
/// SwplDdg を作成後に SwplLoop::BodyInsts に含まれるPhi命令を SwplLoop::PhiInsts に収集し直し、
/// SwplLoop::BodyInsts からは削除する。\n
void SwplLoop::recollectPhiInsts() {
  auto S = bodyinsts_begin();
  auto E = bodyinsts_end();

  auto itr = S;
  while (itr != E) {
    SwplInst *inst = *itr;
    if (inst->isPhi()) {
      addPhiInsts(inst);
      itr = eraseBodyInsts(itr);
    } else {
      itr++;
    }
  }
}

static MachineOperand* used_reg(MachineInstr &phi) {
  Register def_r = phi.getOperand(0).getReg();
  unsigned opid;
  if (phi.getOperand(2).getMBB()==phi.getParent()) {
    opid=1;
  } else {
    opid=3;
  }
  if (phi.getOperand(opid).getSubReg()) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): own-operand has subreg!:" << phi;
    }
    return nullptr;
  }
  auto own_r = phi.getOperand(opid).getReg();

  if (!SWPipeliner::STM->isEnableRegAlloc() && !IgnoreRegClass_SuppressCopy) {
    if (def_r.isVirtual() && own_r.isVirtual()) {
      auto def_r_class = SWPipeliner::MRI->getRegClass(def_r);
      auto own_r_class = SWPipeliner::MRI->getRegClass(own_r);
      if (def_r_class->getID() != own_r_class->getID()) {
         if (DebugPrepare) {
           dbgs() << "DEBUG(used_reg): def-class is not use-class!:" << phi;
         }
         return nullptr;
      }
    } else {
      if (DebugPrepare) {
         dbgs() << "DEBUG(used_reg): def-class is not use-class!:" << phi;
      }
      return nullptr;
    }
  }

  auto *def_op = SWPipeliner::MRI->getOneDef(own_r);

  if (def_op==nullptr) {
    // 物理レジスタの場合:copy必須
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): MRI->getOneDef(own_r) is nullptr\n";
    }
    return nullptr;
  }

  if (def_op->getParent()->getParent()!=phi.getParent()) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): own_r is livein!\n";
    }
    return nullptr;

  }

  auto *defMI = def_op->getParent();
  unsigned use_tied_ix;
  bool t = defMI->isRegTiedToUseOperand(defMI->getOperandNo(def_op), &use_tied_ix);
  if (t && defMI->getOperand(use_tied_ix).getReg() == def_r) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(used_reg): tied-def!\n";
    }
    return nullptr;
  }

  MachineInstr *start_instr = def_op->getParent();
  for (auto *I=start_instr->getNextNode();I;I=I->getNextNode()) {
    for ( auto &o:I->operands()) {
      if (!o.isReg()) continue;
      if (o.isDef()) continue;
      auto r=o.getReg();
      if (r.id()==own_r.id()) {
         if (DebugPrepare) {
           dbgs() << "DEBUG(used_reg): own-reg is used!\n";
         }
         return nullptr;
      }
      if (r.id()==def_r.id()) {
         if (DebugPrepare) {
           dbgs() << "DEBUG(used_reg): def-reg is used!\n";
         }
         return nullptr;
      }
    }
  }

  return def_op;
}

void SwplLoop::convertNonSSA(llvm::MachineBasicBlock *body, llvm::MachineBasicBlock *pre, const DebugLoc &dbgloc,
                             llvm::MachineBasicBlock *org, const SwplScr::UseMap &LiveOutReg) {
  std::vector<MachineInstr*> phis;

  DenseMap<MachineInstr*, Register> flow;
  DenseMap<MachineInstr*, MachineOperand*> uses;
  for (auto &phi:body->phis()) {
    phis.push_back(&phi);
    if (DisableSuppressCopy)
      uses[&phi]=nullptr;
    else
      uses[&phi]=used_reg(phi);
  }
  /// (1). Phi命令を検索し、Phi命令の単位に以下の処理を行う。
  for (auto *phi:phis) {
    unsigned opix;
    unsigned num=phi->getNumOperands();
    llvm::Register def_r=0, Reg=0, own_r=0, in_r=0;
    unsigned own_subreg=0;

    ///   (1)-1. Phiの参照レジスタのうち、自身のMBBで定義されるレジスタ(own_r)とLiveInのレジスタ(in_r)をおさえておく。
    int own_opix=0;
    def_r = phi->getOperand(0).getReg();
    for (opix=1; opix<num; opix++) {
      Reg = phi->getOperand(opix).getReg();
      opix++;
      assert(phi->getOperand(opix).isMBB());
      auto *mbb=phi->getOperand(opix).getMBB();
      if (mbb==org) {
        assert(own_r == 0);
        own_r = Reg;
        own_subreg = phi->getOperand(opix-1).getSubReg();
        own_opix = opix-1;
      } else {
        assert(in_r == 0);
        in_r = Reg;
      }
    }

    // def_rは後続するown_rで参照されるか調査する
    for (auto *t=phi->getNextNode();t;t=t->getNextNode()) {
      if (!t->isPHI()) break;
      Register t_own_r;
      if (t->getOperand(2).getMBB()==org) {
        t_own_r = t->getOperand(1).getReg();
      } else {
        t_own_r = t->getOperand(3).getReg();
      }
      if (def_r==t_own_r) {
        llvm::Register newReg = SWPipeliner::MRI->cloneVirtualRegister(def_r);
        MachineInstr *c=BuildMI(*body, body->getFirstTerminator(), dbgloc,
                                  SWPipeliner::TII->get(TargetOpcode::COPY), newReg)
                              .addReg(def_r /* def_rはsubreg指定は無い */);
        flow[t]=newReg;
        if (DebugPrepare) {
           dbgs() << "DEBUG(convertNonSSA): def_r is referenced by subsequent own_r:" << *c;
        }
        break;
      }
    }

    ///   (1)-2. Phiの定義を参照する命令を検索し2.の参照レジスタに置きかえる。 \n
    ///      defregはliveoutしているかを確認する。liveoutしていれば、PHIから生成されるCOPYの定義レジスタは新規を利用。
    const auto *org_phi=NewMI2OrgMI[phi];
    auto org_def_r=org_phi->getOperand(0).getReg();
    auto org_own_r=org_phi->getOperand(own_opix).getReg();
    bool liveout_def = LiveOutReg.count(org_def_r)!=0;
    bool liveout_own = LiveOutReg.count(org_own_r)!=0;

    if (liveout_def) {
      auto backup_def_r=def_r;
      def_r=SWPipeliner::MRI->cloneVirtualRegister(def_r);
      MachineInstr *Copy = BuildMI(*body, body->getFirstNonPHI(), dbgloc,SWPipeliner::TII->get(TargetOpcode::COPY), backup_def_r)
              .addReg(def_r);
      NewMI2OrgMI[Copy]=org_phi;
    }
    for (auto &x:OrgReg2NewReg) {
      Register r = x.second;
      if (r.id() == own_r.id()) {
        if (LiveOutReg.count(x.first)) {
           liveout_own = true;

        }
      }
    }


    ///      PHIを元にCOPY命令を生成する際、COPYの定義レジスタクラスはPHIの定義レジスタクラスであるべき。

    ///      phiを２つのCOPY命令に変換し、predecessorとbodyの下方に挿入する。
    ///````
    ///   body :
    ///          phi_def = PHI own_r, in_r
    ///          inst
    ///          inst ...
    ///   ↓
    ///   predecessor of body :
    ///          inst
    ///          inst ...
    ///      --> phi_def = COPY in_r
    ///   body :
    ///          inst
    ///          inst ...
    ///      --> phi_def = COPY own_r
    ///````

    ///   (1)-3. preにin_rからown_rへのCopy命令を挿入する(def_r = Copy in_r)。
    MachineInstr *Copy = BuildMI(*pre, pre->getFirstTerminator(), dbgloc,SWPipeliner::TII->get(TargetOpcode::COPY), def_r)
                                .addReg(in_r);
    addCopies(Copy);

    ///   (1)-4. preにin_rからown_rへのCopy命令を挿入する(def_r = Copy own_r)。
    /// own_r/def_rの参照がない場合はCOPY生成不要
    auto *def_op = uses[phi];
    if (liveout_def || liveout_own || def_op==nullptr) {
      if (flow.count(phi)) {
        if (DebugPrepare) {
           dbgs() << "DEBUG(convertNonSSA): change own_r " << printReg(own_r, SWPipeliner::TRI)
                  << " to " << printReg(flow[phi], SWPipeliner::TRI) << "\n";
        }
        own_r = flow[phi];
      }

      MachineInstr *c=BuildMI(*body, body->getFirstTerminator(), dbgloc,
                                SWPipeliner::TII->get(TargetOpcode::COPY), def_r)
                            .addReg(own_r,0, own_subreg);
      NewMI2OrgMI[c]=org_phi;
      if (DebugPrepare) {
        if (def_op && liveout_def)
          dbgs() << "DEBUG(convertNonSSA): Generate copy: def-reg is liveout!\n";
        else if (def_op && liveout_own)
          dbgs() << "DEBUG(convertNonSSA): Generate copy: own-reg is liveout!\n";
      }
    } else {
      if (DebugPrepare) {
        dbgs() << "DEBUG(convertNonSSA): Suppress the generation of COPY: " << *phi;
      }
      def_op->setReg(def_r);
    }

    ///          OrgMI2NewMIがorgのphiとnewのphiとなっているので、new側をCopy命令に変更する。
    for (auto &itr: getOrgMI2NewMI()) {
      if (itr.second == phi) {
        addOrgMI2NewMI(itr.first, const_cast<llvm::MachineInstr *>(Copy));
      }
    }  
  }

  /// (2). Phi命令を削除する。
  for (auto *phi:phis) {
    phi->eraseFromParent();
  }
}

/// orgの定義レジスタを収集しレジスタを複写する。
/// \note この時点ではまだrenamingせずにorg2newへの登録のみをおこなう。
/// \param[in]  &orgMI コピー元のMI
/// \param[inout]  org2new 定義するレジスタをKeyに複製したレジスタをValueに持つMap.
static void collectDefReg(llvm::MachineInstr &orgMI, llvm::DenseMap<Register, Register> &org2new) {
  for (unsigned i = 0, e = orgMI.getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = orgMI.getOperand(i);
    if (!(MO.isReg())) { continue; }
    llvm::Register orgReg = MO.getReg();
    if (!(orgReg.isVirtual())) { continue; }
    if (MO.isDef()) {
      llvm::Register newReg = SWPipeliner::MRI->cloneVirtualRegister(orgReg);
      // 多重定義はないはず。
      assert(org2new.find(orgReg) == org2new.end());
      org2new[orgReg] = newReg;
    }
  }
}

void SwplLoop::renameReg() {
  for (auto &m_itr: getOrgMI2NewMI()) {
    const MachineInstr *orgMI = m_itr.first;
    MachineInstr *newMI = m_itr.second;
    for (unsigned i = 0, e = orgMI->getNumOperands(); i != e; ++i) {
      const MachineOperand &MO = orgMI->getOperand(i);
      if (!(MO.isReg())) { continue; }
      llvm::Register orgReg = MO.getReg();
      auto r_itr = getOrgReg2NewReg().find(orgReg);
      if (r_itr != getOrgReg2NewReg().end()) {
        newMI->getOperand(i).setReg(r_itr->second);
      }
    }
  }
}

void SwplLoop::cloneBody(llvm::MachineBasicBlock*newBody, llvm::MachineBasicBlock*oldBody) {
  auto S=oldBody->begin();
  auto E=oldBody->end();

  for (auto J=S; J!=E;) {
    MachineInstr *org=&*J++;
    const MachineInstr *mi = const_cast<MachineInstr *>(org);
    // orgMIをベースにnewMIをduplicate
    MachineInstr &newMI = SWPipeliner::TII->duplicate(*newBody, newBody->getFirstTerminator(), *mi);
    // レジスタリネーミング向けに定義レジスタを複製し、orgとnewのレジスタをmapにため込む
    collectDefReg(*org, getOrgReg2NewReg());
    addOrgMI2NewMI(mi, &newMI);
  }
  // レジスタを変換する
  renameReg();
}

bool SwplLoop::convertSSAtoNonSSA(MachineLoop &L, const SwplScr::UseMap &LiveOutReg) {
  MachineBasicBlock *ob=L.getTopBlock();
  const BasicBlock *ob_bb=ob->getBasicBlock();
  MachineFunction *MF=ob->getParent();
  const auto &dbgloc=L.getTopBlock()->begin()->getDebugLoc();
  
  /// MachineBasicBlockの追加
  MachineBasicBlock *new_bb=MF->CreateMachineBasicBlock(ob_bb);
  setNewBodyMBB(new_bb);
  /// MBBをMFの連鎖の最後に挿入する。制御フローは未設定。
  auto itr = MF->end();
  MF->insert(itr, new_bb);

  /// 非SSA用の直前のMachineBasicBlockを用意。
  MachineBasicBlock *pre=MF->CreateMachineBasicBlock(ob_bb);
  /// new_bbの直前に挿入する。制御フローは未設定。
  MF->insert(new_bb->getIterator(), pre);

  pre->addSuccessor(new_bb);
  
  /// cloneBody() を呼び出し、MachineBasickBlockの複製とレジスタリネーミングを行う。(OrgReg2NewRegを生成する)
  cloneBody(new_bb, ob);

  auto regmap=getOrgReg2NewReg();
  for (auto &p:LiveOutReg) {
    // p.first: reg, p.second:vector<MO*>
    auto newreg=regmap[p.first];
    liveOuts.insert(newreg);
  }

  if (!DisableRmCopy)
    removeCopy(new_bb);
  /// convertPostPreIndeTo()を呼び出し、post/pre index命令を演算＋load/store命令に変換する
  if (!DisableConvPrePost)
    convertPrePostIndexInstr(new_bb);

  if (SWPipeliner::STM->isEnableRegAlloc()) {
    if (checkRestrictions(new_bb)) return true;
  }

  /// convertNonSSA() を呼び出し、非SSA化と複製したMachineBasicBlockの回収を行う。
  /// PHIからCOPYを生成する際、liveOutsにPHIで定義されるvregを追加している。
  convertNonSSA(new_bb, pre, dbgloc, ob, LiveOutReg);

  if (SWPipeliner::STM->isEnableRegAlloc() || EnableNormalizeTieddef)
    // レジスタ割付が動作するなら、tied-defにCOPY命令を追加する
    normalizeTiedDef(new_bb);

  return false;
}

bool SwplLoop::check_need_copy4TiedUseReg(const MachineBasicBlock* body, const MachineInstr* tiedInstr,
                                      Register tiedDefReg, Register tiedUseReg) const {

  // 1) tiedInstrの定義参照が同じレジスタの場合のみ不要

  if (tiedDefReg == tiedUseReg) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(SwplLoop::check_need_copy4TiedUseReg): tiedUseReg("
             << printReg(tiedUseReg, SWPipeliner::TRI) << ") eq tiedDef:" << *tiedInstr;
    }
    return false;
  }
  if (tiedInstr->getOpcode() == TargetOpcode::INSERT_SUBREG || tiedInstr->getOpcode() == TargetOpcode::EXTRACT_SUBREG ||
      tiedInstr->getOpcode() == TargetOpcode::EXTRACT_SUBREG || tiedInstr->getOpcode() == TargetOpcode::SUBREG_TO_REG) {
    if (DebugPrepare) {
      dbgs() << "DEBUG(SwplLoop::check_need_copy4TiedUseReg): Instruction to exclude allocation of physical registers\n"
                "  mi: " << *tiedInstr;
    }
    return false;
  }
  if (DebugPrepare) {
    dbgs() << "DEBUG(SwplLoop::check_need_copy4TiedUseReg): Decide that the COPY instruction is necessary\n";
  }
  return true;
}

void SwplLoop::normalizeTiedDef(MachineBasicBlock *body) {

  // tied-def operands that require COPY
  SmallVector<std::tuple<MachineInstr*, MachineOperand*>, 20> targets;

  for (auto &MI:*body) {
    for (auto &MO:MI.defs()) {
      if (!MO.isReg()) continue;
      if (!MO.isTied()) continue;
      unsigned use_tied_index=0;
      bool t = MI.isRegTiedToUseOperand(MI.getOperandNo(&MO), &use_tied_index);
      assert(t && "isRegTiedToUseOperand");
      auto &useMO = MI.getOperand(use_tied_index);
      auto use_tied_r = useMO.getReg();

      if (check_need_copy4TiedUseReg(body, &MI, MO.getReg(), use_tied_r)) {
        targets.push_back({&MI, &useMO});
        if (DebugPrepare) {
          dbgs() << "DEBUG(SwplLoop::normalizeTiedDef): Copy is required.: " << MI;
        }
      } else {
        if (DebugPrepare) {
          dbgs() << "DEBUG(SwplLoop::normalizeTiedDef): Copy is not required.: " << MI;
        }
      }
    }
  }

  for (auto &p:targets) {
    auto *mi  = std::get<0>(p);
    auto *tied_mo = std::get<1>(p);
    auto tied_reg = tied_mo->getReg();
    auto new_reg = SWPipeliner::MRI->cloneVirtualRegister(tied_reg);
    auto dbgloc = mi->getDebugLoc();
    MachineInstr *Copy = BuildMI(*body, mi, dbgloc, SWPipeliner::TII->get(TargetOpcode::COPY), new_reg)
                             .addReg(tied_reg);
    tied_mo->setReg(new_reg);
    if (DebugPrepare) {
      dbgs() << "DEBUG(SwplLoop::normalizeTiedDef):\n  generate: " << *Copy;
      dbgs() << "  chenge op: " << *mi;
    }
  }
}

void SwplLoop::removeCopy(MachineBasicBlock *body) {
  std::vector<llvm::MachineInstr *> delete_mi;
  std::vector<llvm::MachineOperand *> target_mo;
  for (auto &mi:*body) {
    if (!mi.isCopy()) continue;
    if (DebugPrepare) {
      dbgs() << "DBG(SwplLoop::removeCopy)\n";
      dbgs() << " target mi: " << mi;
    }

    auto &op0=mi.getOperand(0);
    const auto *org_copy=NewMI2OrgMI.at(&mi);
    if (liveOuts.contains(op0.getReg())) {
      if (DebugPrepare) {
        dbgs() << " org mi: " << org_copy;
        dbgs() << " op0 is liveout!\n";
      }
      continue;
    }
    if (!mi.getOperand(1).isReg()) {
      if (DebugPrepare) {
        dbgs() << " op1 isnot reg!\n";
      }
      continue;
    }
    Register r0 = op0.getReg();
    if (r0.isPhysical()) {
      if (DebugPrepare) {
        dbgs() << " op0 isnot virtual-reg!\n";
      }
      continue;
    }
    target_mo.clear();
    bool hasSubreg = mi.getOperand(1).getSubReg() != 0;
    bool foundSubreg = false;
    bool foundTied = false;
    for (auto &u:SWPipeliner::MRI->use_operands(r0)) {
      if (hasSubreg && u.isTied()) {
        foundTied = true;
        break;
      }
      if (hasSubreg && u.getSubReg() != 0) {
        foundSubreg = true;
        break;
      }
      target_mo.push_back(&u);
    }
    if (foundTied) {
      if (DebugPrepare) {
        dbgs() << " op1 has subreg && use operand is tied!\n";
      }
      continue;
    }
    if (foundSubreg) {
      if (DebugPrepare) {
        dbgs() << " op1 has subreg!\n";
      }
      continue;
    }
    bool use_onlyphi=true;
    for (auto *op:target_mo) {
      auto *umi = op->getParent();
      if (!umi->isPHI()) use_onlyphi=false;
    }

    if (use_onlyphi ||
        SWPipeliner::TII->canRemoveCopy(*body, mi, *SWPipeliner::MRI,
                                        (SWPipeliner::STM->isEnableRegAlloc() || IgnoreRegClass_RmCopy))){

      // op1定義からop0定義までに間にop0の参照がある場合はCOPYを削除できない
      Register r1 = mi.getOperand(1).getReg();
      bool use_op0=false;
      auto *def_use = SWPipeliner::MRI->getOneDef(r1);
      if (def_use!=nullptr) {
        for (MachineInstr *t = def_use->getParent()->getNextNode(); t!=nullptr && !use_op0; t=t->getNextNode()) {
          if (t==&mi) break;
          for (auto &MO:t->uses()) {
            if (MO.isReg() && MO.getReg()==r0) {
              use_op0 = true;
              break;
            }
          }
        }
        if (use_op0) {
          if (DebugPrepare) {
            dbgs() << " op0 is referenced from the definition of op1 to the definition of op0.!\n";
          }
          continue;
        }
      }
      for (auto *op:target_mo) {
        auto *umi=op->getParent();
        if (DebugPrepare) {
          dbgs() << " before: " << *umi;
        }
        // r0を参照しているオペランドを書き換える
        auto &op1 = mi.getOperand(1);
        op->setReg(op1.getReg());
        if (hasSubreg) {
          op->setSubReg(op1.getSubReg());
        }
        op->setIsKill(false);
        if (DebugPrepare) {
          dbgs() << " after: " << *umi;
        }
      }

      // クローン前命令とクローン後命令の再紐づけ
      auto *org = NewMI2OrgMI.at(&mi);
      NewMI2OrgMI.erase(&mi);
      OrgMI2NewMI[org]=nullptr;
      if (DebugPrepare) {
        dbgs() << " removed!\n";
      }
      // MBBから削除する命令の収集
      delete_mi.push_back(&mi);
    } else {
      if (DebugPrepare) {
        dbgs() << " canRemoveCopy() is false!\n";
      }
    }
  }
  for (auto *mi:delete_mi)
    mi->eraseFromParent();

}

void SwplLoop::convertPrePostIndexInstr(MachineBasicBlock *body) {
  std::vector<llvm::MachineInstr *> delete_mi;
  for (auto &mi:*body) {
    MachineInstr *ldst=nullptr;
    MachineInstr *add=nullptr;
    if (SWPipeliner::TII->splitPrePostIndexInstr(*body, mi, &ldst, &add)){
      // クローン前命令とクローン後命令の再紐づけ
      auto *org = NewMI2OrgMI.at(&mi);
      NewMI2OrgMI.erase(&mi);
      OrgMI2NewMI[org] = ldst;
      NewMI2OrgMI[ldst] = org;
      NewMI2OrgMI[add] = org;
      if (DebugPrepare) {
        dbgs() << "DBG(SwplLoop::convertPrePostIndexInstr)\n";
        dbgs() << " before:" << mi;
        dbgs() << " after 1:" << *ldst;
        dbgs() << " after 2:" << *add;
      }

      // MBBから削除する命令の収集
      delete_mi.push_back(&mi);
    }
  }
  for (auto *mi:delete_mi)
    mi->eraseFromParent();

}

bool SwplLoop::checkRestrictions(const MachineBasicBlock *body) {
  if (SWPipeliner::isDisableRestrictionsCheck(
          SWPipeliner::SwplRestrictionsFlag::All)) return false;
  for (auto &mi:*body) {
    if (!SWPipeliner::isDisableRestrictionsCheck(
            SWPipeliner::SwplRestrictionsFlag::MultipleDef)) {
      int num=0;
      for (auto &def:mi.defs()) {
        if (def.isImplicit()) continue;
        num++;
      }
      if (num > 1) {
        SWPipeliner::makeMissedMessage_RestrictionsDetected(mi);
        return true;
      }
    }
    if (!SWPipeliner::isDisableRestrictionsCheck(
            SWPipeliner::SwplRestrictionsFlag::MultipleReg)) {
      for (auto &mo:mi.operands()) {
        if (mo.isReg()) {
          auto r = mo.getReg();
          if (r.isPhysical()) continue;
          auto r_class = SWPipeliner::MRI->getRegClass(r);
          if (SWPipeliner::TII->isMultipleReg(r_class)) {
            SWPipeliner::makeMissedMessage_RestrictionsDetected(mi);
            return true;
          }
        }
      }
    }
  }
  return false;
}

void SwplInst::pushAllRegs(SwplLoop *loop) {
  for (auto *reg:getUseRegs()) {
    loop->addRegs(reg);
  }
  for (auto *reg:getDefRegs()) {
    loop->addRegs(reg);
  }
}

/// 全ての SwplInst を走査し SwplInst::DefRegs / SwplInst::UseRegs を走査し SwplLoop::Regs に登録する。
/// \note SwplLoop 生成時rmapを使用しているが、すべての SwplReg が登録されているわけではない。\n
///       そのため最後に SwplInst が使用する SwplReg を収集する。
void SwplLoop::collectRegs() {
  for (auto *inst:getPreviousInsts()) {
    inst->pushAllRegs(this);
  }
  for (auto *inst:getPhiInsts()) {
    inst->pushAllRegs(this);
  }
  for (auto *inst:getBodyInsts()) {
    inst->pushAllRegs(this);
  }
  // 同一要素の削除
  SwplRegs &regs = getRegs();
  std::sort(regs.begin(), regs.end());
  regs.erase(std::unique(regs.begin(), regs.end()), regs.end());
}

bool SwplLoop::containsLiveOutReg(Register vreg) {
  return liveOuts.contains(vreg);
}

void SwplLoop::deleteNewBodyMBB() {
  MachineBasicBlock *newMBB = getNewBodyMBB();
  if (newMBB == nullptr) { return; }
  MachineBasicBlock *preMBB = getNewBodyPreMBB(newMBB);

  preMBB->removeSuccessor(newMBB);
  preMBB->eraseFromParent();
  newMBB->eraseFromParent();

  setNewBodyMBB(nullptr);
}


int SwplMem::calcEachMemAddressIncrement() {
  int sum = 0;
  int old_sum = 0;
  if (SWPipeliner::isDebugDdgOutput()) {
    if (Inst->getMI()==nullptr)
      dbgs() << "DBG(calcEachMemAddressIncrement): \n";
    else
      dbgs() << "DBG(calcEachMemAddressIncrement): " << *(Inst->getMI());
  }
  for (auto *reg:getUseRegs()) {

    int increment = SWPipeliner::TII->calcEachRegIncrement(reg);
    if (SWPipeliner::isDebugDdgOutput()) {
      dbgs() << " calcEachRegIncrement(" << printReg(reg->getReg(), SWPipeliner::TRI)  << "): "
             << increment << "\n";
    }
    if (increment == SwplDdg::UNKNOWN_MEM_DIFF) {
      return increment;
    }
    old_sum = sum;
    sum += increment;

    // 計算結果の溢れチェック
    if (increment > 0) {
      if (sum < old_sum) {
        return SwplDdg::UNKNOWN_MEM_DIFF;
      }
    } else if (increment < 0) {
      if (sum > old_sum) {
        return SwplDdg::UNKNOWN_MEM_DIFF;
      }
    }
  }
  return sum;
}

void SwplLoop::print() {
  dbgs() << "DBG(SwplLoop::print) SwplLoop Print. \n";
  dbgs() << "DBG(SwplLoop::print) ------------------ NewPreBodyMBB. \n";
  dbgs() << *getNewBodyPreMBB(NewBodyMBB);
  dbgs() << "DBG(SwplLoop::print) ------------------ NewBodyMBB. \n";
  dbgs() << *NewBodyMBB;
  dbgs() << "\n";

  dbgs() << "DBG(SwplLoop::print) ------------------ PreviousInsts. \n";
  for ( auto *inst:getPreviousInsts()) {
    inst->print();
    dbgs() << "\n";
  }
  dbgs() << "DBG(SwplLoop::print) ------------------ SwplLoop PhiInsts. \n";
  for ( auto *inst:getPhiInsts()) {
    inst->print();
    dbgs() << "\n";
  }
  dbgs() << "DBG(SwplLoop::print) ------------------ SwplLoop BodyInsts. \n";
  for ( auto *inst:getBodyInsts()) {
    inst->print();
    dbgs() << "\n";
  }
}

void SwplInst::print() {
  dbgs() << "DBG(SwplInst::print) SwplInst. \n";
  if (getMI() != nullptr) {
    dbgs() << "  ### MI: " << *getMI();
  } else {
    dbgs() << "  ### MI: MI == null\n";
  }

  dbgs() << "  ### dump UseRegs \n";
  for ( auto *reg:getUseRegs()) {
    dbgs() << "    ";
    reg->print();
  }  
  dbgs() << "  ### dump DefRegs \n";
  for ( auto *reg:getDefRegs()) {
    dbgs() << "    ";
    reg->print();
  }  
}

void SwplLoop::dumpOrgMI2NewMI() {
  // @todo オリジナル命令がpost/pre命令の場合は、変換後のadd命令が出力されない
  // これは、MAPがオリジナル命令とクローンした命令が1対1を前提としているため
  for (auto &itr: getOrgMI2NewMI()) {
    dbgs() << "OldMI=" << *itr.first << "\n";
    dbgs() << "NewMI=" << *itr.second << "\n";
  }
}

void SwplLoop::dumpOrgReg2NewReg() {
  for (auto &itr: getOrgReg2NewReg()) {
    dbgs() << "OldReg=" << printReg(itr.first, SWPipeliner::TRI)
           << "NewReg=" << printReg(itr.second, SWPipeliner::TRI) << "\n";
  }
}

void SwplLoop::dumpCopies() {
  for (auto *itr: getCopies()) {
    dbgs() << "MI=" << *itr << "\n";
  }
}

void SwplLoop::printRegID(llvm::Register r) {
  dbgs() << "RegID=" << printReg(r, SWPipeliner::TRI) << "\n";
}

SwplReg::SwplReg(const SwplReg &s) {
  Reg = s.Reg;
  DefInst = s.DefInst;
  DefIndex = s.DefIndex;
  Predecessor = s.Predecessor;
  Successor = s.Successor;
  EarlyClobber=s.EarlyClobber;
  units = s.units;
  rk = SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, s.Reg);
}

SwplReg::SwplReg(Register r, SwplInst &i, size_t def_index, bool earlyclober) {
  Reg = r;
  DefInst = &i;
  DefIndex = def_index;
  Predecessor = nullptr;
  Successor = nullptr;
  EarlyClobber=earlyclober;
  rk = SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, r);
  units = rk->getUnitNum();

}

void SwplReg::print() {
  dbgs() << "DBG(SwplReg::print) SwplReg. : ";
  dbgs() << "SwplReg=" << this << ": RegID=" << printReg(getReg(), SWPipeliner::TRI) << "\n";
}

void SwplReg::printDefInst() {
  dbgs() << *(getDefInst()->getMI());
}

void SwplMem::printInst() {
  dbgs() << *(getInst()->getMI());
}

/// Immediate-Predecessor が一つである限り、次々たどって、スタックに入れる
/// \param [in] L MachineLoop
/// \param [in,out] BBs BasicBlocks
static void follow_single_predecessor_MBBs(MachineLoop *L, BasicBlocks *BBs) {
  MachineBasicBlock *LoopBB = L->getTopBlock();

  // 対象ループ判定でbodyのbasicBlockが一つであることをチェック済み
  assert(L->getBlocks().size() == 1);
  // predecessorは自分自身とPreheaderの２つ(Preheaderはない場合があるので)
  if (LoopBB->pred_size() != 2) {
    return;
  }

  MachineBasicBlock *pred_BB = nullptr;
  for (auto *MBB:LoopBB->predecessors()) {
    if (LoopBB == MBB) { continue; }
    pred_BB = MBB;
  }
  
  MachineBasicBlock *BB = pred_BB;
  while(BB) {
    for (auto &MI:BB->instrs()) {
      if (SWPipeliner::TII->isNonTargetMI4SWPL(MI)) { return; }
    }
    BBs->push_back(BB);
    BB = getPredecessorBB(*BB);
  }
  
  return;
}

/// PredecessorのBBを取得
/// \param [in] BB MachineBasicBlock
/// \return MachineBasicBlock
static MachineBasicBlock *getPredecessorBB(MachineBasicBlock &BB) {
  if (BB.pred_size() != 1) { return nullptr; }
  for (MachineBasicBlock *S : BB.predecessors())
    return S;

  llvm_unreachable("getPredecessorBB: not_reach_here");
  return nullptr;
}

/// オペランドのuse情報を設定する
/// \param [in,out] rmap llvm::Register と SwplReg のマップ
/// \param [in,out] inst SwplInst
/// \param [in] MO MachineOperand
/// \param [in,out] insts SwplInsts
static void construct_use(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO, SwplInsts &insts) {
  SwplReg *reg;

  auto itr = rmap.find(MO.getReg());
  if (itr != rmap.end()) {
    // SwplRegが構築済み
    reg = itr->second;
  } else {
    // SwplRegを生成しrmapとinstを登録する。
    SwplInst *d_inst = new SwplInst(nullptr, nullptr);
    llvm::Register r = MO.getReg();
    reg = new SwplReg(r, *(d_inst), 0, false);
    rmap[r] = reg;
    insts.push_back(d_inst);
  }

  reg->addUseInsts(&inst);
  inst.addUseRegs(reg);
}

/// MachineMemOperand から SwplMem を生成する
/// \param [in,out] rmap llvm::Register と SwplReg のマップ
/// \param [in,out] inst SwplInst
/// \param [in] MMO MachineMemOperand
/// \param [in,out] insts SwplInsts
/// \param [in,out] mem SwplMems
/// \param [in,out] memsOtherBody SwplMems
static void construct_mem_use(Register2SwplRegMap &rmap, SwplInst &inst, const MachineMemOperand *MMO, SwplInsts &insts,
                              SwplMems *mems, SwplMems *memsOtherBody) {
  SwplMem *mem = new SwplMem(MMO, inst, (MMO==nullptr)?0:MMO->getSize());

  const MachineOperand *BaseOp=nullptr;
  int64_t Offset=0;
  bool OffsetIsScalable=false;
  if (SWPipeliner::TII->getMemOperandWithOffset(*(inst.getMI()), BaseOp, Offset, OffsetIsScalable, SWPipeliner::TRI)
      && BaseOp->isReg()) {
    mem->addUseReg(BaseOp->getReg(), rmap);
  }


  // 登録はBodyInstのみ
  if (mems != nullptr) {
    mems->push_back(mem);
  } else {
    // メモリ解放用にmemsOtherBodyにmemを登録する。
    memsOtherBody->push_back(mem);
  }
}

/// オペランドのdef情報を設定する
/// \param [in,out] rmap llvm::Register と SwplReg のマップ
/// \param [in,out] inst SwplInst
/// \param [in] MO MachineOperand
static void construct_def(Register2SwplRegMap &rmap, SwplInst &inst, MachineOperand &MO) {
  size_t indx = inst.getSizeDefRegs();
  llvm::Register r = MO.getReg();
  SwplReg *reg = new SwplReg(r, inst, indx, MO.isEarlyClobber());
  inst.addDefRegs(reg);
  rmap[r] = reg;
}

void SwplLoop::destroy() {
  for (auto *inst:PreviousInsts) {
    delete inst;
  }
  for (auto *inst:PhiInsts) {
    delete inst;
  }
  for (auto *inst:BodyInsts) {
    delete inst;
  }
  for (auto *mem:Mems) {
    delete mem;
  }
  for (auto *mem:MemsOtherBody) {
    delete mem;
  }
  for (auto *reg:Regs) {
    delete reg;
  }
  deleteNewBodyMBB();
}
