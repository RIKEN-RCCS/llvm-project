//=- LS_DDG_change_for_regs.h - SWPL(Local Scheduler) Pass -*- c++ -*--------=//
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

#include <algorithm>
#include <cassert>
#include <list>
#include <string>
#include <unordered_map>
#include "llvm/ADT/StringRef.h"

namespace llvm {
class MachineInstr;
class LsDdg;
} // namespace llvm

namespace LS {

class Vertex;

enum DataType { FLOAT_TYPE, INT_TYPE, PREDICATE_TYPE, NO_TYPE };

using VertexList = std::list<Vertex *>;
using EdgeList = std::list<std::pair<Vertex *, Vertex *>>;

using VertexInfo = std::unordered_map<Vertex *, int>;
using EdgeInfo = std::unordered_map<Vertex *, std::list<Vertex *>>;
using EdgeIntInfo =
    std::unordered_map<Vertex *, std::list<std::pair<Vertex *, int>>>;
using EdgeTypeInfo =
    std::unordered_map<Vertex *, std::list<std::pair<Vertex *, DataType>>>;

using VtoV = std::unordered_map<Vertex *, Vertex *>;
using IntToV = std::unordered_map<int, Vertex *>;
using IntToV = std::unordered_map<int, Vertex *>;
using IntToVList = std::unordered_map<int, VertexList>;


extern bool isMember(const EdgeList &Edges, Vertex *U, Vertex *V);

extern VertexList SetAnd(const VertexList &S0, const VertexList &S1);
extern VertexList SetMinus(const VertexList &S0, const VertexList &S1);
extern VertexList SetUnion(const VertexList &S0, const VertexList &S1);
extern bool SetEqual(const VertexList &S0, const VertexList &S1);

extern void printVList(const VertexList &Arg);
extern void printEList(const EdgeList &Arg);

extern void setEdgeIntInfo(EdgeIntInfo &AnyInfo, Vertex *U, Vertex *V,
                           int Value);
extern bool getEdgeIntInfoValue(EdgeIntInfo &AnyInfo, Vertex *U, Vertex *V,
                                int &Value);
extern void addEdgeIntInfo(EdgeIntInfo &AnyInfo, Vertex *U, Vertex *V,
                           int Value);

extern EdgeList getEdgePairList(const VertexList &Vertices, EdgeInfo &Edges);

class SimpleGraph {
public:
  explicit SimpleGraph(const VertexList &V, const EdgeInfo &E)
      : Vertices(V), Edges(E) {}

  SimpleGraph &operator=(const SimpleGraph &) = delete;

  // SimpleGraph(const SimpleGraph&) = delete;

  SimpleGraph &operator=(SimpleGraph &) = delete;

  // SimpleGraph(SimpleGraph&) = delete;

  const VertexList &getVertices() const { return Vertices; }

  EdgeInfo &getEdges() { return Edges; }

  VertexList getGammaMinus(Vertex *V) {
    VertexList Result;
    for (Vertex *X : Vertices) {
      VertexList &YList = Edges[X];
      if (YList.end() != find(YList.begin(), YList.end(), V))
        Result.push_back(X);
    }
    return Result;
  }

  VertexList &getGammaPlus(Vertex *U) { return Edges[U]; }

  void calculateMAk(IntToVList &GroupNumberToVList);

  void show(llvm::StringRef Tag);

private:
  VertexList Vertices;
  EdgeInfo Edges;
};

class Vertex {
public:
  explicit Vertex(llvm::StringRef n) : Name(n) {}

  explicit Vertex(int id) : Name(std::to_string(id)) {}

  Vertex &operator=(const Vertex &) = delete;

  Vertex(const Vertex &) = delete;

  Vertex &operator=(Vertex &) = delete;

  Vertex(Vertex &) = delete;

  const std::string &getName() const { return Name; }
  void set(const llvm::MachineInstr *mi) {MI=mi;}
  const llvm::MachineInstr* get() {return MI;}

private:
  std::string Name;
  const llvm::MachineInstr *MI=nullptr;
};


class BipartieGraph {
public:
  explicit BipartieGraph(VertexList &ArgScb, VertexList &ArgTcb,
                         EdgeList &ArgEcb) {
    Scb = ArgScb;
    Tcb = ArgTcb;
    Ecb = ArgEcb;
  }

  BipartieGraph &operator=(const BipartieGraph &) = delete;

  // BipartieGraph(const BipartieGraph&) = delete;

  BipartieGraph &operator=(BipartieGraph &) = delete;

  // BipartieGraph(BipartieGraph&) = delete;

  const VertexList &getScb() const { return Scb; }

  const VertexList &getTcb() const { return Tcb; }

  const EdgeList &getEcb() const { return Ecb; }

  VertexList getGammaMinus(Vertex *V) const {
    VertexList Result;
    for (const std::pair<Vertex *, Vertex *> &P : Ecb) {
      Vertex *X = P.first;
      Vertex *Y = P.second;
      ;
      if (Y == V) {
        if (Result.end() == find(Result.begin(), Result.end(), X)) {
          Result.push_back(X);
        }
      }
    }
    assert(Result.size() > 0);
    return Result;
  }

private:
  VertexList Scb;
  VertexList Tcb;
  EdgeList Ecb;
};

class Graph {
public:
  // Graph& operator=(const Graph&) = delete;

  // Graph(const Graph&) = delete;

  // Graph& operator=(Graph&) = delete;

  // Graph(Graph&) = delete;

  Graph()=default;
  virtual ~Graph()=default;

  explicit Graph(const llvm::LsDdg& LsDdg);

  void addEdge(Vertex *U, Vertex *V, DataType Dt, int Latency) {
    Edges[U].push_front(V);
    EdgeType[U].emplace_front(V, Dt);
    Delta[U].emplace_front(V, Latency);
  }

  VertexList &getVertices() { return Vertices; }

  EdgeInfo &getEdges() { return Edges; }

  VertexList getVerticesWithType(DataType Dt) {
    VertexList Result;
    for (Vertex *V : Vertices) {
      std::list<std::pair<Vertex *, DataType>> &TmpList = EdgeType[V];
      for (std::pair<Vertex *, DataType> &P : TmpList) {
        if (P.second == Dt) {
          Result.push_back(V);
          break;
        }
      }
    }
    return Result;
  }

  EdgeIntInfo &getDelta() { return Delta; }

  bool isVertex(Vertex *V) const {
    return (Vertices.end() != find(Vertices.begin(), Vertices.end(), V));
  }

  bool isEdge(Vertex *U, Vertex *V) {
    VertexList &VList = Edges[U];
    return (VList.end() != find(VList.begin(), VList.end(), V));
  }

  bool isEdgeType(Vertex *U, Vertex *V, DataType Dt) {
    auto &TmpList = EdgeType[U];
    for (auto &P : TmpList) {
      if (P.first == V && P.second == Dt) {
        return true;
      }
    }
    return false;
  }

  int getLatency(Vertex *U, Vertex *V) {
    int MaxLatency = -1;
    auto &TmpList = Delta[U];
    for (auto &P : TmpList) {
      int Latency = P.second;
      if (P.first == V)
        if (MaxLatency == -1 || MaxLatency < Latency)
          MaxLatency = Latency;
    }
    assert(0 <= MaxLatency);
    return MaxLatency;
  }

  bool isDescendant(Vertex *U, Vertex *V) {
    VertexList &&DaList = getDownArrow(U);
    return (DaList.end() != find(DaList.begin(), DaList.end(), V));
  }

  bool isGroupDescendant(VertexList &UList, VertexList &VList) {
    for (Vertex *U : UList) {
      for (Vertex *V : VList) {
        if (isDescendant(U, V))
          return true;
      }
    }
    return false;
  }

  bool generateCyclicPath(Vertex *U, Vertex *V) {
    VertexList &&DaList = getDownArrow(V);
    return (DaList.end() != find(DaList.begin(), DaList.end(), U));
  }

  VertexList getGammaMinus(Vertex *V) {
    VertexList Result;
    for (Vertex *X : Vertices) {
      VertexList &YList = Edges[X];
      if (YList.end() != find(YList.begin(), YList.end(), V))
        Result.push_back(X);
    }
    return Result;
  }

  VertexList &getGammaPlus(Vertex *U) { return Edges[U]; }

  VertexList getConsumer(DataType Dt, Vertex *U) {
    VertexList Result;
    VertexList &VList = Edges[U];
    for (Vertex *V : VList) {
      if (isEdgeType(U, V, Dt))
        if (Result.end() == find(Result.begin(), Result.end(), V))
          Result.push_back(V);
    }
    return Result;
  }

  VertexList getDownArrow(Vertex *V) {
    VertexInfo Ht;
    VertexList WorkList;
    VertexList Result;
    WorkList.push_front(V);
    Result.push_front(V);
    Ht[V] = 1;
    for (;;) {
      VertexList NextWorkList;
      for (Vertex *X : WorkList) {
        VertexList &YList = getGammaPlus(X);
        for (Vertex *Y : YList) {
          if (Ht.end() == Ht.find(Y)) {
            Ht[Y] = 1;
            NextWorkList.push_front(Y);
            Result.push_front(Y);
          }
        }
      }
      if (NextWorkList.empty())
        break;
      WorkList = NextWorkList;
    }
    return Result;
  }

  VertexList getDownArrowWithType(DataType Dt, Vertex *V) {
    VertexList &&Tmp1 = getVerticesWithType(Dt);
    VertexList &&Tmp2 = getDownArrow(V);
    VertexList Result;
    for (Vertex *X : Tmp2) {
      if (Tmp1.end() != find(Tmp1.begin(), Tmp1.end(), X))
        Result.push_front(X);
    }
    return Result;
  }

  VertexList getPKill(DataType Dt, Vertex *U) {
    VertexList &&Tmp0 = getConsumer(Dt, U);
    VertexList Result;
    for (Vertex *V : Tmp0) {
      VertexList &&Tmp1 = getDownArrow(V);
      VertexList &&Tmp2 = SetAnd(Tmp0, Tmp1);
      if (Tmp2.size() == 1 && V == Tmp2.front())
        Result.push_front(V);
    }
    return Result;
  }

  void show(llvm::StringRef Tag);
  

  SimpleGraph makePK(DataType Dt) {
    EdgeInfo EPK;
    for (Vertex *U : Vertices) {
      VertexList &VList = Edges[U];
      for (Vertex *V : VList) {
        VertexList &&XList = getPKill(Dt, U);
        if (XList.end() != find(XList.begin(), XList.end(), V))
          EPK[U].push_front(V);
      }
    }
    SimpleGraph PK(Vertices, EPK);
    return PK;
  }

  void bipartieDecomposition(DataType Dt, SimpleGraph &PK,
                             std::list<BipartieGraph> &BG) {
    VertexList &&VR = getVerticesWithType(Dt);
    EdgeList &&ListArcs = getEdgePairList(PK.getVertices(), PK.getEdges());
    std::unordered_map<Vertex *, bool> Visited;
    for (Vertex *U : VR) {
      Visited[U] = false;
    }
    for (Vertex *U : VR) {
      if (!Visited[U]) {
        VertexList Scb;
        Scb.push_front(U);
        EdgeList Ecb;
        VertexList &Tcb = PK.getGammaPlus(U);
        VertexList SS;
        VertexList TT;
        for (;;) {
          if (SetEqual(SS, Scb) || SetEqual(TT, Tcb))
            break;
          SS = Scb;
          TT = Tcb;
          for (Vertex *Tx : Tcb) {
            VertexList &&Tmp = PK.getGammaMinus(Tx);
            Scb = SetUnion(Scb, Tmp);
          }
          for (Vertex *Sx : Scb) {
            VertexList &Tmp = PK.getGammaPlus(Sx);
            Tcb = SetUnion(Tcb, Tmp);
          }
        }
        for (Vertex *Sx : Scb) {
          Visited[Sx] = true;
          if (Tcb.end() != find(Tcb.begin(), Tcb.end(), Sx))
            Tcb.remove(Sx);
        }
        EdgeList Next;
        for (std::pair<Vertex *, Vertex *> &P : ListArcs) {
          Vertex *U = P.first;
          Vertex *V = P.second;
          if ((Scb.end() != find(Scb.begin(), Scb.end(), U)) &&
              (Tcb.end() != find(Tcb.begin(), Tcb.end(), V))) {
            Ecb.push_front(P);
          } else {
            Next.push_back(P);
          }
        }
        ListArcs = Next;
        BipartieGraph CB = BipartieGraph(Scb, Tcb, Ecb);
        BG.push_front(CB);
      }
    }
  }

  Graph makeGk(DataType Dt, VtoV &KStar) {
    Graph Gk = *this;
    EdgeInfo Ek;
    EdgeIntInfo NewDelta;
    EdgeTypeInfo NewEdgeType;
    makeEkAndDelta(Dt, KStar, Ek, NewDelta, NewEdgeType);
    for (Vertex *U : Vertices) {
      VertexList &VList = Ek[U];
      for (Vertex *V : VList) {
        Gk.Edges[U].push_front(V);
      }
    }
    for (Vertex *V : Vertices) {
      if (NewDelta.end() != NewDelta.find(V)) {
        std::list<std::pair<Vertex *, int>> &Tmp0 = NewDelta[V];
        std::list<std::pair<Vertex *, int>> &Tmp1 = Gk.Delta[V];
        for (std::pair<Vertex *, int> X : Tmp0) {
          Tmp1.push_front(X);
        }
      }
      if (NewEdgeType.end() != NewEdgeType.find(V)) {
        auto &Tmp0 = NewEdgeType[V];
        auto &Tmp1 = Gk.EdgeType[V];
        for (auto X : Tmp0) {
          Tmp1.push_front(X);
        }
      }
    }
    return Gk;
  }

  void makeEkAndDelta(DataType Dt, VtoV &KStar, EdgeInfo &Ek,
                      EdgeIntInfo &NewDelta, EdgeTypeInfo &NewEdgeType) {
    VertexList &&VR = getVerticesWithType(Dt);
    for (Vertex *U : VR) {
      Vertex *Ku = KStar[U];
      if (Ku != 0) {
        VertexList &&VList = getPKill(Dt, U);
        assert(VList.end() != find(VList.begin(), VList.end(), Ku));
        for (Vertex *V : VList) {
          if (V != Ku) {
            VertexList NList = Ek[V];
            if (NList.end() == find(NList.begin(), NList.end(), Ku))
              Ek[V].push_front(Ku);
          }
        }
      }
    }
    for (Vertex *X : Vertices) {
      VertexList YList = Ek[X];
      for (Vertex *Y : YList) {
        NewDelta[X].emplace_front(Y, 1);
        NewEdgeType[X].emplace_front(Y, NO_TYPE);
      }
    }
  }

  void greedyK(DataType Dt, VtoV &KStar);

  SimpleGraph makeDV(DataType Dt, VtoV &K) {
    VertexList &&VList = getVerticesWithType(Dt);
    EdgeInfo EDV;
    for (Vertex *U : VList) {
      Vertex *Ku = K[U];
      VertexList &&DList = getDownArrowWithType(Dt, Ku);
      for (Vertex *V : DList) {
        if (VList.end() != find(VList.begin(), VList.end(), V)) {
          VertexList NList = EDV[U];
          if (NList.end() == find(NList.begin(), NList.end(), V)) {
            EDV[U].push_front(V);
          }
        }
      }
    }
    SimpleGraph GDV(VList, EDV);
    return GDV;
  }

  SimpleGraph makeDVWithK(DataType Dt) {
    VertexList &&VList = getVerticesWithType(Dt);
    EdgeInfo EDV;
    for (Vertex *U : VList) {
      VertexList &&KList = getPKill(Dt, U);
      assert(KList.size() == 1);
      Vertex *Ku = KList.front();
      VertexList &&DList = getDownArrowWithType(Dt, Ku);
      for (Vertex *V : DList) {
        if (VList.end() != find(VList.begin(), VList.end(), V)) {
          VertexList NList = EDV[U];
          if (NList.end() == find(NList.begin(), NList.end(), V)) {
            EDV[U].push_front(V);
          }
        }
      }
    }
    SimpleGraph GDV(VList, EDV);
    return GDV;
  }

  void calculateMAk0(DataType, VtoV &, IntToVList &);

  int getCriticalPathLength(VertexInfo &);

  int updateCriticalPathLength(VertexInfo &, Vertex *);

  int getCriticalPathLengthIf(VertexList &, Vertex *, int, const VertexInfo &);

  void serializePair(DataType, Vertex *, Vertex *, EdgeList &);

  bool serialize(DataType, VtoV &, int, EdgeList &);

private:
  VertexList Vertices;
  EdgeInfo Edges;
  EdgeTypeInfo EdgeType;
  EdgeIntInfo Delta;
};



class MaximumFlow {
public:
  explicit MaximumFlow(VertexList &V, EdgeInfo &E) : Vertices(V), Edges(E) {}

  MaximumFlow &operator=(const MaximumFlow &) = delete;

  MaximumFlow(const MaximumFlow &) = delete;

  MaximumFlow &operator=(MaximumFlow &) = delete;

  MaximumFlow(MaximumFlow &) = delete;

  VertexList &getVertices() { return Vertices; }

  EdgeInfo &getEdges() { return Edges; }

  bool isEdge(Vertex *U, Vertex *V) {
    VertexList &VList = Edges[U];
    return (VList.end() != find(VList.begin(), VList.end(), V));
  }

  void initializePreflow(EdgeIntInfo &Capacity, Vertex *StartNode,
                         VertexInfo &HInfo, VertexInfo &EInfo,
                         EdgeIntInfo &FInfo);

  void initResidualCapacity(EdgeIntInfo &Capacity, EdgeIntInfo &FInfo,
                            EdgeIntInfo &ResidualCapacity);

  Vertex *pushWorkCondition(EdgeIntInfo &CapacityF, VertexInfo &HInfo,
                            Vertex *U);

  void relabelWork(EdgeIntInfo &CapacityF, VertexInfo &HInfo, VertexInfo &EInfo,
                   Vertex *U);

  void pushWork(EdgeIntInfo &Capacity, EdgeIntInfo &ResidualCapacity,
                VertexInfo &HInfo, VertexInfo &EInfo, EdgeIntInfo &FInfo,
                Vertex *U, Vertex *V);

  void genericPushRelabel(EdgeIntInfo &Capacity, Vertex *StartNode,
                          Vertex *EndNode, EdgeIntInfo &FInfo);

private:
  VertexList Vertices;
  EdgeInfo Edges;
};



} // namespace LS
