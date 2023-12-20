//=- SwplTargetMachine.h - Target Machine Info for SWP --*- C++ -*----=//
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
#ifndef LLVM_CODEGEN_SWPLTARGETMACHINE_H
#define LLVM_CODEGEN_SWPLTARGETMACHINE_H

#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetSchedule.h"

namespace llvm {

/// ReourceID
using StmResourceId=int ;

/// StmPipeline patternID
using StmPatternId=unsigned ;

/// MachineInstr::Opcode
using StmOpcodeId=unsigned ;

/// Represents the resources used by the instruction.
/// ex.：LDNP EAG* / EAG*
/// \dot "Simplified diagram"
/// digraph g {
///  graph [ rankdir = "LR" fontsize=10];
///  node [ fontsize = "10" shape = "record" ];
///  edge [];
///  pipe1  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGA|EAGA}"];
///  pipe2  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGA|EAGB}"];
///  pipe3  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGB|EAGA}"];
///  pipe4  [label = "{pipeline:}|{stage:|1|1}|{resource:|EAGB|EAGB}"];
/// }
/// \enddot
struct StmPipeline {
  SmallVector<unsigned,4> stages;
  SmallVector<StmResourceId,4> resources;
};

/// Handles the register types used by the SWPL function
class StmRegKind {
protected:
  enum RegKindID {unknown=0, IntReg=1, FloatReg=2, PredicateReg=3, CCReg=4};
  unsigned registerClassId = RegKindID::unknown;
  bool allocated = false;
  unsigned unitNum=0;

public:
  /// constructor

  StmRegKind():registerClassId(RegKindID::unknown),allocated(false)  {}
  StmRegKind(const StmRegKind&s):registerClassId(s.registerClassId),allocated(s.allocated),unitNum(s.unitNum) {}
  virtual ~StmRegKind() {}

  /// \param id register class id
  /// \param isAllocated  physical register
  /// \param n unit number
  StmRegKind(unsigned id, bool isAllocated, unsigned n)
      : registerClassId(id), allocated(isAllocated), unitNum(n) {}

  /// check Interger
  /// \retval true Interger
  /// \retval false not Integer
  virtual bool isInteger(void) const {
    return registerClassId == RegKindID::IntReg;
  }
  static bool isInteger(unsigned regclass) {
    return regclass == RegKindID::IntReg;
  }
  static unsigned getIntRegID() {
    return RegKindID::IntReg;
  }

  /// check Floating
  /// \retval true Floating
  /// \retval false not Floating
  virtual bool isFloating(void) const {
    return registerClassId == RegKindID::FloatReg;
  }
  static bool isFloating(unsigned regclass) {
    return regclass == RegKindID::FloatReg;
  }
  static unsigned getFloatRegID() {
    return RegKindID::FloatReg;
  }

  /// check Predicate
  /// \retval true Predicate
  /// \retval false not Predicate
  virtual bool isPredicate(void) const {
    return registerClassId == RegKindID::PredicateReg;
  }
  static bool isPredicate(unsigned regclass) {
    return regclass == RegKindID::PredicateReg;
  }
  static unsigned getPredicateRegID() {
    return RegKindID::PredicateReg;
  }

  /// check CC
  /// \retval true CC
  /// \retval false not CC
  virtual bool isCCRegister(void) const {
    return registerClassId == RegKindID::CCReg;
  }
  static bool isCC(unsigned regclass) {
    return regclass == RegKindID::CCReg;
  }
  static unsigned getCCRegID() {
    return RegKindID::CCReg;
  }

  /// check CC(for compatible)
  /// \retval true CC
  /// \retval false not CC
  virtual bool isIntegerCCRegister(void) const {
    return isCCRegister();
  }

  /// check Allocated
  ///
  /// \retval true allocated
  /// \retval false not allocated
  virtual bool isAllocalted(void) const { return allocated; }

  /// number of regkind
  /// \return bunber of regkind
  virtual int getKindNum(void) const { return 4; }

  virtual int getNumIntReg() const {
    return 32;
  }
  virtual int getNumFloatReg() const {
    return 32;
  }
  virtual int getNumPredicateReg() const {
    return 8;
  }
  virtual int getNumCCReg() const {
    return 1;
  }

  /// num of reg
  /// \return num of reg
  virtual int getNum(void) const {
    if (isInteger()) return getNumIntReg();
    if (isFloating()) return getNumFloatReg();
    if (isPredicate()) return getNumPredicateReg();
    if (isCCRegister()) return getNumCCReg();
    llvm_unreachable("registerClassId is unknown");
  }

  /// print StmRegKind
  /// \param [out] os
  virtual void print(llvm::raw_ostream &os) const {
    switch (registerClassId) {
    case RegKindID::IntReg:
      os << "StmRegKind:IntReg\n";
      break;
    case RegKindID::FloatReg:
      os << "StmRegKind:FloatReg\n";
      break;
    case RegKindID::PredicateReg:
      os << "StmRegKind:FloatReg\n";
      break;
    case RegKindID::CCReg:
      os << "StmRegKind:CCReg\n";
      break;
    default:
      os << "StmRegKind:unknown\n";
      break;
    }
  }

  /// same regkind?
  /// \param [in] kind2 target
  /// \retval true same
  /// \retval false not same
  virtual bool isSameKind(unsigned id) const {
    return registerClassId == id;
  }
  /// same regkind?
  /// \param [in] kind1 target1
  /// \param [in] kind2 target2
  /// \retval true same
  /// \retval false not same
  static bool isSameKind(const StmRegKind &kind1,
                         const StmRegKind &kind2) {
    return kind1.registerClassId == kind2.registerClassId;
  }
  /// same regkind?
  /// \param [in] kind1 target1
  /// \param [in] regclassid target2
  /// \retval true same
  /// \retval false not same
  static bool isSameKind(const StmRegKind &kind1, RegKindID regclassid) {
    return kind1.registerClassId == regclassid;
  }
  /// return unit number
  unsigned getUnitNum() {return unitNum;}
};

/// list of StmPipeline(for argument)
using StmPipelinesImpl =std::vector<const StmPipeline *>;

/// list of StmPipeline(for var)
using StmPipelines =std::vector<const StmPipeline *>;

/// Target machine information used by the SWPL function
class SwplTargetMachine {
protected:
  const MachineFunction *MF=nullptr; ///< Remember the MachineFunction to get the information needed by SwplTargetMachine
  TargetSchedModel SM;         ///< SchedModel

public:
  /// constructor
  SwplTargetMachine() {}
  /// destructor
  virtual ~SwplTargetMachine() {
  }

  /// Initialize the SwpltargetMachine object.
  /// \details
  /// Each time runOnFunction is called, it is necessary to execute initialize() and
  /// pass MachineFunction information to be processed.
  /// \param mf Target MachineFunction
  virtual void initialize(const MachineFunction &mf) = 0;

  /// Calculate flow-dependent latency.
  /// \param [in] def
  /// \param [in] use
  /// \return latency
  virtual int computeRegFlowDependence(const MachineInstr* def, const MachineInstr* use) const = 0;

  /// Calculate store->load latency.
  /// \param [in] def store inst.
  /// \param [in] use load inst.
  /// \return latency
  virtual int computeMemFlowDependence(const MachineInstr* def, const MachineInstr* use) const = 0;

  /// Calculate load->store latency.
  /// \param [in] use load inst.
  /// \param [in] def store inst.
  /// \return latency
  virtual int computeMemAntiDependence(const MachineInstr* use, const MachineInstr* def) const = 0;

  /// Calculate store->store latency.
  /// \param [in] def1 store inst.
  /// \param [in] def2 store inst.
  /// \return latency
  virtual int computeMemOutputDependence(const MachineInstr* def1, const MachineInstr* def2) const = 0;

  /// Returns all resource usage patterns used by the specified instruction.
  /// \param [in] mi Target instruction
  /// \return StmPipelines
  virtual const StmPipelinesImpl * getPipelines(const MachineInstr& mi) const = 0;

  /// returns the number of resources available
  /// \return number of resources
  virtual unsigned int getNumResource(void) const = 0;

  /// return the name of the ResourceId
  /// \param [in] resource
  /// \return name
  virtual const char* getResourceName(StmResourceId resource) const = 0;

  /// print pipeline
  /// \param [in] ost
  /// \param [in] pipeline
  virtual void print(llvm::raw_ostream &ost, const StmPipeline &pipeline) const = 0;

  /// Determine if an instruction is Pseudo
  /// \details Judge as Pseudo only if instruction is not defined in SchedModel
  /// \param [in] mi instruction
  /// \retval true Psedo
  /// \retval false not Pseudo
  virtual bool isPseudo(const MachineInstr& mi) const = 0;

  /// Get instruction-type-ID from instruction
  /// \param [in] mi Target instruction
  /// \return instruction-type-ID (ex. AArch64SwplSchedA64FX::INT_OP)
  virtual unsigned getInstType(const MachineInstr &mi) const = 0;

  /// Returns a string corresponding to instruction-type-ID.
  /// \param [in] insttypeid instruction-type-ID
  /// \return string corresponding to instruction-type-ID
  virtual const char* getInstTypeString(unsigned insttypeid) const = 0;

  /// Calculate penalty by instruction type and dependent register.
  /// \param [in] prod MI of producer instruction
  /// \param [in] cons MI of consumer instruction
  /// \param [in] regs depend reg
  /// \return penalty by instruction type and dependent register
  virtual unsigned calcPenaltyByInsttypeAndDependreg(const MachineInstr& prod, const MachineInstr& cons,
                                                     const llvm::Register& reg) const = 0;

  /// Determine if the register allocation has invalidated.
  /// \retval true Invalid the register allocation in the swpl pass.
  /// \retval false Valid the register allocation in the swpl pass.
  virtual bool isDisableRegAlloc(void) const = 0;

  /// Determine if the register allocation has validated.
  /// \retval true Valid the register allocation in the swpl pass.
  /// \retval false Invalid the register allocation in the swpl pass.
  virtual bool isEnableRegAlloc(void) const = 0;
};



}

#endif
