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
protected:
  enum RegKindID {unknown=0, IntReg=1, FloatReg=2, PredicateReg=3, CCReg=4};
  unsigned registerClassId = RegKindID::unknown;
  bool allocated = false;

public:
  /// constructor

  StmRegKind():registerClassId(RegKindID::unknown),allocated(false)  {}
  StmRegKind(const StmRegKind&s):registerClassId(s.registerClassId),allocated(s.allocated) {}
  virtual ~StmRegKind() {}

  /// \param id register class id
  /// \param isAllocated  physical register
  StmRegKind(unsigned id, bool isAllocated)
      : registerClassId(id), allocated(isAllocated) {}

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
};

}

#endif
