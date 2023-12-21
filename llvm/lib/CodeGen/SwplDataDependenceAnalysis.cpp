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
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "swp-ddg"

static cl::opt<bool> DebugDumpDdg("swpl-debug-dump-ddg",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> EnableInstDep("swpl-enable-instdep",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> DisableRegDep4tied("swpl-disable-regdep-4-tied",cl::init(false), cl::ReallyHidden);

static void update_distance_and_delay(SwplDdg &ddg, SwplInst &former_inst, SwplInst &latter_inst, int distance, int delay);

SwplDdg *SwplDdg::Initialize (SwplLoop &loop, bool Nodep) {
  SwplDdg *ddg = new SwplDdg(loop);

  /// 1. Call generateInstGraph() to initialize instruction dependency graph information SwplInstGraph.
  ddg->generateInstGraph();
  /// 2. Call analysisRegDependence() to analyze dependency information between registers.
  ddg->analysisRegDependence();
  /// 3. Call analysisMemDependence() to analyze dependency information between memories.
  if (!Nodep)
    ddg->analysisMemDependence();
  /// 4. Call analysisInstDependence() to analyze instruction dependency information.
  if (EnableInstDep)
    ddg->analysisInstDependence();

  /// 5. Call SwplLoop::recollectPhiInsts() to recollect Phi contained in SwplLoop::BodyInsts
  /// into PhiInsts after generating SwplDdg.
  loop.recollectPhiInsts();

  // Import yaml of DDG
  if (SWPipeliner::isImportDDG())
    ddg->importYaml();

  // Export yaml of DDG
  if (SWPipeliner::isExportDDG())
    ddg->exportYaml();

  if (DebugDumpDdg) {
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
        if (SWPipeliner::isDebugDdgOutput()) {
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
  /// 1. Call analysisRegsFlowDependence() to analyze flow dependence by registers.
  analysisRegsFlowDependence();
  /// 2. Call analysisRegsAntiDependence() to analyze anti-dependence caused by registers.
  analysisRegsAntiDependence();
  /// 3. Call analysisRegsOutputDependence() to analyze output dependence by registers.
  analysisRegsOutputDependence();
  if (!DisableRegDep4tied) {
    /// 4. Call analysisRegsDependence_for_tieddef() to add dependency for tied-def.
    analysisRegDependence_for_tieddef();
  }
}

void SwplDdg::analysisRegDependence_for_tieddef() {

  auto findSwplReg = [](SwplRegs& regs, Register r)->SwplReg * {
    for (auto *p:regs) {
      if (p->getReg() == r) return p;
    }
    assert(0 && "register not found");
    return nullptr;
  };
  for (auto *def_inst : getLoopBodyInsts()) {

    auto *mi = def_inst->getMI();

//    dbgs() << *mi;

    for (unsigned def_ix=0; def_ix < mi->getNumDefs(); def_ix++) {
      unsigned int use_ix = 0;
      if (!mi->isRegTiedToUseOperand(def_ix, &use_ix)) {
        continue;
      }
      auto mi_def_reg = mi->getOperand(def_ix).getReg();
      auto mi_use_reg = mi->getOperand(use_ix).getReg();

      auto *def_op = findSwplReg(def_inst->getDefRegs(), mi_def_reg);
      auto *use_op = findSwplReg(def_inst->getUseRegs(),mi_use_reg);
      auto *COPY = use_op->getDefInst();
      if (!COPY->isCopy()) {
        continue;
      }
      if (SWPipeliner::isDebugDdgOutput()) {
        dbgs() << "SwplDdg::analysisRegDependence_for_tieddef target:" << *mi;
      }

      for (auto *p : def_op->getUseInsts()) {
        int distance = 1;
        int delay = 1;

        if (SWPipeliner::isDebugDdgOutput()) {
          dbgs() << "DBG(SwplDdg::analysisRegsAntiDependence for tied):\n"
                 << " former_inst:" << *(p->getMI())
                 << " latter_inst:" << *(COPY->getMI())
                 << " use reg:" << printReg(def_op->getReg(), SWPipeliner::TRI)
                 << "\n"
                 << " distance:" << distance << "\n"
                 << " delay:" << delay << "\n";
        }
        update_distance_and_delay(*this, *p, *COPY, distance, delay);
      }
    }
  }
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
      int delay = SWPipeliner::STM->computeRegFlowDependence(def_inst->getMI(), use_inst->getMI());
      if (SWPipeliner::isDebugDdgOutput()) {
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
        if (SWPipeliner::isDebugDdgOutput()) {
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
      if (SWPipeliner::isDebugDdgOutput()) {
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
template <>
struct yaml::MappingTraits<SwplDdg::IOmi> {
  static void mapping(IO &io, SwplDdg::IOmi &info) {
    io.mapRequired("id", info.id);
    io.mapOptional("mi", info.mi, "");
  }
  static const bool flow = true;
};

template<>
struct yaml::MappingTraits<SwplDdg::IOddgnodeinfo> {
  static void mapping(IO& io, SwplDdg::IOddgnodeinfo& info) {
    io.mapRequired("distance", info.distance);
    io.mapRequired("delay", info.delay);
  }
  static const bool flow=true;
};
LLVM_YAML_IS_SEQUENCE_VECTOR(SwplDdg::IOddgnodeinfo)

template <>
struct yaml::MappingTraits<SwplDdg::IOddgnode> {
  static void mapping(IO &io, SwplDdg::IOddgnode &info) {
    io.mapRequired("from", info.from);
    io.mapRequired("to", info.to);
    io.mapRequired("infos", info.infos);
  }
};
LLVM_YAML_IS_SEQUENCE_VECTOR(SwplDdg::IOddgnode)

template <>
struct yaml::MappingTraits<SwplDdg::IOddg> {
  static void mapping(IO &io, SwplDdg::IOddg &info) {
    io.mapRequired("fname", info.fname);
    io.mapRequired("loopid", info.loopid);
    io.mapRequired("deps", info.ddgnodes);
  }
};
typedef std::vector<SwplDdg::IOddg> IOddgDocumentList;
LLVM_YAML_IS_DOCUMENT_LIST_VECTOR(SwplDdg::IOddg)
IOddgDocumentList yamlddgList;

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

      unsigned distance, delay;
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
      if (SWPipeliner::isDebugDdgOutput()) {
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

void SwplDdg::importYaml() {
  StringRef fname;
  fname = getLoop()->getML()->getTopBlock()->getParent()->getName();

  if (yamlddgList.size()==0) {
    ErrorOr<std::unique_ptr<MemoryBuffer>>  Buffer = MemoryBuffer::getFile(SWPipeliner::getDDGFileName());
    std::error_code EC = Buffer.getError();
    if (EC)
      report_fatal_error("can not open yaml file", false);
    yaml::Input yin(Buffer.get()->getMemBufferRef());
    yin >> yamlddgList;
  }

  IOddg *target_yamlddg = nullptr;
  for (auto &y: yamlddgList) {
    if (y.fname != fname) continue;
    if (y.loopid != SWPipeliner::loop_number) continue;
    target_yamlddg = &y;
    break;
  }

  if (!target_yamlddg)
    return;

  std::map<const MachineInstr*, unsigned> mimap;
  int mi_no=0;
  for (auto *inst : getLoopBodyInsts()) {
    auto *mi = inst->getMI();
    mimap[mi] = mi_no++;
  }

  SwplInstGraph *graph = getGraph();
  SwplInstEdges &edges = graph->getEdges();
  for (auto &ddgnode : target_yamlddg->ddgnodes) {
    auto found = false;

    for (auto &d : ddgnode.infos)
      if (d.distance > 20)
        report_fatal_error("value too large, distance > 20", false);

    for (auto *edge : edges) {
      const SwplInst *leading_inst = edge->getInitial();
      const SwplInst *trailing_inst = edge->getTerminal();
      unsigned from = mimap[leading_inst->getMI()];
      unsigned to = mimap[trailing_inst->getMI()];

      if (ddgnode.from.id == from && ddgnode.to.id == to) {
        auto &distances = getDistancesFor(*edge);
        auto &delays = getDelaysFor(*edge);
        // check number of elements
        if (ddgnode.infos.size() != distances.size())
          report_fatal_error("number of pairs of distance and delay is incorrect", false);

        found = true;
        auto distance = distances.begin();
        auto distance_end = distances.end();
        auto delay = delays.begin();
        auto ddginfo = ddgnode.infos.begin();
        for(  ; distance != distance_end ; ++distance, ++delay, ++ddginfo) {
          *distance = ddginfo->distance;
          *delay = ddginfo->delay;
        }
        break;
      }
    }

    if (!found)
      report_fatal_error("from-mi or to-mi not found", false);
  }

}

void SwplDdg::exportYaml() {
  std::error_code EC;
  std::unique_ptr<raw_fd_ostream> OutStrm;
  OutStrm = std::make_unique<raw_fd_ostream>(SWPipeliner::getDDGFileName(), EC, sys::fs::OF_Append);
  if (EC)
    report_fatal_error("can not open yaml file", false);

  int mi_no = 0;
  std::map<const MachineInstr*, unsigned> mimap;
  *OutStrm << "# No., MI\n";
  for (auto *inst : getLoopBodyInsts()) {
    auto *mi = inst->getMI();
    mimap[mi] = mi_no;
    *OutStrm << "# " << mi_no << ":" << *mi;
    ++mi_no;
  }

  IOddg yamlddg;
  StringRef fname;
  fname = getLoop()->getML()->getTopBlock()->getParent()->getName();
  yamlddg.fname = fname;
  yamlddg.loopid = SWPipeliner::loop_number;
  std::string s;
  raw_string_ostream strstream(s);
  SwplInstGraph *graph = getGraph();
  SwplInstEdges &edges = graph->getEdges();
  for (auto *edge : edges) {
    const auto *from_mi = edge->getInitial()->getMI();
    const auto *to_mi = edge->getTerminal()->getMI();
    s.clear();
    from_mi->print(strstream, true, false, false, false);
    IOmi from{mimap[from_mi], strstream.str()};
    s.clear();
    to_mi->print(strstream, true, false, false, false);
    IOmi to{mimap[to_mi], strstream.str()};
    IOddgnode n{from, to};

    auto &distances = getDistancesFor(*edge);
    auto &delays = getDelaysFor(*edge);
    auto distance = distances.begin();
    auto distance_end = distances.end();
    auto delay = delays.begin();
    for(  ; distance != distance_end ; ++distance, ++delay) {
      IOddgnodeinfo d{*distance, *delay};
      n.infos.push_back(d);
    }

    yamlddg.ddgnodes.push_back(n);
  }

  yaml::Output yout(*OutStrm);
  yout << yamlddg;
}
