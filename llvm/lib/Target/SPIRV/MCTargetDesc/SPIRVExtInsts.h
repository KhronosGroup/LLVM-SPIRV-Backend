//===-- SPIRVExtInsts.h - SPIR-V Extended Instructions ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Define the different SPIR-V extended instruction sets plus helper functions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVEXTINSTS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVEXTINSTS_H

#include <string>

namespace InstructionSet {
#define GET_InstructionSet_DECL
#include "SPIRVGenTables.inc"
}

namespace OpenCLExtInst {
#define GET_OpenCLExtInst_DECL
#include "SPIRVGenTables.inc"
}

namespace GLSLExtInst {
#define GET_GLSLExtInst_DECL
#include "SPIRVGenTables.inc"
}

std::string getExtInstSetName(InstructionSet::InstructionSet Set);
InstructionSet::InstructionSet getExtInstSetFromString(std::string SetName);
std::string getExtInstName(InstructionSet::InstructionSet Set, uint32_t InstructionNumber);

#endif
