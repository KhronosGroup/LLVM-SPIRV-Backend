//===-- SPIRVCapabilitiesPass.cpp - SPIR-V Capability Pass Impl -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implementation of SPIRVCapabilitiesPass, which iterates over all instructions
// and inserts OpCapability instructions whenever the other instructions do
// something that requires them (e.g. an OpTypeInt instruction with a width of
// 64 requires the Int64 capability to be explicitly declared).
//
// All OpCapability instructions generated are function-local, and get hoisted
// and deduplicated later in SPIRVGlobalTypesAndRegNums.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassSupport.h"

#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTypeRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-requirements"

namespace {
class SPIRVAddRequirements : public MachineFunctionPass {
public:
  static char ID;

  SPIRVAddRequirements() : MachineFunctionPass(ID) {
    initializeSPIRVAddRequirementsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace

// Add the all the requirements needed for the given instruction.
static void addInstrRequirements(const MachineInstr &MI,
                                 SPIRVRequirementHandler &reqs,
                                 const SPIRVSubtarget &ST);

// Insert a deduplicated list of all OpCapability instructions required for MF.
bool SPIRVAddRequirements::runOnMachineFunction(MachineFunction &MF) {
  const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());
  SPIRVRequirementHandler reqHandler;
  // Loop through all instrs and add their required capabilities to the list
  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      addInstrRequirements(MI, reqHandler, *ST);
    }
  }

  // Build the OpCapability instructions at the start of the function
  MachineIRBuilder MIRBuilder;
  auto MBB = MF.begin();
  MIRBuilder.setMF(MF);
  MIRBuilder.setMBB(*MBB);
  MIRBuilder.setInstr(*MBB->begin());

  const auto *caps = reqHandler.getMinimalCapabilities();
  for (const auto &cap : *caps) {
    MIRBuilder.buildInstr(SPIRV::OpCapability).addImm(cap);
  }
  const auto *exts = reqHandler.getExtensions();
  for (const auto &ext : *exts) {
    MIRBuilder.buildInstr(SPIRV::OpExtension).addImm(ext);
  }
  // TODO add a pseudo instr for SPIR-V version
  return false;
}

// Add VariablePointers to the requirements if this instruction defines a
// pointer (Logical only).
static void addVariablePtrInstrReqs(const MachineInstr &MI,
                                    SPIRVRequirementHandler &reqs,
                                    const SPIRVSubtarget &ST) {
  if (ST.isLogicalAddressing()) {
    auto &MRI = MI.getMF()->getRegInfo();
    Register type = MI.getOperand(1).getReg();
    if (MRI.getVRegDef(type)->getOpcode() == SPIRV::OpTypePointer) {
      reqs.addCapability(Capability::VariablePointers);
    }
  }
}

// Add the required capabilities from a decoration instruction (including
// BuiltIns).
static void addOpDecorateReqs(const MachineInstr &MI, unsigned int decIndex,
                              SPIRVRequirementHandler &reqs,
                              const SPIRVSubtarget &ST) {
  auto decOp = MI.getOperand(decIndex).getImm();
  auto dec = static_cast<Decoration::Decoration>(decOp);
  reqs.addRequirements(getDecorationRequirements(dec, ST));

  if (dec == Decoration::BuiltIn) {
    auto builtInOp = MI.getOperand(decIndex + 1).getImm();
    auto builtIn = static_cast<BuiltIn::BuiltIn>(builtInOp);
    reqs.addRequirements(getBuiltInRequirements(builtIn, ST));
  }
}

// Add requirements for image handling.
static void addOpTypeImageReqs(const MachineInstr &MI,
                               SPIRVRequirementHandler &reqs,
                               const SPIRVSubtarget &ST) {
  assert(MI.getNumOperands() >= 8 && "Insufficient operands for OpTypeImage");

  using namespace Capability;
  using namespace Dim;

  // The operand indices used here are based on the OpTypeImage layout, which
  // the MachineInstr follows as well.
  auto imgFormatOp = MI.getOperand(7).getImm();
  auto imgFormat = static_cast<ImageFormat::ImageFormat>(imgFormatOp);
  reqs.addRequirements(getImageFormatRequirements(imgFormat, ST));

  bool isArrayed = MI.getOperand(4).getImm() == 1;
  bool isMultisampled = MI.getOperand(5).getImm() == 1;
  bool noSampler = MI.getOperand(6).getImm() == 2;

  // Add dimension requirements
  auto dim = MI.getOperand(2).getImm();
  switch (dim) {
  case DIM_1D:
    reqs.addRequirements(noSampler ? Image1D : Sampled1D);
    break;
  case DIM_2D:
    if (isMultisampled && noSampler) {
      reqs.addRequirements(ImageMSArray);
    }
    break;
  case DIM_Cube:
    reqs.addRequirements(Shader);
    if (isArrayed) {
      reqs.addRequirements(noSampler ? ImageCubeArray : SampledCubeArray);
    }
    break;
  case DIM_Rect:
    reqs.addRequirements(noSampler ? ImageRect : SampledRect);
    break;
  case DIM_Buffer:
    reqs.addRequirements(noSampler ? ImageBuffer : SampledBuffer);
    break;
  case DIM_SubpassData:
    reqs.addRequirements(InputAttachment);
    break;
  }

  if (ST.isKernel()) {
    // Has optional access qualifier
    if (MI.getNumOperands() > 8 && MI.getOperand(8).getImm() == AQ::ReadWrite) {
      reqs.addRequirements(ImageReadWrite);
    } else {
      reqs.addRequirements(ImageBasic);
    }
  }
}

static void addInstrRequirements(const MachineInstr &MI,
                                 SPIRVRequirementHandler &reqs,
                                 const SPIRVSubtarget &ST) {
  using namespace Capability;
  switch (MI.getOpcode()) {
  case SPIRV::OpMemoryModel: {
    auto addr = MI.getOperand(0).getImm();
    reqs.addRequirements(getAddressingModelRequirements(addr, ST));
    auto mem = MI.getOperand(1).getImm();
    reqs.addRequirements(getMemoryModelRequirements(mem, ST));
    break;
  }
  case SPIRV::OpEntryPoint: {
    auto exe = MI.getOperand(0).getImm();
    reqs.addRequirements(getExecutionModelRequirements(exe, ST));
    break;
  }
  case SPIRV::OpExecutionMode:
  case SPIRV::OpExecutionModeId: {
    auto exe = MI.getOperand(1).getImm();
    reqs.addRequirements(getExecutionModeRequirements(exe, ST));
    break;
  }
  case SPIRV::OpTypeMatrix:
    reqs.addCapability(Matrix);
    break;
  case SPIRV::OpTypeInt: {
    unsigned bitWidth = MI.getOperand(1).getImm();
    if (bitWidth == 64) {
      reqs.addCapability(Int64);
    } else if (bitWidth == 16) {
      reqs.addCapability(Int16);
    } else if (bitWidth == 8) {
      reqs.addCapability(Int8);
    }
    break;
  }
  case SPIRV::OpTypeFloat: {
    unsigned bitWidth = MI.getOperand(1).getImm();
    if (bitWidth == 64) {
      reqs.addCapability(Float64);
    } else if (bitWidth == 16) {
      reqs.addCapability(Float16);
    }
    break;
  }
  case SPIRV::OpTypeVector: {
    unsigned numComponents = MI.getOperand(2).getImm();
    if (numComponents == 8 || numComponents == 16) {
      reqs.addCapability(Vector16);
    }
    break;
  }
  case SPIRV::OpTypePointer: {
    auto sc = MI.getOperand(1).getImm();
    reqs.addRequirements(getStorageClassRequirements(sc, ST));
    break;
  }
  case SPIRV::OpTypeRuntimeArray:
    reqs.addCapability(Shader);
    break;
  case SPIRV::OpTypeOpaque:
  case SPIRV::OpTypeEvent:
    reqs.addCapability(Kernel);
    break;
  case SPIRV::OpTypePipe:
  case SPIRV::OpTypeReserveId:
    reqs.addCapability(Pipes);
    break;
  case SPIRV::OpTypeDeviceEvent:
  case SPIRV::OpTypeQueue:
    reqs.addCapability(DeviceEnqueue);
    break;
  case SPIRV::OpDecorate:
  case SPIRV::OpDecorateId:
  case SPIRV::OpDecorateString:
    addOpDecorateReqs(MI, 1, reqs, ST);
    break;
  case SPIRV::OpMemberDecorate:
  case SPIRV::OpMemberDecorateString:
    addOpDecorateReqs(MI, 2, reqs, ST);
    break;
  case SPIRV::OpInBoundsPtrAccessChain:
    reqs.addCapability(Addresses);
    break;
  case SPIRV::OpConstantSampler:
    reqs.addCapability(LiteralSampler);
    break;
  case SPIRV::OpTypeImage:
    addOpTypeImageReqs(MI, reqs, ST);
    break;
  case SPIRV::OpTypeSampler:
    reqs.addCapability(ImageBasic);
    break;
  case SPIRV::OpTypeForwardPointer:
    if (ST.isKernel()) {
      reqs.addCapability(Addresses);
    } else {
      reqs.addCapability(PhysicalStorageBufferAddressesEXT);
    }
    break;
  case SPIRV::OpSelect:
  case SPIRV::OpPhi:
  case SPIRV::OpFunctionCall:
  case SPIRV::OpPtrAccessChain:
  case SPIRV::OpLoad:
  case SPIRV::OpConstantNull:
    addVariablePtrInstrReqs(MI, reqs, ST);
    break;
  default:
    break;
  }
}

INITIALIZE_PASS(SPIRVAddRequirements, DEBUG_TYPE,
                "SPIRV add requirements instrs", false, false)

char SPIRVAddRequirements::ID = 0;

FunctionPass *llvm::createSPIRVAddRequirementsPass() {
  return new SPIRVAddRequirements();
}

