//===-- SPIRVEnumRequirements.h - Check Enum Requirements -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains macros implementing the enum helper functions defined in
// SPIRVEnums.h, such as getEnumName(Enum e) and getEnumCapabilities(Enum e)
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_ENUMREQUIREMENTS_H
#define LLVM_LIB_TARGET_SPIRV_ENUMREQUIREMENTS_H

#include "SPIRVEnums.h"
#include "llvm/ADT/Optional.h"

namespace llvm {
class SPIRVSubtarget;
}

class SPIRVRequirements {
public:
  const bool isSatisfiable;
  const llvm::Optional<Capability::Capability> cap;
  const std::vector<Extension::Extension> exts;
  const uint32_t minVer; // 0 if no min version is required
  const uint32_t maxVer; // 0 if no max version is required

  SPIRVRequirements(bool isSatisfiable = false,
                    llvm::Optional<Capability::Capability> cap = {},
                    std::vector<Extension::Extension> exts = {},
                    uint32_t minVer = 0, uint32_t maxVer = 0)
      : isSatisfiable(isSatisfiable), cap(cap), exts(exts), minVer(minVer),
        maxVer(maxVer) {}
  SPIRVRequirements(Capability::Capability cap)
      : SPIRVRequirements(true, {cap}) {}
};

#define DEF_CAPABILITY_FUNC_HEADER(EnumName)                                   \
  const std::vector<Capability::Capability> get##EnumName##Capabilities(       \
      EnumName::EnumName e);

#define DEF_EXTENSION_FUNC_HEADER(EnumName)                                    \
  const std::vector<Extension::Extension> get##EnumName##Extensions(           \
      EnumName::EnumName e);

#define DEF_MIN_VERSION_FUNC_HEADER(EnumName)                                  \
  uint32_t get##EnumName##MinVersion(EnumName::EnumName e);

#define DEF_MAX_VERSION_FUNC_HEADER(EnumName)                                  \
  uint32_t get##EnumName##MaxVersion(EnumName::EnumName e);

#define DEF_REQUIREMENTS_FUNC_HEADER(EnumName)                                 \
  SPIRVRequirements get##EnumName##Requirements(                               \
      uint32_t i, const llvm::SPIRVSubtarget &ST);

#define DEF_CAN_USE_FUNC_HEADER(EnumName)                                      \
  bool canUse##EnumName(EnumName::EnumName e, const llvm::SPIRVSubtarget &ST);

#define GEN_ENUM_REQS_HEADER(EnumName)                                         \
  DEF_CAPABILITY_FUNC_HEADER(EnumName)                                         \
  DEF_EXTENSION_FUNC_HEADER(EnumName)                                          \
  DEF_MIN_VERSION_FUNC_HEADER(EnumName)                                        \
  DEF_MAX_VERSION_FUNC_HEADER(EnumName)                                        \
  DEF_REQUIREMENTS_FUNC_HEADER(EnumName)                                       \
  DEF_CAN_USE_FUNC_HEADER(EnumName)

GEN_ENUM_REQS_HEADER(Capability)
GEN_ENUM_REQS_HEADER(SourceLanguage)
GEN_ENUM_REQS_HEADER(ExecutionModel)
GEN_ENUM_REQS_HEADER(AddressingModel)
GEN_ENUM_REQS_HEADER(MemoryModel)
GEN_ENUM_REQS_HEADER(ExecutionMode)
GEN_ENUM_REQS_HEADER(StorageClass)
GEN_ENUM_REQS_HEADER(Dim)
GEN_ENUM_REQS_HEADER(SamplerAddressingMode)
GEN_ENUM_REQS_HEADER(SamplerFilterMode)
GEN_ENUM_REQS_HEADER(ImageFormat)
GEN_ENUM_REQS_HEADER(ImageChannelOrder)
GEN_ENUM_REQS_HEADER(ImageChannelDataType)
GEN_ENUM_REQS_HEADER(ImageOperand)
GEN_ENUM_REQS_HEADER(FPFastMathMode)
GEN_ENUM_REQS_HEADER(FPRoundingMode)
GEN_ENUM_REQS_HEADER(LinkageType)
GEN_ENUM_REQS_HEADER(AccessQualifier)
GEN_ENUM_REQS_HEADER(FunctionParameterAttribute)
GEN_ENUM_REQS_HEADER(Decoration)
GEN_ENUM_REQS_HEADER(BuiltIn)
GEN_ENUM_REQS_HEADER(SelectionControl)
GEN_ENUM_REQS_HEADER(LoopControl)
GEN_ENUM_REQS_HEADER(FunctionControl)
GEN_ENUM_REQS_HEADER(MemorySemantics)
GEN_ENUM_REQS_HEADER(MemoryOperand)
GEN_ENUM_REQS_HEADER(Scope)
GEN_ENUM_REQS_HEADER(GroupOperation)
GEN_ENUM_REQS_HEADER(KernelEnqueueFlags)
GEN_ENUM_REQS_HEADER(KernelProfilingInfo)

#endif
