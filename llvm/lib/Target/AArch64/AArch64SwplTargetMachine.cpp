//=- AArch64SwplTargetMachine.cpp - Target Machine Info for SWP -*- C++ -*---=//
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

#include "AArch64.h"

#include "AArch64SWPipeliner.h"
#include "AArch64SwplTargetMachine.h"
#include "AArch64TargetTransformInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace swpl;

#define DEBUG_TYPE "swpl-tm"
#undef ALLOCATED_IS_CCR_ONLY

static cl::opt<bool> DebugTm("swpl-debug-tm",cl::init(false), cl::ReallyHidden);
static cl::opt<int> OptionStoreLatency("swpl-store-latency",cl::init(6), cl::ReallyHidden);
static cl::opt<int> OptionFlowDep("swpl-flow-dep",cl::init(10), cl::ReallyHidden);
static cl::opt<int> OptionRealFetchWidth("swpl-real-fetch-width",cl::init(4), cl::ReallyHidden);
static cl::opt<int> OptionVirtualFetchWidth("swpl-virtual-fetch-width",cl::init(4), cl::ReallyHidden);
static cl::opt<bool> OptionCopyIsVirtual("swpl-copy-is-virtual",cl::init(false), cl::ReallyHidden);


namespace swpl {

/// 利用資源パターンを生成するための生成過程で使われるデータ構造
struct work_node {
  TmResourceId id; ///< 利用資源
  int startCycle=0; ///< 開始サイクル
  llvm::SmallVector<work_node*, 8> nodes; ///< 次の利用資源

  /// constructor
  explicit work_node(TmResourceId id):id(id){}

  /// destructor
  ~work_node() {
    /// 次の利用資源を削除する
    for (auto *n:nodes) delete n;
  }

  /// 前の利用資源につなげる
  ///
  /// \param [in] p つなげる利用資源
  void append(work_node*p) {
    if (p==nullptr) return;
    nodes.push_back(p);
  }

  /// 資源の利用パターンを生成する
  ///
  /// \param [in] SM SchedModel
  /// \param [out] tmPipelines 生成結果
  void gen_patterns(llvm::TargetSchedModel&SM, TmPipelinesImpl &tmPipelines) {
    std::vector<TmResourceId> ptn;
    std::vector<int> cycle;
    TmPatternId patternId=0;
    gen_pattern(SM, patternId, ptn, cycle, tmPipelines);
  }

  /// 資源の利用パターンを生成する
  ///
  /// \param [in] SM SchedModel
  /// \param [in] patternId 利用資源パターンのID
  /// \param [in] ptn 利用資源パターン
  /// \param [in] cycle 開始サイクル
  /// \param [out] tmPipelines 生成結果
  void gen_pattern(llvm::TargetSchedModel&SM, TmPatternId &patternId, std::vector<TmResourceId> ptn, std::vector<int> cycle, TmPipelinesImpl &tmPipelines) {
    // 引数：ptnはコピーコンストラクタで複製させている。
    if (id!=llvm::A64FXRes::PortKind::P_NULL) {
      ptn.push_back(id);
      cycle.push_back(startCycle);
      if (nodes.empty()) {
        TmPipeline *t=new TmPipeline(SM);
        tmPipelines.push_back(t);
        t->patternId=patternId++;
        for (auto resource:ptn) {
          t->resources.push_back(resource);
        }
        for (auto stage:cycle) {
          t->stages.push_back(stage);
        }
        return;
      }
    }
    for ( work_node* c : nodes ) {
      // and-node
      c->gen_pattern(SM, patternId, ptn, cycle, tmPipelines);
    }
  }
};

}

using nodelist=std::vector<work_node*>;

/// 利用資源パターン生成の準備
/// \param [in] sm SchedModel
/// \param [in,out] next_target 次の資源をつなげる親
/// \param [in] target
/// \param [in] portKindList
/// \param [in] cycle 開始サイクル
static void makePrePattern(llvm::TargetSchedModel sm, nodelist&next_target, nodelist&target, const llvm::SmallVectorImpl<A64FXRes::PortKind> &portKindList, int cycle) {
  for (const auto portKind: portKindList) {
    for (auto *t:target) {
      work_node *p=new work_node(portKind);
      p->startCycle=cycle;
      t->nodes.push_back(p);
      next_target.push_back(p);
    }

  }
}

/// 利用資源パターン生成の準備
/// \param [in] sm SchedModel
/// \param [in] resInfo
/// \param [in] mi 対象のMachineInstr
/// \return 利用資源パターンを生成するための中間表現
/// @note
/// stage値はAArch64A64FXResInfoがstartCycleを真面目に対応しているため、歯抜けになることがある。<br>
/// スケジューリングの際に問題が出たら、歯抜けステージ値が無くなるよう、「0,1,7」から「0,1,2」に変更する。（2020年11月30日鎌塚氏より指示）<br>
/// 例：FMAXNMPv2i64pという命令では、以下のようにstage値が0,1,7と歯抜けになる。<br>
/// (パターン=0) stage/resource: 0/FLA, 1/FLA, 7/FLA<br>
/// (パターン=1) stage/resource: 0/FLA, 1/FLA, 7/FLB
static work_node* makePrePatterns(llvm::TargetSchedModel& sm, const llvm::AArch64A64FXResInfo& resInfo,  const llvm::MachineInstr& mi) {
  work_node *root=nullptr;
  nodelist *target=nullptr;
  nodelist *next_target=nullptr;
  auto *IResDesc = resInfo.getInstResDesc(mi);
  if (IResDesc==nullptr) {
    llvm_unreachable("AArch64A64FXResInfo::getInstResDesc() is nullptr");
  }
  assert(IResDesc!=nullptr);
  const auto &cycleList=IResDesc->getStartCycleList();
  int ix=0;
  for (const auto &PRE : IResDesc->getResList()) {

    if (root==nullptr) {
      root=new work_node(llvm::A64FXRes::PortKind::P_NULL);
      target=new nodelist;
      next_target=new nodelist;
      target->push_back(root);
    }
    makePrePattern(sm, *next_target, *target, PRE, cycleList[ix++]);
    nodelist* t=target;
    target=next_target;
    next_target=t;
    t->clear();
  }
  if  (target!=nullptr) {
    target->clear();
    delete target;
  }
  if (next_target!=nullptr) {
    next_target->clear();
    delete next_target;
  }
  return root;
}

void Tm::initialize(const llvm::MachineFunction &mf) {
  if (MF==nullptr) {
    const llvm::TargetSubtargetInfo &ST = mf.getSubtarget();
    SM.init(&ST);
    ResInfo=new AArch64A64FXResInfo(ST);
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_FLA]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_FLB]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EXA]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EXB]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EAGA]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_EAGB]=2;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_PRX]=1;
    tmNumSameKindResources[llvm::A64FXRes::PortKind::P_BR]=1;
    numResource=8; // 資源管理がSchedModelとは別になったので、ハードコードする
  }
  MF=&mf;
  MRI=&(MF->getRegInfo());
}

unsigned int Tm::getFetchBandwidth(void) const {
  return getRealFetchBandwidth()+OptionVirtualFetchWidth;
}

unsigned int Tm::getRealFetchBandwidth(void) const {
  return OptionRealFetchWidth;
}

int Tm::getNumSameKindResources(TmResourceId resource) {
  int num=tmNumSameKindResources[resource];
  assert(num && "Tm::getNumSameKindResources: invalid resource");
  return num;
}

int TmPipeline::getNResources(TmResourceId resource) const {
  int counter=0;
  for (TmResourceId r: resources) {
    if (r==resource) counter++;
  }
  return counter;
}

void TmPipeline::print(raw_ostream &ost) const {
  ost << "DBG(TmPipeline::print) stage/resource("<< patternId << "): ";
  int last=stages.size();
  const char *sep="";
  for (int ix=0; ix<last; ix++) {
    ost << sep << stages[ix] << "/" << getResourceName(resources[ix]);
    sep=", ";
  }
  ost << "\n";
}

const char * TmPipeline::getResourceName(TmResourceId resource) {
  const char *name="";
  switch (resource) {
  case llvm::A64FXRes::PortKind::P_FLA:name="FLA";break;
  case llvm::A64FXRes::PortKind::P_FLB:name="FLB";break;
  case llvm::A64FXRes::PortKind::P_EXA:name="EXA";break;
  case llvm::A64FXRes::PortKind::P_EXB:name="EXB";break;
  case llvm::A64FXRes::PortKind::P_EAGA:name="EAGA";break;
  case llvm::A64FXRes::PortKind::P_EAGB:name="EAGB";break;
  case llvm::A64FXRes::PortKind::P_PRX:name="PRX";break;
  case llvm::A64FXRes::PortKind::P_BR:name="BR";break;
  default:
    llvm_unreachable("unknown resourceid");
  }
  return name;
}

int Tm::getNumSlot(void) const {
  return getFetchBandwidth();
}

int Tm::computeMemFlowDependence(const llvm::MachineInstr *, const llvm::MachineInstr *) const {
  if (OptionStoreLatency > 0) return OptionStoreLatency;
  if (OptionFlowDep > 0) return OptionFlowDep;
  return 1;
}

const TmPipelinesImpl *Tm::getPipelines(const MachineInstr &mi) {
  auto *tps=tmPipelines[mi.getOpcode()];
  if (tps==nullptr) {
    tps=generateTmPipelines(mi);
    tmPipelines[mi.getOpcode()]=tps;
  }
  return tps;
}

const TmPipeline *Tm::getPipeline(const MachineInstr &mi, TmPatternId patternid) {
  auto *tps= getPipelines(mi);
  if (tps==nullptr) return nullptr;
  if (patternid >= tps->size()) return nullptr;
  auto *t=(*tps)[patternid];
  return t;
}

TmPipelinesImpl *Tm::generateTmPipelines(const MachineInstr &mi) {

  work_node *t=nullptr;
  if (isImplimented(mi)) {
    t=makePrePatterns(SM, *ResInfo, mi);
    if (t==nullptr) {
      if (DebugTm)
        dbgs() << "DBG(Tm::generateTmPipelines): makePrePatterns() is nullptr. MIR=" << mi;
      return nullptr;
    }
  }
  auto *pipelines=new TmPipelines;
  if (t) {
    t->gen_patterns(SM, *pipelines);
    delete t;
  } else {
    pipelines->push_back(new TmPipeline(SM));
  }
  if (DebugTm) {
    for (auto*pipeline:*pipelines) {
      pipeline->print(dbgs());
    }
  }
  return pipelines;
}
int Tm::computeRegFlowDependence(const llvm::MachineInstr* def, const llvm::MachineInstr* use) const {
  const auto *IResDesc=ResInfo->getInstResDesc(*def);
  if (IResDesc==nullptr) return 1;
  return IResDesc->getLatency();
}

int Tm::computeMemAntiDependence(const llvm::MachineInstr *, const llvm::MachineInstr *) const {
  return 1;
}

int Tm::computeMemOutputDependence(const llvm::MachineInstr *, const llvm::MachineInstr *) const {
  return 1;
}

int Tm::getMinNResources(TmOpcodeId opcode, TmResourceId resource) {
  auto *tps=tmPipelines[opcode];
  int min_n=INT_MAX;
  for (auto *t:*tps) {
    int n=t->getNResources(resource);
    if (n)
      min_n=std::min(min_n, n);
  }
  if (min_n==INT_MAX)
    return 0;
  return min_n;
}

unsigned int Tm::getNumResource(void) const {
  return numResource;
}


TmRegKind Tm::getRegKind(llvm::Register reg) const {
  unsigned regClassId=0;
  bool pReg=false;
  if (reg.isVirtual()) {
    const auto * regClass=MRI->getRegClass(reg);
    LLVM_DEBUG(dbgs() << "Tm::getRegKind: register is " << MRI->getTargetRegisterInfo()->getRegClassName(regClass) << "\n");
    if (regClass->hasSuperClassEq(&AArch64::GPR64allRegClass) || regClass->hasSuperClassEq(&AArch64::GPR32allRegClass)) {
      regClassId = AArch64::GPR64RegClassID;
    } else if (regClass->hasSuperClassEq(&AArch64::FPR8RegClass) ||
        regClass->hasSuperClassEq(&AArch64::FPR16RegClass) ||
        regClass->hasSuperClassEq(&AArch64::FPR32RegClass) ||
        regClass->hasSuperClassEq(&AArch64::FPR64RegClass) ||
        regClass->hasSuperClassEq(&AArch64::FPR128RegClass) ||
        regClass->hasSuperClassEq(&AArch64::DDRegClass) ||
        regClass->hasSuperClassEq(&AArch64::DDDRegClass) ||
        regClass->hasSuperClassEq(&AArch64::DDDDRegClass) ||
        regClass->hasSuperClassEq(&AArch64::QQRegClass) ||
        regClass->hasSuperClassEq(&AArch64::QQQRegClass) ||
        regClass->hasSuperClassEq(&AArch64::QQQQRegClass) ||
        regClass->hasSuperClassEq(&AArch64::FPR16_loRegClass) ||
        regClass->hasSuperClassEq(&AArch64::FPR64_loRegClass) ||
        regClass->hasSuperClassEq(&AArch64::FPR128_loRegClass) ||
        regClass->hasSuperClassEq(&AArch64::ZPRRegClass)) {
      regClassId=AArch64::FPR64RegClassID;
    }else if (regClass->hasSuperClassEq(&AArch64::PPRRegClass) ||
        regClass->hasSuperClassEq(&AArch64::PPR_3bRegClass)) {
      regClassId=AArch64::PPRRegClassID;
    }else {
      llvm_unreachable("unknown register");
    }

  } else {
#ifndef ALLOCATED_IS_CCR_ONLY
    pReg=true;
#endif
    if (AArch64::GPR64allRegClass.contains(reg) ||
        AArch64::GPR32allRegClass.contains(reg)) {
      regClassId=AArch64::GPR64RegClassID;
    } else if (AArch64::FPR128RegClass.contains(reg) ||
        AArch64::FPR64RegClass.contains(reg) ||
        AArch64::FPR32RegClass.contains(reg) ||
        AArch64::FPR16RegClass.contains(reg) ||
        AArch64::FPR8RegClass.contains(reg) ||
        AArch64::DDRegClass.contains(reg) ||
        AArch64::DDDRegClass.contains(reg) ||
        AArch64::DDDDRegClass.contains(reg) ||
        AArch64::QQRegClass.contains(reg) ||
        AArch64::QQQRegClass.contains(reg) ||
        AArch64::QQQQRegClass.contains(reg) ||
        AArch64::FPR128_loRegClass.contains(reg) ||
        AArch64::FPR64_loRegClass.contains(reg) ||
        AArch64::FPR16_loRegClass.contains(reg) ||
        AArch64::ZPRRegClass.contains(reg)) {
      regClassId=AArch64::FPR64RegClassID;
    } else if (AArch64::PPRRegClass.contains(reg) ||
        AArch64::PPR_3bRegClass.contains(reg)) {
      regClassId=AArch64::PPRRegClassID;
    } else if (AArch64::CCRRegClass.contains(reg)) {
#ifdef ALLOCATED_IS_CCR_ONLY
      pReg=true;
#endif
      regClassId=AArch64::CCRRegClassID;
    } else {
      llvm_unreachable("unknown register");
    }
  }

  return TmRegKind(regClassId, pReg, *MRI);
}
bool Tm::isImplimented(const llvm::MachineInstr&mi) const {
  if (OptionCopyIsVirtual) {
    if (mi.isCopy()) return false;
  }

  return ResInfo->getInstResDesc(mi)!=nullptr;
}

bool Tm::isPseudo(const MachineInstr &mi) const {
  return !isImplimented(mi);
}

#ifdef STMTEST

void TmX4TmTest::init(llvm::MachineFunction&mf, bool first, int TestID) {
  Tm::initialize(mf);
  if (TestID==1) {

    const char *result="OK";
    const char *firstcall=first?"first-call":"not first-call";

    if (MF!=&mf || MRI==nullptr || tmNumSameKindResources.empty()) {
      result="NG";
    }
    dbgs() << "<<<TEST: 001 Tm>>>\n";
    dbgs() << "DBG(TmX::init): " << firstcall << " is  " << result << "\n";
  }
}

#define DEF_RESOURCE(N) {llvm::A64FXRes::PortKind::P_##N, #N}

void StmTest::run(llvm::MachineFunction&MF) {
  bool firstCall=TM.getMachineRegisterInfo()==nullptr;
  DebugTm = true;
  TM.init(MF, firstCall, TestID);
  const std::map<TmResourceId, const char *> resources={
          DEF_RESOURCE(FLA),
          DEF_RESOURCE(FLB),
          DEF_RESOURCE(EXA),
          DEF_RESOURCE(EXB),
          DEF_RESOURCE(EAGA),
          DEF_RESOURCE(EAGB),
          DEF_RESOURCE(PRX),
          DEF_RESOURCE(BR)
  };
  const std::vector<TmResourceId> resourceIds= {
          llvm::A64FXRes::PortKind::P_FLA,
          llvm::A64FXRes::PortKind::P_FLB,
          llvm::A64FXRes::PortKind::P_EXA,
          llvm::A64FXRes::PortKind::P_EXB,
          llvm::A64FXRes::PortKind::P_EAGA,
          llvm::A64FXRes::PortKind::P_EAGB,
          llvm::A64FXRes::PortKind::P_PRX,
          llvm::A64FXRes::PortKind::P_BR
  };
  if (TestID==100) {
    // MIRに対応した資源情報が揃っているか確認する
    dbgs() << "<<<TEST: 100>>>\n";
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        dbgs() << "*************************\nmir:" << instr;
        if (TM.isPseudo(instr)) {
          dbgs() << "Pseudo\n";
        }
        (void) TM.getPipelines(instr);
        dbgs() << "latency:" << TM.computeRegFlowDependence(&instr, nullptr) << "\n";
      }
    }
  } else if (TestID==1) {
    if (!firstCall) return;
    dbgs() << "<<<TEST: 001-01 latency>>>\n";
    int defaultStoreLatency = OptionStoreLatency;
    int defaultFlowDep = OptionFlowDep;
    dbgs() << "option:swpl-store-latency is default, swpl-flow-dep is default \n";
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is default, swpl-flow-dep is 0 \n";
    OptionFlowDep = 0;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is default, swpl-flow-dep is 1 \n";
    OptionFlowDep = 1;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";

    OptionStoreLatency = 0;
    dbgs() << "option:swpl-store-latency is 0, swpl-flow-dep is default \n";
    OptionFlowDep = defaultFlowDep;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 0, swpl-flow-dep is 0 \n";
    OptionFlowDep = 0;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 0, swpl-flow-dep is 1 \n";
    OptionFlowDep = 1;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";

    OptionStoreLatency = 1;
    dbgs() << "option:swpl-store-latency is 1, swpl-flow-dep is default \n";
    OptionFlowDep = defaultFlowDep;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 1, swpl-flow-dep is 0 \n";
    OptionFlowDep = 0;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    dbgs() << "option:swpl-store-latency is 1, swpl-flow-dep is 1 \n";
    OptionFlowDep = 1;
    dbgs() << "Tm::computeMemFlowDependence():" << TM.computeMemFlowDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemAntiDependence():" << TM.computeMemAntiDependence(nullptr, nullptr) << ", ";
    dbgs() << "Tm::computeMemOutputDependence():" << TM.computeMemOutputDependence(nullptr, nullptr) << "\n";
    OptionFlowDep = defaultFlowDep;
    OptionStoreLatency = defaultStoreLatency;

    dbgs() << "<<<TEST: 001-02 Bandwidth>>>\n";
    int defaultRealFetchWidth = OptionRealFetchWidth;
    int defaultVirtualFetchWidth = OptionVirtualFetchWidth;
    dbgs() << "option:swpl-real-fetch-width is default, swpl-virtual-fetch-width is default\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 0;
    dbgs() << "option:swpl-real-fetch-width is default, swpl-virtual-fetch-width is 0\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 1;
    dbgs() << "option:swpl-real-fetch-width is default, swpl-virtual-fetch-width is 1\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";

    OptionRealFetchWidth = 0;
    OptionVirtualFetchWidth = defaultVirtualFetchWidth;
    dbgs() << "option:swpl-real-fetch-width is 0, swpl-virtual-fetch-width is default\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 0;
    dbgs() << "option:swpl-real-fetch-width is 0, swpl-virtual-fetch-width is 0\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 1;
    dbgs() << "option:swpl-real-fetch-width is 0, swpl-virtual-fetch-width is 1\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";

    OptionRealFetchWidth = 1;
    OptionVirtualFetchWidth = defaultVirtualFetchWidth;
    dbgs() << "option:swpl-real-fetch-width is 1, swpl-virtual-fetch-width is default\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 0;
    dbgs() << "option:swpl-real-fetch-width is 1, swpl-virtual-fetch-width is 0\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";
    OptionVirtualFetchWidth = 1;
    dbgs() << "option:swpl-real-fetch-width is 1, swpl-virtual-fetch-width is 1\n";
    dbgs() << "Tm::getFetchBandwidth():" << TM.getFetchBandwidth() << ", ";
    dbgs() << "Tm::getRealFetchBandwidth():" << TM.getRealFetchBandwidth() << ", ";
    dbgs() << "Tm::getNumSlot():" << TM.getNumSlot() << "\n";

    OptionRealFetchWidth = defaultRealFetchWidth;
    OptionVirtualFetchWidth = defaultVirtualFetchWidth;


    dbgs() << "<<<TEST: 001-03 getNumResource>>>\n";
    dbgs() << "Tm::getNumResource():" << TM.getNumResource() << "\n";
    dbgs() << "<<<TEST: 001-04 getNumSameResource>>>\n";
    for (const auto &rc:resourceIds) {
      // 同じ資源数を出力
      dbgs() << "Tm::getNumSameKindResources(" << resources.at(rc) << "):" << TM.getNumSameKindResources(rc) << "\n";
    }

    dbgs() << "<<<TEST: 001-05 by MachineInstr/>>>\n";
    bool alreadyLD1W=false;
    bool alreadyST1W=false;
    bool alreadySUBSXri=false;
    bool alreadyPHI=false;
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        bool target=false;
        if (instr.getOpcode()==AArch64::LD1W && !alreadyLD1W) {
          alreadyLD1W = true;
          target = true;
        } else if (instr.getOpcode()==AArch64::ST1W && !alreadyST1W) {
          alreadyST1W = true;
          target = true;
        } else if (instr.getOpcode()==AArch64::SUBSXri && !alreadySUBSXri) {
          alreadySUBSXri = true;
          target = true;
        } else if (instr.getOpcode()==AArch64::PHI && !alreadyPHI) {
          alreadyPHI = true;
          target = true;
        }
        if (target) {
          bool isPseudo = TM.isPseudo(instr);
          dbgs() << "**\nmir:" << instr;
          if (!isPseudo)
            dbgs() << "Tm::computeRegFlowDependence(): " << TM.computeRegFlowDependence(&instr, &instr) << ", ";
          dbgs() << "Tm::isIssuedOneByOne(): " << TM.isIssuedOneByOne(instr) << ", ";
          dbgs() << "Tm::isPseudo(): " << isPseudo << "\n";
        }
      }
    }

  } else if (TestID==2) {
    dbgs() << "<<<TEST: 002 TmRegKindNum>>>\n";
    dbgs() << "<<<TEST: 002-01 getKindNum>>>\n";
    dbgs() << "TmRegKind::getKindNum():" << TmRegKind::getKindNum() << "\n";
    dbgs() << "<<<TEST: 002-02 >>>\n";
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        dbgs() << "**\nmir:" << instr;
        int opix = 0;
        for (const auto &op:instr.operands()) {
          opix++;
          dbgs() << "operator(ix=" << opix << "): " << op << "\n";
          if (!op.isReg())
            continue;
          // register kind
          const auto usereg = op.getReg();
          auto regkind = TM.getRegKind(usereg);
          regkind.print(dbgs());
          dbgs() << "TmRegKind::isInteger:" << regkind.isInteger() << " isFloating:" << regkind.isFloating()
                 << " isPredicate:" << regkind.isPredicate() << " isisCCRegister:" << regkind.isCCRegister()
                 << " isIntegerCCRegister:" << regkind.isIntegerCCRegister()
                 << " isAllocated:" << regkind.isAllocalted() << " getNum:" << regkind.getNum() << "\n";
        }
      }
    }
  } else if (TestID==3) {
    bool alreadyLD1W = false;
    bool alreadyST1W = false;
    bool alreadySUBSXri = false;
    bool alreadyFMAXNMPv2i64p = false;
    bool alreadyADDVv4i32v = false;
    bool alreadyST2Twov4s_POST = false;
    bool alreadyBFMXri = false;
    dbgs() << "<<<TEST: 003 TmPipeline>>>\n";
    for (auto &bb:MF) {
      for (auto &instr:bb) {
        bool target = false;
        if (instr.getOpcode() == AArch64::LD1W && !alreadyLD1W) {
          alreadyLD1W = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::ST1W && !alreadyST1W) {
          alreadyST1W = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::SUBSXri && !alreadySUBSXri) {
          alreadySUBSXri = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::FMAXNMPv2i64p && !alreadyFMAXNMPv2i64p) {
          alreadyFMAXNMPv2i64p = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::ADDVv4i32v && !alreadyADDVv4i32v) {
          alreadyADDVv4i32v = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::ST2Twov4s_POST && !alreadyST2Twov4s_POST) {
          alreadyST2Twov4s_POST = true;
          target = true;
        } else if (instr.getOpcode() == AArch64::BFMXri && !alreadyBFMXri) {
          alreadyBFMXri = true;
          target = true;
        }
        if (target) {
          dbgs() << "OPCODE: " << TII->getName(instr.getOpcode()) << "\n";
          const auto *pipes = TM.getPipelines(instr);
          assert(pipes && pipes->size() >= 1);
          dbgs() << "TmPipeline::getPipeline(" << TII->getName(instr.getOpcode()) << ", 0): ";
          const TmPipeline *p = TM.getPipeline(instr, 0);
          p->print(dbgs());
          p = TM.getPipeline(instr, 1000);
          dbgs() << "TmPipeline::getPipeline(" << TII->getName(instr.getOpcode()) << ", 1000): " << (p?"NG\n":"OK\n");
          for (const auto &rc:resourceIds) {
            int ptn = 0;
            dbgs() << "Tmpipeline::getNResources(" << resources.at(rc) << "):";
            char sep = ' ';
            for (const auto *pipe:*pipes) {
              dbgs() << sep << " ptn=" << ptn++ << "/N=" << pipe->getNResources(rc);
              sep = ',';
            }
            dbgs() << "\n";
            dbgs() << "Tm::getMinNResources(" << resources.at(rc) << "): "
                   << TM.getMinNResources(instr.getOpcode(), rc) << "\n";
          }
        }
      }
    }
  }
}
#endif
