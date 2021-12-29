//===-- SPIRVOpenCLBIFs.h - SPIR-V OpenCL Built-In-Functions ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Functions for lowering OpenCL builtin types and function calls using their
// demangled names
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVOPENCLBIFS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVOPENCLBIFS_H

#include "SPIRVGlobalRegistry.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

namespace AQ = AccessQualifier;

namespace llvm {
bool generateOpenCLBuiltinCall(const StringRef demangledName,
                               MachineIRBuilder &MIRBuilder, Register OrigRet,
                               const Type *OrigRetTy,
                               const SmallVectorImpl<Register> &args,
                               SPIRVGlobalRegistry *GR);
bool getSpirvBuilInIdByName(StringRef Name, BuiltIn::BuiltIn &BI);
} // end namespace llvm
#endif
