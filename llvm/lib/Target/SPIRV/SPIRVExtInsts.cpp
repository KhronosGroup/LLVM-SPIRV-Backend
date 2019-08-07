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
#include "llvm/Support/ErrorHandling.h"
#include <map>

using namespace llvm;

const char *getExtInstSetName(ExtInstSet e) {
  switch (e) {
  case ExtInstSet::OpenCL_std:
    return "OpenCL.std";
  case ExtInstSet::GLSL_std_450:
    return "GLSL.std.450";
  case ExtInstSet::SPV_AMD_shader_trinary_minmax:
    return "SPV_AMD_shader_trinary_minmax";
  }
  return "UNKNOWN_EXT_INST_SET";
}

ExtInstSet getExtInstSetFromString(const std::string &nameStr) {
  for (auto set : {ExtInstSet::GLSL_std_450, ExtInstSet::OpenCL_std}) {
    if (nameStr == getExtInstSetName(set)) {
      return set;
    }
  }
  llvm_unreachable("UNKNOWN_EXT_INST_SET");
}

const char *getExtInstName(ExtInstSet set, uint32_t inst) {
  switch (set) {
  case ExtInstSet::OpenCL_std:
    return getOpenCL_stdName(inst);
  case ExtInstSet::GLSL_std_450:
    return getGLSL_std_450Name(inst);
  case ExtInstSet::SPV_AMD_shader_trinary_minmax:
    return getSPV_AMD_shader_trinary_minmaxName(inst);
  }
  return "UNKNOWN_EXT_INST_SET";
}

GEN_EXT_INST_IMPL(OpenCL_std)
GEN_EXT_INST_IMPL(GLSL_std_450)
GEN_EXT_INST_IMPL(SPV_AMD_shader_trinary_minmax)

