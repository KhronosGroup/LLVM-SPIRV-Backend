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

#include "SPIRVTypeRegistry.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

namespace AQ = AccessQualifier;

namespace llvm {
bool handleConvert(const StringRef demangledName, MachineIRBuilder &MIRBuilder,
    Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleReadImage(const StringRef demangledName,
    MachineIRBuilder &MIRBuilder, Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleAtomic(const StringRef demangledName, MachineIRBuilder &MIRBuilder,
    Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleBarrier(const StringRef demangledName, MachineIRBuilder &MIRBuilder,
    Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleDot(const StringRef demangledName, MachineIRBuilder &MIRBuilder,
    Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);
bool handleLocal(const StringRef demangledName, MachineIRBuilder &MIRBuilder,
    Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleGlobal(const StringRef demangledName, MachineIRBuilder &MIRBuilder,
    Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleWorkGroup(const StringRef demangledName,
    MachineIRBuilder &MIRBuilder, Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleImageQuery(const StringRef demangledName,
    MachineIRBuilder &MIRBuilder, Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool handleSamplerLiteral(const StringRef demangledName,
    MachineIRBuilder &MIRBuilder, Register OrigRet, const SPIRVType *retTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);

bool generateOpenCLBuiltinCall(const StringRef demangledName,
    MachineIRBuilder &MIRBuilder, Register OrigRet, const Type *OrigRetTy,
    const SmallVectorImpl<Register> &args, SPIRVTypeRegistry *TR);
} // end namespace llvm
#endif
