//===-- SPIRVTargetInfo.cpp - SPIR-V Target Implementation ----*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/SPIRVTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheSPIRV32Target() {
  static Target TheSPIRV32Target;
  return TheSPIRV32Target;
}
Target &llvm::getTheSPIRV64Target() {
  static Target TheSPIRV64Target;
  return TheSPIRV64Target;
}
Target &llvm::getTheSPIRVLogicalTarget() {
  static Target TheSPIRVLogicalTarget;
  return TheSPIRVLogicalTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSPIRVTargetInfo() {
  RegisterTarget<Triple::spirv32> X(getTheSPIRV32Target(), "spirv32",
                                    "SPIR-V 32-bit", "SPIRV");
  RegisterTarget<Triple::spirv64> Y(getTheSPIRV64Target(), "spirv64",
                                    "SPIR-V 64-bit", "SPIRV");
  RegisterTarget<Triple::spirvlogical> Z(
      getTheSPIRVLogicalTarget(), "spirvlogical", "SPIR-V Logical", "SPIRV");
}
