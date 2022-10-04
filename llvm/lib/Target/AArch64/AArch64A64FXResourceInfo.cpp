//=- AArch64A64FXResourceInfo.cpp -  AArch64 A64FX ResourceInfo -*- cpp -*---=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 A64FX Resource Infomation for Instructions.
//
//===----------------------------------------------------------------------===//
//===- Copyright FUJITSU LIMITED 2021 and FUJITSU LABORATORIES LTD. 2021  -===//
//===----------------------------------------------------------------------===//

#include "AArch64A64FXResourceInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace llvm;

void AArch64A64FXResInfo::init() {
  for (auto KV : InstResKV) {
    InstResDescMap[StringRef(KV.Key)]
      = genInstResDesc(KV.ResList, KV.CycleList, KV.DepList);
  }

  for (auto KV : InstInfoKV) {
    InstInfoMap[KV.Opcode] = StringRef(KV.ResKey);
  }
}

InstResDesc *
AArch64A64FXResInfo::genInstResDesc(const std::vector<unsigned> &ResList,
                                    const std::vector<unsigned> &CycleList,
                                    const std::vector<unsigned> &DepList) {
  assert((ResList.size() == CycleList.size())
         && "list size differnt: ResList.size() != CycleList.size().");
  assert((ResList.size() == DepList.size())
         && "list size differnt: ResList.size() != DepList.size().");

  SmallVector<UOpResDesc *, 8> UOpResDescs;
  for (size_t i = 0; i < ResList.size(); ++i) {
    UOpResDesc *UOpRes = genUOpResDesc(ResList[i], CycleList[i], DepList[i]);
    UOpResDescs.push_back(UOpRes);
  }
  InstResDesc * IResDesc = new InstResDesc(UOpResDescs);
  return IResDesc;
}

UOpResDesc *
AArch64A64FXResInfo::genUOpResDesc(const unsigned ResKind,
                                   const unsigned CycleKind,
                                   const unsigned DepKind) {
  AArch64A64FXResInfo::PortVector Ports;
  AArch64A64FXResInfo::SubPortsVector SubPorts;
  genPorts(ResKind, Ports, SubPorts);

  AArch64A64FXResInfo::CycleVector Cycles;
  genCycles(CycleKind, Cycles);

  AArch64A64FXResInfo::UOpDepsVector PrevUOpDeps;
  genUOpDeps(DepKind, Ports, PrevUOpDeps);

  AArch64A64FXResInfo::PortRelVector PrevPortRels;
  genPortRels(ResKind, PrevPortRels);
  
  return new UOpResDesc(Ports, SubPorts, Cycles,
                        PrevUOpDeps, PrevPortRels);
}

void AArch64A64FXResInfo::genPorts(const unsigned ResKind,
                                   AArch64A64FXResInfo::PortVector &Ports,
                                   AArch64A64FXResInfo::SubPortsVector &SubPorts) {
  
  AArch64A64FXResInfo::PortVector EmptyPorts;
  switch(ResKind) {
  case A64FXRes::EXA: {
    Ports.push_back(A64FXRes::P_EXA);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXB: {
    Ports.push_back(A64FXRes::P_EXB);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGA: {
    Ports.push_back(A64FXRes::P_EAGA);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGB: {
    Ports.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::FLA: {
    Ports.push_back(A64FXRes::P_FLA);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::FLB: {
    Ports.push_back(A64FXRes::P_FLB);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::PRX: {
    Ports.push_back(A64FXRes::P_PRX);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::BR: {
    Ports.push_back(A64FXRes::P_BR);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXx: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EXA);
    OrPorts.push_back(A64FXRes::P_EXB);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::EAGx: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::FLx: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_FLA);
    OrPorts.push_back(A64FXRes::P_FLB);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::EXxoEAGx: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EXA);
    OrPorts.push_back(A64FXRes::P_EXB);
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::EAGAsEXA: {
    Ports.push_back(A64FXRes::P_EAGA);
    Ports.push_back(A64FXRes::P_EXA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::FLAsEAGx: {
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_OR_PORTS);
    SubPorts.push_back(EmptyPorts);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXAcEAGx: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_OR_PORTS);
    SubPorts.push_back(EmptyPorts);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXAcBR: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_BR);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGAcEXA: {
    Ports.push_back(A64FXRes::P_EAGA);
    Ports.push_back(A64FXRes::P_EXA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGAcPRX: {
    Ports.push_back(A64FXRes::P_EAGA);
    Ports.push_back(A64FXRes::P_PRX);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGAcFLA: {
    Ports.push_back(A64FXRes::P_EAGA);
    Ports.push_back(A64FXRes::P_FLA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGBcBR: {
    Ports.push_back(A64FXRes::P_EAGB);
    Ports.push_back(A64FXRes::P_BR);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGBcEXAcBR: {
    Ports.push_back(A64FXRes::P_EAGB);
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_BR);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::PRXcFLA: {
    Ports.push_back(A64FXRes::P_PRX);
    Ports.push_back(A64FXRes::P_FLA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGxcEXA: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    Ports.push_back(A64FXRes::P_EXA);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(OrPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EAGxcFLA: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    Ports.push_back(A64FXRes::P_FLA);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(OrPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::PiEAGA4oPiEAGB4: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::PiEAGA4oPiEAGB4cPiFLA4: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    Ports.push_back(A64FXRes::P_FLA);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(OrPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXApEXA: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_EXA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXApPRX: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_PRX);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXBpEXB: {
    Ports.push_back(A64FXRes::P_EXB);
    Ports.push_back(A64FXRes::P_EXB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::FLApFLA: {
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_FLA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXxpEXx: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EXA);
    OrPorts.push_back(A64FXRes::P_EXB);
    SubPorts.push_back(OrPorts);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::FLxpFLx: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_FLA);
    OrPorts.push_back(A64FXRes::P_FLB);
    SubPorts.push_back(OrPorts);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::EXApEXAoEXBpEXB: {
    Ports.push_back(A64FXRes::P_OR_PORTS);
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EXA);
    OrPorts.push_back(A64FXRes::P_EXB);
    SubPorts.push_back(OrPorts);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::FLApNULLsEAGx: {
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_NULL);
    Ports.push_back(A64FXRes::P_OR_PORTS);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::EXApNULLpFLA: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_NULL);
    Ports.push_back(A64FXRes::P_FLA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::FLApNULLpEAGx: {
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_NULL);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(OrPorts);
    break;
  }
  case A64FXRes::PRXpNULLpFLA: {
    Ports.push_back(A64FXRes::P_PRX);
    Ports.push_back(A64FXRes::P_NULL);
    Ports.push_back(A64FXRes::P_FLA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::PRXpNULLpEXA: {
    Ports.push_back(A64FXRes::P_PRX);
    Ports.push_back(A64FXRes::P_NULL);
    Ports.push_back(A64FXRes::P_EXA);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXApNULLpEAGxpNULLpFLA: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_NULL);
    Ports.push_back(A64FXRes::P_OR_PORTS);
    Ports.push_back(A64FXRes::P_FLA);
    AArch64A64FXResInfo::PortVector OrPorts;
    OrPorts.push_back(A64FXRes::P_EAGA);
    OrPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(OrPorts);
    SubPorts.push_back(EmptyPorts);
    break;
  }
  case A64FXRes::EXApNULLpFLApPiEAGAB4:
  case A64FXRes::EXApNULLpFLApPiEAGAB2: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_NULL);
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_AND_PORTS);
    AArch64A64FXResInfo::PortVector AndPorts;
    AndPorts.push_back(A64FXRes::P_EAGA);
    AndPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(AndPorts);
    break;
  }
  case A64FXRes::EXApNULLpFLApFLApPiEAGAB8: {
    Ports.push_back(A64FXRes::P_EXA);
    Ports.push_back(A64FXRes::P_NULL);
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_AND_PORTS);
    AArch64A64FXResInfo::PortVector AndPorts;
    AndPorts.push_back(A64FXRes::P_EAGA);
    AndPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(AndPorts);
    break;
  }
  case A64FXRes::FLApPiEAGAB4:
  case A64FXRes::FLApPiEAGAB2: {
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_AND_PORTS);
    AArch64A64FXResInfo::PortVector AndPorts;
    AndPorts.push_back(A64FXRes::P_EAGA);
    AndPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(AndPorts);
    break;
  }
  case A64FXRes::FLApFLApPiEAGAB8:
  case A64FXRes::FLApFLApPiEAGAB2: {
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_FLA);
    Ports.push_back(A64FXRes::P_AND_PORTS);
    AArch64A64FXResInfo::PortVector AndPorts;
    AndPorts.push_back(A64FXRes::P_EAGA);
    AndPorts.push_back(A64FXRes::P_EAGB);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(EmptyPorts);
    SubPorts.push_back(AndPorts);
    break;
  }
  case A64FXRes::NORES:
    Ports.push_back(A64FXRes::P_NULL);
    SubPorts.push_back(EmptyPorts);
    break;
  default:
    llvm_unreachable("undifined ResKind!");
  }
}

void AArch64A64FXResInfo::genCycles(const unsigned CycleKind,
                                    AArch64A64FXResInfo::CycleVector &Cycles) {
  for (auto KV :CycleKV) {
    if (KV.Key == CycleKind) {
      for (auto cycle : KV.Cycles)
        switch(cycle) {
        case A64FXRes::NA: {
          Cycles.push_back(8);
          break;
        }
        case A64FXRes::NA2: {
          Cycles.push_back(9);
          break;
        }
        case A64FXRes::NA4: {
          Cycles.push_back(11);
          break;
        }
        case A64FXRes::NA8: {
          Cycles.push_back(15);
          break;
        }
        default:
          Cycles.push_back(cycle);
        }
    }    
  }
}

void AArch64A64FXResInfo::genUOpDeps(const unsigned DepKind,
                                     AArch64A64FXResInfo::PortVector &Ports,
                                     AArch64A64FXResInfo::UOpDepsVector &PrevUOpDeps) {
  PrevUOpDeps.resize(Ports.size(), SmallVector<unsigned, 2>());
  switch (DepKind) {
  case A64FXRes::D0:
  case A64FXRes::D1:
  case A64FXRes::D2:
  case A64FXRes::D3:
  case A64FXRes::D4: {
    assert(PrevUOpDeps.size() > 0 && "The array size is not large enough.");
    PrevUOpDeps[0].push_back(DepKind);
    break;
  }
  case A64FXRes::_D1: {
    assert(PrevUOpDeps.size() > 1 && "The array size is not large enough.");
    PrevUOpDeps[1].push_back(1);
    break;
  }
  case A64FXRes::_D4: {
    assert(PrevUOpDeps.size() > 1 && "The array size is not large enough.");
    PrevUOpDeps[1].push_back(4);
    break;
  }
  case A64FXRes::__D1: {
    assert(PrevUOpDeps.size() > 2 && "The array size is not large enough.");
      PrevUOpDeps[2].push_back(1);
    break;
  }
  case A64FXRes::D1cD2: {
    assert(PrevUOpDeps.size() > 0 && "The array size is not large enough.");
    PrevUOpDeps[0].push_back(1);
    PrevUOpDeps[0].push_back(2);
    break;
  }
  default:
    llvm_unreachable("undifined DepKind!");
  }
}

void AArch64A64FXResInfo::genPortRels(const unsigned ResKind,
                                      AArch64A64FXResInfo::PortRelVector &PortRels) {
  
  switch(ResKind) {
  case A64FXRes::EXA:
  case A64FXRes::EXB:
  case A64FXRes::EAGA:
  case A64FXRes::EAGB:
  case A64FXRes::FLA:
  case A64FXRes::FLB:
  case A64FXRes::PRX:
  case A64FXRes::BR:
  case A64FXRes::EXx:
  case A64FXRes::EAGx:
  case A64FXRes::FLx:
  case A64FXRes::EXxoEAGx:
  case A64FXRes::PiEAGA4oPiEAGB4: {
    PortRels.push_back(A64FXRes::NO);
    break;
  }
  case A64FXRes::EAGAsEXA:
  case A64FXRes::FLAsEAGx: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::DEP_AND);
    break;
  }
  case A64FXRes::EXAcEAGx:
  case A64FXRes::EXAcBR:
  case A64FXRes::EAGAcEXA:
  case A64FXRes::EAGAcPRX:
  case A64FXRes::EAGAcFLA:
  case A64FXRes::EAGBcBR:
  case A64FXRes::PRXcFLA:
  case A64FXRes::EAGxcEXA:
  case A64FXRes::EAGxcFLA:
  case A64FXRes::PiEAGA4oPiEAGB4cPiFLA4: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::NODEP_AND);
    break;
  }
  case A64FXRes::EAGBcEXAcBR: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::NODEP_AND);
    PortRels.push_back(A64FXRes::NODEP_AND);
    break;
  }
  case A64FXRes::EXApEXA:
  case A64FXRes::EXApPRX:
  case A64FXRes::EXBpEXB:
  case A64FXRes::FLApFLA:
  case A64FXRes::EXxpEXx:
  case A64FXRes::FLxpFLx:
  case A64FXRes::FLApPiEAGAB4:
  case A64FXRes::FLApPiEAGAB2: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::SEQ);
    break;
  }
  case A64FXRes::EXApEXAoEXBpEXB: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::SEQ_SAME);
    break;
  }
  case A64FXRes::FLApNULLsEAGx: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::SEQ);
    PortRels.push_back(A64FXRes::DEP_AND);
    break;
  }
  case A64FXRes::EXApNULLpFLA:
  case A64FXRes::FLApNULLpEAGx:
  case A64FXRes::PRXpNULLpFLA:
  case A64FXRes::PRXpNULLpEXA:
  case A64FXRes::FLApFLApPiEAGAB8:
  case A64FXRes::FLApFLApPiEAGAB2: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::SEQ);
    PortRels.push_back(A64FXRes::SEQ);
    break;
  }
  case A64FXRes::EXApNULLpFLApPiEAGAB4:
  case A64FXRes::EXApNULLpFLApPiEAGAB2: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::SEQ);
    PortRels.push_back(A64FXRes::SEQ);
    PortRels.push_back(A64FXRes::SEQ);
    break;
  }
  case A64FXRes::EXApNULLpEAGxpNULLpFLA:
  case A64FXRes::EXApNULLpFLApFLApPiEAGAB8: {
    PortRels.push_back(A64FXRes::NO);
    PortRels.push_back(A64FXRes::SEQ);
    PortRels.push_back(A64FXRes::SEQ);
    PortRels.push_back(A64FXRes::SEQ);
    PortRels.push_back(A64FXRes::SEQ);
    break;
  }
  case A64FXRes::NORES: {
    PortRels.push_back(A64FXRes::NO);
    break;
  }
  default:
    llvm_unreachable("undifined ResKind!");
  }
}

// 使用リソースの2次元リストを生成する
// 1次元目はAND 関係にあるリソース
// 2次元目はOR関係にあるリソース
void InstResDesc::genResList() {
  for (auto *UOpDesc : UOpResDescs) {
    for (unsigned i = 0; i < UOpDesc->getNumPorts(); ++i) {
      auto *PortDesc = UOpDesc->getPortResDesc(i);

      if (PortDesc->getPort() == A64FXRes::P_OR_PORTS) {
        ResList.push_back(PortDesc->getSubPorts());
      }
      else if (PortDesc->getPort() == A64FXRes::P_AND_PORTS) {
        for (auto Port : PortDesc->getSubPorts()) {
          SmallVector<A64FXRes::PortKind, 4> or_ports;
          or_ports.push_back(Port);
          ResList.push_back(or_ports);
        }
      }
      else {
        SmallVector<A64FXRes::PortKind, 4> or_ports;
        or_ports.push_back(PortDesc->getPort());
        ResList.push_back(or_ports);
      }
    }
  }
}

// StartCycleとLatencyのリストを生成する。
// AND 関係にあるリソース単位で生成する。
void InstResDesc::genCycleAndLatencyList() {
  SmallVector<unsigned, 16> UOpLatencyList;
  unsigned StartCycle = 0;
  UOpResDesc *PrevUOpDesc = nullptr;

  for (auto *UOpDesc : UOpResDescs) {
    auto &UOpDeps = UOpDesc->getUOpDeps();
    
    // 前uOPが同じリソースを使用する可能性がある場合、開始サイクルに1加算
    bool UsedSamePort = usedSamePort(PrevUOpDesc, UOpDesc);
    StartCycle += (UsedSamePort)? 1 : 0;

    // uOP間の依存関係によるStartCycleの決定
    for (auto Dep : UOpDeps) {
      unsigned Pos = UOpLatencyList.size() - Dep;
      unsigned PrevLatency = (Dep == 0)? StartCycle : UOpLatencyList[Pos];
      StartCycle = std::max(StartCycle, PrevLatency);
    }
    UOpLatencyList.push_back(StartCycle + UOpDesc->getLatency());
    PrevUOpDesc = UOpDesc;

    // リストへの追加処理
    for (unsigned i = 0; i < UOpDesc->getNumPorts(); ++i) {
      auto *PortDesc = UOpDesc->getPortResDesc(i);
      
      if (PortDesc->getPort() == A64FXRes::P_AND_PORTS) {
        for (unsigned i = 0; i < PortDesc->getSubPorts().size(); ++i) {
          StartCycleList.push_back(StartCycle);
          LatencyList.push_back(PortDesc->getCycle());
        }
      }
      else {
        StartCycleList.push_back(StartCycle);
        LatencyList.push_back(PortDesc->getCycle());
      }
    }
  }
}

// 命令単位でのトータルレイテンシを算出する。
unsigned InstResDesc::computeTotalLatency() {
  SmallVector<unsigned, 16> UOpLatencyList;
  unsigned StartCycle = 0;
  UOpResDesc *PrevUOpDesc = nullptr;

  for (auto *UOpDesc : UOpResDescs) {
    auto &UOpDeps = UOpDesc->getUOpDeps();

    // 前uOPが同じリソースを使用する可能性がある場合、開始サイクルに1加算
    bool UsedSamePort = usedSamePort(PrevUOpDesc, UOpDesc);
    StartCycle += (UsedSamePort)? 1 : 0;
    
    // uOP間の依存関係によるStartCycleの決定
    for (auto Dep : UOpDeps) {
      unsigned Pos = UOpLatencyList.size() - Dep;
      unsigned PrevLatency = (Dep == 0)? StartCycle : UOpLatencyList[Pos];
      StartCycle = std::max(StartCycle, PrevLatency);
    }
    UOpLatencyList.push_back(StartCycle + UOpDesc->getLatency());
    PrevUOpDesc = UOpDesc;
  }
  unsigned TotalLatency = 0;
  for (auto Latency : UOpLatencyList)
    TotalLatency = std::max(TotalLatency, Latency);
  return TotalLatency;
}

// 同じポートを使用しているかどうかを判定する。
bool InstResDesc::usedSamePort(UOpResDesc *UOpRes1, UOpResDesc *UOpRes2) {
  if (!UOpRes1 || !UOpRes2)
    return false;
  
  auto UOpPorts1 = UOpRes1->getUOpPorts();
  auto UOpPorts2 = UOpRes2->getUOpPorts();

  bool Used = false;
  for (auto Port1 : UOpPorts1) {
    for (auto Port2 : UOpPorts2) {
      if (Port1 == Port2) {
        Used = true;
        break;
      }
    }
    if (Used) 
      break;
  }
  
  return Used;
}

// uOP単位の使用ポートのリストを生成する。
void UOpResDesc::genUOpPorts() {
  for (auto *PortRes : PortResDescs) {
    auto Port = PortRes->getPort();
    if (Port != A64FXRes::P_OR_PORTS && Port != A64FXRes::P_AND_PORTS)
      UOpPorts.push_back(Port);
    auto SubPorts = PortRes->getSubPorts();
    UOpPorts.append(SubPorts.begin(), SubPorts.end());
  }
}

// ポート単位で付随しているuOP間の依存情報をuOP単位として見られるように変換する。
void UOpResDesc::genUOpDeps() {
  for (auto *Port : PortResDescs) {
    if (Port->getUOpDeps().size() != 0)
      UOpDeps = Port->getUOpDeps();
  }
}

/// uOp単位のレイテンシを返す。
unsigned UOpResDesc::computeTotalLatency() {
  unsigned TotalLatency = 0;
  unsigned StartCycle = 0;
  for (auto *Port : PortResDescs) {
    if (Port->getPortRel() == A64FXRes::SEQ || Port->getPortRel() == A64FXRes::SEQ_SAME)
      StartCycle = TotalLatency;
    TotalLatency = std::max(TotalLatency, Port->getCycle()+StartCycle);
  }
  return TotalLatency;
}
