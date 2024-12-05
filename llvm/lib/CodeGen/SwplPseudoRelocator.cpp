//===- SwplPseudoRelocator.cpp - Pseudo Inst Relocator for SWP ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Pseudo Inst Relocator for SWP
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/InitializePasses.h"


using namespace llvm;

#define DEBUG_TYPE "swpl-relocator"

namespace llvm {

class SwplPseudoRelocator : public MachineFunctionPass {
public:
  static char ID;               ///< PassのID
  SwplPseudoRelocator(): MachineFunctionPass(ID) {
    initializeSwplPseudoRelocatorPass(*PassRegistry::getPassRegistry());
  }
  bool runOnMachineFunction(MachineFunction &mf) override;
};

FunctionPass *createSwplPseudoRelocatorPass() {
    return new SwplPseudoRelocator();
}

}

char SwplPseudoRelocator::ID = 0;


INITIALIZE_PASS_BEGIN(SwplPseudoRelocator, DEBUG_TYPE,
                      "Swpl Pseudo Instr Relocator", false, false)
INITIALIZE_PASS_END(SwplPseudoRelocator, DEBUG_TYPE,
                    "Swpl Pseudo Instr Relocator", false, false)


bool SwplPseudoRelocator::runOnMachineFunction(MachineFunction &mf) {
  if (skipFunction(mf.getFunction()))
    return false;
  const TargetInstrInfo *TII = mf.getSubtarget().getInstrInfo();
  MachineInstr *livein=nullptr;
  MachineInstr *liveout=nullptr;
  for (auto &MBB:mf) {
    if (TII->getSwplPseudoInstr(MBB, livein, liveout)) {
      if (livein) {
        const auto T = MBB.getFirstNonPHI();
        MBB.splice(T, &MBB, livein);
      }
      if (liveout) {
        const auto T = MBB.getFirstTerminator();
        MBB.splice(T, &MBB, liveout);

      }
    }
  }
  return true;
}

