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

namespace llvm {

class StmRegKind {
public:
private:
  enum RegKindID {unknown=0, IntReg=1, FloatReg=2, PredicateReg=3, CCReg=4};
  unsigned registerClassId = RegKindID::unknown;
  bool allocated = false;

public:
  /// constructor
  /// \param id register class id
  /// \param isAllocated  physical register
  StmRegKind(unsigned id, bool isAllocated)
      : registerClassId(id), allocated(isAllocated) {}

  /// check Interger
  /// \retval true Interger
  /// \retval false not Integer
  bool isInteger(void) {
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
  bool isFloating(void) {
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
  bool isPredicate(void) {
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
  bool isCCRegister(void) {
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
  bool isIntegerCCRegister(void) {
    return registerClassId == RegKindID::CCReg;
  }

  /// check Allocated
  ///
  /// \retval true allocated
  /// \retval false not allocated
  bool isAllocalted(void) { return allocated; }

  /// number of regkind
  /// \return bunber of regkind
  static int getKindNum(void) { return 4; }

  static int getNumIntReg() {
    return 31;
  }
  static int getNumFloatReg() {
    return 32;
  }
  static int getNumPredicateReg() {
    return 3;
  }

  /// num of reg
  /// \return num of reg
  int getNum(void) {
    switch (registerClassId) {
    case RegKindID::IntReg:
      return getNumIntReg();
    case RegKindID::FloatReg:
      return getNumFloatReg();
    case RegKindID::PredicateReg:
      return getNumPredicateReg();
    case RegKindID::CCReg:
      return 1;
    default:
      llvm_unreachable("registerClassId is unknown");
    }
  }

  /// print StmRegKind
  /// \param [out] os
  void print(llvm::raw_ostream &os) {
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
};

}

#endif
