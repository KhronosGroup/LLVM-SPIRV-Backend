//===-- SPIRVCapabilityUtils.cpp - SPIR-V Capability Utils ------*- C++ -*-===//
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

#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVSubtarget.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Remove a list of capabilities from dedupedCaps and add them to allCaps,
// recursing through their implicitly declared capabilities too.
void SPIRVRequirementHandler::pruneCapabilities(const CapabilityList &toPrune) {
  for (const auto &cap : toPrune) {
    allCaps.insert(cap);
    auto foundIndex = std::find(minimalCaps.begin(), minimalCaps.end(), cap);
    if (foundIndex != minimalCaps.end()) {
      minimalCaps.erase(foundIndex);
    }
    CapabilityList implicitDeclares = getCapabilityCapabilities(cap);
    pruneCapabilities(implicitDeclares);
  }
}

// Add a list of capabilities, ensuring allCaps captures all the implicitly
// declared capabilities, and dedupedCaps has the minimal number of required
// capabilities (so all implicitly declared ones are removed)
void SPIRVRequirementHandler::addCapabilities(const CapabilityList &toAdd) {
  for (const auto &cap : toAdd) {
    bool isNewlyInserted = allCaps.insert(cap).second;
    if (isNewlyInserted) { // Don't re-add if it's already been declared
      CapabilityList implicitDeclares = getCapabilityCapabilities(cap);
      pruneCapabilities(implicitDeclares);
      minimalCaps.push_back(cap);
    }
  }
}
void SPIRVRequirementHandler::addCapability(Capability::Capability toAdd) {
  addCapabilities({toAdd});
}

void SPIRVRequirementHandler::addExtensions(const ExtensionList &toAdd) {
  allExtensions.insert(toAdd.begin(), toAdd.end());
}
void SPIRVRequirementHandler::addExtension(Extension::Extension toAdd) {
  allExtensions.insert(toAdd);
}

void SPIRVRequirementHandler::addRequirements(const SPIRVRequirements &req) {
  if (!req.isSatisfiable) {
    report_fatal_error("Adding SPIR-V requirements this target can't satisfy.");
  }

  if (req.cap.hasValue()) {
    addCapabilities({req.cap.getValue()});
  }

  addExtensions(req.exts);

  if (req.minVer) {
    if (maxVersion && req.minVer > maxVersion) {
      errs() << "Conflicting version requirements: >= " << req.minVer
             << " and <= " << maxVersion << "\n";
      report_fatal_error("Adding SPIR-V requirements that can't be satisfied.");
    }

    if (minVersion == 0 || req.minVer > minVersion) {
      minVersion = req.minVer;
    }
  }

  if (req.maxVer) {
    if (minVersion && req.maxVer < minVersion) {
      errs() << "Conflicting version requirements: <= " << req.maxVer
             << " and >= " << minVersion << "\n";
      report_fatal_error("Adding SPIR-V requirements that can't be satisfied.");
    }

    if (maxVersion == 0 || req.maxVer < maxVersion) {
      maxVersion = req.maxVer;
    }
  }
}

void SPIRVRequirementHandler::checkSatisfiable(
    const llvm::SPIRVSubtarget &ST) const {
  // Report as many errors as possible before aborting the compilation.
  bool isSatisfiable = true;
  auto targetVer = ST.getTargetSPIRVVersion();

  if (maxVersion && targetVer && maxVersion < targetVer) {
    errs() << "Target SPIR-V version too high for required features\n";
    errs() << "Required max version: " << maxVersion << " target version "
           << targetVer << "\n";
    isSatisfiable = false;
  }

  if (minVersion && targetVer && minVersion > targetVer) {
    errs() << "Target SPIR-V version too low for required features\n";
    errs() << "Required min version: " << minVersion << " target version "
           << targetVer << "\n";
    isSatisfiable = false;
  }

  if (minVersion && maxVersion && minVersion > maxVersion) {
    errs() << "Version is too low for some features and too high for others.\n";
    errs() << "Required SPIR-V min version: " << minVersion
           << " required SPIR-V max version " << maxVersion << "\n";
    isSatisfiable = false;
  }

  for (auto cap : minimalCaps) {
    if (!ST.canUseCapability(cap)) {
      errs() << "Capability not supported: " << getCapabilityName(cap) << "\n";
      isSatisfiable = false;
    }
  }

  for (auto ext : allExtensions) {
    if (!ST.canUseExtension(ext)) {
      errs() << "Extension not suported: " << getExtensionName(ext) << "\n";
      isSatisfiable = false;
    }
  }

  if (!isSatisfiable) {
    report_fatal_error("Unable to meet SPIR-V requirements for this target.");
  }
}
