//===-- SPIRVExtInsts.cpp - SPIR-V Extended Instructions --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implement the SPIR-V extended instruction sets plus helper functions.
//
//===----------------------------------------------------------------------===//

#include "SPIRVExtInsts.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/ArrayRef.h"

using namespace llvm;

namespace {
  struct ExtendedBuiltin {
  StringRef Name;
  InstructionSet::InstructionSet Set;
  uint32_t Number;
};

using namespace InstructionSet;
#define GET_ExtendedBuiltins_DECL
#define GET_ExtendedBuiltins_IMPL
#include "SPIRVGenTables.inc"
}

std::string getExtInstSetName(::InstructionSet::InstructionSet Set) {
  switch (Set) {
  case ::InstructionSet::OpenCL_std:
    return "OpenCL.std";
  case ::InstructionSet::GLSL_std_450:
    return "GLSL.std.450";
  case ::InstructionSet::SPV_AMD_shader_trinary_minmax:
    return "SPV_AMD_shader_trinary_minmax";
  }
  return "UNKNOWN_EXT_INST_SET";
}

::InstructionSet::InstructionSet getExtInstSetFromString(std::string SetName) {
  for (auto Set : {::InstructionSet::GLSL_std_450, ::InstructionSet::OpenCL_std}) {
    if (SetName == getExtInstSetName(Set)) {
      return Set;
    }
  }
  llvm_unreachable("UNKNOWN_EXT_INST_SET");
}

std::string getExtInstName(::InstructionSet::InstructionSet Set,
                           uint32_t InstructionNumber) {
  switch (Set) {
  case ::InstructionSet::OpenCL_std: {
    const ExtendedBuiltin *Lookup = lookupExtendedBuiltinBySetAndNumber(
        ::InstructionSet::OpenCL_std, InstructionNumber);
    if (Lookup)
      return Lookup->Name.str();
  }
  case ::InstructionSet::GLSL_std_450: {
    const ExtendedBuiltin *Lookup = lookupExtendedBuiltinBySetAndNumber(
        ::InstructionSet::GLSL_std_450, InstructionNumber);

    if (Lookup)
      return Lookup->Name.str();
  }
  }

  return "UNKNOWN_EXT_INST_SET";
}
