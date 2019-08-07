//===-- SPIRVCapabilityUtils.h - SPIR-V Capability Utils --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Utility class for adding capabilities to a collection while avoiding
// duplicated capabilities, and redundant declarations of capabilities that are
// already implicitly declared by others.
//
// This is used by both the SPIRVCapabilitiesPass and SPIRVGlobalTypesAndRegNums
// to deduplicate capabilities at both the function-local and global scopes.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVCAPABILITYUTILS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVCAPABILITYUTILS_H

#include "SPIRVEnumRequirements.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include <set>
#include <vector>

using CapabilityList = std::vector<Capability::Capability>;
using ExtensionList = std::vector<Extension::Extension>;

class SPIRVRequirementHandler {
private:
  CapabilityList minimalCaps;
  std::set<Capability::Capability> allCaps;
  std::set<Extension::Extension> allExtensions;
  uint32_t minVersion; // 0 if no min version is defined
  uint32_t maxVersion; // 0 if no max version is defined

  void pruneCapabilities(const CapabilityList &toPrune);

public:
  uint32_t getMinVersion() const { return minVersion; }
  uint32_t getMaxVersion() const { return maxVersion; }
  const CapabilityList *getMinimalCapabilities() const { return &minimalCaps; };
  const std::set<Extension::Extension> *getExtensions() const {
    return &allExtensions;
  };

  // Add a list of capabilities, ensuring allCaps captures all the implicitly
  // declared capabilities, and minimalCaps has the minimal set of required
  // capabilities (so all implicitly declared ones are removed).
  void addCapabilities(const CapabilityList &toAdd);
  void addCapability(Capability::Capability toAdd);
  void addExtensions(const ExtensionList &toAdd);
  void addExtension(Extension::Extension toAdd);

  // Add the given requirements to the lists. If constraints conflict, or these
  // requirements cannot be satisfied, then abort the compilation.
  void addRequirements(const SPIRVRequirements &requirements);

  // Check if all the requirements can be satisfied for the given subtarget, and
  // if not abort compilation.
  void checkSatisfiable(const llvm::SPIRVSubtarget &ST) const;
};

#endif
