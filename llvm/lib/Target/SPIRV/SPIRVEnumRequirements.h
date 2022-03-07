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

#include "SPIRVSymbolicOperands.h"
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

SPIRVRequirements
getSymbolicOperandRequirements(OperandCategory::OperandCategory Category,
                               uint32_t i, const llvm::SPIRVSubtarget &ST);

#endif
