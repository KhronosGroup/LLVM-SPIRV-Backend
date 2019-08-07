//===-- SPIRVEnumRequirements.cpp - Check Enum Requirements -----*- C++ -*-===//
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

#include "SPIRVEnumRequirements.h"
#include "SPIRVSubtarget.h"
#include <algorithm>

#define MAKE_CAPABILITY_CASE(Enum, Var, Val, Caps, Exts, MinVer, MaxVer)       \
  case Enum::Var:                                                              \
    return Caps;

#define MAKE_EXTENSION_CASE(Enum, Var, Val, Caps, Exts, MinVer, MaxVer)        \
  case Enum::Var:                                                              \
    return Exts;

#define MAKE_MIN_VERSION_CASE(Enum, Var, Val, Caps, Exts, MinVer, MaxVer)      \
  case Enum::Var:                                                              \
    return MinVer;

#define MAKE_MAX_VERSION_CASE(Enum, Var, Val, Caps, Exts, MinVer, MaxVer)      \
  case Enum::Var:                                                              \
    return MaxVer;

#define DEF_CAPABILITY_FUNC_BODY(EnumName, DefEnumCommand)                     \
  const std::vector<Capability::Capability> get##EnumName##Capabilities(       \
      EnumName::EnumName e) {                                                  \
    using namespace Capability;                                                \
    switch (e) { DefEnumCommand(EnumName, MAKE_CAPABILITY_CASE) }              \
    return {};                                                                 \
  }

#define DEF_EXTENSION_FUNC_BODY(EnumName, DefEnumCommand)                      \
  const std::vector<Extension::Extension> get##EnumName##Extensions(           \
      EnumName::EnumName e) {                                                  \
    using namespace Extension;                                                 \
    switch (e) { DefEnumCommand(EnumName, MAKE_EXTENSION_CASE) }               \
    return {};                                                                 \
  }

#define DEF_MIN_VERSION_FUNC_BODY(EnumName, DefEnumCommand)                    \
  uint32_t get##EnumName##MinVersion(EnumName::EnumName e) {                   \
    switch (e) { DefEnumCommand(EnumName, MAKE_MIN_VERSION_CASE) }             \
    return 0;                                                                  \
  }

#define DEF_MAX_VERSION_FUNC_BODY(EnumName, DefEnumCommand)                    \
  uint32_t get##EnumName##MaxVersion(EnumName::EnumName e) {                   \
    switch (e) { DefEnumCommand(EnumName, MAKE_MAX_VERSION_CASE) }             \
    return 0;                                                                  \
  }

// Using the subtarget, determine the minimal requirements to use the enum.
#define DEF_REQUIREMENTS_FUNC_BODY(EnumName)                                   \
  SPIRVRequirements get##EnumName##Requirements(                               \
      uint32_t i, const llvm::SPIRVSubtarget &ST) {                            \
    auto e = static_cast<EnumName::EnumName>(i);                               \
    auto reqMinVer = get##EnumName##MinVersion(e);                             \
    auto reqMaxVer = get##EnumName##MaxVersion(e);                             \
    auto targetVer = ST.getTargetSPIRVVersion();                               \
    bool minVerOK = !reqMinVer || !targetVer || targetVer >= reqMinVer;        \
    bool maxVerOK = !reqMaxVer || !targetVer || targetVer <= reqMaxVer;        \
    auto reqCaps = get##EnumName##Capabilities(e);                             \
    auto reqExts = get##EnumName##Extensions(e);                               \
    if (reqCaps.empty()) {                                                     \
      if (reqExts.empty()) {                                                   \
        if (minVerOK && maxVerOK) {                                            \
          return {true, {}, {}, reqMinVer, reqMaxVer};                         \
        } else {                                                               \
          return {false, {}, {}, 0, 0};                                        \
        }                                                                      \
      }                                                                        \
    } else if (minVerOK && maxVerOK) {                                         \
      for (auto cap : reqCaps) { /* Only need 1 of the capabilities to work */ \
        if (ST.canUseCapability(cap)) {                                        \
          return {true, {cap}, {}, reqMinVer, reqMaxVer};                      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    /* If there are no capabilities, or we can't satisfy the version or        \
     * capability requirements, use the list of extensions (if the subtarget   \
     * can handle them all) */                                                 \
    if (std::all_of(reqExts.begin(), reqExts.end(),                            \
                    [&ST](const Extension::Extension &ext) {                   \
                      return ST.canUseExtension(ext);                          \
                    })) {                                                      \
      return {true, {}, reqExts, 0, 0}; /*TODO Add versions to extensions */   \
    }                                                                          \
    return {false, {}, {}, 0, 0};                                              \
  }

#define DEF_CAN_USE_FUNC_BODY(EnumName)                                        \
  bool canUse##EnumName(EnumName::EnumName e,                                  \
                        const llvm::SPIRVSubtarget &ST) {                      \
    return get##EnumName##Requirements(e, ST).isSatisfiable;                   \
  }

#define GEN_ENUM_REQS_IMPL(EnumName)                                           \
  DEF_CAPABILITY_FUNC_BODY(EnumName, DEF_##EnumName)                           \
  DEF_EXTENSION_FUNC_BODY(EnumName, DEF_##EnumName)                            \
  DEF_MIN_VERSION_FUNC_BODY(EnumName, DEF_##EnumName)                          \
  DEF_MAX_VERSION_FUNC_BODY(EnumName, DEF_##EnumName)                          \
  DEF_REQUIREMENTS_FUNC_BODY(EnumName)                                         \
  DEF_CAN_USE_FUNC_BODY(EnumName)

GEN_ENUM_REQS_IMPL(Capability)
GEN_ENUM_REQS_IMPL(SourceLanguage)
GEN_ENUM_REQS_IMPL(ExecutionModel)
GEN_ENUM_REQS_IMPL(AddressingModel)
GEN_ENUM_REQS_IMPL(MemoryModel)
GEN_ENUM_REQS_IMPL(ExecutionMode)
GEN_ENUM_REQS_IMPL(StorageClass)
GEN_ENUM_REQS_IMPL(Dim)
GEN_ENUM_REQS_IMPL(SamplerAddressingMode)
GEN_ENUM_REQS_IMPL(SamplerFilterMode)
GEN_ENUM_REQS_IMPL(ImageFormat)
GEN_ENUM_REQS_IMPL(ImageChannelOrder)
GEN_ENUM_REQS_IMPL(ImageChannelDataType)
GEN_ENUM_REQS_IMPL(ImageOperand)
GEN_ENUM_REQS_IMPL(FPFastMathMode)
GEN_ENUM_REQS_IMPL(FPRoundingMode)
GEN_ENUM_REQS_IMPL(LinkageType)
GEN_ENUM_REQS_IMPL(AccessQualifier)
GEN_ENUM_REQS_IMPL(FunctionParameterAttribute)
GEN_ENUM_REQS_IMPL(Decoration)
GEN_ENUM_REQS_IMPL(BuiltIn)
GEN_ENUM_REQS_IMPL(SelectionControl)
GEN_ENUM_REQS_IMPL(LoopControl)
GEN_ENUM_REQS_IMPL(FunctionControl)
GEN_ENUM_REQS_IMPL(MemorySemantics)
GEN_ENUM_REQS_IMPL(MemoryOperand)
GEN_ENUM_REQS_IMPL(Scope)
GEN_ENUM_REQS_IMPL(GroupOperation)
GEN_ENUM_REQS_IMPL(KernelEnqueueFlags)
GEN_ENUM_REQS_IMPL(KernelProfilingInfo)

