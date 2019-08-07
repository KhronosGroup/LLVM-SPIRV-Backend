//===-- SPIRVSubtarget.cpp - SPIR-V Subtarget Information ------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the SPIR-V specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "SPIRVSubtarget.h"
#include "SPIRV.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVLegalizerInfo.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVTypeRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "SPIRVGenSubtargetInfo.inc"

// Format version numbers as 32-bit integers |0|Maj|Min|Rev|
static uint32_t v(uint8_t Maj, uint8_t Min, uint8_t Rev = 0) {
  return (uint32_t(Maj) << 16) | uint32_t(Min) << 8 | uint32_t(Rev);
}

// Compare version numbers, but allow 0 to mean unspecified
static bool isAtLeastVer(uint32_t target, uint32_t verToCompareTo) {
  return target == 0 || target >= verToCompareTo;
}

static unsigned computePointerSize(const Triple &TT) {
  const auto arch = TT.getArch();
  return arch == Triple::spirv32 ? 32 : arch == Triple::spirv64 ? 64 : 8;
}

// TODO use command line args for this rather than defaulting to 1.4
static uint32_t computeTargetSPIRVVersion(const Triple &TT) {
  return v(1, 4); // TODO - remove this as it's just here for the ptrcmp.ll test
  // if (TT.isVulkanEnvironment()) {
  //   return v(1, 0);
  // } else {
  //   return v(1, 2);
  // }
}

// TODO use command line args for this rather than defaulting to 2.2
static uint32_t computeTargetOpenCLVersion(const Triple &TT) {
  if (TT.isVulkanEnvironment()) {
    return 0;
  } else {
    return v(2, 2);
  }
}

// TODO use command line args for this rather than defaulting to 1.1
static uint32_t computeTargetVulkanVersion(const Triple &TT) {
  if (TT.isVulkanEnvironment()) {
    return v(1, 1);
  } else {
    return 0;
  }
}

// TODO use command line args for this rather than defaulting to true
static bool computeOpenCLImageSupport(const Triple &TT) { return true; }

// TODO use command line args for this rather than defaulting to true
static bool computeOpenCLFullProfile(const Triple &TT) { return true; }

SPIRVSubtarget::SPIRVSubtarget(const Triple &TT, const std::string &CPU,
                               const std::string &FS,
                               const SPIRVTargetMachine &TM)
    : SPIRVGenSubtargetInfo(TT, CPU, FS), InstrInfo(),
      FrameLowering(initSubtargetDependencies(CPU, FS)), TLInfo(TM, *this),
      pointerSize(computePointerSize(TT)),
      usesLogicalAddressing(TT.isSPIRVLogical()),
      usesVulkanEnv(TT.isVulkanEnvironment()),
      usesOpenCLEnv(TT.isOpenCLEnvironment()),
      targetSPIRVVersion(computeTargetSPIRVVersion(TT)),
      targetOpenCLVersion(computeTargetOpenCLVersion(TT)),
      targetVulkanVersion(computeTargetVulkanVersion(TT)),
      openCLFullProfile(computeOpenCLFullProfile(TT)),
      openCLImageSupport(computeOpenCLImageSupport(TT)),
      TR(new SPIRVTypeRegistry(pointerSize)),
      CallLoweringInfo(new SPIRVCallLowering(TLInfo, TR.get())),
      RegBankInfo(new SPIRVRegisterBankInfo()) {

  initAvailableExtensions(TT);
  initAvailableExtInstSets(TT);
  initAvailableCapabilities(TT);

  Legalizer.reset(new SPIRVLegalizerInfo(*this));
  InstSelector.reset(
      createSPIRVInstructionSelector(TM, *this, *RegBankInfo.get()));
}

SPIRVSubtarget &SPIRVSubtarget::initSubtargetDependencies(StringRef CPU,
                                                          StringRef FS) {
  ParseSubtargetFeatures(CPU, FS);
  return *this;
}

bool SPIRVSubtarget::canUseCapability(Capability::Capability c) const {
  const auto &caps = availableCaps;
  return std::find(caps.begin(), caps.end(), c) != caps.end();
}

bool SPIRVSubtarget::canUseExtension(Extension::Extension e) const {
  const auto &exts = availableExtensions;
  return std::find(exts.begin(), exts.end(), e) != exts.end();
}

bool SPIRVSubtarget::canUseExtInstSet(ExtInstSet e) const {
  const auto &sets = availableExtInstSets;
  return std::find(sets.begin(), sets.end(), e) != sets.end();
}

bool SPIRVSubtarget::isLogicalAddressing() const {
  return usesLogicalAddressing;
}

bool SPIRVSubtarget::isKernel() const {
  return usesOpenCLEnv || !usesLogicalAddressing;
}

bool SPIRVSubtarget::isShader() const {
  return usesVulkanEnv || usesLogicalAddressing;
}

// If the SPIR-V version is >= 1.4 we can call OpPtrEqual and OpPtrNotEqual
bool SPIRVSubtarget::canDirectlyComparePointers() const {
  return isAtLeastVer(targetSPIRVVersion, v(1, 4));
}

// TODO use command line args for this rather than defaults
void SPIRVSubtarget::initAvailableExtensions(const Triple &TT) {
  using namespace Extension;
  if (TT.isVulkanEnvironment()) {
    availableExtensions = {};
  } else {
    // A default extension for testing - should use command line args
    availableExtensions = {SPV_KHR_no_integer_wrap_decoration};
  }
}

// Add the given capabilities and all their implicitly defined capabilities too
static void addCaps(std::unordered_set<Capability::Capability> &caps,
                    const std::vector<Capability::Capability> &toAdd) {
  for (const auto cap : toAdd) {
    if (caps.insert(cap).second) {
      addCaps(caps, getCapabilityCapabilities(cap));
    }
  }
}

// TODO use command line args for this rather than defaults
// Must have called initAvailableExtensions first.
void SPIRVSubtarget::initAvailableCapabilities(const Triple &TT) {
  using namespace Capability;
  if (TT.isVulkanEnvironment()) {
    // These are the min requirements
    addCaps(availableCaps,
            {Matrix, Shader, InputAttachment, Sampled1D, Image1D, SampledBuffer,
             ImageBuffer, ImageQuery, DerivativeControl});
  } else {
    // Add the min requirements for different OpenCL and SPIR-V versions
    addCaps(availableCaps,
            {Addresses, Float16Buffer, Int16, Int8, Kernel, Linkage, Vector16});
    if (openCLFullProfile) {
      addCaps(availableCaps, {Int64});
    }
    if (openCLImageSupport) {
      addCaps(availableCaps, {ImageBasic, LiteralSampler, Image1D,
                              SampledBuffer, ImageBuffer});
      if (isAtLeastVer(targetOpenCLVersion, v(2, 0))) {
        addCaps(availableCaps, {ImageReadWrite});
      }
    }
    if (isAtLeastVer(targetSPIRVVersion, v(1, 1)) &&
        isAtLeastVer(targetOpenCLVersion, v(2, 2))) {
      addCaps(availableCaps, {SubgroupDispatch, PipeStorage});
    }

    // TODO Remove this - it's only here because the tests assume it's supported
    addCaps(availableCaps, {Float16, Float64});

    // TODO add OpenCL extensions
  }
}

// TODO use command line args for this rather than just defaults
// Must have called initAvailableExtensions first.
void SPIRVSubtarget::initAvailableExtInstSets(const Triple &TT) {
  if (usesVulkanEnv) {
    availableExtInstSets.insert(ExtInstSet::GLSL_std_450);
  } else {
    availableExtInstSets.insert(ExtInstSet::OpenCL_std);
  }

  // Handle extended instruction sets from extensions.
  if (canUseExtension(Extension::SPV_AMD_shader_trinary_minmax)) {
    availableExtInstSets.insert(ExtInstSet::SPV_AMD_shader_trinary_minmax);
  }
}
