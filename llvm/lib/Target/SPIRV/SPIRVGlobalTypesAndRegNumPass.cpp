//===-- SPIRVGlobalTypesAndRegNum.cpp - Hoist Globals & Number VRegs - C++ ===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass hoists all the required function-local instructions to global
// scope, deduplicating them where necessary.
//
// Before this pass, all SPIR-V instructions were local scope, and registers
// were numbered function-locally. However, SPIR-V requires Type instructions,
// global variables, capabilities, annotations etc. to be in global scope and
// occur at the start of the file. This pass copies these as necessary to dummy
// function which represents the global scope. The original local instructions
// are left to keep MIR consistent. They will not be emitted to asm/object file.
//
// This pass also re-numbers the registers globally, and saves global aliases of
// local registers to the global registry. AsmPrinter uses these aliases to
// output instructions that refers global registers.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVGlobalRegistry.h"
#include "SPIRVSubtarget.h"
#include "SPIRVUtils.h"
#include "TargetInfo/SPIRVTargetInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-global-types-vreg"

namespace {
struct SPIRVGlobalTypesAndRegNum : public ModulePass {
  static char ID;

  SPIRVGlobalTypesAndRegNum() : ModulePass(ID) {
    initializeSPIRVGlobalTypesAndRegNumPass(*PassRegistry::getPassRegistry());
  }

public:
  // Perform passes such as hoisting global instructions and numbering vregs.
  // See end of file for details
  bool runOnModule(Module &M) override;

  // State that we need MachineModuleInfo to operate on MachineFunctions
//  void getAnalysisUsage(AnalysisUsage &AU) const override {
//    AU.addRequired<MachineModuleInfoWrapperPass>();
//  }
};
} // namespace

// Add a meta function containing all OpType, OpConstant etc.
// Extract all OpType, OpConst etc. into this meta block
// Number registers globally, including references to global OpType etc.
bool SPIRVGlobalTypesAndRegNum::runOnModule(Module &M) {
  return false;
}

INITIALIZE_PASS(SPIRVGlobalTypesAndRegNum, DEBUG_TYPE,
                "SPIRV hoist OpType etc. & number VRegs globally", false, false)

char SPIRVGlobalTypesAndRegNum::ID = 0;

ModulePass *llvm::createSPIRVGlobalTypesAndRegNumPass() {
  return new SPIRVGlobalTypesAndRegNum();
}
