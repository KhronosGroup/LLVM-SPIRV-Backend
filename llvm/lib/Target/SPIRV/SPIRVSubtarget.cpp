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
#include "SPIRVGlobalRegistry.h"
#include "SPIRVLegalizerInfo.h"
#include "SPIRVRegisterBankInfo.h"
#include "SPIRVTargetMachine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "SPIRVGenSubtargetInfo.inc"

// Format version numbers as 32-bit integers |0|Maj|Min|Rev|.
static uint32_t v(uint8_t Maj, uint8_t Min, uint8_t Rev = 0) {
  return (uint32_t(Maj) << 16) | uint32_t(Min) << 8 | uint32_t(Rev);
}

// Compare version numbers, but allow 0 to mean unspecified.
static bool isAtLeastVer(uint32_t Target, uint32_t VerToCompareTo) {
  return Target == 0 || Target >= VerToCompareTo;
}

static unsigned computePointerSize(const Triple &TT) {
  const auto Arch = TT.getArch();
  // TODO: unify this with pointers legalization.
  assert(TT.isSPIRV());
  return Arch == Triple::spirv32 ? 32 : Arch == Triple::spirv64 ? 64 : 8;
}

// TODO: implement command line args or other ways to determine this.
static uint32_t computeTargetVulkanVersion(const Triple &TT) {
  if (TT.isVulkanEnvironment())
    return v(1, 1);
  return 0;
}

// TODO: implement command line args or other ways to determine this.
static bool computeOpenCLImageSupport(const Triple &TT) { return true; }

// TODO: implement command line args or other ways to determine this.
static bool computeOpenCLFullProfile(const Triple &TT) { return true; }

SPIRVSubtarget::SPIRVSubtarget(const Triple &TT, const std::string &CPU,
                               const std::string &FS,
                               const SPIRVTargetMachine &TM)
    : SPIRVGenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS), TargetTriple(TT),
      PointerSize(computePointerSize(TT)), SPIRVVersion(0), OpenCLVersion(0),
      VulkanVersion(computeTargetVulkanVersion(TT)),
      OpenCLFullProfile(computeOpenCLFullProfile(TT)),
      OpenCLImageSupport(computeOpenCLImageSupport(TT)), InstrInfo(),
      FrameLowering(initSubtargetDependencies(CPU, FS)), TLInfo(TM, *this) {
  // The order of initialization is important.
  initAvailableExtensions();
  initAvailableExtInstSets();

  GR = std::make_unique<SPIRVGlobalRegistry>(PointerSize);
  CallLoweringInfo = std::make_unique<SPIRVCallLowering>(TLInfo, GR.get());
  Legalizer = std::make_unique<SPIRVLegalizerInfo>(*this);
  RegBankInfo = std::make_unique<SPIRVRegisterBankInfo>();
  InstSelector.reset(
      createSPIRVInstructionSelector(TM, *this, *RegBankInfo.get()));
}

SPIRVSubtarget &SPIRVSubtarget::initSubtargetDependencies(StringRef CPU,
                                                          StringRef FS) {
  ParseSubtargetFeatures(CPU, /*TuneCPU=*/CPU, FS);
  if (SPIRVVersion == 0)
    SPIRVVersion = 14;
  if (OpenCLVersion == 0)
    OpenCLVersion = 22;
  return *this;
}

bool SPIRVSubtarget::canUseExtension(SPIRV::Extension::Extension E) const {
  return AvailableExtensions.contains(E);
}

bool SPIRVSubtarget::canUseExtInstSet(
    SPIRV::InstructionSet::InstructionSet E) const {
  return AvailableExtInstSets.contains(E);
}

bool SPIRVSubtarget::isOpenCLEnv() const {
  // TODO: this env is not implemented in llvm-project, we need to decide
  // how to standartize its support. For now, let's assume that we always
  // operate with OpenCL.
  return true; // TargetTriple.isOpenCLEnvironment();
}

bool SPIRVSubtarget::isVulkanEnv() const {
  // TODO: this env is not implemented in llvm-project, we need to decide
  // how to standartize its support.
  return TargetTriple.isVulkanEnvironment();
}

bool SPIRVSubtarget::isLogicalAddressing() const {
  return TargetTriple.isSPIRVLogical();
}

bool SPIRVSubtarget::isKernel() const {
  return isOpenCLEnv() || !isLogicalAddressing();
}

bool SPIRVSubtarget::isShader() const {
  return isVulkanEnv() || isLogicalAddressing();
}

bool SPIRVSubtarget::isAtLeastSPIRVVer(uint32_t VerToCompareTo) const {
  return isAtLeastVer(SPIRVVersion, VerToCompareTo);
}

bool SPIRVSubtarget::isAtLeastOpenCLVer(uint32_t VerToCompareTo) const {
  return isAtLeastVer(OpenCLVersion, VerToCompareTo);
}

// If the SPIR-V version is >= 1.4 we can call OpPtrEqual and OpPtrNotEqual.
bool SPIRVSubtarget::canDirectlyComparePointers() const {
  return isAtLeastVer(SPIRVVersion, 14);
}

// TODO: use command line args for this rather than defaults.
void SPIRVSubtarget::initAvailableExtensions() {
  AvailableExtensions.clear();
  if (!isOpenCLEnv())
    return;
  // A default extension for testing.
  AvailableExtensions.insert(
      SPIRV::Extension::SPV_KHR_no_integer_wrap_decoration);
}

// TODO: use command line args for this rather than just defaults.
// Must have called initAvailableExtensions first.
void SPIRVSubtarget::initAvailableExtInstSets() {
  AvailableExtInstSets.clear();
  if (!isOpenCLEnv())
    AvailableExtInstSets.insert(SPIRV::InstructionSet::GLSL_std_450);
  else
    AvailableExtInstSets.insert(SPIRV::InstructionSet::OpenCL_std);

  // Handle extended instruction sets from extensions.
  if (canUseExtension(
          SPIRV::Extension::SPV_AMD_shader_trinary_minmax_extension)) {
    AvailableExtInstSets.insert(
        SPIRV::InstructionSet::SPV_AMD_shader_trinary_minmax);
  }
}
