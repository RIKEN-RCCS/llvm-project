//=- AArch64SwplExpandPseudo.cpp - Expand software pipelining pseudo instructions -=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Expand software pipelining pseudo instructions pass.
//
//===----------------------------------------------------------------------===//
#include "AArch64.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpl-expand-pseudo"

namespace {
class AArch64SwplExpandPseudo : public MachineFunctionPass {

public:
  static char ID; ///< Pass ID
  
  /**
   * \brief Constructor of AArch64SwplExpandPseudo
   */
  AArch64SwplExpandPseudo() : MachineFunctionPass(ID) {
    initializeAArch64SwplExpandPseudoPass(
        *PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  /**
   * \brief Get Pass Name
   *
   * Specify the name of the Pass to be displayed, 
   * for example, when using the -debug-pass option.
   */
  StringRef getPassName() const override {
    return "AArch64 swpl pseudo instruction expansion pass";
  }

private:
  bool expandMF(MachineFunction &MF);
};
char AArch64SwplExpandPseudo::ID = 0;
}

INITIALIZE_PASS(AArch64SwplExpandPseudo, DEBUG_TYPE,
                "AArch64 swpl pseudo instruction expansion pass", false, false)

/**
 * \brief runOnMachineFunction
 *
 * \param MF MachineFunction
 * \retval true  MF is changed
 * \retval false MF not changed
 */
bool AArch64SwplExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;
  Changed = expandMF(MF);
  return Changed;
}

/**
 * \brief expandMF
 *
 * \param MF MachineFunction
 * \retval true  MF MF is changed
 * \retval false MF MF not changed
 */
bool AArch64SwplExpandPseudo::expandMF(MachineFunction &MF) {
  bool Changed = false;
  SmallVector<MachineInstr*, 100> eraseInstrs;

  for (auto &MBB:MF) {
    for (auto &MI:MBB) {
      auto op = MI.getOpcode(); 
      if (op == AArch64::SWPLIVEIN) {
        eraseInstrs.push_back(&MI);
        auto e = MI.getNumOperands();
        for (size_t i = 0; i < e; i++) {
          Register LiveinReg = MI.getOperand(i).getReg();
          /// After register allocation, 
          /// liveins are used to represent registers that span the MBB.
          if (!MBB.isLiveIn(LiveinReg))
            MBB.addLiveIn(LiveinReg);
        }
      } else if (op == AArch64::SWPLIVEOUT){
        eraseInstrs.push_back(&MI);
      }
    }
  }
  for (auto *MI:eraseInstrs) {
    MI->eraseFromParent();
    Changed = true;
  }

  return Changed;
}


/**
 * \brief Create AArch64SwplExpandPseudo
 *
 * \retval FunctionPass Created AArch64SwplExpandPseudo
 */
FunctionPass *llvm::createAArch64SwplExpandPseudoPass() {
  return new AArch64SwplExpandPseudo();
}
