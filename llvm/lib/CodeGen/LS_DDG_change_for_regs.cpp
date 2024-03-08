//=- LS_DDG_change_for_regs.cpp - SWPL(Local Scheduler) Pass -*- c++ -*------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SWPL (Local Scheduler) Pass.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "LS_DDG_change_for_regs.h"
#include "SWPipeliner.h"

static double rho(LS::DataType Dt, LS::Graph &Gk, LS::BipartieGraph &Cb, LS::VertexList &X,
                  LS::VertexList &Y, LS::Vertex *tx) {
  LS::VertexList &&Tmp1 = Cb.getGammaMinus(tx);
  LS::VertexList &&Tmp2 = SetAnd(X, Tmp1);
  LS::VertexList &&Tmp3 = Gk.getDownArrowWithType(Dt, tx);
  LS::VertexList &&Tmp4 = SetUnion(Y, Tmp3);
  if (Tmp4.size() > 0) {
    return (double)Tmp2.size() / (double)Tmp4.size();
  }
  return (double)Tmp2.size();
}

void LS::Graph::greedyK(DataType Dt, VtoV &KStar) {
  Graph Gk = *this;
  VertexList &&VR = getVerticesWithType(Dt);
  for (Vertex *U : VR) {
    KStar[U] = nullptr;
  }
  SimpleGraph PK = makePK(Dt);
  std::list<BipartieGraph> BG;
  Gk.bipartieDecomposition(Dt, PK, BG);
  for (BipartieGraph &Cb : BG) {
    VertexList Scb = Cb.getScb();
    VertexList Tcb = Cb.getTcb();
    VertexList X = Scb;
    VertexList Y;
    VertexList SKSStarCb;
#ifdef DEBUG
    llvm::dbgs() << "INIT-X: ";
    printVList(X);
    llvm::dbgs() << "\n";
#endif
    for (;;) {
      if (X.empty())
        break;
      Vertex *MaxTx = nullptr;
      double MaxValue = 0.0;
      for (Vertex *Tx : Tcb) {
        if (SKSStarCb.end() == find(SKSStarCb.begin(), SKSStarCb.end(), Tx)) {
          double Value = rho(Dt, Gk, Cb, X, Y, Tx);
#ifdef DEBUG
          llvm::dbgs() << "RHO: (";
          printVList(X);
          llvm::dbgs() << " ";
          printVList(Y);
          llvm::dbgs() << " " << Tx->getName() << ") -> " << Value << "\n";
#endif
          if (MaxTx == nullptr || MaxValue < Value) {
            MaxTx = Tx;
            MaxValue = Value;
          }
        }
      }
      if (MaxTx == nullptr)
        break;
#ifdef DEBUG
      llvm::dbgs() << "MAXTX: " << MaxTx->getName() << "\n";
#endif
      SKSStarCb.push_back(MaxTx);
      VertexList &&Tmp0 = Cb.getGammaMinus(MaxTx);
#ifdef DEBUG
      llvm::dbgs() << "TMP0: ";
      printVList(Tmp0);
      llvm::dbgs() << "\n";
#endif
      X = SetMinus(X, Tmp0);
      VertexList &&Tmp1 = Gk.getDownArrowWithType(Dt, MaxTx);
      Y = SetUnion(Y, Tmp1);
#ifdef DEBUG
      llvm::dbgs() << "TMP1: ";
      printVList(Tmp1);
      llvm::dbgs() << "\nX: ";
      printVList(X);
      llvm::dbgs() << "\nY: ";
      printVList(Y);
      llvm::dbgs() << "\n";
#endif
    }
    for (Vertex *Tx : SKSStarCb) {
      VertexList &&Tmp2 = Cb.getGammaMinus(Tx);
      for (Vertex *S : Tmp2) {
        if (KStar[S] == nullptr) {
          VertexList &&VList = getPKill(Dt, S);
#ifdef DEBUG
          llvm::dbgs() << "TX: " << Tx->getName() << "\n";
          llvm::dbgs() << "S: " << S->getName() << "\n";
          llvm::dbgs() << "VLIST: ";
          printVList(VList);
          llvm::dbgs() << "\n";
#endif
          bool Exist = false;
          for (Vertex *V : VList) {
            if (Gk.isDescendant(Tx, V)) {
              Exist = true;
              break;
            }
          }
          if (!Exist) {
#ifdef DEBUG
            llvm::dbgs() << "USE-T: " << S->getName() << " " << Tx->getName()
                      << "\n";
#endif
            KStar[S] = Tx;
          } else {
            Vertex *W = nullptr;
            for (Vertex *Aa : VList) {
              W = Aa;
              for (Vertex *Bb : VList) {
                if (Aa != Bb) {
                  if (Gk.isDescendant(Aa, Bb)) {
                    W = nullptr;
                    break;
                  }
                }
              }
              if (W) {
                break;
              }
            }
            assert(W);
#ifdef DEBUG
            llvm::dbgs() << "NOT-USE-T: " << S->getName() << " " << W->getName()
                      << "\n";
#endif
            KStar[S] = W;
          }
          // TODO:
          // Gk を G から完全に作り直しているので無駄が多い．
          // 更新部分のみ追加するようにするべき．
          Gk = makeGk(Dt, KStar);
#ifdef DEBUG
          Gk.show("DEBUG");
#endif
        }
      }
    }
  }
}

void make2Graph(LS::Graph &G, LS::VertexList &MFVertices, LS::IntToV &IndexToP,
                LS::VertexInfo &VtoIndex, LS::Vertex *StartP, LS::Vertex *EndP,
                LS::EdgeList &MFEdges, LS::EdgeIntInfo &FC) {
  auto &Vertices = G.getVertices();
  int EndId = G.getVertices().size() * 2 + 1;
  int Count = 1;
  for (auto *V : Vertices) {
    auto *P = IndexToP[Count];
    VtoIndex[V] = Count;
    MFEdges.emplace_front(StartP, P);
    setEdgeIntInfo(FC, StartP, P, 1);
    auto *NP = IndexToP[Count + 1];
    MFEdges.emplace_front(NP, EndP);
    setEdgeIntInfo(FC, NP, EndP, 1);
    Count += 2;
  }
  assert(Count == EndId);
  auto &Edges = G.getEdges();
  for (auto *U : Vertices) {
    auto &VList = Edges[U];
    for (auto *V : VList) {
      int Ui = VtoIndex[U];
      int Vi = VtoIndex[V];
      auto *Src = IndexToP[Ui];
      auto *Dest = IndexToP[Vi + 1];
      MFEdges.emplace_front(Src, Dest);
      setEdgeIntInfo(FC, Src, Dest, 1);
    }
  }
}

void LS::Graph::calculateMAk0(DataType Dt, VtoV &K,
                          IntToVList &GroupNumberToVList) {
  SimpleGraph DV = makeDV(FLOAT_TYPE, K);
  DV.calculateMAk(GroupNumberToVList);
}

LS::VertexList makeMAkSet(LS::IntToVList &GroupNumberToVList) {
  LS::VertexList MAk;
  int NGroups = GroupNumberToVList.size();
  for (int I = 0; I < NGroups; I++) {
    auto &VList = GroupNumberToVList[I];
    auto *V = VList.front();
    MAk.push_front(V);
  }
  return MAk;
}

void LS::Graph::show(llvm::StringRef Tag) {
  VertexInfo VtoIndex;
  int Index = 0;
  for (Vertex *V : Vertices) {
    VtoIndex[V] = Index;
    Index++;
  }
  llvm::dbgs() << "digraph " << Tag << " {\n";
  for (Vertex *V : Vertices) {
    int Index = VtoIndex[V];
    llvm::dbgs() << Index << " [label=\"" << Index << "=" << V->getName()
              << "\"];\n";
  }
  for (Vertex *X : Vertices) {
    VertexList &YList = Edges[X];
    for (Vertex *Y : YList) {
      int Xi = VtoIndex[X];
      int Yi = VtoIndex[Y];
      int Latency = getLatency(X, Y);
      llvm::dbgs() << " " << Xi << " -> " << Yi << " [label=\"" << Latency
                << "\"];\n";
    }
  }
  llvm::dbgs() << "}\n";
}

struct UkData {
  LS::Vertex *U;
  LS::Vertex *V;
  int W1;
  int W2;
};

static int calculateU1(LS::DataType Dt, LS::Graph &G, LS::VertexList &MAk, LS::Vertex *V) {
  auto &&Tmp1 = G.getDownArrowWithType(Dt, V);
  auto &&Tmp2 = SetAnd(Tmp1, MAk);
  return Tmp2.size();
}

static int calculateU2(LS::DataType Dt, LS::Graph &G, LS::Vertex *U, LS::Vertex *V) {
  auto &&VpList = G.getPKill(Dt, U);
  auto &&Tmp0 = G.getDownArrowWithType(Dt, V);
  LS::VertexList Tmp1;
  if (VpList.end() == find(VpList.begin(), VpList.end(), V))
    return 0;
  for (auto *Vp : VpList) {
    auto &&Tmp2 = G.getDownArrowWithType(Dt, Vp);
    Tmp1 = SetUnion(Tmp1, Tmp2);
  }
  auto &&Tmp3 = SetMinus(Tmp1, Tmp0);
  return Tmp3.size();
}

static int calculateW1(LS::DataType Dt, LS::Graph &G, LS::VertexList &MAk, LS::Vertex *U,
                       LS::Vertex *V) {
  int U1 = calculateU1(Dt, G, MAk, V);
  int U2 = calculateU2(Dt, G, U, V);
  return U1 - U2;
}

int LS::Graph::getCriticalPathLength(VertexInfo &Time) {
  for (auto *V : Vertices) {
    Time[V] = -1;
  }
  for (;;) {
    bool Changed = false;
    for (auto *V : Vertices) {
      if (Time[V] == -1) {
        VertexList &&PList = getGammaMinus(V);
        bool AllDone = true;
        int MaxC = -1;
        for (auto *P : PList) {
          int B = Time[P];
          if (B == -1) {
            AllDone = false;
            break;
          }
          int C = B + getLatency(P, V);
          if (MaxC == -1 || MaxC < C)
            MaxC = C;
        }
        if (AllDone) {
          if (MaxC == -1)
            MaxC = 0;
          Time[V] = MaxC;
          Changed = true;
        }
      }
    }
    if (!Changed)
      break;
  }
  int MaxTime = 0;
  for (auto *V : Vertices) {
    int C = Time[V];
    if (MaxTime < C)
      MaxTime = C;
  }
  return MaxTime;
}

int LS::Graph::updateCriticalPathLength(VertexInfo &Time, Vertex *ChangedV) {
  VertexList UpdateList;
  UpdateList.push_front(ChangedV);
  for (;;) {
    VertexList NextUpdateList;
    for (auto *V : UpdateList) {
      int Old = Time[V];
      VertexList &&PList = getGammaMinus(V);
      int MaxC = -1;
      for (auto *P : PList) {
        int B = Time[P];
        int C = B + getLatency(P, V);
        if (MaxC == -1 || MaxC < C)
          MaxC = C;
      }
      if (Old != MaxC) {
        Time[V] = MaxC;
        VertexList &NList = getGammaPlus(V);
        for (Vertex *N : NList) {
          if (NextUpdateList.end() ==
              find(NextUpdateList.begin(), NextUpdateList.end(), N))
            NextUpdateList.push_back(N);
        }
      }
    }
    if (NextUpdateList.size() == 0)
      break;
    UpdateList = NextUpdateList;
  }
  int MaxTime = 0;
  for (auto *V : Vertices) {
    int C = Time[V];
    if (MaxTime < C)
      MaxTime = C;
  }
  return MaxTime;
}

int LS::Graph::getCriticalPathLengthIf(VertexList &UpList, Vertex *V, int OldCp,
                                   const VertexInfo &Time) {
  for (auto *Up : UpList) {
    Edges[Up].push_front(V);
    Delta[Up].emplace_front(V, 1);
  }
  VertexInfo CopyTime = Time;
  int NewCp = updateCriticalPathLength(CopyTime, V);
  int Diff = NewCp - OldCp;
  for (auto *Up : UpList) {
    Edges[Up].pop_front();
    Delta[Up].pop_front();
  }
  return Diff;
}

static void makeUkSub(LS::DataType Dt, LS::Graph &G, LS::VertexList &MAk, LS::Vertex *U,
                      LS::Vertex *V, LS::VertexList &UList, LS::VertexList &VList,
                      std::list<UkData> &Uk, int OldCp,
                      const LS::VertexInfo &Time) {
  auto TmpList = UList;
  auto &&UpList = G.getPKill(Dt, U);
  bool Valid = true;
  for (auto *Up : UpList) {
#if 1
    if (G.generateCyclicPath(Up, V) || G.isDescendant(Up, V)) {
      Valid = false;
      break;
    }
#else
    TmpList.push_front(Up);
    if (G.generateCyclicPath(Up, V) || G.isGroupDescendant(TmpList, VList)) {
      Valid = false;
      break;
    }
    TmpList.pop_front();
#endif
  }
  if (!Valid)
    return;
  int W1 = calculateW1(Dt, G, MAk, U, V);
  int W2 = G.getCriticalPathLengthIf(UpList, V, OldCp, Time);
  UkData X = {U, V, W1, W2};
  Uk.push_back(X);
}

static std::list<UkData> makeUk(LS::DataType Dt, LS::Graph &G,
                                LS::IntToVList &GroupNumberToVList) {
  LS::VertexInfo Time;
  int OldCp = G.getCriticalPathLength(Time);
  std::list<UkData> Uk;
  auto MAk = makeMAkSet(GroupNumberToVList);
  int NGroups = GroupNumberToVList.size();
  for (int X = 0; X < NGroups; X++) {
    auto &XList = GroupNumberToVList[X];
    auto *XHead = XList.front();
    auto *XTail = XList.back();
    for (int Y = X + 1; Y < NGroups; Y++) {
      auto &YList = GroupNumberToVList[Y];
      auto *YHead = YList.front();
      auto *YTail = YList.back();
      makeUkSub(Dt, G, MAk, XTail, YHead, XList, YList, Uk, OldCp, Time);
      makeUkSub(Dt, G, MAk, YTail, XHead, YList, XList, Uk, OldCp, Time);
    }
  }
  return Uk;
}

static UkData *choosePair(std::list<UkData> &Uk, int N) {
  UkData *FirstC = nullptr;
  UkData *SecondC = nullptr;
  for (UkData &X : Uk) {
    if (X.W2 == 0 && 0 < X.W1) {
      if (N < X.W1) {
        if (SecondC == nullptr || X.W1 < SecondC->W1)
          SecondC = &X;
      } else {
        if (FirstC == nullptr || FirstC->W1 < X.W1)
          FirstC = &X;
      }
    }
  }
  if (FirstC)
    return FirstC;
  if (SecondC)
    return SecondC;
  UkData *ThirdC = nullptr;
  for (UkData &X : Uk) {
    if (0 < X.W1) {
      if (ThirdC == nullptr || X.W2 < ThirdC->W2)
        ThirdC = &X;
      else {
        if (X.W2 == ThirdC->W2) {
          if (ThirdC->W1 < X.W1)
            ThirdC = &X;
        }
      }
    }
  }
  return ThirdC;
}

void LS::Graph::serializePair(DataType Dt, Vertex *U, Vertex *V,
                          EdgeList &AddEdges) {
  VertexList &&UpList = getPKill(Dt, U);
  for (auto *Up : UpList) {
    std::pair<Vertex *, Vertex *> P = std::make_pair(Up, V);
    Edges[Up].push_front(V);
    Delta[Up].emplace_front(V, 0);
    AddEdges.push_back(P);
  }
#ifdef DEBUG
  show("SEL");
#endif
}

void printUk(UkData &Uk) {
  llvm::dbgs() << "(" << Uk.U->getName() << " " << Uk.V->getName() << " " << Uk.W1
            << " " << Uk.W2 << ")";
}

void printUkList(std::list<UkData> &Uk) {
  llvm::dbgs() << "(";
  bool First = true;
  for (UkData &x : Uk) {
    if (First)
      First = false;
    else
      llvm::dbgs() << " ";
    printUk(x);
  }
  llvm::dbgs() << ")";
}

bool LS::Graph::serialize(DataType Dt, VtoV &K, int R, EdgeList &AddEdges) {
  for (;;) {
    IntToVList GroupNumberToVList;
    calculateMAk0(Dt, K, GroupNumberToVList);
    int CurrentSize = GroupNumberToVList.size();
#ifdef DEBUG
    llvm::dbgs() << "SERIALIZEE-0: " << CurrentSize << "\n";
#endif
    if (CurrentSize <= R)
      break;
    auto Uk = makeUk(Dt, *this, GroupNumberToVList);
#ifdef DEBUG
    llvm::dbgs() << "U-K: ";
    printUkList(Uk);
    llvm::dbgs() << "\n";
#endif
    auto *Choice = choosePair(Uk, CurrentSize - R);
    if (!Choice)
      return false;
#ifdef DEBUG
    llvm::dbgs() << "SERIALIZEE-1: " << CurrentSize - R << " " << Uk.size() << " ";
    printUk(*Choice);
    llvm::dbgs() << "\n";
#endif
    serializePair(Dt, Choice->U, Choice->V, AddEdges);
  }
  return true;
}

static int getCapacity(LS::EdgeIntInfo &Capacity, LS::Vertex *U, LS::Vertex *V) {
  int Value = -1;
  if (getEdgeIntInfoValue(Capacity, U, V, Value))
    return Value;
  return 0;
}

static int getResidualCapacity(LS::EdgeIntInfo &ResidualCapacity, LS::Vertex *U,
                               LS::Vertex *V) {
  int Value = -1;
  if (getEdgeIntInfoValue(ResidualCapacity, U, V, Value))
    return Value;
  return 0;
}

void LS::MaximumFlow::initializePreflow(EdgeIntInfo &Capacity, Vertex *StartNode,
                                    VertexInfo &HInfo, VertexInfo &EInfo,
                                    EdgeIntInfo &FInfo) {
  for (auto *V : Vertices) {
    HInfo[V] = 0;
    EInfo[V] = 0;
  }
  for (auto *U : Vertices) {
    VertexList &VList = Edges[U];
    for (auto *V : VList) {
      FInfo[U].emplace_back(V, 0);
    }
  }
  HInfo[StartNode] = Vertices.size();
  for (auto *U : Vertices) {
    auto &VList = Edges[U];
    for (auto *V : VList) {
      if (U == StartNode) {
        int CSV = getCapacity(Capacity, StartNode, V);
        setEdgeIntInfo(FInfo, StartNode, V, CSV);
        EInfo[V] = CSV;
        EInfo[StartNode] = EInfo[StartNode] - CSV;
      }
    }
  }
}

void LS::MaximumFlow::initResidualCapacity(EdgeIntInfo &Capacity,
                                       EdgeIntInfo &FInfo,
                                       EdgeIntInfo &ResidualCapacity) {
  for (auto *U : Vertices) {
    auto &VList = Edges[U];
    for (auto *V : VList) {
      int Value = -1;
      bool Flag = getEdgeIntInfoValue(FInfo, U, V, Value);
      assert(Flag);
      int Tmp = getCapacity(Capacity, U, V) - Value;
      setEdgeIntInfo(ResidualCapacity, U, V, Tmp);
      setEdgeIntInfo(ResidualCapacity, V, U, Value);
    }
  }
}

LS::Vertex *LS::MaximumFlow::pushWorkCondition(EdgeIntInfo &ResidualCapacity,
                                       VertexInfo &HInfo, Vertex *U) {
  for (auto *V : Vertices) {
    if (0 < getResidualCapacity(ResidualCapacity, U, V) &&
        HInfo[U] == 1 + HInfo[V])
      return V;
  }
  return nullptr;
}

void LS::MaximumFlow::relabelWork(EdgeIntInfo &ResidualCapacity, VertexInfo &HInfo,
                              VertexInfo &EInfo, Vertex *U) {
  assert(0 < EInfo[U]);
  int UH = HInfo[U];
  bool First = true;
  int MinH = 0;
  for (auto *V : Vertices) {
    if (0 < getResidualCapacity(ResidualCapacity, U, V)) {
      int VH = HInfo[V];
      assert(UH <= VH);
      if (First || VH < MinH) {
        MinH = VH;
        First = false;
      }
    }
  }
  assert(!First);
  HInfo[U] = 1 + MinH;
}

static void updateResidualCapacityOnEdge(LS::EdgeIntInfo &Capacity,
                                         LS::EdgeIntInfo &ResidualCapacity,
                                         LS::EdgeIntInfo &FInfo, LS::Vertex *U,
                                         LS::Vertex *V) {
  int Value = -1;
  bool Flag = getEdgeIntInfoValue(FInfo, U, V, Value);
  assert(Flag);
  setEdgeIntInfo(ResidualCapacity, U, V, (getCapacity(Capacity, U, V) - Value));
  setEdgeIntInfo(ResidualCapacity, V, U, Value);
}

static void updateResidualCapacityOnReverseEdge(LS::EdgeIntInfo &ResidualCapacity,
                                                LS::EdgeIntInfo &FInfo, LS::Vertex *U,
                                                LS::Vertex *V) {
  int Value = -1;
  bool Flag = getEdgeIntInfoValue(FInfo, V, U, Value);
  assert(Flag);
  setEdgeIntInfo(ResidualCapacity, U, V, Value);
}

void LS::MaximumFlow::pushWork(EdgeIntInfo &Capacity, EdgeIntInfo &ResidualCapacity,
                           VertexInfo &HInfo, VertexInfo &EInfo,
                           EdgeIntInfo &FInfo, Vertex *U, Vertex *V) {
  int CF = getResidualCapacity(ResidualCapacity, U, V);
  assert(0 < CF);
  assert(HInfo[U] == 1 + HInfo[V]);
  int DeltaFUV = std::min(EInfo[U], CF);
  if (isEdge(U, V)) {
    addEdgeIntInfo(FInfo, U, V, DeltaFUV);
    updateResidualCapacityOnEdge(Capacity, ResidualCapacity, FInfo, U, V);
  } else {
    assert(isEdge(V, U));
    addEdgeIntInfo(FInfo, V, U, -DeltaFUV);
    updateResidualCapacityOnReverseEdge(ResidualCapacity, FInfo, U, V);
  }
  EInfo[U] = EInfo[U] - DeltaFUV;
  EInfo[V] = EInfo[V] + DeltaFUV;
}

#ifdef DEBUG
static void checkEdgeInfo(VertexList &Vertices, EdgeIntInfo &AnyInfo) {
  for (Vertex *U : Vertices) {
    for (Vertex *V : Vertices) {
      if (U != V) {
        int Value = -1;
        if (getEdgeIntInfoValue(AnyInfo, U, V, Value))
          llvm::dbgs() << "(" << U->getName() << " " << V->getName() << ") -> "
                    << Value << "\n";
      }
    }
  }
}

static void checkInfo(VertexList &Vertices, VertexInfo &HInfo,
                      VertexInfo &EInfo, EdgeIntInfo &FInfo) {
  llvm::dbgs() << "H-INFO:\n";
  for (Vertex *V : Vertices) {
    llvm::dbgs() << V->getName() << " -> " << HInfo[V] << "\n";
  }
  llvm::dbgs() << "F-INFO:\n";
  checkEdgeInfo(Vertices, FInfo);
  llvm::dbgs() << "E-INFO:\n";
  for (Vertex *V : Vertices) {
    llvm::dbgs() << V->getName() << " -> " << EInfo[V] << "\n";
  }
}
#endif

void LS::MaximumFlow::genericPushRelabel(EdgeIntInfo &Capacity, Vertex *StartNode,
                                     Vertex *EndNode, EdgeIntInfo &FInfo) {
  VertexInfo HInfo;
  VertexInfo EInfo;
  initializePreflow(Capacity, StartNode, HInfo, EInfo, FInfo);
#ifdef DEBUG
  checkInfo(Vertices, HInfo, EInfo, FInfo);
#endif
  EdgeIntInfo ResidualCapacity;
  initResidualCapacity(Capacity, FInfo, ResidualCapacity);
#ifdef DEBUG
  llvm::dbgs() << "RESIDUAL-CAPACITY:\n";
  checkEdgeInfo(Vertices, ResidualCapacity);
#endif
  for (;;) {
    bool Changed = false;
    for (auto *U : Vertices) {
      if (U != StartNode && U != EndNode && 0 < EInfo[U]) {
        auto *V = pushWorkCondition(ResidualCapacity, HInfo, U);
        if (V == nullptr) {
#ifdef DEBUG
          llvm::dbgs() << "RELABEL\n";
#endif
          relabelWork(ResidualCapacity, HInfo, EInfo, U);
        } else {
#ifdef DEBUG
          llvm::dbgs() << "PUSH-WORK\n";
#endif
          pushWork(Capacity, ResidualCapacity, HInfo, EInfo, FInfo, U, V);
        }
#ifdef DEBUG
        checkInfo(Vertices, HInfo, EInfo, FInfo);
#endif
        Changed = true;
      }
    }
    if (!Changed)
      break;
  }
}

static void makeMFVertices(int EndId, LS::VertexList &MFVertices,
                           LS::IntToV &IndexToP) {
  for (int I = 0; I <= EndId; I++) {
    auto *Vp = new LS::Vertex(I);
    IndexToP[I] = Vp;
    MFVertices.push_back(Vp);
  }
}

static void deleteMFVertices(LS::VertexList &MFVertices) {
  for (auto *V : MFVertices) {
    delete V;
  }
}

static void make2Graph(LS::SimpleGraph &G, const LS::VertexList &MFVertices,
                       LS::IntToV &IndexToP, LS::VertexInfo &VtoIndex, LS::Vertex *StartP,
                       LS::Vertex *EndP, LS::EdgeInfo &MFEdges, LS::EdgeIntInfo &FC) {
  const auto &Vertices = G.getVertices();
  LS::EdgeList NewEList;
  int EndId = G.getVertices().size() * 2 + 1;
  int Count = 1;
  for (auto *v : Vertices) {
    auto *P = IndexToP[Count];
    VtoIndex[v] = Count;
    NewEList.emplace_front(StartP, P);
    setEdgeIntInfo(FC, StartP, P, 1);
    auto *NP = IndexToP[Count + 1];
    NewEList.emplace_front(NP, EndP);
    setEdgeIntInfo(FC, NP, EndP, 1);
    Count += 2;
  }
  assert(Count == EndId);
  auto &Edges = G.getEdges();
  for (auto *U : Vertices) {
    auto &VList = Edges[U];
    for (auto *V : VList) {
      int Ui = VtoIndex[U];
      int Vi = VtoIndex[V];
      auto *Src = IndexToP[Ui];
      auto *Dest = IndexToP[Vi + 1];
      NewEList.emplace_front(Src, Dest);
      setEdgeIntInfo(FC, Src, Dest, 1);
    }
  }
  for (const auto &P : NewEList) {
    auto *U = P.first;
    auto *V = P.second;
    MFEdges[U].push_front(V);
  }
}
#ifdef DEBUG
static void debugMAk(const LS::SimpleGraph &G, LS::VertexInfo &VtoIndex,
                     LS::MaximumFlow &MF, LS::EdgeIntInfo &FInfo) {
  const auto &Vertices = G.getVertices();
  llvm::dbgs() << "digraph D2 {\n";
  for (auto *V : Vertices) {
    int Index = VtoIndex[V];
    llvm::dbgs() << Index << " [label=\"X(" << V->getName() << ")\"];\n";
    llvm::dbgs() << Index + 1 << " [label=\"Y(" << V->getName() << ")\"];\n";
  }
  llvm::dbgs() << "0 [label=\"start\"];\n";
  int EndId = Vertices.size() * 2 + 1;
  llvm::dbgs() << EndId << " [label=\"end\"];\n";
  auto &Edges = MF.getEdges();
  for (auto *U : MF.getVertices()) {
    auto &VList = Edges[U];
    for (auto *V : VList) {
      llvm::dbgs() << U->getName() << " -> " << V->getName();
      int Value = -1;
      if (getEdgeIntInfoValue(FInfo, U, V, Value)) {
        if (Value == 0) {
          llvm::dbgs() << " [style=dotted]";
        }
      }
      llvm::dbgs() << ";\n";
    }
  }
  llvm::dbgs() << "}\n";
}
#endif
static LS::Vertex *getHead(LS::VtoV &GHead, LS::Vertex *V) {
  if (GHead.end() == GHead.find(V))
    return V;
  auto *Head = GHead[V];
  return getHead(GHead, Head);
}

static void mergeGroup(LS::VtoV &GHead, LS::Vertex *X, LS::Vertex *Y) {
  auto *Xhead = getHead(GHead, X);
  auto *Yhead = getHead(GHead, Y);
  if (Xhead != Yhead) {
    GHead[Y] = Xhead;
  }
}

static void unionFind(LS::SimpleGraph &G, LS::IntToV &IndexToP, LS::VertexInfo VtoIndex,
                      LS::EdgeIntInfo &FInfo, LS::IntToVList &GroupNumberToVList) {
  LS::VtoV GHead;
  auto &Edges = G.getEdges();
  for (auto *U : G.getVertices()) {
    auto &VList = Edges[U];
    for (auto *V : VList) {
      int Ui = VtoIndex[U];
      int Vi = VtoIndex[V];
      int Value = -1;
      auto *Up = IndexToP[Ui];
      auto *Vp = IndexToP[Vi + 1];
      if (getEdgeIntInfoValue(FInfo, Up, Vp, Value)) {
        if (0 < Value) {
          mergeGroup(GHead, U, V);
        }
      }
    }
  }
  LS::VertexInfo VtoGroupNumber;
  int NGroups = 0;
  for (auto *V : G.getVertices()) {
    auto *Head = getHead(GHead, V);
    int GN = -1;
    if (VtoGroupNumber.end() == VtoGroupNumber.find(Head)) {
      GN = NGroups;
      VtoGroupNumber[Head] = GN;
      NGroups++;
    } else {
      GN = VtoGroupNumber[Head];
    }
    GroupNumberToVList[GN].push_front(V);
  }
}

static void assignTopologicalOrder(LS::SimpleGraph &G, LS::VertexInfo &VtoOrder) {
  for (auto *v : G.getVertices()) {
    VtoOrder[v] = 0;
  }
  for (;;) {
    bool Changed = false;
    auto &Edges = G.getEdges();
    for (auto *X : G.getVertices()) {
      auto &YList = Edges[X];
      for (auto *Y : YList) {
        int XOrder = VtoOrder[X];
        int YOrder = VtoOrder[Y];
        if (XOrder >= YOrder) {
          VtoOrder[Y] = YOrder + 1;
          Changed = true;
        }
      }
    }
    if (!Changed)
      break;
  }
#ifdef DEBUG
  for (Vertex *v : G.getVertices()) {
    int Order = VtoOrder[v];
    llvm::dbgs() << v->getName() << " : " << Order << "\n";
  }
#endif
}

static LS::VertexInfo *GVtoOrder;

static bool comp(LS::Vertex *X, LS::Vertex *Y) {
  auto &VtoOrder = *GVtoOrder;
  int XOrder = VtoOrder[X];
  int YOrder = VtoOrder[Y];
  return (XOrder < YOrder);
}

static void sortGroupNumberToVList(LS::SimpleGraph &G,
                                   LS::IntToVList &GroupNumberToVList) {
  LS::VertexInfo VtoOrder;
  assignTopologicalOrder(G, VtoOrder);
  GVtoOrder = &VtoOrder;
  int NGroups = GroupNumberToVList.size();
  for (int I = 0; I < NGroups; I++) {
    auto &VList = GroupNumberToVList[I];
    VList.sort(comp);
  }
  GVtoOrder = nullptr;
}

void LS::SimpleGraph::calculateMAk(IntToVList &GroupNumberToVList) {
  VertexList MFVertices;
  EdgeInfo MFEdges;
  IntToV IndexToP;
  VertexInfo VtoIndex;
  int EndId = getVertices().size() * 2 + 1;
  makeMFVertices(EndId, MFVertices, IndexToP);
  Vertex *StartP = IndexToP[0];
  Vertex *EndP = IndexToP[EndId];
  EdgeIntInfo FC;
  make2Graph(*this, MFVertices, IndexToP, VtoIndex, StartP, EndP, MFEdges, FC);

  MaximumFlow MF(MFVertices, MFEdges);
  EdgeIntInfo FInfo;
  MF.genericPushRelabel(FC, StartP, EndP, FInfo);
#ifdef DEBUG
  debugMAk(*this, VtoIndex, MF, FInfo);
#endif
  unionFind(*this, IndexToP, VtoIndex, FInfo, GroupNumberToVList);
  sortGroupNumberToVList(*this, GroupNumberToVList);
  deleteMFVertices(MFVertices);
}

void LS::SimpleGraph::show(llvm::StringRef Tag) {
  VertexInfo VtoIndex;
  int index = 0;
  for (auto *V : Vertices) {
    VtoIndex[V] = index;
    index++;
  }
  llvm::dbgs() << "digraph " << Tag << " {\n";
  for (auto *V : Vertices) {
    int Index = VtoIndex[V];
    llvm::dbgs() << Index << " [label=\"" << Index << "=" << V->getName()
              << "\"];\n";
  }
  for (auto *X : Vertices) {
    VertexList &YList = Edges[X];
    for (auto *Y : YList) {
      int XIndex = VtoIndex[X];
      int YIndex = VtoIndex[Y];
      llvm::dbgs() << " " << XIndex << " -> " << YIndex << ";\n";
    }
  }
  llvm::dbgs() << "}\n";
}

bool LS::isMember(const LS::EdgeList &Edges, LS::Vertex *U, LS::Vertex *V) {
  for (const auto &P : Edges) {
    if (P.first == U && P.second == V)
      return true;
  }
  return false;
}

bool LS::SetEqual(const LS::VertexList &S0, const LS::VertexList &S1) {
  if (S0.size() != S1.size())
    return false;
  for (auto *x : S0) {
    if (S1.end() == find(S1.begin(), S1.end(), x)) {
      return false;
    }
  }
  return true;
}

LS::VertexList LS::SetUnion(const LS::VertexList &S0, const LS::VertexList &S1) {
  auto Result = S0;
  for (auto *x : S1) {
    if (Result.end() == find(Result.begin(), Result.end(), x)) {
      Result.push_front(x);
    }
  }
  return Result;
}

LS::VertexList LS::SetAnd(const LS::VertexList &S0, const LS::VertexList &S1) {
  LS::VertexList Result;
  for (auto *x : S0) {
    if (S1.end() != find(S1.begin(), S1.end(), x)) {
      Result.push_front(x);
    }
  }
  return Result;
}

LS::VertexList LS::SetMinus(const LS::VertexList &S0, const LS::VertexList &S1) {
  LS::VertexList Result;
  for (auto *x : S0) {
    if (S1.end() == find(S1.begin(), S1.end(), x)) {
      Result.push_back(x);
    }
  }
  return Result;
}

void LS::printVList(const LS::VertexList &Arg) {
  int Size = Arg.size();
  if (Size == 0) {
    llvm::dbgs() << "NIL";
    return;
  }
  llvm::dbgs() << "(";
  bool First = true;
  for (auto *x : Arg) {
    if (First)
      First = false;
    else
      llvm::dbgs() << " ";
    llvm::dbgs() << x->getName();
  }
  llvm::dbgs() << ")";
}

void LS::printEList(const LS::EdgeList &Arg) {
  int Size = Arg.size();
  if (Size == 0) {
    llvm::dbgs() << "NIL";
    return;
  }
  llvm::dbgs() << "(";
  bool First = true;
  for (const auto &P : Arg) {
    if (First)
      First = false;
    else
      llvm::dbgs() << " ";
    auto *X = P.first;
    auto *Y = P.second;
    llvm::dbgs() << "(" << X->getName() << " " << Y->getName() << ")";
  }
  llvm::dbgs() << ")";
}

bool LS::getEdgeIntInfoValue(LS::EdgeIntInfo &AnyInfo, LS::Vertex *U, LS::Vertex *V,
                         int &Value) {
  if (AnyInfo.end() == AnyInfo.find(U)) {
    return false;
  }
  auto &Tmp = AnyInfo[U];
  for (auto &P : Tmp) {
    auto *X = P.first;
    if (X == V) {
      Value = P.second;
      return true;
    }
  }
  return false;
}

void LS::setEdgeIntInfo(LS::EdgeIntInfo &AnyInfo, LS::Vertex *U, LS::Vertex *V, int Value) {
  if (AnyInfo.end() == AnyInfo.find(U)) {
    std::list<std::pair<LS::Vertex *, int>> Tmp;
    Tmp.emplace_front(V, Value);
    AnyInfo[U] = Tmp;
  } else {
    auto &Tmp = AnyInfo[U];
    for (auto &P : Tmp) {
      auto *X = P.first;
      if (X == V) {
        P.second = Value;
        return;
      }
    }
    Tmp.emplace_front(V, Value);
  }
}

void LS::addEdgeIntInfo(LS::EdgeIntInfo &AnyInfo, LS::Vertex *U, LS::Vertex *V, int Value) {
  assert(AnyInfo.end() != AnyInfo.find(U));
  auto &Tmp = AnyInfo[U];
  for (auto &P : Tmp) {
    auto *X = P.first;
    if (X == V) {
      P.second += Value;
      return;
    }
  }
  assert(false);
}

extern LS::EdgeList LS::getEdgePairList(const LS::VertexList &Vertices, LS::EdgeInfo &Edges) {
  LS::EdgeList Result;
  for (auto *U : Vertices) {
    auto &VList = Edges[U];
    for (auto *V : VList) {
      Result.emplace_back(U, V);
    }
  }
  return Result;
}

LS::Graph::Graph(const llvm::LsDdg& LsDdg, VertexMap& forDelete) {
  const auto &G=LsDdg.getGraph();
  VertexList &Vlist = this->getVertices();
  int id=0;
  for (const auto &V:G.getVertices()) {
    auto* newV=new Vertex(id++);
    forDelete[V]=newV;
    Vlist.push_back(newV);
  }
  for (auto *E:G.getEdges()) {
    DataType Ty=NO_TYPE;
    const auto *Reg=LsDdg.getReg(*E);
    auto Latency=LsDdg.getDelay(*E);
    if (Reg) {
      if (Reg->isIntReg()) Ty=INT_TYPE;
      else if (Reg->isFloatReg()) Ty=FLOAT_TYPE;
      else if (Reg->isPredReg()) Ty=PREDICATE_TYPE;
    }
    this->addEdge(forDelete.at(E->getInitial()), forDelete.at(E->getTerminal()), Ty, Latency);
  }
}
