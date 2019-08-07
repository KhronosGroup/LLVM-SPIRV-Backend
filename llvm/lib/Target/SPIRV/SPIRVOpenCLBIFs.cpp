//===-- SPIRVOpenCLBIFs.cpp - SPIR-V OpenCL Built-In-Functions --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Function implementations for lowering OpenCL builtin types and function calls
// using their demangled names.
//
//===----------------------------------------------------------------------===//

#include "SPIRVOpenCLBIFs.h"
#include "SPIRV.h"
#include "SPIRVEnums.h"
#include "SPIRVExtInsts.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVStrings.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

#include <map>
#include <string>
#include <vector>

#define DEBUG_TYPE "opencl-bifs"

using namespace llvm;

enum CLSamplerMasks {
  CLK_ADDRESS_NONE = 0x0,
  CLK_ADDRESS_CLAMP = 0x4,
  CLK_ADDRESS_CLAMP_TO_EDGE = 0x2,
  CLK_ADDRESS_REPEAT = 0x6,
  CLK_ADDRESS_MIRRORED_REPEAT = 0x8,
  CLK_NORMALIZED_COORDS_FALSE = 0x0,
  CLK_NORMALIZED_COORDS_TRUE = 0x1,
  CLK_FILTER_NEAREST = 0x10,
  CLK_FILTER_LINEAR = 0x20,
};

enum CLMemOrder {
  memory_order_relaxed = std::memory_order::memory_order_relaxed,
  memory_order_acquire = std::memory_order::memory_order_acquire,
  memory_order_release = std::memory_order::memory_order_release,
  memory_order_acq_rel = std::memory_order::memory_order_acq_rel,
  memory_order_seq_cst = std::memory_order::memory_order_seq_cst,
};

enum CLMemScope {
  memory_scope_work_item,
  memory_scope_work_group,
  memory_scope_device,
  memory_scope_all_svm_devices,
};

enum CLMemFenceFlags {
  CLK_LOCAL_MEM_FENCE = 0x1,
  CLK_GLOBAL_MEM_FENCE = 0x2,
  CLK_IMAGE_MEM_FENCE = 0x4
};

static SamplerAddressingMode::SamplerAddressingMode
getSamplerAddressingModeFromBitmask(unsigned int bitmask) {
  if (bitmask & CLK_ADDRESS_CLAMP) {
    return SamplerAddressingMode::Clamp;
  } else if (bitmask & CLK_ADDRESS_CLAMP_TO_EDGE) {
    return SamplerAddressingMode::ClampToEdge;
  } else if (bitmask & CLK_ADDRESS_REPEAT) {
    return SamplerAddressingMode::Repeat;
  } else if (bitmask & CLK_ADDRESS_MIRRORED_REPEAT) {
    return SamplerAddressingMode::RepeatMirrored;
  } else {
    return SamplerAddressingMode::None;
  }
}

static SamplerFilterMode::SamplerFilterMode
getSamplerFilterModeFromBitmask(unsigned int bitmask) {
  if (bitmask & CLK_FILTER_LINEAR) {
    return SamplerFilterMode::Linear;
  } else if (bitmask & CLK_FILTER_NEAREST) {
    return SamplerFilterMode::Nearest;
  } else {
    return SamplerFilterMode::Nearest;
  }
}

static MemorySemantics::MemorySemantics
getSPIRVMemSemantics(CLMemOrder clMemOrder) {
  switch (clMemOrder) {
  case memory_order_relaxed:
    return MemorySemantics::None;
  case memory_order_acquire:
    return MemorySemantics::Acquire;
  case memory_order_release:
    return MemorySemantics::Release;
  case memory_order_acq_rel:
    return MemorySemantics::AcquireRelease;
  case memory_order_seq_cst:
    return MemorySemantics::SequentiallyConsistent;
  }
  llvm_unreachable("Unknown CL memory scope");
}


static Scope::Scope getSPIRVScope(CLMemScope clScope) {
  switch (clScope) {
  case memory_scope_work_item:
    return Scope::Invocation;
  case memory_scope_work_group:
    return Scope::Workgroup;
  case memory_scope_device:
    return Scope::Device;
  case memory_scope_all_svm_devices:
    return Scope::CrossDevice;
  }
  llvm_unreachable("Unknown CL memory scope");
}

static unsigned int getSamplerParamFromBitmask(unsigned int bitmask) {
  return (bitmask & CLK_NORMALIZED_COORDS_TRUE) ? 1 : 0;
}

static bool buildSamplerLiteral(uint64_t samplerBitmask, Register res,
                                SPIRVType *resTy, MachineIRBuilder &MIRBuilder,
                                SPIRVTypeRegistry *TR) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantSampler)
                 .addDef(res)
                 .addUse(TR->getSPIRVTypeID(resTy))
                 .addImm(getSamplerAddressingModeFromBitmask(samplerBitmask))
                 .addImm(getSamplerParamFromBitmask(samplerBitmask))
                 .addImm(getSamplerFilterModeFromBitmask(samplerBitmask));
  return TR->constrainRegOperands(MIB);
}

static Register buildSamplerLiteral(uint64_t samplerBitmask,
                                    MachineIRBuilder &MIRBuilder,
                                    SPIRVTypeRegistry *TR) {
  SPIRVType *sampTy = TR->getSamplerType(MIRBuilder);
  auto sampler = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  buildSamplerLiteral(samplerBitmask, sampler, sampTy, MIRBuilder, TR);
  return sampler;
}

static bool decorate(Register target, Decoration::Decoration decoration,
                     MachineIRBuilder &MIRBuilder, SPIRVTypeRegistry *TR) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpDecorate)
                 .addUse(target)
                 .addImm(decoration);
  return TR->constrainRegOperands(MIB);
}

static bool decorate(Register target, Decoration::Decoration decoration,
                     uint32_t decArg, MachineIRBuilder &MIRBuilder,
                     SPIRVTypeRegistry *TR) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpDecorate)
                 .addUse(target)
                 .addImm(decoration)
                 .addImm(decArg);
  return TR->constrainRegOperands(MIB);
}

static bool decorateLinkage(Register target, const StringRef name,
                            LinkageType::LinkageType linkageType,
                            MachineIRBuilder &MIRBuilder,
                            SPIRVTypeRegistry *TR) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpDecorate)
                 .addUse(target)
                 .addImm(Decoration::LinkageAttributes);
  addStringImm(name, MIB);
  MIB.addImm(LinkageType::Import);
  bool success = TR->constrainRegOperands(MIB);

  auto nameMIB = MIRBuilder.buildInstr(SPIRV::OpName).addUse(target);
  addStringImm(name, nameMIB);
  return success;
}

static bool decorateConstant(Register target, MachineIRBuilder &MIRBuilder,
                             SPIRVTypeRegistry *TR) {
  return decorate(target, Decoration::Constant, MIRBuilder, TR);
}

static bool decorateBuiltIn(Register target, BuiltIn::BuiltIn builtInID,
                            MachineIRBuilder &MIRBuilder,
                            SPIRVTypeRegistry *TR) {
  bool succ = decorate(target, Decoration::BuiltIn, builtInID, MIRBuilder, TR);
  succ = succ && decorateLinkage(target, getLinkStrForBuiltIn(builtInID),
                                 LinkageType::Import, MIRBuilder, TR);
  return succ;
}

static Register buildOpVariable(SPIRVType *baseType,
                                StorageClass::StorageClass storage,
                                MachineIRBuilder &MIRBuilder,
                                SPIRVTypeRegistry *TR) {

  auto resVReg = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  SPIRVType *ptrTy = TR->getOpTypePointer(storage, baseType, MIRBuilder);
  TR->assignSPIRVTypeToVReg(ptrTy, resVReg, MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpVariable)
                 .addDef(resVReg)
                 .addUse(TR->getSPIRVTypeID(ptrTy))
                 .addImm(storage);
  TR->constrainRegOperands(MIB);
  return resVReg;
}

static Register buildLoad(SPIRVType *baseType, Register ptrVRegToLoadFrom,
                          MachineIRBuilder &MIRBuilder, SPIRVTypeRegistry *TR) {
  const auto MRI = MIRBuilder.getMRI();
  Register resVReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
  TR->assignSPIRVTypeToVReg(baseType, resVReg, MIRBuilder);

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpLoad)
                 .addDef(resVReg)
                 .addUse(TR->getSPIRVTypeID(baseType))
                 .addUse(ptrVRegToLoadFrom);

  TR->constrainRegOperands(MIB);
  return resVReg;
}

static uint64_t getLiteralValueForConstant(Register constVReg,
                                           const MachineRegisterInfo *MRI) {
  MachineInstr *constInstr = MRI->getVRegDef(constVReg);
  return constInstr->getOperand(1).getCImm()->getZExtValue();
}

static Register buildOpSampledImage(Register image, Register sampler,
                                    MachineIRBuilder &MIRBuilder,
                                    SPIRVTypeRegistry *TR) {
  const auto MRI = MIRBuilder.getMRI();
  SPIRVType *imageType = TR->getSPIRVTypeForVReg(image);
  SPIRVType *sampImTy = TR->getSampledImageType(imageType, MIRBuilder);

  Register sampledImage = MRI->createVirtualRegister(&SPIRV::IDRegClass);
  auto SampImMIB = MIRBuilder.buildInstr(SPIRV::OpSampledImage)
                       .addDef(sampledImage)
                       .addUse(TR->getSPIRVTypeID(sampImTy))
                       .addUse(image)
                       .addUse(sampler);
  TR->constrainRegOperands(SampImMIB);
  return sampledImage;
}

static uint64_t getIConstVal(Register constReg,
                             const MachineRegisterInfo *MRI) {
  const auto c = MRI->getVRegDef(constReg);
  assert(c && c->getOpcode() == TargetOpcode::G_CONSTANT);
  return c->getOperand(1).getCImm()->getValue().getZExtValue();
}

static void buildIConstant(Register Reg, uint64_t Val, SPIRVType *type,
                           MachineIRBuilder &MIRBuilder,
                           SPIRVTypeRegistry *TR) {
  assert(type->getOpcode() == SPIRV::OpTypeInt);
  TR->assignSPIRVTypeToVReg(type, Reg, MIRBuilder);
  auto &C = MIRBuilder.getMF().getFunction().getContext();
  const auto intTy = IntegerType::get(C, TR->getScalarOrVectorBitWidth(type));
  MIRBuilder.buildConstant(Reg, *ConstantInt::get(intTy, Val));
}

static Register buildIConstant(uint64_t Val, SPIRVType *type,
                               MachineIRBuilder &MIRBuilder,
                               SPIRVTypeRegistry *TR) {
  auto llt = LLT::scalar(TR->getScalarOrVectorBitWidth(type));
  auto Reg = MIRBuilder.getMRI()->createGenericVirtualRegister(llt);
  buildIConstant(Reg, Val, type, MIRBuilder, TR);
  return Reg;
}

static Register buildConstantFZero(MachineIRBuilder &MIRBuilder,
                                   SPIRVTypeRegistry *TR) {
  const auto MRI = MIRBuilder.getMRI();
  Register resVReg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  const auto &fsingle = APFloat::IEEEsingle();
  auto &Context = MIRBuilder.getMF().getFunction().getContext();
  const auto fzero = ConstantFP::get(Context, APFloat::getZero(fsingle));

  MIRBuilder.buildFConstant(resVReg, *fzero);
  TR->assignTypeToVReg(fzero->getType(), resVReg, MIRBuilder);
  return resVReg;
}

// These queries ask for a single size_t result for a given dimension index, e.g
// size_t get_global_id(uintt dimindex). In SPIR-V, the builtins corresonding to
// these values are all vec3 types, so we need to extract the correct index or
// return defaultVal (0 or 1 depending on the query). We also handle extending
// or tuncating in case size_t does not match the expected result type's
// bitwidth.
//
// For a constant index >= 3 we generate:
//  %res = OpConstant %SizeT 0
//
// For other indices we generate:
//  %g = OpVariable %ptr_V3_SizeT Input
//  OpDecorate %g BuiltIn XXX
//  OpDecorate %g LinkageAttributes "__spirv_BuiltInXXX"
//  OpDecorate %g Constant
//  %loadedVec = OpLoad %V3_SizeT %g
//
//  Then, if the index is constant < 3, we generate:
//    %res = OpCompositeExtract %SizeT %loadedVec idx
//  If the index is dynamic, we generate:
//    %tmp = OpVectorExtractDynamic %SizeT %loadedVec %idx
//    %cmp = OpULessThan %bool %idx %const_3
//    %res = OpSelect %SizeT %cmp %tmp %const_0
//
//  If the bitwidth of %res does not match the expected return type, we add an
//  extend or truncate.
//
static bool genWorkgroupQuery(MachineIRBuilder &MIRBuilder, Register resVReg,
                              SPIRVType *retType,
                              const SmallVectorImpl<Register> &OrigArgs,
                              SPIRVTypeRegistry *TR, BuiltIn::BuiltIn builtIn,
                              uint64_t defaultVal) {
  Register idxVReg = OrigArgs[0];

  const unsigned resWidth = retType->getOperand(1).getImm();
  const unsigned int ptrSize = TR->getPointerSize();

  const auto sizeT = TR->getPtrUIntType(MIRBuilder);

  const auto MRI = MIRBuilder.getMRI();
  auto idxInstr = MRI->getVRegDef(idxVReg);

  // Set up the final register to do truncation or extension on at the end.
  Register toTruncate = resVReg;

  // If the index is constant, we can statically determine if it is in range
  bool isConstantIndex = idxInstr->getOpcode() == TargetOpcode::G_CONSTANT;

  // If it's out of range (max dimension is 3), we can just return the constant
  // default value(0 or 1 depending on which query function)
  if (isConstantIndex && getLiteralValueForConstant(idxVReg, MRI) >= 3) {
    Register defaultReg = resVReg;
    if (ptrSize != resWidth) {
      defaultReg = MRI->createGenericVirtualRegister(LLT::scalar(ptrSize));
      TR->assignSPIRVTypeToVReg(sizeT, defaultReg, MIRBuilder);
      toTruncate = defaultReg;
    }
    buildIConstant(defaultReg, defaultVal, sizeT, MIRBuilder, TR);
  } else { // If it could be in range, we need to load from the given builtin

    auto v3 = TR->getOpTypeVector(3, sizeT, MIRBuilder);

    // Set up the global OpVariable with the necessary builtin decorations
    Register globVar = buildOpVariable(v3, StorageClass::Input, MIRBuilder, TR);
    decorateBuiltIn(globVar, builtIn, MIRBuilder, TR);
    decorateConstant(globVar, MIRBuilder, TR);

    // Load the Vec3 from the global variable
    Register loadedVector = buildLoad(v3, globVar, MIRBuilder, TR);
    MRI->setType(loadedVector, LLT::vector(3, ptrSize));

    // Set up the vreg to extract the result to (possibly a new temporary one)
    Register extr = resVReg;
    if (!isConstantIndex || ptrSize != resWidth) {
      extr = MRI->createGenericVirtualRegister(LLT::scalar(ptrSize));
      TR->assignSPIRVTypeToVReg(sizeT, extr, MIRBuilder);
    }

    // Use G_EXTRACT_VECTOR_ELT so dynamic vs static extraction is handled later
    MIRBuilder.buildExtractVectorElement(extr, loadedVector, idxVReg);

    // If the index is dynamic, need check if it's < 3, and then use a select
    if (!isConstantIndex) {
      auto idxTy = TR->getSPIRVTypeForVReg(idxVReg);
      auto boolTy = TR->getOpTypeBool(MIRBuilder);

      Register cmpReg = MRI->createGenericVirtualRegister(LLT::scalar(1));
      TR->assignSPIRVTypeToVReg(boolTy, cmpReg, MIRBuilder);

      // Use G_ICMP to check if idxVReg < 3
      Register three = buildIConstant(3, idxTy, MIRBuilder, TR);
      MIRBuilder.buildICmp(CmpInst::ICMP_ULT, cmpReg, idxVReg, three);

      // Get constant for the default value (0 or 1 depending on which function)
      Register defaultReg = buildIConstant(defaultVal, sizeT, MIRBuilder, TR);

      // Get a register for the selection result (possibly a new temporary one)
      Register selRes = resVReg;
      if (ptrSize != resWidth) {
        selRes = MRI->createGenericVirtualRegister(LLT::scalar(ptrSize));
        TR->assignSPIRVTypeToVReg(sizeT, selRes, MIRBuilder);
      }

      // Create the final G_SELECT to return the extracted value or the default
      MIRBuilder.buildSelect(selRes, cmpReg, extr, defaultReg);
      toTruncate = selRes;
    } else {
      toTruncate = extr;
    }
  }

  // Alter the result's bitwidth if it does not match the SizeT value extracted
  if (ptrSize != resWidth) {
    MIRBuilder.buildZExtOrTrunc(resVReg, toTruncate);
  }

  return true;
}

static bool genSampledReadImage(MachineIRBuilder &MIRBuilder, Register resVReg,
                                SPIRVType *retType,
                                const SmallVectorImpl<Register> &OrigArgs,
                                SPIRVTypeRegistry *TR) {

  const auto MRI = MIRBuilder.getMRI();

  Register image = OrigArgs[0];

  Register sampler = OrigArgs[1];
  if (!TR->isScalarOfType(OrigArgs[1], SPIRV::OpTypeSampler)) {
    auto sampMask = getLiteralValueForConstant(OrigArgs[1], MRI);
    sampler = buildSamplerLiteral(sampMask, MIRBuilder, TR);
  }

  Register sampledImage = buildOpSampledImage(image, sampler, MIRBuilder, TR);

  Register coord = OrigArgs[2];
  Register lod = buildConstantFZero(MIRBuilder, TR);

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageSampleExplicitLod)
                 .addDef(resVReg)
                 .addUse(TR->getSPIRVTypeID(retType))
                 .addUse(sampledImage)
                 .addUse(coord)
                 .addImm(ImageOperand::Lod)
                 .addUse(lod);
  return TR->constrainRegOperands(MIB);
}

static bool genReadImage(MachineIRBuilder &MIRBuilder, Register resVReg,
                         SPIRVType *retType,
                         const SmallVectorImpl<Register> &OrigArgs,
                         SPIRVTypeRegistry *TR) {
  Register image = OrigArgs[0];
  Register coord = OrigArgs[1];
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageRead)
                 .addDef(resVReg)
                 .addUse(TR->getSPIRVTypeID(retType))
                 .addUse(image)
                 .addUse(coord);
  return TR->constrainRegOperands(MIB);
}

static bool genWriteImage(MachineIRBuilder &MIRBuilder,
                          const SmallVectorImpl<Register> &OrigArgs,
                          SPIRVTypeRegistry *TR) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageWrite)
                 .addUse(OrigArgs[0])  // Image
                 .addUse(OrigArgs[1])  // Coord vec2
                 .addUse(OrigArgs[2]); // Texel to write vec4
  return TR->constrainRegOperands(MIB);
}

static unsigned getNumComponentsForDim(Dim::Dim dim) {
  using namespace Dim;
  switch (dim) {
  case DIM_1D:
  case DIM_Buffer:
    return 1;
  case DIM_2D:
  case DIM_Cube:
  case DIM_Rect:
    return 2;
  case DIM_3D:
    return 3;
  default:
    llvm_unreachable("Cannot get num components for given Dim");
  }
}

static unsigned int getNumSizeComponents(SPIRVType *imgType) {
  assert(imgType->getOpcode() == SPIRV::OpTypeImage);
  auto dim = static_cast<Dim::Dim>(imgType->getOperand(2).getImm());
  unsigned numComps = getNumComponentsForDim(dim);
  bool arrayed = imgType->getOperand(4).getImm() == 1;
  return arrayed ? numComps + 1 : numComps;
}

// Used for get_image_width, get_image_dim etc. via OpImageQuerySize
static bool genImageSizeQuery(MachineIRBuilder &MIRBuilder, Register resVReg,
                              SPIRVType *retType, Register img,
                              unsigned component, SPIRVTypeRegistry *TR) {
  const auto MRI = MIRBuilder.getMRI();

  unsigned numRetComps = 1;
  if (retType->getOpcode() == SPIRV::OpTypeVector) {
    numRetComps = retType->getOperand(2).getImm();
  }

  SPIRVType *imgType = TR->getSPIRVTypeForVReg(img);
  unsigned numTempComps = getNumSizeComponents(imgType);

  Register sizeVec = resVReg;
  SPIRVType *sizeVecTy = retType;
  if (numTempComps != numRetComps) {
    sizeVec = MRI->createGenericVirtualRegister(LLT::vector(numTempComps, 32));
    auto I32Ty = TR->getOpTypeInt(32, MIRBuilder);
    sizeVecTy = TR->getOpTypeVector(numTempComps, I32Ty, MIRBuilder);
    TR->assignSPIRVTypeToVReg(sizeVecTy, sizeVec, MIRBuilder);
  }

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageQuerySize)
                 .addDef(sizeVec)
                 .addUse(TR->getSPIRVTypeID(sizeVecTy))
                 .addUse(img);
  bool success = TR->constrainRegOperands(MIB);
  if (numTempComps == numRetComps || !success) {
    return success;
  }
  if (numRetComps == 1) {
    // Need an OpCompositeExtract
    assert(component < numTempComps && "Invalid comp index");
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpCompositeExtract)
                   .addDef(resVReg)
                   .addUse(TR->getSPIRVTypeID(retType))
                   .addUse(sizeVec)
                   .addImm(component);
    return TR->constrainRegOperands(MIB);
  } else {
    // Need an OpShuffleVector to make the struct vector the right size
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpVectorShuffle)
                   .addDef(resVReg)
                   .addUse(TR->getSPIRVTypeID(retType))
                   .addUse(sizeVec)
                   .addUse(sizeVec);
    for (unsigned int i = 0; i < numRetComps; ++i) {
      // TODO is padding with undefined indices ok, or do we need an explicit 0
      uint32_t compIdx = i < numTempComps ? i : 0xffffffff;
      MIB.addImm(compIdx);
    }
    return TR->constrainRegOperands(MIB);
  }
}

static bool genGlobalLocalQuery(MachineIRBuilder &MIRBuilder,
                                const StringRef globLocStr, bool global,
                                Register ret, SPIRVType *retTy,
                                const SmallVectorImpl<Register> &args,
                                SPIRVTypeRegistry *TR) {
  if (globLocStr.startswith("id")) {
    auto BI = global ? BuiltIn::GlobalInvocationId : BuiltIn::LocalInvocationId;
    return genWorkgroupQuery(MIRBuilder, ret, retTy, args, TR, BI, 0);
  } else if (globLocStr.startswith("size")) {
    auto BI = global ? BuiltIn::GlobalSize : BuiltIn::WorkgroupSize;
    return genWorkgroupQuery(MIRBuilder, ret, retTy, args, TR, BI, 1);
  } else if (globLocStr.startswith("linear_id")) {
    // TODO
  } else if (global && globLocStr.startswith("offset")) {
    // TODO
  }
  report_fatal_error("Cannot handle OpenCL global/local query: " + globLocStr);
}

static bool genImageQuery(MachineIRBuilder &MIRBuilder,
                          const StringRef queryStr, Register resVReg,
                          SPIRVType *retType,
                          const SmallVectorImpl<Register> &OrigArgs,
                          SPIRVTypeRegistry *TR) {
  assert(OrigArgs.size() == 1 && "Image queries require 1 arguAment");
  Register img = OrigArgs[0];
  if (queryStr.startswith("width")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 0, TR);
  } else if (queryStr.startswith("height")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 1, TR);
  } else if (queryStr.startswith("depth")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 2, TR);
  } else if (queryStr.startswith("dim")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 0, TR);
  }
  report_fatal_error("Cannot handle OpenCL img query: get_image_" + queryStr);
}

static bool genAtomicCmpXchg(MachineIRBuilder &MIRBuilder, Register resVReg,
                             SPIRVType *retType, bool isWeak,
                             const SmallVectorImpl<Register> &OrigArgs,
                             SPIRVTypeRegistry *TR) {
  assert(OrigArgs.size() >= 3 && "Need 3+ args to atomic_compare_exchange");
  using namespace SPIRV;
  const auto MRI = MIRBuilder.getMRI();

  Register objectPtr = OrigArgs[0];   // Pointer (volatile A *object)
  Register expectedPtr = OrigArgs[1]; // Comparator (C* expected)
  Register desired = OrigArgs[2];     // Value (C desired)
  SPIRVType *spvDesiredTy = TR->getSPIRVTypeForVReg(desired);
  LLT desiredLLT = MRI->getType(desired);

  assert(TR->getSPIRVTypeForVReg(objectPtr)->getOpcode() == OpTypePointer);
  assert(TR->getSPIRVTypeForVReg(expectedPtr)->getOpcode() == OpTypePointer);
  assert(TR->isScalarOfType(desired, OpTypeInt));

  SPIRVType *spvObjectPtrTy = TR->getSPIRVTypeForVReg(objectPtr);
  auto storageClass = static_cast<StorageClass::StorageClass>(
      spvObjectPtrTy->getOperand(1).getImm());
  auto memSemStorage = getMemSemanticsForStorageClass(storageClass);

  Register memSemEqualReg;
  Register memSemUnequalReg;
  auto memSemEqual = MemorySemantics::SequentiallyConsistent | memSemStorage;
  auto memSemUnequal = MemorySemantics::SequentiallyConsistent | memSemStorage;
  if (OrigArgs.size() >= 4) {
    assert(OrigArgs.size() >= 5 && "Need 5+ args for explicit atomic cmpxchg");
    auto memOrdEq = static_cast<CLMemOrder>(getIConstVal(OrigArgs[3], MRI));
    auto memOrdNeq = static_cast<CLMemOrder>(getIConstVal(OrigArgs[4], MRI));
    memSemEqual = getSPIRVMemSemantics(memOrdEq) | memSemStorage;
    memSemUnequal = getSPIRVMemSemantics(memOrdNeq) | memSemStorage;
    if (memOrdEq == memSemEqual)
      memSemEqualReg = OrigArgs[3];
    if (memOrdNeq == memSemEqual)
      memSemUnequalReg = OrigArgs[4];
  }
  auto I32Ty = TR->getOpTypeInt(32, MIRBuilder);
  if (!memSemEqualReg.isValid())
    memSemEqualReg = buildIConstant(memSemEqual, I32Ty, MIRBuilder, TR);
  if (!memSemUnequalReg.isValid())
    memSemUnequalReg = buildIConstant(memSemUnequal, I32Ty, MIRBuilder, TR);

  Register scopeReg;
  auto scope = Scope::Device;
  if (OrigArgs.size() >= 6) {
    assert(OrigArgs.size() == 6 && "Extra args for explicit atomic cmpxchg");
    auto clScope = static_cast<CLMemScope>(getIConstVal(OrigArgs[5], MRI));
    scope = getSPIRVScope(clScope);
    if (clScope == static_cast<unsigned>(scope))
      scopeReg = OrigArgs[5];
  }
  if (!scopeReg.isValid())
    scopeReg = buildIConstant(scope, I32Ty, MIRBuilder, TR);

  Register expected = buildLoad(spvDesiredTy, expectedPtr, MIRBuilder, TR);
  MRI->setType(expected, desiredLLT);
  Register tmp = MRI->createGenericVirtualRegister(desiredLLT);
  TR->assignSPIRVTypeToVReg(spvDesiredTy, tmp, MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(OpAtomicCompareExchange)
                 .addDef(tmp)
                 .addUse(TR->getSPIRVTypeID(retType))
                 .addUse(objectPtr)
                 .addUse(scopeReg)
                 .addUse(memSemEqualReg)
                 .addUse(memSemUnequalReg)
                 .addUse(desired)
                 .addUse(expected);
  MIRBuilder.buildInstr(OpStore).addUse(expected).addUse(tmp);
  MIRBuilder.buildICmp(CmpInst::ICMP_EQ, resVReg, tmp, expected);
  return TR->constrainRegOperands(MIB);
}

static bool genAtomicRMW(Register resVReg, const SPIRVType *resType,
                         unsigned RMWOpcode, MachineIRBuilder &MIRBuilder,
                         const SmallVectorImpl<Register> &OrigArgs,
                         SPIRVTypeRegistry *TR) {
  assert(OrigArgs.size() >= 2 && "Need 2+ args to atomic RMW instr");
  const auto MRI = MIRBuilder.getMRI();
  auto I32Ty = TR->getOpTypeInt(32, MIRBuilder);

  Register scopeReg;
  auto scope = Scope::Device;
  if (OrigArgs.size() >= 4) {
    assert(OrigArgs.size() == 4 && "Extra args for explicit atomic RMW");
    auto clScope = static_cast<CLMemScope>(getIConstVal(OrigArgs[5], MRI));
    scope = getSPIRVScope(clScope);
    if (clScope == static_cast<unsigned>(scope))
      scopeReg = OrigArgs[5];
  }
  if (!scopeReg.isValid())
    scopeReg = buildIConstant(scope, I32Ty, MIRBuilder, TR);

  auto ptr = OrigArgs[0];
  auto scSem = getMemSemanticsForStorageClass(TR->getPointerStorageClass(ptr));

  Register memSemReg;
  auto memSem = MemorySemantics::SequentiallyConsistent | scSem;
  if (OrigArgs.size() >= 3) {
    auto memOrd = static_cast<CLMemOrder>(getIConstVal(OrigArgs[2], MRI));
    memSem = getSPIRVMemSemantics(memOrd) | scSem;
    if (memOrd == memSem)
      memSemReg = OrigArgs[3];
  }
  if (!memSemReg.isValid())
    memSemReg = buildIConstant(memSem, I32Ty, MIRBuilder, TR);

  auto MIB = MIRBuilder.buildInstr(RMWOpcode)
                 .addDef(resVReg)
                 .addUse(TR->getSPIRVTypeID(resType))
                 .addUse(ptr)
                 .addUse(scopeReg)
                 .addUse(memSemReg)
                 .addUse(OrigArgs[1]);
  return TR->constrainRegOperands(MIB);
}

static bool genAtomicInstr(MachineIRBuilder &MIRBuilder,
                           const StringRef atomicStr, Register ret,
                           SPIRVType *retTy,
                           const SmallVectorImpl<Register> &args,
                           SPIRVTypeRegistry *TR) {
  using namespace SPIRV;
  if (atomicStr.startswith("compare_exchange_")) {
    const auto cmp_xchg = atomicStr.substr(strlen("compare_exchange_"));
    if (cmp_xchg.startswith("weak")) {
      return genAtomicCmpXchg(MIRBuilder, ret, retTy, true, args, TR);
    } else if (cmp_xchg.startswith("strong")) {
      return genAtomicCmpXchg(MIRBuilder, ret, retTy, false, args, TR);
    }
  } else if (atomicStr.startswith("add")) {
    return genAtomicRMW(ret, retTy, OpAtomicIAdd, MIRBuilder, args, TR);
  } else if (atomicStr.startswith("sub")) {
    return genAtomicRMW(ret, retTy, OpAtomicISub, MIRBuilder, args, TR);
  } else if (atomicStr.startswith("or")) {
    return genAtomicRMW(ret, retTy, OpAtomicOr, MIRBuilder, args, TR);
  } else if (atomicStr.startswith("xor")) {
    return genAtomicRMW(ret, retTy, OpAtomicXor, MIRBuilder, args, TR);
  } else if (atomicStr.startswith("and")) {
    return genAtomicRMW(ret, retTy, OpAtomicAnd, MIRBuilder, args, TR);
  } else if (atomicStr[0] == 'm') {
    // TODO check the arg types for signedness
    // if (atomicStr.startswith("min")) {
    //   return genAtomicRMW(ret, retTy, OpAtomicUMax, MIRBuilder, args, TR);
    // } else if (atomicStr.startswith("max")) {
    //   return genAtomicRMW(ret, retTy, OpAtomicUMax, MIRBuilder, args, TR);
    // }
  } else if (atomicStr.startswith("exchange")) {
    return genAtomicRMW(ret, retTy, OpAtomicExchange, MIRBuilder, args, TR);
  }

  report_fatal_error("Cannot handle OpenCL atomic func: atomic_" + atomicStr);
}

static bool genBarrier(MachineIRBuilder &MIRBuilder,
                       const SmallVectorImpl<Register> &OrigArgs,
                       SPIRVTypeRegistry *TR) {
  assert(OrigArgs.size() >= 1 && "Missing args for OpenCL barrier func");
  const auto MRI = MIRBuilder.getMRI();
  const auto I32Ty = TR->getOpTypeInt(32, MIRBuilder);

  unsigned memFlags = getIConstVal(OrigArgs[0], MRI);
  unsigned memSem = MemorySemantics::None;
  if (memFlags & CLK_LOCAL_MEM_FENCE) {
    memSem |= MemorySemantics::WorkgroupMemory;
  }
  if (memFlags & CLK_GLOBAL_MEM_FENCE) {
    memSem |= MemorySemantics::CrossWorkgroupMemory;
  }
  if (memFlags & CLK_IMAGE_MEM_FENCE) {
    memSem |= MemorySemantics::ImageMemory;
  }
  Register memSemReg;
  if (memFlags == memSem) {
    memSemReg = OrigArgs[0];
  } else {
    memSemReg = buildIConstant(memSem, I32Ty, MIRBuilder, TR);
  }

  Register scopeReg;
  auto scope = Scope::Workgroup;
  if (OrigArgs.size() >= 2) {
    assert(OrigArgs.size() == 2 && "Extra args for explicitly scoped barrier");
    auto clScope = static_cast<CLMemScope>(getIConstVal(OrigArgs[1], MRI));
    if (!(memFlags & CLK_LOCAL_MEM_FENCE)) {
      scope = getSPIRVScope(clScope);
    }
    if (clScope == static_cast<unsigned>(scope))
      scopeReg = OrigArgs[1];
  }
  if (!scopeReg.isValid())
    scopeReg = buildIConstant(scope, I32Ty, MIRBuilder, TR);

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpControlBarrier)
                 .addUse(scopeReg)
                 .addUse(scopeReg)
                 .addUse(memSemReg);
  return TR->constrainRegOperands(MIB);
}

static bool genConvertInstr(MachineIRBuilder &MIRBuilder,
                            const StringRef convertStr, Register ret,
                            SPIRVType *retTy,
                            const SmallVectorImpl<Register> &args,
                            SPIRVTypeRegistry *TR) {
  using namespace SPIRV;
  Register src = args[0];
  bool isFromInt = TR->isScalarOrVectorOfType(src, OpTypeInt);
  bool isFromFloat = !isFromInt && TR->isScalarOrVectorOfType(src, OpTypeFloat);

  bool isToInt = TR->isScalarOrVectorOfType(ret, OpTypeInt);
  bool isToFloat = !isToInt && TR->isScalarOrVectorOfType(ret, OpTypeFloat);

  bool dstSign = convertStr[0] != 'u';

  auto braceIdx = convertStr.find_first_of('(');
  bool srcSign = convertStr[braceIdx + 1] != 'u';

  bool isSat = false;
  bool isRounded = false;
  auto roundingMode = FPRoundingMode::RTE;

  auto underscore0 = convertStr.find_first_of('_');
  if (underscore0 != convertStr.npos) {
    auto tok = convertStr.substr(underscore0 + 1, braceIdx - underscore0 - 1);
    auto underscore1 = tok.find_first_of('_');
    if (underscore1 != convertStr.npos) {
      auto tok0 = tok.substr(0, underscore1);
      auto tok1 = tok.substr(underscore1 + 1);
      assert(tok0 == "sat");
      isSat = true;
      tok = tok1;
    }
    if (tok == "sat") {
      isSat = true;
    } else if (tok.startswith("rt")) {
      using namespace FPRoundingMode;
      isRounded = true;
      char r = tok[2]; // rtz, rtp, rtn, rte
      roundingMode = r == 'z' ? RTZ : r == 'p' ? RTP : r == 'n' ? RTN : RTE;
    }
  }

  namespace Dec = Decoration;
  bool success = true;
  if (isSat)
    success &= decorate(ret, Dec::SaturatedConversion, MIRBuilder, TR);
  if (isRounded && success)
    success &= decorate(ret, Dec::FPRoundingMode, roundingMode, MIRBuilder, TR);
  if (!success)
    return false;

  if (isFromInt) {
    if (isToInt) { // I -> I
      auto op = dstSign ? OpUConvert : OpSConvert;
      op = !isSat ? op : (dstSign ? OpSatConvertUToS : OpSatConvertSToU);
      auto MIB = MIRBuilder.buildInstr(dstSign ? OpUConvert : OpSConvert)
                     .addDef(ret)
                     .addUse(TR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return TR->constrainRegOperands(MIB);
    } else if (isToFloat) { // I -> F
      auto MIB = MIRBuilder.buildInstr(srcSign ? OpConvertSToF : OpConvertUToF)
                     .addDef(ret)
                     .addUse(TR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return TR->constrainRegOperands(MIB);
    }
  } else if (isFromFloat) {
    if (isToInt) { // F -> I
      auto MIB = MIRBuilder.buildInstr(dstSign ? OpConvertFToS : OpConvertFToU)
                     .addDef(ret)
                     .addUse(TR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return TR->constrainRegOperands(MIB);
    } else if (isToFloat) { // F -> F
      auto MIB = MIRBuilder.buildInstr(OpFConvert)
                     .addDef(ret)
                     .addUse(TR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return TR->constrainRegOperands(MIB);
    }
  }

  report_fatal_error("Convert instr not implemented yet: " + convertStr);
}

static SPIRVType *buildOpTypeImageCL(Dim::Dim dim, AQ::AccessQualifier access,
                                     MachineIRBuilder &MIRBuilder,
                                     SPIRVTypeRegistry *TR) {
  SPIRVType *voidTy = TR->getOpTypeVoid(MIRBuilder);
  return TR->getOpTypeImage(MIRBuilder, voidTy, dim, 0, 0, 0, 0,
                            ImageFormat::Unknown, access);
}

static bool genOpenCLExtInst(OpenCL_std::OpenCL_std extInstID,
                             MachineIRBuilder &MIRBuilder, Register ret,
                             const SPIRVType *retTy,
                             const SmallVectorImpl<Register> &args,
                             SPIRVTypeRegistry *TR) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                 .addDef(ret)
                 .addUse(TR->getSPIRVTypeID(retTy))
                 .addImm(static_cast<uint32_t>(ExtInstSet::OpenCL_std))
                 .addImm(extInstID);
  for (const auto &arg : args) {
    MIB.addUse(arg);
  }
  return TR->constrainRegOperands(MIB);
}

SPIRVType *llvm::generateOpenCLOpaqueType(const StringRef name,
                                          MachineIRBuilder &MIRBuilder,
                                          SPIRVTypeRegistry *TR,
                                          AQ::AccessQualifier accessQual) {
  assert(name.startswith("opencl.") && "CL types should start with 'opencl.'");
  using namespace Dim;
  auto typeName = name.substr(strlen("opencl."));

  if (typeName.startswith("image")) {
    if (typeName.endswith("_ro_t")) {
      accessQual = AQ::ReadOnly;
    } else if (typeName.endswith("_wo_t")) {
      accessQual = AQ::WriteOnly;
    } else if (typeName.endswith("_rw_t")) {
      accessQual = AQ::ReadWrite;
    }
    char dimC = typeName[strlen("image")];
    if (dimC >= '1' && dimC <= '3') {
      auto dim = dimC == '1' ? DIM_1D : dimC == '2' ? DIM_2D : DIM_3D;
      return buildOpTypeImageCL(dim, accessQual, MIRBuilder, TR);
    }
  } else if (typeName.startswith("sampler_t")) {
    return TR->getSamplerType(MIRBuilder);
  }
  report_fatal_error("Cannot generate OpenCL type: " + name);
}

bool llvm::generateOpenCLBuiltinCall(const StringRef demangledName,
                                     MachineIRBuilder &MIRBuilder, Register ret,
                                     const Type *OrigRetTy,
                                     const SmallVectorImpl<Register> &args,
                                     SPIRVTypeRegistry *TR) {

  LLVM_DEBUG(dbgs() << "Generating OpenCL Builtin: " << demangledName << "\n");

  SPIRVType *retTy = nullptr;
  if (OrigRetTy && !OrigRetTy->isVoidTy()) {
    retTy = TR->assignTypeToVReg(OrigRetTy, ret, MIRBuilder);
  }

  auto firstBraceIdx = demangledName.find_first_of('(');
  auto nameNoArgs = demangledName.substr(0, firstBraceIdx);

  auto extInstIDOpt = getOpenCL_stdFromName(nameNoArgs);
  if (extInstIDOpt.hasValue()) {
    auto extInstID = extInstIDOpt.getValue();
    return genOpenCLExtInst(extInstID, MIRBuilder, ret, retTy, args, TR);
  }

  namespace CL = OpenCL_std;
  static const std::map<std::string, std::vector<CL::OpenCL_std>>
      typeDependantExtInstMap = {
          {"clamp", {CL::u_clamp, CL::s_clamp, CL::fclamp}},
          {"max", {CL::u_max, CL::s_max, CL::fmax_common}},
          {"min", {CL::u_min, CL::s_min, CL::fmin_common}},
          {"abs", {CL::u_abs, CL::s_abs, CL::fabs}},
          {"abs_diff", {CL::u_abs_diff, CL::s_abs_diff}},
          {"add_sat", {CL::u_add_sat, CL::s_add_sat}},
          {"hadd", {CL::u_hadd, CL::s_hadd}},
          {"mad24", {CL::u_mad24, CL::s_mad24}},
          {"mad_hi", {CL::u_mad_hi, CL::s_mad_hi}},
          {"mul24", {CL::u_mul24, CL::s_mul24}},
          {"mul_hi", {CL::u_mul_hi, CL::s_mul_hi}},
          {"rhadd", {CL::u_rhadd, CL::s_rhadd}},
          {"sub_sat", {CL::u_sub_sat, CL::s_sub_sat}},
          {"upsample", {CL::u_upsample, CL::s_upsample}}};

  auto extInstMatch = typeDependantExtInstMap.find(nameNoArgs);
  if (extInstMatch != typeDependantExtInstMap.end()) {
    char typeChar = demangledName[firstBraceIdx + 1];
    int idx = -1;
    if (typeChar == 'u') {
      idx = 0;
    } else if (typeChar == 'i') {
      idx = 1;
    } else if (typeChar == 'f' || typeChar == 'd') {
      idx = 2;
    }
    if (idx != -1) {
      assert(unsigned(idx) < extInstMatch->second.size());
      CL::OpenCL_std extInstId = extInstMatch->second[idx];
      return genOpenCLExtInst(extInstId, MIRBuilder, ret, retTy, args, TR);
    }
  }

  char firstChar = nameNoArgs[0];
  switch (firstChar) {
  case 'a':
    if (nameNoArgs.startswith("atom")) {
      // Handle atom_add, atomic_add, and atomic_fetch_add etc.
      auto idx = strlen("atom_");
      if (nameNoArgs.startswith("atomic_")) {
        idx = nameNoArgs.startswith("atomic_fetch_") ? strlen("atomic_fetch_")
                                                     : strlen("atomic_");
      }
      const auto atomic = nameNoArgs.substr(idx);
      return genAtomicInstr(MIRBuilder, atomic, ret, retTy, args, TR);
    }
    break;
  case 'b':
    if (nameNoArgs.startswith("barrier")) {
      return genBarrier(MIRBuilder, args, TR);
    }
    break;
  case 'c':
    if (nameNoArgs.startswith("convert_")) {
      const auto convTy = demangledName.substr(strlen("convert_"));
      return genConvertInstr(MIRBuilder, convTy, ret, retTy, args, TR);
    }
    break;
  case 'g': {
    bool local = nameNoArgs.startswith("get_local_");
    bool global = !local && nameNoArgs.startswith("get_global_");
    if (local || global) {
      auto subIdx = global ? strlen("get_global_") : strlen("get_local_");
      const auto str = nameNoArgs.substr(subIdx);
      return genGlobalLocalQuery(MIRBuilder, str, global, ret, retTy, args, TR);
    } else if (nameNoArgs.startswith("get_image_")) {
      const auto imgStr = nameNoArgs.substr(strlen("get_image_"));
      return genImageQuery(MIRBuilder, imgStr, ret, retTy, args, TR);
    } else if (nameNoArgs.startswith("get_group_id")) {
      return genWorkgroupQuery(MIRBuilder, ret, retTy, args, TR,
                               BuiltIn::WorkgroupId, 0);
    } else if (nameNoArgs.startswith("get_enqueued_local_size")) {
      return genWorkgroupQuery(MIRBuilder, ret, retTy, args, TR,
                               BuiltIn::EnqueuedWorkgroupSize, 1);
    } else if (nameNoArgs.startswith("get_num_groups")) {
      return genWorkgroupQuery(MIRBuilder, ret, retTy, args, TR,
                               BuiltIn::NumWorkGroups, 1);
    } else if (nameNoArgs.startswith("get_work_dim")) {
      // TODO
    }
    break;
  }
  case 'r':
    if (nameNoArgs.startswith("read_image")) {
      if (args.size() > 2) {
        return genSampledReadImage(MIRBuilder, ret, retTy, args, TR);
      } else {
        return genReadImage(MIRBuilder, ret, retTy, args, TR);
      }
    }
    break;
  case 'w':
    if (nameNoArgs.startswith("write_image")) {
      return genWriteImage(MIRBuilder, args, TR);
    } else if (nameNoArgs.startswith("work_group_barrier")) {
      return genBarrier(MIRBuilder, args, TR);
    }
    break;
  case '_':
    if (nameNoArgs.startswith("__translate_sampler_initializer")) {
      return buildSamplerLiteral(args[0], ret, retTy, MIRBuilder, TR);
    }
    break;
  default:
    break;
  }
  report_fatal_error("Cannot translate OpenCL built-in func: " + demangledName);
}

