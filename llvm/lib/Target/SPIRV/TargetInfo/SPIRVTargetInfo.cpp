//===-- SPIRVTargetInfo.cpp - SPIR-V Target Implementation ----*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "llvm/Support/TargetRegistry.h"

namespace llvm {
Target &getTheSPIRV32Target() {
  static Target TheSPIRVTarget;
  return TheSPIRVTarget;
}
Target &getTheSPIRV64Target() {
  static Target TheSPIRVTarget;
  return TheSPIRVTarget;
}
Target &getTheSPIRVLogicalTarget() {
  static Target TheSPIRVTarget;
  return TheSPIRVTarget;
}

extern "C" void LLVMInitializeSPIRVTargetInfo() {
  RegisterTarget<Triple::spirv32, /*HasJIT=*/false> X(
      getTheSPIRV32Target(), "spirv32", "SPIRV", "SPIRV");
  RegisterTarget<Triple::spirv64, /*HasJIT=*/false> Y(
      getTheSPIRV64Target(), "spirv64", "SPIRV", "SPIRV");
  RegisterTarget<Triple::spirvlogical, /*HasJIT=*/false> Z(
      getTheSPIRVLogicalTarget(), "spirvlogical", "SPIRV", "SPIRV");
}
} // namespace llvm
