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
static bool isAtLeastVer(uint32_t Target, uint32_t VerToCompareTo) {
  return Target == 0 || Target >= VerToCompareTo;
}

static unsigned computePointerSize(const Triple &TT) {
  const auto Arch = TT.getArch();
  // TODO: unify this with pointers legalization
  return Arch == Triple::spirv32 ? 32 : Arch == Triple::spirv64 ? 64 : 8;
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
    : SPIRVGenSubtargetInfo(TT, CPU, /* TuneCPU */ CPU, FS),
      PointerSize(computePointerSize(TT)),
      UsesLogicalAddressing(TT.isSPIRVLogical()),
      UsesVulkanEnv(TT.isVulkanEnvironment()),
      UsesOpenCLEnv(TT.isOpenCLEnvironment()), TargetSPIRVVersion(0),
      TargetOpenCLVersion(0),
      TargetVulkanVersion(computeTargetVulkanVersion(TT)),
      OpenCLFullProfile(computeOpenCLFullProfile(TT)),
      OpenCLImageSupport(computeOpenCLImageSupport(TT)), InstrInfo(),
      FrameLowering(initSubtargetDependencies(CPU, FS)), TLInfo(TM, *this),

      DT(new SPIRVGeneralDuplicatesTracker()),
      // FIXME:
      // .get() here is unsafe, works due to subtarget is destroyed
      // later than these objects
      // see comment in the SPIRVSubtarget.h
      TR(new SPIRVTypeRegistry(*DT.get(), PointerSize)),
      CallLoweringInfo(new SPIRVCallLowering(TLInfo, TR.get(), DT.get())),
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
  ParseSubtargetFeatures(CPU, /* TuneCPU */ CPU, FS);

  if (TargetSPIRVVersion == 0)
    TargetSPIRVVersion = 14;
  if (TargetOpenCLVersion == 0)
    TargetOpenCLVersion = 22;

  return *this;
}

bool SPIRVSubtarget::canUseCapability(Capability::Capability C) const {
  const auto &Caps = AvailableCaps;
  return std::find(Caps.begin(), Caps.end(), C) != Caps.end();
}

bool SPIRVSubtarget::canUseExtension(Extension::Extension E) const {
  const auto &Exts = AvailableExtensions;
  return std::find(Exts.begin(), Exts.end(), E) != Exts.end();
}

bool SPIRVSubtarget::canUseExtInstSet(ExtInstSet E) const {
  const auto &Sets = AvailableExtInstSets;
  return std::find(Sets.begin(), Sets.end(), E) != Sets.end();
}

bool SPIRVSubtarget::isLogicalAddressing() const {
  return UsesLogicalAddressing;
}

bool SPIRVSubtarget::isKernel() const {
  return UsesOpenCLEnv || !UsesLogicalAddressing;
}

bool SPIRVSubtarget::isShader() const {
  return UsesVulkanEnv || UsesLogicalAddressing;
}

// If the SPIR-V version is >= 1.4 we can call OpPtrEqual and OpPtrNotEqual
bool SPIRVSubtarget::canDirectlyComparePointers() const {
  bool Res = isAtLeastVer(TargetSPIRVVersion, 14);
  return Res;
}

// TODO use command line args for this rather than defaults
void SPIRVSubtarget::initAvailableExtensions(const Triple &TT) {
  using namespace Extension;
  if (TT.isVulkanEnvironment()) {
    AvailableExtensions = {};
  } else {
    // A default extension for testing - should use command line args
    AvailableExtensions = {SPV_KHR_no_integer_wrap_decoration};
  }
}

// Add the given capabilities and all their implicitly defined capabilities too
static void addCaps(std::unordered_set<Capability::Capability> &Caps,
                    const std::vector<Capability::Capability> &ToAdd) {
  for (const auto cap : ToAdd) {
    if (Caps.insert(cap).second) {
      addCaps(Caps, getCapabilityCapabilities(cap));
    }
  }
}

// TODO use command line args for this rather than defaults
// Must have called initAvailableExtensions first.
void SPIRVSubtarget::initAvailableCapabilities(const Triple &TT) {
  using namespace Capability;
  if (TT.isVulkanEnvironment()) {
    // These are the min requirements
    addCaps(AvailableCaps,
            {Matrix, Shader, InputAttachment, Sampled1D, Image1D, SampledBuffer,
             ImageBuffer, ImageQuery, DerivativeControl});
  } else {
    // Add the min requirements for different OpenCL and SPIR-V versions
    addCaps(AvailableCaps, {Addresses, Float16Buffer, Int16, Int8, Kernel,
                            Linkage, Vector16, Groups});
    if (OpenCLFullProfile) {
      addCaps(AvailableCaps, {Int64, Int64Atomics});
    }
    if (OpenCLImageSupport) {
      addCaps(AvailableCaps, {ImageBasic, LiteralSampler, Image1D,
                              SampledBuffer, ImageBuffer});
      if (isAtLeastVer(TargetOpenCLVersion, 20)) {
        addCaps(AvailableCaps, {ImageReadWrite});
      }
    }
    if (isAtLeastVer(TargetSPIRVVersion, 11) &&
        isAtLeastVer(TargetOpenCLVersion, 22)) {
      addCaps(AvailableCaps, {SubgroupDispatch, PipeStorage});
    }
    if (isAtLeastVer(TargetSPIRVVersion, 13)) {
      addCaps(AvailableCaps,
              {GroupNonUniform, GroupNonUniformVote, GroupNonUniformArithmetic,
               GroupNonUniformBallot, GroupNonUniformClustered,
               GroupNonUniformShuffle, GroupNonUniformShuffleRelative});
    }

    // TODO Remove this - it's only here because the tests assume it's supported
    addCaps(AvailableCaps, {Float16, Float64});

    // TODO add OpenCL extensions
  }
}

// TODO use command line args for this rather than just defaults
// Must have called initAvailableExtensions first.
void SPIRVSubtarget::initAvailableExtInstSets(const Triple &TT) {
  if (UsesVulkanEnv) {
    AvailableExtInstSets.insert(ExtInstSet::GLSL_std_450);
  } else {
    AvailableExtInstSets.insert(ExtInstSet::OpenCL_std);
  }

  // Handle extended instruction sets from extensions.
  if (canUseExtension(Extension::SPV_AMD_shader_trinary_minmax)) {
    AvailableExtInstSets.insert(ExtInstSet::SPV_AMD_shader_trinary_minmax);
  }
}
