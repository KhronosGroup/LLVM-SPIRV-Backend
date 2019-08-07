//===-- SPIRVBasicBlockDominance.cpp - Sort basic blocks --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Before lowering Basic Blocks to MBBs, sort them so that blocks appear
// before all blocks they dominate.
//
// This is required by the SPIR-V 1.4 unified spec section 2.16.1 - Universal
// Validation Rules, which states that for a control flow graph (CFG) to be
// valid "The order of blocks in a function must satisfy the rule that blocks
// appear before all blocks they dominate". Also, spirv-val will reject SPIR-V
// files that do not follow this rule.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "llvm/IR/Dominators.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-block-dominance"

namespace {
class SPIRVBasicBlockDominance : public FunctionPass {
public:
  static char ID;
  SPIRVBasicBlockDominance() : FunctionPass(ID) {
    initializeSPIRVBlockLabelerPass(*PassRegistry::getPassRegistry());
  }
  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    FunctionPass::getAnalysisUsage(AU);
  }
};
} // namespace

bool SPIRVBasicBlockDominance::runOnFunction(Function &F) {
  DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  // Build up a list of blocks in an order such that they satisfy the SPIR-V
  // dominance constraint that "blocks appear before all blocks they dominate".
  //
  // Note that this requires n(n-1)/2 = O(n^2) comparisons. We could use fewer
  // comparisons by iterating over the dominator tree using depth_first from
  // llvm/ADT/DepthFirstIterator.h. However, this often results in unnecessary
  // re-ordering of basic blocks that already satisfied the necessary dominance
  // constraints. We chose to the O(n^2) implementation here to maintain the
  // natural basic block ordering, and avoid any PassManager state changes that
  // would be required if we unnecessarily shuffled the basic blocks.
  SmallVector<BasicBlock *, 16> newBBOrder;
  bool changedOrder = false;
  for (auto &BB : F) {
    bool inserted = false;
    // Check all previously inserted blocks for dominance
    for (unsigned i = 0; i < newBBOrder.size(); ++i) {
      if (DT.dominates(&BB, newBBOrder[i])) { // This should be quite rare
        newBBOrder.insert(newBBOrder.begin() + i, &BB);
        inserted = true;
        changedOrder = true;
        break;
      }
    }
    // If we didn't need to insert the BB earlier to satisfy dominance rules, it
    // can be added to the end to preserve the order it was in previously.
    if (!inserted) {
      newBBOrder.push_back(&BB);
    }
  }
  // If we had to re-order anything, rebuild the basic block list
  if (changedOrder) {
    for (auto BB : newBBOrder) {
      BB->removeFromParent();
      F.getBasicBlockList().push_back(BB);
    }
  }
  // Only return true if we had to re-order the blocks
  return changedOrder;
}

INITIALIZE_PASS(SPIRVBasicBlockDominance, DEBUG_TYPE,
                "SPIRV sort basic blocks by dominance", false, false)

char SPIRVBasicBlockDominance::ID = 0;

FunctionPass *llvm::createSPIRVBasicBlockDominancePass() {
  return new SPIRVBasicBlockDominance();
}
