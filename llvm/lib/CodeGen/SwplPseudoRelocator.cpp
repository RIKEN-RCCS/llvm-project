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
#include "llvm/CodeGen/SlotIndexes.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/TargetPassConfig.h"


using namespace llvm;

#define DEBUG_TYPE "swpl-relocator"

namespace llvm {

class SwplPseudoRelocator : public MachineFunctionPass {
public:
  static char ID;               ///< PassID
  SwplPseudoRelocator(): MachineFunctionPass(ID) {
    initializeSwplPseudoRelocatorPass(*PassRegistry::getPassRegistry());
  }
  bool runOnMachineFunction(MachineFunction &mf) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

};

void SwplPseudoRelocator::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<SlotIndexes>();
  AU.addPreserved<SlotIndexes>();
  AU.addRequired<LiveIntervals>();
  AU.addPreserved<LiveIntervals>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

FunctionPass *createSwplPseudoRelocatorPass() {
    return new SwplPseudoRelocator();
}

}

char SwplPseudoRelocator::ID = 0;

INITIALIZE_PASS(SwplPseudoRelocator, DEBUG_TYPE,
                      "Swpl Pseudo Instr Relocator", false, false)

bool SwplPseudoRelocator::runOnMachineFunction(MachineFunction &mf) {
  bool rc=false;
  if (skipFunction(mf.getFunction()))
    return false;
  const TargetInstrInfo *TII = mf.getSubtarget().getInstrInfo();
  LiveIntervals *LIS = &getAnalysis<LiveIntervals>();
  for (auto &MBB:mf) {
    MachineInstr *livein=nullptr;
    MachineInstr *liveout=nullptr;
    if (TII->getSwplPseudoInstr(MBB, livein, liveout)) {
      rc=true;
      if (livein) {
        const auto T = MBB.getFirstNonPHI();
        MBB.splice(T, &MBB, livein);
        // Update LiveIntervals
        if (LIS)
          LIS->handleMove(*livein, /*UpdateFlags=*/true);
      }
      if (liveout) {
        const auto T = MBB.getFirstTerminator();
        MBB.splice(T, &MBB, liveout);
        // Update LiveIntervals
        if (LIS)
          LIS->handleMove(*liveout, /*UpdateFlags=*/true);
      }
    }
  }
  return rc;
}

