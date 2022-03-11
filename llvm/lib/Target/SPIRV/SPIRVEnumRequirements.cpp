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

#include "MCTargetDesc/SPIRVBaseInfo.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVSubtarget.h"

SPIRVRequirements getSymbolicOperandRequirements(OperandCategory::OperandCategory Category, uint32_t i,
                                              const llvm::SPIRVSubtarget &ST) {
  auto reqMinVer =
      getSymbolicOperandMinVersion(Category, i);
  auto reqMaxVer =
      getSymbolicOperandMaxVersion(Category, i);
  auto targetVer = ST.getTargetSPIRVVersion();
  bool minVerOK = !reqMinVer || !targetVer || targetVer >= reqMinVer;
  bool maxVerOK = !reqMaxVer || !targetVer || targetVer <= reqMaxVer;
  auto reqCaps =
      getSymbolicOperandCapabilities(Category, i);
  auto reqExts =
      getSymbolicOperandExtensions(Category, i);
  if (reqCaps.empty()) {
    if (reqExts.empty()) {
      if (minVerOK && maxVerOK) {
        return {true, {}, {}, reqMinVer, reqMaxVer};
      } else {
        return {false, {}, {}, 0, 0};
      }
    }
  } else if (minVerOK && maxVerOK) {
    for (auto cap : reqCaps) { /* Only need 1 of the capabilities to work */
      if (ST.canUseCapability(cap)) {
        return {true, {cap}, {}, reqMinVer, reqMaxVer};
      }
    }
  }
  /* If there are no capabilities, or we can't satisfy the version or
   * capability requirements, use the list of extensions (if the subtarget
   * can handle them all) */
  if (std::all_of(reqExts.begin(), reqExts.end(),
                  [&ST](const Extension::Extension &ext) {
                    return ST.canUseExtension(ext);
                  })) {
    return {true, {}, reqExts, 0, 0}; /*TODO Add versions to extensions */
  }
  return {false, {}, {}, 0, 0};
}
