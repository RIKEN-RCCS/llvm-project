//=- SwplDataDependenceAnalysis.cpp - SWPL DDG -*- c++ -*--------------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// 
///  Data Dependency analysis Generation (SwplDdg)
//
//===----------------------------------------------------------------------===//

#include "SWPipeliner.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"


using namespace llvm;

#define DEBUG_TYPE "swp-ddg"

static cl::opt<bool> DebugDdg("swpl-debug-ddg",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableInstDep("swpl-enable-instdep",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableCheckEarlyClobber("swpl-enable-check-early-clobber",cl::init(false), cl::ReallyHidden);

static void update_distance_and_delay(SwplDdg &ddg, SwplInst &former_inst, SwplInst &latter_inst, int distance, int delay);

SwplDdg *SwplDdg::Initialize (SwplLoop &loop) {
  SwplDdg *ddg = new SwplDdg(loop);

  /// 1. generateInstGraph() を呼び出し、命令の依存グラフ情報 SwplInstGraph を初期化する。
  ddg->generateInstGraph();
  /// 2. analysisRegDependence() を呼び出し、レジスタ間の依存情報を解析する。
  ddg->analysisRegDependence();
  /// 3. analysisMemDependence() を呼び出し、メモリ間の依存情報を解析する。
  ddg->analysisMemDependence();
  /// 4. analysisInstDependence() を呼び出し、命令依存情報を解析する。
  if (EnableInstDep)
    ddg->analysisInstDependence();

  /// 5. SwplLoop::recollectPhiInsts() を呼び出し、
  /// SwplDdg を生成後に SwplLoop::BodyInsts に含まれるPhiを PhiInsts に再収集する。
  loop.recollectPhiInsts();

  if (DebugDdg) {
    ddg->print();
  }  
  return ddg;
}

/// Phiの初期値(Register)、Loopで定義した値（Register）を取得する
/// \param [in] Phi
/// \param [in] Loop
/// \param [out] InitVal
/// \param [out] LoopVal
/// \note MachinePipelinerより移植
static void getPhiRegs(const MachineInstr &Phi, const MachineBasicBlock *Loop,
                       unsigned &InitVal, unsigned &LoopVal) {
  assert(Phi.isPHI() && "Expecting a Phi.");

  InitVal = 0;
  LoopVal = 0;
  for (unsigned i = 1, e = Phi.getNumOperands(); i != e; i += 2)
    if (Phi.getOperand(i + 1).getMBB() != Loop)
      InitVal = Phi.getOperand(i).getReg();
    else
      LoopVal = Phi.getOperand(i).getReg();

  assert(InitVal != 0 && LoopVal != 0 && "Unexpected Phi structure.");
}

/// Return the Phi register value that comes the loop block.
/// \param [in] Phi
/// \param [in] LoopBB
/// \return register
/// \note MachinePipelinerより移植
static unsigned getLoopPhiReg(const MachineInstr &Phi, const MachineBasicBlock *LoopBB) {
  for (unsigned i = 1, e = Phi.getNumOperands(); i != e; i += 2)
    if (Phi.getOperand(i + 1).getMBB() == LoopBB)
      return Phi.getOperand(i).getReg();
  return 0;
}

/// 指定命令の差分を計算する
/// \param [in] MI
/// \param [out] Delta
/// \retval true 計算成功
/// \retval false 計算失敗
/// \note MachinePipelinerより移植
static bool computeDelta(const MachineInstr &MI, unsigned &Delta) {
  const MachineOperand *BaseOp;
  int64_t Offset;
  bool OffsetIsScalable;
  if (!SWPipeliner::TII->getMemOperandWithOffset(MI, BaseOp, Offset, OffsetIsScalable, SWPipeliner::TRI))
    return false;

  // FIXME: This algorithm assumes instructions have fixed-size offsets.
  if (OffsetIsScalable)
    return false;

  if (!BaseOp->isReg())
    return false;

  Register BaseReg = BaseOp->getReg();

  // Check if there is a Phi. If so, get the definition in the loop.
  MachineInstr *BaseDef = SWPipeliner::MRI->getVRegDef(BaseReg);
  if (BaseDef && BaseDef->isPHI()) {
    BaseReg = getLoopPhiReg(*BaseDef, MI.getParent());
    BaseDef = SWPipeliner::MRI->getVRegDef(BaseReg);
  }
  if (!BaseDef)
    return false;

  int D = 0;
  if (!SWPipeliner::TII->getIncrementValue(*BaseDef, D) && D >= 0)
    return false;

  Delta = D;
  return true;
}

/// MachineInstr 同士に依存があるか確認する
/// \param [in] p 対象となる先命令
/// \param [in] q 対象となる後命令
/// \retval true 依存あり
/// \retval false 依存なし
/// \note MachinePipeliner より移植
static bool isDependence(const MachineInstr *p, const MachineInstr *q) {
  if (p->hasUnmodeledSideEffects() || q->hasUnmodeledSideEffects() ||
      p->mayRaiseFPException() || q->mayRaiseFPException() ||
      p->hasOrderedMemoryRef() || q->hasOrderedMemoryRef()) {
    return true;
  }
  if (!p->mayStore() || !q->mayLoad())
    return false;

  unsigned DeltaS, DeltaD;
  if (!computeDelta(*p, DeltaS) || !computeDelta(*q, DeltaD))
    return true;
  const MachineOperand *BaseOpS, *BaseOpD;

  int64_t OffsetS, OffsetD;
  bool OffsetSIsScalable, OffsetDIsScalable;
  if (!SWPipeliner::TII->getMemOperandWithOffset(*p, BaseOpS, OffsetS, OffsetSIsScalable,
                                                 SWPipeliner::TRI) ||
      !SWPipeliner::TII->getMemOperandWithOffset(*q, BaseOpD, OffsetD, OffsetDIsScalable,
                                    SWPipeliner::TRI))
    return true;

  assert(!OffsetSIsScalable && !OffsetDIsScalable &&
         "Expected offsets to be byte offsets");

  if (!BaseOpS->isIdenticalTo(*BaseOpD))
    return true;

  // Check that the base register is incremented by a constant value for each
  // iteration.
  MachineInstr *Def = SWPipeliner::MRI->getVRegDef(BaseOpS->getReg());
  if (!Def || !Def->isPHI())
    return true;
  unsigned InitVal = 0;
  unsigned LoopVal = 0;

  getPhiRegs(*Def, p->getParent(), InitVal, LoopVal);

  MachineInstr *LoopDef = SWPipeliner::MRI->getVRegDef(LoopVal);
  int D = 0;
  if (!LoopDef || !SWPipeliner::TII->getIncrementValue(*LoopDef, D))
    return true;

  uint64_t AccessSizeS = (*p->memoperands_begin())->getSize();
  uint64_t AccessSizeD = (*q->memoperands_begin())->getSize();

  // This is the main test, which checks the offset values and the loop
  // increment value to determine if the accesses may be loop carried.
  if (AccessSizeS == MemoryLocation::UnknownSize ||
      AccessSizeD == MemoryLocation::UnknownSize)
    return true;

  if (DeltaS != DeltaD || DeltaS < AccessSizeS || DeltaD < AccessSizeD)
    return true;

  return (OffsetS + (int64_t)AccessSizeS < OffsetD + (int64_t)AccessSizeD);
}

void SwplDdg::analysisInstDependence() {
  for (auto *p:getLoopBodyInsts()) {
    const auto *pmi=Loop->getOrgMI(p->getMI());
    if (pmi->isPHI()) continue;
    bool succ=false;
    for (auto *q:getLoopBodyInsts()) {
      if (!succ) {
        if (p==q) succ=true;
        continue;
      }
      const auto *qmi=Loop->getOrgMI(q->getMI());
      if (qmi->isPHI()) continue;
      if (isDependence(pmi, qmi)) {
        if (SWPipeliner::isDebugOutput()) {
          dbgs() << "DBG(SwplDdg::analysisInstDependence):\n"
                 << " former_inst:" << *(p->getMI())
                 << " latter_inst:" << *(q->getMI())
                 << " distance:" << 0 << "\n"
                 << " delay:" << 1 << "\n";
        }

        update_distance_and_delay(*this, *p, *q, 0, 1);
      }
    }
  }
}

void SwplDdg::analysisRegDependence() {
  /// 1. analysisRegsFlowDependence() を呼び出し、レジスタによる真依存を解析する。
  analysisRegsFlowDependence();
  /// 2. analysisRegsAntiDependence() を呼び出し、レジスタによる逆依存を解析する。
  analysisRegsAntiDependence();
  /// 3. analysisRegsOutputDependence() を呼び出し、レジスタによる出力依存を解析する。
  analysisRegsOutputDependence();
}

/// ループボディ内の全ての SwplInst (inst)に対して、
/// 参照レジスタを定義する命令(def_inst)を検索し、依存関係（def_inst → inst）
/// の依存を貼り命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。
void SwplDdg::analysisRegsFlowDependence() {
  for (auto *use_inst:getLoopBodyInsts()) {
    for (auto *reg:use_inst->getUseRegs()) {
      SwplInst *def_inst = nullptr;
      int def_indx = -1;
      reg->getDefPortInLoopBody(&def_inst, &def_indx);
      if (def_inst == nullptr) { continue; }
      assert(def_indx >= 0);
      int distance = (getLoop()->areBodyInstsOrder(def_inst, use_inst) ? 0 : 1);
      int delay;
      if (EnableCheckEarlyClobber && def_inst->getDefRegs(def_indx).isEarlyClobber()) {
        // def-regがearly-clobberの場合は、命令のLatencyに関係なく１とする
        delay = 1;
      } else {
        delay = SWPipeliner::STM->computeRegFlowDependence(def_inst->getMI(), use_inst->getMI());
      }
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "DBG(SwplDdg::analysisRegsFlowDependence):\n"
               << " former_inst:" << *(def_inst->getMI())
               << " latter_inst:" << *(use_inst->getMI())
               << " use reg:" << printReg(reg->getReg(), SWPipeliner::TRI) << "\n"
               << " distance:" << distance << "\n"
               << " delay:" << delay << "\n";
      }
      update_distance_and_delay(*this, *def_inst, *use_inst, distance, delay);
    }
  }
}

/// ループボディ内の全ての SwplInst (def_inst)に対して、
/// 定義レジスタを定義する命令(inst)を検索し、依存関係（inst → def_inst）
/// の依存を貼り命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。
void SwplDdg::analysisRegsAntiDependence() {
  for (auto *def_inst:getLoopBodyInsts()) {
    for (auto *reg:def_inst->getDefRegs()) {
      // predecesserのregを取得
      SwplReg *pred_reg = reg->getPredecessor();
      if (pred_reg == nullptr) {
        continue;
      }
      if (pred_reg->isUseInstsEmpty()) { continue; }
      for(auto *use_inst:pred_reg->getUseInsts()) {
        assert(use_inst->isBodyInst());
        if (use_inst->isPrefetch()) {
          continue;
        }
        int distance = (getLoop()->areBodyInstsOrder(use_inst, def_inst) ? 0 : 1);
        int delay = 1;
        if (SWPipeliner::isDebugOutput()) {
          dbgs() << "DBG(SwplDdg::analysisRegsAntiDependence):\n"
                 << " former_inst:" << *(use_inst->getMI())
                 << " latter_inst:" << *(def_inst->getMI())
                 << " use reg:" << printReg(reg->getReg(), SWPipeliner::TRI) << "\n"
                 << " distance:" << distance << "\n"
                 << " delay:" << delay << "\n";
        }
        update_distance_and_delay(*this, *use_inst, *def_inst, distance, delay);
      }
    }
  }
}

/// ループボディ内の全ての SwplInst (def_inst)に対して、
/// 定義レジスタを定義する命令(def_inst2)を検索し、依存関係（def_inst → def_inst2）
/// の依存を貼り命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。
void SwplDdg::analysisRegsOutputDependence() {
  for (auto *def_inst:getLoopBodyInsts()) {
    for (auto *reg:def_inst->getDefRegs()) {
      // predecesserのregを取得
      SwplReg *pred_reg = reg->getPredecessor();
      if (pred_reg == nullptr) {
        continue;
      }
      SwplInst *pred_def_inst = nullptr;
      int def_indx = -1;
      pred_reg->getDefPortInLoopBody(&pred_def_inst, &def_indx);
      if (pred_def_inst == nullptr) { continue; }
      assert(def_indx >= 0);
      int distance = (getLoop()->areBodyInstsOrder(pred_def_inst, def_inst) ? 0 : 1);
      int delay = 1;
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "DBG(SwplDdg::analysisRegsOutputDependence):\n"
               << " former_inst:" << *(pred_def_inst->getMI())
               << " latter_inst:" << *(def_inst->getMI())
               << " use reg:" << printReg(reg->getReg(), SWPipeliner::TRI) << "\n"
               << " distance:" << distance << "\n"
               << " delay:" << delay << "\n";
      }
      update_distance_and_delay(*this, *pred_def_inst, *def_inst, distance, delay);
    }
  }
}

/// SwplMem同士の依存関係を解析し、
/// 命令間のエッジに対してDistance/DelayそれぞれのMapを登録する。\n
/// 真依存、逆依存、出力依存のそれぞれの依存を以下のルーチンを利用してDelayを算出する。\n
void SwplDdg::analysisMemDependence() {
  for (auto *former_mem:getLoopMems()) {
    for (auto *latter_mem:getLoopMems()) {
      if (former_mem == latter_mem) {
        continue;
      }
      if ( (former_mem->getInst())->isPrefetch()  ||
           (latter_mem->getInst())->isPrefetch())  {
        continue;
      }

      int distance, delay;
      enum class DepKind {init, flow, anti, output};
      DepKind depKind = DepKind::init;

      if (former_mem->isMemDef()) {
        if (latter_mem->isMemDef()) {
          delay = SWPipeliner::STM->computeMemOutputDependence(former_mem->getInst()->getMI(), latter_mem->getInst()->getMI());
          depKind = DepKind::output;
        } else {
          delay = SWPipeliner::STM->computeMemFlowDependence(former_mem->getInst()->getMI(), latter_mem->getInst()->getMI());
          depKind = DepKind::flow;
        }
      } else {
        if (latter_mem->isMemDef()) {
          delay = SWPipeliner::STM->computeMemAntiDependence(former_mem->getInst()->getMI(), latter_mem->getInst()->getMI());
          depKind = DepKind::anti;
        } else {
          // Not dependence (input-dependence)
          continue;
        }
      }
      distance = getLoop()->getMemsMinOverlapDistance(former_mem, latter_mem);
      if (SWPipeliner::isDebugOutput()) {
        auto *p="";
        switch (depKind) {
        case DepKind::flow: p="flow"; break;
        case DepKind::anti: p="anti"; break;
        case DepKind::output: p="output"; break;
        case DepKind::init:
          llvm_unreachable("Unknown Dependency Kind");
        }
        dbgs() << "DBG(SwplDdg::analysisMemDependence):" << p << "\n"
        << " former_inst:" << *(former_mem->getInst()->getMI())
        << " latter_inst:" << *(latter_mem->getInst()->getMI())
        << " distance:" << distance << "\n"
        << " delay:" << delay << "\n";
      }

      update_distance_and_delay(*this, *(former_mem->getInst()), *(latter_mem->getInst()), distance, delay);
    }
  }
}
/// Swpl_Inst 間のDistanceとDelayを更新する
/// \param [in,out] ddg     SwplDdg  処理対象のSwplDdg
/// \param [in] former_inst SwplInst 先行の命令のSwplInst 
/// \param [in] latter_inst SwplInst 後続の命令のSwplInst
/// \param [in] distance    int      命令間で依存が問題にならない範囲の回転の数      
/// \param [in] delay       int      命令間であける必要があるcycle数
static void update_distance_and_delay(SwplDdg &ddg, SwplInst &former_inst, SwplInst &latter_inst, int distance, int delay) {
  SwplInstGraph *graph = ddg.getGraph();
  SwplInstEdge *edge = graph->findEdge(former_inst, latter_inst);

  if (edge == nullptr) {
    edge = graph->createEdge(former_inst, latter_inst);
  }

  auto &distances = ddg.getDistancesFor(*edge);
  auto &delays = ddg.getDelaysFor(*edge);
  distances.push_back(distance);
  delays.push_back(delay);
}

/// 命令の依存グラフ情報の SwplInstGraph::Edges 単位でModuloDelayMap情報を収集する。
SwplInstEdge2ModuloDelay *SwplDdg::getModuloDelayMap(int ii) const {
  SwplInstEdge2ModuloDelay *map =  new SwplInstEdge2ModuloDelay();

  const SwplInstGraph &graph = getGraph();
  const SwplInstEdges &edges = graph.getEdges();

  for (auto *edge:edges) {
    int max_modulo_delay = INT_MIN;
    std::vector<unsigned> distances = getDistancesFor(*edge);
    std::vector<int> delays = getDelaysFor(*edge);

    auto distance = distances.begin();
    auto delay = delays.begin();
    auto distance_end = distances.end();
    for(  ; distance != distance_end ; ++distance, ++delay) {
      int delay_val = *delay;

      if( delay_val == 0 ) {
        // delayが0となるのは、edgeのfromの命令がPseudo命令である場合のみを想定
        assert( SWPipeliner::STM->isPseudo( *(edge->getInitial()->getMI() )) );
        delay_val=1;
      }

      int modulo_delay = delay_val - ii * (int)(*distance);

      max_modulo_delay = (max_modulo_delay > modulo_delay) ? max_modulo_delay : modulo_delay;
    }  
    map->insert(std::make_pair(const_cast<SwplInstEdge*>(edge), max_modulo_delay));
  }
  return map;
}

void SwplDdg::print() const {
  dbgs() << "DBG(SwplDdg::print) SwplDdg. \n";
  dbgs() << "### SwplDdg: "<< this << "\n";

  const SwplInstGraph &graph = getGraph();
  const SwplInstEdges &edges = graph.getEdges();
  
  for (auto *edge:edges) {
    const SwplInst *leading_inst = edge->getInitial();
    const SwplInst *trailing_inst = edge->getTerminal();

    dbgs() << "### SwplEdge: " << edge << "\n";
    dbgs() << "### from: " << *leading_inst->getMI() ;
    dbgs() << "### to  : " << *trailing_inst->getMI();
    std::vector<unsigned> distances = getDistancesFor(*edge);
    std::vector<int> delays = getDelaysFor(*edge);

    auto distance = distances.begin();
    auto delay = delays.begin();
    auto distance_end = distances.end();
    for(  ; distance != distance_end ; ++distance, ++delay) {
      dbgs() << "### distance:" << *distance << " delay:" << *delay << "\n";
    }  
    dbgs() << "\n";
  }
}
