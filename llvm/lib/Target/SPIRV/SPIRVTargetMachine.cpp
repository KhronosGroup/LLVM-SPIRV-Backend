//===- SPIRVTargetMachine.cpp - Define TargetMachine for SPIR-V -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about SPIR-V target spec.
//
//===----------------------------------------------------------------------===//

#include "SPIRVTargetMachine.h"
#include "MCTargetDesc/SPIRVMCAsmInfo.h"
#include "SPIRV.h"
#include "SPIRVCallLowering.h"
#include "SPIRVIRTranslator.h"
#include "SPIRVLegalizerInfo.h"
#include "SPIRVRegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/CodeGen/GlobalISel/RegBankSelect.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"

#include "SPIRVSubtarget.h"
#include "SPIRVTypeRegistry.h"

#include <string>

#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/PassSupport.h"

using namespace llvm;
InstructionSelector *
createSPIRVInstructionSelector(const SPIRVTargetMachine &TM,
                               SPIRVSubtarget &Subtarget,
                               SPIRVRegisterBankInfo &RBI);

extern "C" void LLVMInitializeSPIRVTarget() {
  // Register the target.
  RegisterTargetMachine<SPIRVTargetMachine> X(getTheSPIRV32Target());
  RegisterTargetMachine<SPIRVTargetMachine> Y(getTheSPIRV64Target());
  RegisterTargetMachine<SPIRVTargetMachine> Z(getTheSPIRVLogicalTarget());

  // Register all custom passes so we can use "-stop-after" etc.
  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializeGlobalISel(PR);
  initializeSPIRVBlockLabelerPass(PR);
  initializeSPIRVGlobalTypesAndRegNumPass(PR);
  initializeSPIRVAddRequirementsPass(PR);
}

// DataLayout: little or big endian
static std::string computeDataLayout(const Triple &TT) {
  std::string dataLayout = "e-m:e";

  const auto arch = TT.getArch();
  if (arch == Triple::spirv32) {
    dataLayout += "-p:32:32";
  } else if (arch == Triple::spirv64) {
    dataLayout += "-p:64:64";
  } else if (arch == Triple::spirvlogical) {
    dataLayout += "-p:8:8";
  }
  return dataLayout;
}


static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.hasValue())
    return Reloc::PIC_;
  return *RM;
}

SPIRVTargetMachine::SPIRVTargetMachine(const Target &T, const Triple &TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Optional<Reloc::Model> RM,
                                       Optional<CodeModel::Model> CM,
                                       CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(TT), TT, CPU, FS, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, CPU, FS, *this) {
  initAsmInfo();
  setGlobalISel(true);
  setGlobalISelAbort(GlobalISelAbortMode::Enable);
  setFastISel(false);
  setO0WantsFastISel(false);
  setRequiresStructuredCFG(TT.isVulkanEnvironment());
}

namespace {
// SPIR-V Code Generator Pass Configuration Options.
class SPIRVPassConfig : public TargetPassConfig {
public:
  SPIRVPassConfig(SPIRVTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  SPIRVTargetMachine &getSPIRVTargetMachine() const {
    return getTM<SPIRVTargetMachine>();
  }
  void addISelPrepare() override;

  bool addIRTranslator() override;
  bool addLegalizeMachineIR() override;
  bool addRegBankSelect() override;
  bool addGlobalInstructionSelect() override;

  FunctionPass *createTargetRegisterAllocator(bool) override;
  void addFastRegAlloc() override {}
  void addOptimizedRegAlloc() override {}

  void addPostRegAlloc() override;

  void addPreEmitPass2() override;
};
} // namespace

// We do not use physical registers, and maintain virtual registers throughout
// the entire pipeline, so return nullptr to disable register allocation.
FunctionPass *SPIRVPassConfig::createTargetRegisterAllocator(bool) {
  return nullptr;
}

// Disable passes that break from assuming no virtual registers exist
void SPIRVPassConfig::addPostRegAlloc() {
  // Do not work with vregs instead of physical regs
  disablePass(&MachineCopyPropagationID);
  disablePass(&PostRAMachineSinkingID);
  disablePass(&PostRASchedulerID);
  disablePass(&FuncletLayoutID);
  disablePass(&StackMapLivenessID);
  disablePass(&PatchableFunctionID);
  disablePass(&ShrinkWrapID);
  disablePass(&LiveDebugValuesID);

  // Do not work with OpPhi
  disablePass(&BranchFolderPassID);
  disablePass(&MachineBlockPlacementID);

  TargetPassConfig::addPostRegAlloc();
}

TargetPassConfig *SPIRVTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new SPIRVPassConfig(*this, PM);
}

void SPIRVPassConfig::addISelPrepare() {
  TargetPassConfig::addISelPrepare();
  addPass(createSPIRVBasicBlockDominancePass());
}

// Add custom passes right before emitting asm/obj files. Global VReg numbering
// is added here, as it emits invalid MIR, so no subsequent passes would work
void SPIRVPassConfig::addPreEmitPass2() {
  // Insert missing block labels and terminators. Fix instrs with MBB references
  addPass(createSPIRVBlockLabelerPass());

  // Add all required OpCapability instructions locally (to be hoisted later)
  addPass(createSPIRVAddRequirementsPass());

  // Hoist all global instructions, and number VRegs globally.
  // We disable verification after this, as global VRegs are invalid in MIR
  addPass(createSPIRVGlobalTypesAndRegNumPass(), false);
}

// Use a customized subclass of IRTranslator, which avoids flattening aggregates
// and maintains type information (see SPIRVIRTranslator.cpp).
bool SPIRVPassConfig::addIRTranslator() {
  addPass(new SPIRVIRTranslator());
  return false;
}

// Use a default legalizer
bool SPIRVPassConfig::addLegalizeMachineIR() {
  addPass(new Legalizer());
  return false;
}

// Do not add a RegBankSelect pass, as we only ever need virtual registers
bool SPIRVPassConfig::addRegBankSelect() {
  disablePass(&RegBankSelect::ID);
  return false;
}

namespace {
// A custom subclass of InstructionSelect, which is mostly the same except from
// not requiring RegBankSelect to occur previously, and making sure a
// SPIRVTypeRegistry is initialized before and reset after each run.
class SPIRVInstructionSelect : public InstructionSelect {

  // We don't use register banks, so unset the requirement for them
  MachineFunctionProperties getRequiredProperties() const override {
    return InstructionSelect::getRequiredProperties().reset(
        MachineFunctionProperties::Property::RegBankSelected);
  }

  // Init a SPIRVTypeRegistry before and reset it after the default parent code
  bool runOnMachineFunction(MachineFunction &MF) override {
    const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());
    auto *TR = ST->getSPIRVTypeRegistry();
    TR->rebuildTypeTablesForFunction(MF);

    bool success = InstructionSelect::runOnMachineFunction(MF);

    TR->reset();
    return success;
  }
};
} // namespace

// Add the custom SPIRVInstructionSelect from above
bool SPIRVPassConfig::addGlobalInstructionSelect() {
  addPass(new SPIRVInstructionSelect());
  return false;
}
