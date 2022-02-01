//===-- SPIRV.h - Top-level interface for SPIR-V representation -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRV_H
#define LLVM_LIB_TARGET_SPIRV_SPIRV_H

#include "MCTargetDesc/SPIRVMCTargetDesc.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class SPIRVTargetMachine;
class SPIRVRegisterBankInfo;
class SPIRVSubtarget;
class InstructionSelector;

ModulePass *createSPIRVPreTranslationLegalizerPass(SPIRVTargetMachine *TM);
FunctionPass *createSPIRVOCLRegularizerPass();
FunctionPass *createSPIRVBasicBlockDominancePass();
FunctionPass *createSPIRVBlockLabelerPass();
FunctionPass *createSPIRVAddRequirementsPass();
ModulePass *createSPIRVGlobalTypesAndRegNumPass();
ModulePass *createSPIRVLowerConstExprLegacyPass();
MachineFunctionPass *createSPIRVGenerateDecorationsPass();
FunctionPass *createSPIRVPreLegalizerPass();

InstructionSelector *
createSPIRVInstructionSelector(const SPIRVTargetMachine &TM,
                               const SPIRVSubtarget &Subtarget,
                               const SPIRVRegisterBankInfo &RBI);

void initializeSPIRVBasicBlockDominancePass(PassRegistry &);
void initializeSPIRVBlockLabelerPass(PassRegistry &);
void initializeSPIRVAddRequirementsPass(PassRegistry &);
void initializeSPIRVGlobalTypesAndRegNumPass(PassRegistry &);
void initializeSPIRVGenerateDecorationsPass(PassRegistry &);
void initializeSPIRVPreLegalizerPass(PassRegistry &);
} // namespace llvm

#endif // LLVM_LIB_TARGET_SPIRV_SPIRV_H
