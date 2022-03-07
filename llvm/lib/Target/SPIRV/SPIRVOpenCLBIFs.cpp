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
#include "SPIRVExtInsts.h"
#include "SPIRVSymbolicOperands.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVUtils.h"

#include "llvm/IR/IntrinsicsSPIRV.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
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
  CLK_ADDRESS_MODE_MASK = 0xE,
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
  memory_scope_sub_group
};

enum CLMemFenceFlags {
  CLK_LOCAL_MEM_FENCE = 0x1,
  CLK_GLOBAL_MEM_FENCE = 0x2,
  CLK_IMAGE_MEM_FENCE = 0x4
};

static StringMap<unsigned> OpenCLBIToOperationMap({
#define _SPIRV_OP(x, y) {#x, SPIRV::Op##y}, {"__spirv_"#y, SPIRV::Op##y},
  _SPIRV_OP(isequal, FOrdEqual)
  _SPIRV_OP(isnotequal, FUnordNotEqual)
  _SPIRV_OP(isgreater, FOrdGreaterThan)
  _SPIRV_OP(isgreaterequal, FOrdGreaterThanEqual)
  _SPIRV_OP(isless, FOrdLessThan)
  _SPIRV_OP(islessequal, FOrdLessThanEqual)
  _SPIRV_OP(islessgreater, FOrdNotEqual)
  _SPIRV_OP(isordered, Ordered)
  _SPIRV_OP(isunordered, Unordered)
  _SPIRV_OP(isfinite, IsFinite)
  _SPIRV_OP(isinf, IsInf)
  _SPIRV_OP(isnan, IsNan)
  _SPIRV_OP(isnormal, IsNormal)
  _SPIRV_OP(signbit, SignBitSet)
  _SPIRV_OP(any, Any)
  _SPIRV_OP(all, All)
#undef _SPIRV_OP
#define _SPIRV_OP(x, y) {#x, SPIRV::Op##y},
  _SPIRV_OP(__spirv_Select, SelectSISCond)
  _SPIRV_OP(__spirv_SpecConstant, SpecConstant)
  // CL 2.0 workgroup builtins
  _SPIRV_OP(group_all, GroupAll)
  _SPIRV_OP(group_any, GroupAny)
  _SPIRV_OP(group_broadcast, GroupBroadcast)
  _SPIRV_OP(group_iadd, GroupIAdd)
  _SPIRV_OP(group_fadd, GroupFAdd)
  _SPIRV_OP(group_fmin, GroupFMin)
  _SPIRV_OP(group_umin, GroupUMin)
  _SPIRV_OP(group_smin, GroupSMin)
  _SPIRV_OP(group_fmax, GroupFMax)
  _SPIRV_OP(group_umax, GroupUMax)
  _SPIRV_OP(group_smax, GroupSMax)
  // cl_khr_subgroup_non_uniform_vote
  _SPIRV_OP(group_elect, GroupNonUniformElect)
  _SPIRV_OP(group_non_uniform_all, GroupNonUniformAll)
  _SPIRV_OP(group_non_uniform_any, GroupNonUniformAny)
  _SPIRV_OP(group_non_uniform_all_equal, GroupNonUniformAllEqual)
  // cl_khr_subgroup_ballot
  _SPIRV_OP(group_non_uniform_broadcast, GroupNonUniformBroadcast)
  _SPIRV_OP(group_broadcast_first, GroupNonUniformBroadcastFirst)
  _SPIRV_OP(group_ballot, GroupNonUniformBallot)
  _SPIRV_OP(group_inverse_ballot, GroupNonUniformInverseBallot)
  _SPIRV_OP(group_ballot_bit_extract, GroupNonUniformBallotBitExtract)
  _SPIRV_OP(group_ballot_bit_count, GroupNonUniformBallotBitCount)
  _SPIRV_OP(group_ballot_inclusive_scan, GroupNonUniformBallotBitCount)
  _SPIRV_OP(group_ballot_exclusive_scan, GroupNonUniformBallotBitCount)
  _SPIRV_OP(group_ballot_find_lsb, GroupNonUniformBallotFindLSB)
  _SPIRV_OP(group_ballot_find_msb, GroupNonUniformBallotFindMSB)
  // cl_khr_subgroup_non_uniform_arithmetic
  _SPIRV_OP(group_non_uniform_iadd, GroupNonUniformIAdd)
  _SPIRV_OP(group_non_uniform_fadd, GroupNonUniformFAdd)
  _SPIRV_OP(group_non_uniform_imul, GroupNonUniformIMul)
  _SPIRV_OP(group_non_uniform_fmul, GroupNonUniformFMul)
  _SPIRV_OP(group_non_uniform_smin, GroupNonUniformSMin)
  _SPIRV_OP(group_non_uniform_umin, GroupNonUniformUMin)
  _SPIRV_OP(group_non_uniform_fmin, GroupNonUniformFMin)
  _SPIRV_OP(group_non_uniform_smax, GroupNonUniformSMax)
  _SPIRV_OP(group_non_uniform_umax, GroupNonUniformUMax)
  _SPIRV_OP(group_non_uniform_fmax, GroupNonUniformFMax)
  _SPIRV_OP(group_non_uniform_iand, GroupNonUniformBitwiseAnd)
  _SPIRV_OP(group_non_uniform_ior, GroupNonUniformBitwiseOr)
  _SPIRV_OP(group_non_uniform_ixor, GroupNonUniformBitwiseXor)
  _SPIRV_OP(group_non_uniform_logical_iand, GroupNonUniformLogicalAnd)
  _SPIRV_OP(group_non_uniform_logical_ior, GroupNonUniformLogicalOr)
  _SPIRV_OP(group_non_uniform_logical_ixor, GroupNonUniformLogicalXor)
  // cl_khr_subgroup_shuffle
  _SPIRV_OP(group_shuffle, GroupNonUniformShuffle)
  _SPIRV_OP(group_shuffle_xor, GroupNonUniformShuffleXor)
  // cl_khr_subgroup_shuffle_relative
  _SPIRV_OP(group_shuffle_up, GroupNonUniformShuffleUp)
  _SPIRV_OP(group_shuffle_down, GroupNonUniformShuffleDown)
#undef _SPIRV_OP
});

static StringMap<BuiltIn::BuiltIn> OpenCLBIToBuiltInVarMap({
#define _SPIRV_OP(x, y) {#x, BuiltIn::y},
  // cl_khr_subgroup_ballot
  _SPIRV_OP(get_sub_group_eq_mask, SubgroupEqMask)
  _SPIRV_OP(get_sub_group_ge_mask, SubgroupGeMask)
  _SPIRV_OP(get_sub_group_gt_mask, SubgroupGtMask)
  _SPIRV_OP(get_sub_group_le_mask, SubgroupLeMask)
  _SPIRV_OP(get_sub_group_lt_mask, SubgroupLtMask)
  _SPIRV_OP(__spirv_BuiltInGlobalLinearId, GlobalLinearId)
  _SPIRV_OP(__spirv_BuiltInGlobalInvocationId, GlobalInvocationId)
#undef _SPIRV_OP
});

static StringMap<GroupOperation::GroupOperation> GroupOperationMap({
#define _SPIRV_OP(x, y) {#x, GroupOperation::y},
  _SPIRV_OP(reduce, Reduce)
  _SPIRV_OP(scan_inclusive, InclusiveScan)
  _SPIRV_OP(scan_exclusive, ExclusiveScan)
  _SPIRV_OP(ballot_bit_count, Reduce)
  _SPIRV_OP(ballot_inclusive_scan, InclusiveScan)
  _SPIRV_OP(ballot_exclusive_scan, ExclusiveScan)
  _SPIRV_OP(non_uniform_reduce, Reduce)
  _SPIRV_OP(non_uniform_scan_inclusive, InclusiveScan)
  _SPIRV_OP(non_uniform_scan_exclusive, ExclusiveScan)
  _SPIRV_OP(non_uniform_reduce_logical, Reduce)
  _SPIRV_OP(non_uniform_scan_inclusive_logical, InclusiveScan)
  _SPIRV_OP(non_uniform_scan_exclusive_logical, ExclusiveScan)
  _SPIRV_OP(clustered_reduce, ClusteredReduce)
  _SPIRV_OP(clustered_reduce_logical, ClusteredReduce)
#undef _SPIRV_OP
});

static bool genBarrier(MachineIRBuilder &MIRBuilder, unsigned opcode,
    const SmallVectorImpl<Register> &OrigArgs, SPIRVGlobalRegistry *GR);

static SamplerAddressingMode::SamplerAddressingMode
getSamplerAddressingModeFromBitmask(unsigned int bitmask) {
  switch (bitmask & CLK_ADDRESS_MODE_MASK) {
  case CLK_ADDRESS_CLAMP:
    return SamplerAddressingMode::Clamp;
  case CLK_ADDRESS_CLAMP_TO_EDGE:
    return SamplerAddressingMode::ClampToEdge;
  case CLK_ADDRESS_REPEAT:
    return SamplerAddressingMode::Repeat;
  case CLK_ADDRESS_MIRRORED_REPEAT:
    return SamplerAddressingMode::RepeatMirrored;
  case CLK_ADDRESS_NONE:
    return SamplerAddressingMode::None;
  default:
    llvm_unreachable("Unknown CL address mode");
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
  case memory_scope_sub_group:
    return Scope::Subgroup;
  }
  llvm_unreachable("Unknown CL memory scope");
}

static unsigned int getSamplerParamFromBitmask(unsigned int bitmask) {
  return (bitmask & CLK_NORMALIZED_COORDS_TRUE) ? 1 : 0;
}

static Register buildSamplerLiteral(uint64_t samplerBitmask, Register res,
                                    const SPIRVType *samplerTy,
                                    MachineIRBuilder &MIRBuilder,
                                    SPIRVGlobalRegistry *GR) {
  auto sampler = GR->buildConstantSampler(res,
      getSamplerAddressingModeFromBitmask(samplerBitmask),
      getSamplerParamFromBitmask(samplerBitmask),
      getSamplerFilterModeFromBitmask(samplerBitmask),
      MIRBuilder, samplerTy);
  return sampler;
}

static Register buildLoad(SPIRVType *BaseType, Register PtrVReg,
                          MachineIRBuilder &MIRBuilder, SPIRVGlobalRegistry *GR,
                          LLT llt, Register dstReg = Register(0)) {
  const auto MRI = MIRBuilder.getMRI();
  if (!dstReg.isValid()) {
    dstReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
    MRI->setType(dstReg, LLT::scalar(32));
    GR->assignSPIRVTypeToVReg(BaseType, dstReg, MIRBuilder);
  }

  // TODO: consider using correct address space and alignment
  // p0 is canonical type for selection though
  auto PtrInfo = MachinePointerInfo();
  MIRBuilder.buildLoad(dstReg, PtrVReg, PtrInfo, Align());
  return dstReg;
}

static Register buildOpSampledImage(Register res, Register image,
                                    Register sampler,
                                    MachineIRBuilder &MIRBuilder,
                                    SPIRVGlobalRegistry *GR) {
  const auto MRI = MIRBuilder.getMRI();
  SPIRVType *imageType = GR->getSPIRVTypeForVReg(image);
  SPIRVType *sampImTy =
      GR->getOrCreateSPIRVSampledImageType(imageType, MIRBuilder);

  Register sampledImage = res.isValid() ? res
      : MRI->createVirtualRegister(&SPIRV::IDRegClass);
  auto SampImMIB = MIRBuilder.buildInstr(SPIRV::OpSampledImage)
                       .addDef(sampledImage)
                       .addUse(GR->getSPIRVTypeID(sampImTy))
                       .addUse(image)
                       .addUse(sampler);
  constrainRegOperands(SampImMIB);
  return sampledImage;
}

static MachineInstr *getDefInstrMaybeConstant(Register &constVReg,
                                      const MachineRegisterInfo *MRI) {
  MachineInstr *constInstr = MRI->getVRegDef(constVReg);
  if (constInstr->getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
      constInstr->getIntrinsicID() == Intrinsic::spv_track_constant) {
    constVReg = constInstr->getOperand(2).getReg();
    constInstr = MRI->getVRegDef(constVReg);
  }
  return constInstr;
}

static uint64_t getIConstVal(Register constReg,
                             const MachineRegisterInfo *MRI) {
  const auto c = getDefInstrMaybeConstant(constReg, MRI);
  assert(c && c->getOpcode() == TargetOpcode::G_CONSTANT);
  return c->getOperand(1).getCImm()->getValue().getZExtValue();
}

static Register buildBuiltInLoad(MachineIRBuilder &MIRBuilder,
                              SPIRVType *varType,
                              SPIRVGlobalRegistry *GR, BuiltIn::BuiltIn builtIn,
                              LLT llt, Register Reg = Register(0)) {
  Register NewReg = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  MIRBuilder.getMRI()->setType(NewReg, LLT::pointer(0, GR->getPointerSize()));
  SPIRVType *PtrTy =
      GR->getOrCreateSPIRVPointerType(varType, MIRBuilder, StorageClass::Input);
  GR->assignSPIRVTypeToVReg(PtrTy, NewReg, MIRBuilder);

  // Set up the global OpVariable with the necessary builtin decorations
  Register Var = GR->buildGlobalVariable(NewReg, PtrTy,
      getLinkStringForBuiltIn(builtIn), nullptr, StorageClass::Input, nullptr,
      true, true, LinkageType::Import, MIRBuilder, false);

  // Load the value from the global variable
  Register loadedReg = buildLoad(varType, Var, MIRBuilder, GR, llt, Reg);
  MIRBuilder.getMRI()->setType(loadedReg, llt);
  return loadedReg;
}

// Insert ASSIGN_TYPE instuction between Reg and its definition, set NewReg as
// a dst of the definition, assign SPIRVType to both registers. If SpirvTy is
// provided, use it as SPIRVType in ASSIGN_TYPE, otherwise create it from Ty.
// Defined in SPIRVIRTranslator.cpp.
extern Register insertAssignInstr(Register Reg, Type *Ty, SPIRVType *SpirvTy,
    SPIRVGlobalRegistry *GR, MachineIRBuilder &MIB, MachineRegisterInfo &MRI);

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
                              SPIRVGlobalRegistry *GR, BuiltIn::BuiltIn builtIn,
                              uint64_t defaultVal) {
  Register idxVReg = OrigArgs[0];

  const unsigned resWidth = retType->getOperand(1).getImm();
  const unsigned int ptrSize = GR->getPointerSize();

  const auto PtrSizeTy = GR->getOrCreateSPIRVIntegerType(ptrSize, MIRBuilder);

  const auto MRI = MIRBuilder.getMRI();
  auto idxInstr = getDefInstrMaybeConstant(idxVReg, MRI);

  // Set up the final register to do truncation or extension on at the end.
  Register toTruncate = resVReg;

  // If the index is constant, we can statically determine if it is in range
  bool isConstantIndex = idxInstr->getOpcode() == TargetOpcode::G_CONSTANT;

  // If it's out of range (max dimension is 3), we can just return the constant
  // default value(0 or 1 depending on which query function)
  if (isConstantIndex && getIConstVal(idxVReg, MRI) >= 3) {
    Register defaultReg = resVReg;
    if (ptrSize != resWidth) {
      defaultReg = MRI->createGenericVirtualRegister(LLT::scalar(ptrSize));
      GR->assignSPIRVTypeToVReg(PtrSizeTy, defaultReg, MIRBuilder);
      toTruncate = defaultReg;
    }
    auto newReg = GR->buildConstantInt(defaultVal, MIRBuilder, PtrSizeTy, true);
    MIRBuilder.buildCopy(defaultReg, newReg);
  } else { // If it could be in range, we need to load from the given builtin

    auto Vec3Ty = GR->getOrCreateSPIRVVectorType(PtrSizeTy, 3, MIRBuilder);
    Register loadedVector = buildBuiltInLoad(MIRBuilder, Vec3Ty, GR, builtIn,
                                             LLT::fixed_vector(3, ptrSize));

    // Set up the vreg to extract the result to (possibly a new temporary one)
    Register extr = resVReg;
    if (!isConstantIndex || ptrSize != resWidth) {
      extr = MRI->createGenericVirtualRegister(LLT::scalar(ptrSize));
      GR->assignSPIRVTypeToVReg(PtrSizeTy, extr, MIRBuilder);
    }

    // Use Intrinsic::spv_extractelt so dynamic vs static extraction is
    // handled later: extr = spv_extractelt loadedVector, idxVReg
    MachineInstrBuilder ExtractInst = MIRBuilder.buildIntrinsic(
        Intrinsic::spv_extractelt, ArrayRef<Register>{extr}, true);
    ExtractInst.addUse(loadedVector)
               .addUse(idxVReg);

    // If the index is dynamic, need check if it's < 3, and then use a select
    if (!isConstantIndex) {
      insertAssignInstr(extr, nullptr, PtrSizeTy, GR, MIRBuilder, *MRI);

      auto idxTy = GR->getSPIRVTypeForVReg(idxVReg);
      auto boolTy = GR->getOrCreateSPIRVBoolType(MIRBuilder);

      Register cmpReg = MRI->createGenericVirtualRegister(LLT::scalar(1));
      GR->assignSPIRVTypeToVReg(boolTy, cmpReg, MIRBuilder);

      // Use G_ICMP to check if idxVReg < 3
      Register three =  GR->buildConstantInt(3, MIRBuilder, idxTy, true);
      MIRBuilder.buildICmp(CmpInst::ICMP_ULT, cmpReg, idxVReg, three);

      // Get constant for the default value (0 or 1 depending on which function)
      Register defaultReg = GR->buildConstantInt(defaultVal, MIRBuilder,
                                                 PtrSizeTy, true);

      // Get a register for the selection result (possibly a new temporary one)
      Register selRes = resVReg;
      if (ptrSize != resWidth) {
        selRes = MRI->createGenericVirtualRegister(LLT::scalar(ptrSize));
        GR->assignSPIRVTypeToVReg(PtrSizeTy, selRes, MIRBuilder);
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
                                SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() > 2);
  const auto MRI = MIRBuilder.getMRI();

  Register image = OrigArgs[0];
  Register sampler = OrigArgs[1];

  if (!GR->isScalarOfType(OrigArgs[1], SPIRV::OpTypeSampler)) {
    MachineInstr *samplerInstr = getDefInstrMaybeConstant(sampler, MRI);;
    if (samplerInstr->getOperand(1).isCImm()) {
      auto sampMask = getIConstVal(OrigArgs[1], MRI);
      Register res;
      sampler = buildSamplerLiteral(sampMask, res,
                                    GR->getSPIRVTypeForVReg(sampler),
                                    MIRBuilder, GR);
    }
  }
  Register reg;
  Register sampledImage = buildOpSampledImage(reg, image, sampler,
                                              MIRBuilder, GR);

  Register coord = OrigArgs[2];
  Register lod = GR->buildConstantFP(APFloat::getZero(APFloat::IEEEsingle()),
                                     MIRBuilder);
  // OpImageSampleExplicitLod always returns 4-element vector.
  SPIRVType *tmpType = retType;
  bool NeedExtract = false;
  if (tmpType->getOpcode() != SPIRV::OpTypeVector) {
    tmpType = GR->getOrCreateSPIRVVectorType(retType, 4, MIRBuilder);
    NeedExtract = true;
  }
  auto llt = LLT::scalar(GR->getScalarOrVectorBitWidth(tmpType));
  Register tmpReg = MRI->createGenericVirtualRegister(llt);
  GR->assignSPIRVTypeToVReg(tmpType, tmpReg, MIRBuilder);

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageSampleExplicitLod)
                 .addDef(NeedExtract ? tmpReg : resVReg)
                 .addUse(GR->getSPIRVTypeID(tmpType))
                 .addUse(sampledImage)
                 .addUse(coord)
                 .addImm(ImageOperand::Lod)
                 .addUse(lod);
  bool Result = constrainRegOperands(MIB);

  if (NeedExtract) {
    MIB = MIRBuilder.buildInstr(SPIRV::OpCompositeExtract)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(retType))
                 .addUse(tmpReg)
                 .addImm(0);
    Result = constrainRegOperands(MIB);
  }
  return Result;
}

static bool genReadImageMSAA(MachineIRBuilder &MIRBuilder, Register resVReg,
                         SPIRVType *retType,
                         const SmallVectorImpl<Register> &OrigArgs,
                         SPIRVGlobalRegistry *GR) {
  Register image = OrigArgs[0];
  Register coord = OrigArgs[1];
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageRead)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(retType))
                 .addUse(image)
                 .addUse(coord)
                 .addImm(ImageOperand::Sample)
                 .addUse(OrigArgs[2]);
  return constrainRegOperands(MIB);
}

static bool genReadImage(MachineIRBuilder &MIRBuilder, Register resVReg,
                         SPIRVType *retType,
                         const SmallVectorImpl<Register> &OrigArgs,
                         SPIRVGlobalRegistry *GR) {
  Register image = OrigArgs[0];
  Register coord = OrigArgs[1];
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageRead)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(retType))
                 .addUse(image)
                 .addUse(coord);
  return constrainRegOperands(MIB);
}

static bool genWriteImage(MachineIRBuilder &MIRBuilder,
                          const SmallVectorImpl<Register> &OrigArgs) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageWrite)
                 .addUse(OrigArgs[0])  // Image
                 .addUse(OrigArgs[1])  // Coord vec2
                 .addUse(OrigArgs[2]); // Texel to write vec4
  return constrainRegOperands(MIB);
}

static bool buildOpImageSampleExplicitLod(MachineIRBuilder &MIRBuilder,
    Register resVReg, const StringRef typeStr,
    const SmallVectorImpl<Register> &args, SPIRVGlobalRegistry *GR) {
  auto spvTy = GR->getOrCreateSPIRVTypeByName(typeStr, MIRBuilder);
  Register image = args[0];
  Register coord = args[1];
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageSampleExplicitLod)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(spvTy))
                 .addUse(image)
                 .addUse(coord)
                 .addImm(ImageOperand::Lod)
                 .addUse(args[3]);
  return constrainRegOperands(MIB);
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

// Used for get_image_width, get_image_dim etc. via OpImageQuerySize[Lod]
static bool genImageSizeQuery(MachineIRBuilder &MIRBuilder, Register resVReg,
                              SPIRVType *retType, Register img,
                              unsigned component, SPIRVGlobalRegistry *GR) {
  const auto MRI = MIRBuilder.getMRI();

  unsigned numRetComps = 1;
  if (retType->getOpcode() == SPIRV::OpTypeVector)
    numRetComps = retType->getOperand(2).getImm();

  SPIRVType *imgType = GR->getSPIRVTypeForVReg(img);
  unsigned numTempComps = getNumSizeComponents(imgType);

  Register sizeVec = resVReg;
  SPIRVType *sizeVecTy = retType;
  auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  if (numTempComps != numRetComps) {
    sizeVec = MRI->createGenericVirtualRegister(LLT::fixed_vector(numTempComps, 32));
    sizeVecTy = GR->getOrCreateSPIRVVectorType(I32Ty, numTempComps, MIRBuilder);
    GR->assignSPIRVTypeToVReg(sizeVecTy, sizeVec, MIRBuilder);
  }
  auto dim = static_cast<Dim::Dim>(imgType->getOperand(2).getImm());
  MachineInstrBuilder MIB;
  if (dim == Dim::DIM_Buffer) {
    MIB = MIRBuilder.buildInstr(SPIRV::OpImageQuerySize)
                   .addDef(sizeVec)
                   .addUse(GR->getSPIRVTypeID(sizeVecTy))
                   .addUse(img);
  } else {
    auto lodReg = GR->buildConstantInt(0, MIRBuilder, I32Ty);
    MIB = MIRBuilder.buildInstr(SPIRV::OpImageQuerySizeLod)
                   .addDef(sizeVec)
                   .addUse(GR->getSPIRVTypeID(sizeVecTy))
                   .addUse(img)
                   .addUse(lodReg);
  }
  bool success = constrainRegOperands(MIB);
  if (numTempComps == numRetComps || !success) {
    return success;
  }
  if (numRetComps == 1) {
    // Need an OpCompositeExtract
    component = component == 3 ? numTempComps - 1 : component;
    assert(component < numTempComps && "Invalid comp index");
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpCompositeExtract)
                   .addDef(resVReg)
                   .addUse(GR->getSPIRVTypeID(retType))
                   .addUse(sizeVec)
                   .addImm(component);
    return constrainRegOperands(MIB);
  } else {
    // Need an OpShuffleVector to make the struct vector the right size
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpVectorShuffle)
                   .addDef(resVReg)
                   .addUse(GR->getSPIRVTypeID(retType))
                   .addUse(sizeVec)
                   .addUse(sizeVec);
    for (unsigned int i = 0; i < numRetComps; ++i) {
      // TODO is padding with undefined indices ok, or do we need an explicit 0
      uint32_t compIdx = i < numTempComps ? i : 0xffffffff;
      MIB.addImm(compIdx);
    }
    return constrainRegOperands(MIB);
  }
}

// Used for get_image_num_samples
static bool genNumSamplesQuery(MachineIRBuilder &MIRBuilder, Register resVReg,
                               SPIRVType *retType, Register img,
                               SPIRVGlobalRegistry *GR) {
  unsigned retOpcode = retType->getOpcode();
  assert(retOpcode == SPIRV::OpTypeInt && "Samples query result must be int");
  
  unsigned imgDim = GR->getSPIRVTypeForVReg(img)->getOperand(2).getImm();
  assert(imgDim == 1 && "Samples query image dim must be 2D");

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageQuerySamples)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(retType))
                 .addUse(img);
  return constrainRegOperands(MIB);
}

static bool genGlobalLocalQuery(MachineIRBuilder &MIRBuilder,
                                const StringRef globLocStr, bool global,
                                Register ret, SPIRVType *retTy,
                                const SmallVectorImpl<Register> &args,
                                SPIRVGlobalRegistry *GR) {
  if (globLocStr.startswith("id")) {
    auto BI = global ? BuiltIn::GlobalInvocationId : BuiltIn::LocalInvocationId;
    return genWorkgroupQuery(MIRBuilder, ret, retTy, args, GR, BI, 0);
  } else if (globLocStr.startswith("size")) {
    auto BI = global ? BuiltIn::GlobalSize : BuiltIn::WorkgroupSize;
    return genWorkgroupQuery(MIRBuilder, ret, retTy, args, GR, BI, 1);
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
                          SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() == 1 && "Image queries require 1 argument");
  Register img = OrigArgs[0];
  if (queryStr.startswith("width")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 0, GR);
  } else if (queryStr.startswith("height")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 1, GR);
  } else if (queryStr.startswith("depth")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 2, GR);
  } else if (queryStr.startswith("dim")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 0, GR);
  } else if (queryStr.startswith("array_size")) {
    return genImageSizeQuery(MIRBuilder, resVReg, retType, img, 3, GR);
  } else if (queryStr.startswith("num_samples")) {
    return genNumSamplesQuery(MIRBuilder, resVReg, retType, img, GR);
  }
  report_fatal_error("Cannot handle OpenCL img query: get_image_" + queryStr);
}

static bool genAtomicLoad(Register resVReg, const SPIRVType* resType,
                          MachineIRBuilder& MIRBuilder,
                          const SmallVectorImpl<Register> &args,
                          SPIRVGlobalRegistry* GR) {
  using namespace SPIRV;

  auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  Register ptr = args[0];
  Register scopeReg;
  if (args.size() > 1) {
    // TODO: Insert call to __translate_ocl_memory_sccope before OpAtomicLoad
    // and the function implementation. We can use Translator's output for
    // transcoding/atomic_explicit_arguments.cl as an example.
    scopeReg = args[1];
  } else {
    auto scope = Scope::Device;
    scopeReg = GR->buildConstantInt(scope, MIRBuilder, I32Ty, true);
  }

  Register memSemReg;
  if (args.size() > 2) {
    // TODO: Insert call to __translate_ocl_memory_order before OpAtomicLoad.
    memSemReg = args[2];
  } else {
    auto scSm = getMemSemanticsForStorageClass(GR->getPointerStorageClass(ptr));
    auto memSem = MemorySemantics::SequentiallyConsistent | scSm;
    memSemReg = GR->buildConstantInt(memSem, MIRBuilder, I32Ty, true);
  }

  auto MIB = MIRBuilder.buildInstr(OpAtomicLoad)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(resType))
                 .addUse(ptr)
                 .addUse(scopeReg)
                 .addUse(memSemReg);

  return constrainRegOperands(MIB);
}

static bool genAtomicStore(MachineIRBuilder &MIRBuilder,
                         const SmallVectorImpl<Register> &OrigArgs,
                         SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() >= 2 && "Need 2 args for atomic store instr");
  using namespace SPIRV;
  auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);

  Register scopeReg;
  auto scope = Scope::Device;
  if (!scopeReg.isValid())
    scopeReg = GR->buildConstantInt(scope, MIRBuilder, I32Ty, true);

  auto ptr = OrigArgs[0];
  auto scSem = getMemSemanticsForStorageClass(GR->getPointerStorageClass(ptr));

  Register memSemReg;
  auto memSem = MemorySemantics::SequentiallyConsistent | scSem;
  if (!memSemReg.isValid())
    memSemReg = GR->buildConstantInt(memSem, MIRBuilder, I32Ty, true);

  auto MIB = MIRBuilder.buildInstr(OpAtomicStore)
                 .addUse(ptr)
                 .addUse(scopeReg)
                 .addUse(memSemReg)
                 .addUse(OrigArgs[1]);
  return constrainRegOperands(MIB);
}

static bool genAtomicCmpXchg(MachineIRBuilder &MIRBuilder, Register resVReg,
                             SPIRVType *retType, bool isWeak, bool isCmpxchg,
                             const SmallVectorImpl<Register> &OrigArgs,
                             SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() >= 3 && "Need 3+ args to atomic_compare_exchange");
  using namespace SPIRV;
  const auto MRI = MIRBuilder.getMRI();

  Register objectPtr = OrigArgs[0];   // Pointer (volatile A *object)
  Register expectedArg = OrigArgs[1]; // Comparator (C* expected)
  Register desired = OrigArgs[2];     // Value (C desired)
  SPIRVType *spvDesiredTy = GR->getSPIRVTypeForVReg(desired);
  LLT desiredLLT = MRI->getType(desired);

  assert(GR->getSPIRVTypeForVReg(objectPtr)->getOpcode() == OpTypePointer);
  auto expectedType = GR->getSPIRVTypeForVReg(expectedArg)->getOpcode();
  assert(isCmpxchg ? expectedType == OpTypeInt : expectedType == OpTypePointer);
  assert(GR->isScalarOfType(desired, OpTypeInt));

  SPIRVType *spvObjectPtrTy = GR->getSPIRVTypeForVReg(objectPtr);
  assert(spvObjectPtrTy->getOperand(2).isReg() && "SPIRV type is expected");
  SPIRVType *spvObjectTy = MRI->getVRegDef(
      spvObjectPtrTy->getOperand(2).getReg());
  auto storageClass = static_cast<StorageClass::StorageClass>(
      spvObjectPtrTy->getOperand(1).getImm());
  auto memSemStorage = getMemSemanticsForStorageClass(storageClass);

  Register memSemEqualReg;
  Register memSemUnequalReg;
  auto memSemEqual = isCmpxchg ? MemorySemantics::None :
      MemorySemantics::SequentiallyConsistent | memSemStorage;
  auto memSemUnequal = isCmpxchg ? MemorySemantics::None :
      MemorySemantics::SequentiallyConsistent | memSemStorage;
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
  auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  if (!memSemEqualReg.isValid())
    memSemEqualReg = GR->buildConstantInt(memSemEqual, MIRBuilder, I32Ty);
  if (!memSemUnequalReg.isValid())
    memSemUnequalReg = GR->buildConstantInt(memSemUnequal, MIRBuilder, I32Ty);

  Register scopeReg;
  auto scope = isCmpxchg ? Scope::Workgroup : Scope::Device;
  if (OrigArgs.size() >= 6) {
    assert(OrigArgs.size() == 6 && "Extra args for explicit atomic cmpxchg");
    auto clScope = static_cast<CLMemScope>(getIConstVal(OrigArgs[5], MRI));
    scope = getSPIRVScope(clScope);
    if (clScope == static_cast<unsigned>(scope))
      scopeReg = OrigArgs[5];
  }
  if (!scopeReg.isValid())
    scopeReg = GR->buildConstantInt(scope, MIRBuilder, I32Ty);

  Register expected = isCmpxchg ? expectedArg :
      buildLoad(spvDesiredTy, expectedArg, MIRBuilder, GR, LLT::scalar(32));
  MRI->setType(expected, desiredLLT);
  Register tmp = !isCmpxchg ? MRI->createGenericVirtualRegister(desiredLLT) :
                              resVReg;
  GR->assignSPIRVTypeToVReg(spvDesiredTy, tmp, MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(OpAtomicCompareExchange)
                 .addDef(tmp)
                 .addUse(GR->getSPIRVTypeID(spvObjectTy))
                 .addUse(objectPtr)
                 .addUse(scopeReg)
                 .addUse(memSemEqualReg)
                 .addUse(memSemUnequalReg)
                 .addUse(desired)
                 .addUse(expected);
  if (!isCmpxchg) {
    MIRBuilder.buildInstr(OpStore).addUse(expectedArg).addUse(tmp);
    MIRBuilder.buildICmp(CmpInst::ICMP_EQ, resVReg, tmp, expected);
  }
  return constrainRegOperands(MIB);
}

static bool genAtomicRMW(Register resVReg, const SPIRVType *resType,
                         unsigned RMWOpcode, MachineIRBuilder &MIRBuilder,
                         const SmallVectorImpl<Register> &OrigArgs,
                         SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() >= 2 && "Need 2+ args to atomic RMW instr");
  const auto MRI = MIRBuilder.getMRI();
  auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);

  Register scopeReg;
  auto scope = Scope::Workgroup;
  if (OrigArgs.size() >= 4) {
    assert(OrigArgs.size() == 4 && "Extra args for explicit atomic RMW");
    auto clScope = static_cast<CLMemScope>(getIConstVal(OrigArgs[5], MRI));
    scope = getSPIRVScope(clScope);
    if (clScope == static_cast<unsigned>(scope))
      scopeReg = OrigArgs[5];
  }
  if (!scopeReg.isValid())
    scopeReg = GR->buildConstantInt(scope, MIRBuilder, I32Ty);

  auto ptr = OrigArgs[0];
  auto scSem = getMemSemanticsForStorageClass(GR->getPointerStorageClass(ptr));

  Register memSemReg;
  unsigned memSem = MemorySemantics::None;
  if (OrigArgs.size() >= 3) {
    auto memOrd = static_cast<CLMemOrder>(getIConstVal(OrigArgs[2], MRI));
    memSem = getSPIRVMemSemantics(memOrd) | scSem;
    if (memOrd == memSem)
      memSemReg = OrigArgs[3];
  }
  if (!memSemReg.isValid())
    memSemReg = GR->buildConstantInt(memSem, MIRBuilder, I32Ty);

  auto MIB = MIRBuilder.buildInstr(RMWOpcode)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(resType))
                 .addUse(ptr)
                 .addUse(scopeReg)
                 .addUse(memSemReg)
                 .addUse(OrigArgs[1]);
  return constrainRegOperands(MIB);
}

static bool genAtomicInstr(MachineIRBuilder &MIRBuilder,
                           const StringRef atomicStr, Register ret,
                           SPIRVType *retTy,
                           const SmallVectorImpl<Register> &args,
                           SPIRVGlobalRegistry *GR) {
  using namespace SPIRV;

  if (atomicStr.startswith("load")) {
      assert((((args.size() == 2 || args.size() == 3) &&
               atomicStr == "load_explicit") || args.size() == 1) &&
             "Need ptr arg for atomic load");
      return genAtomicLoad(ret, retTy, MIRBuilder, args, GR);
  } else if (atomicStr.startswith("store")) {
      return genAtomicStore(MIRBuilder, args, GR);
  } else if (atomicStr.startswith("compare_exchange_")) {
    const auto cmp_xchg = atomicStr.substr(strlen("compare_exchange_"));
    if (cmp_xchg.startswith("weak")) {
      return genAtomicCmpXchg(MIRBuilder, ret, retTy, true, false, args, GR);
    } else if (cmp_xchg.startswith("strong")) {
      return genAtomicCmpXchg(MIRBuilder, ret, retTy, false, false, args, GR);
    }
  } else if (atomicStr.startswith("cmpxchg")) {
    return genAtomicCmpXchg(MIRBuilder, ret, retTy, false, true, args, GR);
  } else if (atomicStr.startswith("add")) {
    return genAtomicRMW(ret, retTy, OpAtomicIAdd, MIRBuilder, args, GR);
  } else if (atomicStr.startswith("sub")) {
    return genAtomicRMW(ret, retTy, OpAtomicISub, MIRBuilder, args, GR);
  } else if (atomicStr.startswith("or")) {
    return genAtomicRMW(ret, retTy, OpAtomicOr, MIRBuilder, args, GR);
  } else if (atomicStr.startswith("xor")) {
    return genAtomicRMW(ret, retTy, OpAtomicXor, MIRBuilder, args, GR);
  } else if (atomicStr.startswith("and")) {
    return genAtomicRMW(ret, retTy, OpAtomicAnd, MIRBuilder, args, GR);
  } else if (atomicStr[0] == 'm') {
    // TODO check the arg types for signedness
    // if (atomicStr.startswith("min")) {
    //   return genAtomicRMW(ret, retTy, OpAtomicUMax, MIRBuilder, args, GR);
    // } else if (atomicStr.startswith("max")) {
    //   return genAtomicRMW(ret, retTy, OpAtomicUMax, MIRBuilder, args, GR);
    // }
  } else if (atomicStr.startswith("work_item_fence")) {
    return genBarrier(MIRBuilder, SPIRV::OpMemoryBarrier, args, GR);
  } else if (atomicStr.startswith("exchange")) {
    return genAtomicRMW(ret, retTy, OpAtomicExchange, MIRBuilder, args, GR);
  }

  report_fatal_error("Cannot handle OpenCL atomic func: atomic_" + atomicStr);
}

static bool genBarrier(MachineIRBuilder &MIRBuilder, unsigned opcode,
                       const SmallVectorImpl<Register> &OrigArgs,
                       SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() >= 1 && "Missing args for OpenCL barrier func");
  const auto MRI = MIRBuilder.getMRI();
  const auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);

  bool isMemBarrier = opcode == SPIRV::OpMemoryBarrier;
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

  if (isMemBarrier) {
    auto memOrd = static_cast<CLMemOrder>(getIConstVal(OrigArgs[1], MRI));
    memSem = getSPIRVMemSemantics(memOrd) | memSem;
  } else {
    memSem |= MemorySemantics::SequentiallyConsistent;
  }

  Register memSemReg;
  if (memFlags == memSem) {
    memSemReg = OrigArgs[0];
  } else {
    memSemReg = GR->buildConstantInt(memSem, MIRBuilder, I32Ty);
  }

  Register scopeReg;
  auto scope = Scope::Workgroup;
  auto MemScope = scope;
  if (OrigArgs.size() >= 2) {
    assert(((!isMemBarrier && OrigArgs.size() == 2) ||
            (isMemBarrier && OrigArgs.size() == 3)) &&
          "Extra args for explicitly scoped barrier");
    auto scopeArg = isMemBarrier ? OrigArgs[2] : OrigArgs[1];
    auto clScope = static_cast<CLMemScope>(getIConstVal(scopeArg, MRI));
    MemScope = getSPIRVScope(clScope);
    if (!(memFlags & CLK_LOCAL_MEM_FENCE) || isMemBarrier) {
      scope = MemScope;
    }
    if (clScope == static_cast<unsigned>(scope))
      scopeReg = OrigArgs[1];
  }
  if (!scopeReg.isValid())
    scopeReg = GR->buildConstantInt(scope, MIRBuilder, I32Ty);

  auto MIB = MIRBuilder.buildInstr(opcode).addUse(scopeReg);
  if (!isMemBarrier)
    MIB.addUse(GR->buildConstantInt(MemScope, MIRBuilder, I32Ty));
  MIB.addUse(memSemReg);
  return constrainRegOperands(MIB);
}

static bool genConvertInstr(MachineIRBuilder &MIRBuilder,
                            const StringRef convertStr, Register ret,
                            SPIRVType *retTy,
                            const SmallVectorImpl<Register> &args,
                            SPIRVGlobalRegistry *GR) {
  using namespace SPIRV;
  Register src = args[0];
  bool isFromInt = GR->isScalarOrVectorOfType(src, OpTypeInt);
  bool isFromFloat = !isFromInt && GR->isScalarOrVectorOfType(src, OpTypeFloat);

  bool isToInt = GR->isScalarOrVectorOfType(ret, OpTypeInt);
  bool isToFloat = !isToInt && GR->isScalarOrVectorOfType(ret, OpTypeFloat);

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

  if (isSat)
    buildOpDecorate(ret, MIRBuilder, Decoration::SaturatedConversion, {});
  if (isRounded)
    buildOpDecorate(ret, MIRBuilder, Decoration::FPRoundingMode,
                               {static_cast<uint32_t>(roundingMode)});
  if (isFromInt) {
    if (isToInt) { // I -> I
      auto op = dstSign ? OpUConvert : OpSConvert;
      op = !isSat ? op : (dstSign ? OpSatConvertUToS : OpSatConvertSToU);
      auto MIB = MIRBuilder.buildInstr(dstSign ? OpUConvert : OpSConvert)
                     .addDef(ret)
                     .addUse(GR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return constrainRegOperands(MIB);
    } else if (isToFloat) { // I -> F
      auto MIB = MIRBuilder.buildInstr(srcSign ? OpConvertSToF : OpConvertUToF)
                     .addDef(ret)
                     .addUse(GR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return constrainRegOperands(MIB);
    }
  } else if (isFromFloat) {
    if (isToInt) { // F -> I
      auto MIB = MIRBuilder.buildInstr(dstSign ? OpConvertFToS : OpConvertFToU)
                     .addDef(ret)
                     .addUse(GR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return constrainRegOperands(MIB);
    } else if (isToFloat) { // F -> F
      auto MIB = MIRBuilder.buildInstr(OpFConvert)
                     .addDef(ret)
                     .addUse(GR->getSPIRVTypeID(retTy))
                     .addUse(src);
      return constrainRegOperands(MIB);
    }
  }

  report_fatal_error("Convert instr not implemented yet: " + convertStr);
}

static bool genDotOrFMul(Register resVReg, const SPIRVType *resType,
                   MachineIRBuilder &MIRBuilder,
                   const SmallVectorImpl<Register> &OrigArgs,
                   SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() >= 2 && "Need 2 args for dot instr");
  using namespace SPIRV;

  unsigned OpCode;

  if (GR->getSPIRVTypeForVReg(OrigArgs[0])->getOpcode() ==
      SPIRV::OpTypeVector) {
    // Use OpDot only in case of vector args.
    OpCode = OpDot;
  } else {
    // Use OpFMul in case of scalar args.
    OpCode = OpFMulS;
  }

  auto MIB = MIRBuilder.buildInstr(OpCode)
                 .addDef(resVReg)
                 .addUse(GR->getSPIRVTypeID(resType))
                 .addUse(OrigArgs[0])
                 .addUse(OrigArgs[1]);
  return constrainRegOperands(MIB);
}

static Register buildScalarOrVectorBoolReg(MachineIRBuilder &MIRBuilder,
    SPIRVType *&boolTy, const SPIRVType *resType, SPIRVGlobalRegistry *GR) {
  LLT lltTy;
  boolTy = GR->getOrCreateSPIRVBoolType(MIRBuilder);
  if (resType->getOpcode() == SPIRV::OpTypeVector) {
    unsigned vecElts = resType->getOperand(2).getImm();
    boolTy = GR->getOrCreateSPIRVVectorType(boolTy, vecElts, MIRBuilder);
    auto LLVMVecTy = cast<FixedVectorType>(GR->getTypeForSPIRVType(boolTy));
    lltTy = LLT::vector(LLVMVecTy->getElementCount(), 1);
  } else {
    lltTy = LLT::scalar(1);
  }
  const auto MRI = MIRBuilder.getMRI();
  auto resReg = MRI->createGenericVirtualRegister(lltTy);
  GR->assignSPIRVTypeToVReg(boolTy, resReg, MIRBuilder);
  return resReg;
}

static bool buildScalarOrVectorSelect(MachineIRBuilder &MIRBuilder,
    Register resReg, Register srcReg, const SPIRVType *resType,
    SPIRVGlobalRegistry *GR) {
  Register trueConst;
  Register falseConst;
  auto bits = GR->getScalarOrVectorBitWidth(resType);
  if (resType->getOpcode() == SPIRV::OpTypeVector) {
     auto ones = APInt::getAllOnesValue(bits).getZExtValue();
     trueConst = GR->buildConstantIntVector(ones, MIRBuilder, resType);
     falseConst = GR->buildConstantIntVector(0, MIRBuilder, resType);
  } else {
     trueConst = GR->buildConstantInt(1, MIRBuilder, resType);
     falseConst = GR->buildConstantInt(0, MIRBuilder, resType);
  }
  return MIRBuilder.buildSelect(resReg, srcReg, trueConst, falseConst);
}

static bool genOpenCLRelational(MachineIRBuilder &MIRBuilder,
    unsigned opcode, Register resVReg, const SPIRVType *resType,
    const SmallVectorImpl<Register> &OrigArgs, SPIRVGlobalRegistry *GR) {
  bool IsBoolReturn = resType->getOpcode() == SPIRV::OpTypeBool;
  SPIRVType *relTy = resType;
  Register cmpReg = resVReg;
  if (!IsBoolReturn)
    cmpReg = buildScalarOrVectorBoolReg(MIRBuilder, relTy, resType, GR);
  // Build relational instruction
  auto MIB = MIRBuilder.buildInstr(opcode)
                 .addDef(cmpReg)
                 .addUse(GR->getSPIRVTypeID(relTy));
  for (auto arg : OrigArgs) {
     MIB.addUse(arg);
  }
  bool Result = constrainRegOperands(MIB);
  if (!IsBoolReturn)
    // Build select instruction to convert bool result to the required type.
    return buildScalarOrVectorSelect(MIRBuilder, resVReg, cmpReg, resType, GR);
  return Result;
}

static bool buildSpecConstant(MachineIRBuilder &MIRBuilder, unsigned Opcode,
    Register ResVReg, const SPIRVType *ResType,
    const SmallVectorImpl<Register> &OrigArgs, SPIRVGlobalRegistry *GR) {
  assert(OrigArgs.size() == 2 && "SpecConstant intrinsic expects two args");
  // Output SpecId decoration.
  const auto MRI = MIRBuilder.getMRI();
  auto SpecId = static_cast<unsigned>(getIConstVal(OrigArgs[0], MRI));
  buildOpDecorate(ResVReg, MIRBuilder, Decoration::SpecId, {SpecId});
  // Determine the constant MI.
  Register ConstReg = OrigArgs[1];
  const auto Const = getDefInstrMaybeConstant(ConstReg, MRI);
  assert(Const && (Const->getOpcode() == TargetOpcode::G_CONSTANT ||
                   Const->getOpcode() == TargetOpcode::G_FCONSTANT) &&
         "Argument should be either int or fp constant");
  // Determine opcode and output OpSpec MI.
  bool IsBool = ResType->getOpcode() == SPIRV::OpTypeBool;
  auto &ConstOp = Const->getOperand(1);
  if (IsBool) {
    assert(ConstOp.isCImm() && "Int constant operand is expected");
    Opcode = ConstOp.getCImm()->getValue().getZExtValue() ?
             SPIRV::OpSpecConstantTrue : SPIRV::OpSpecConstantFalse;
  }
  auto MIB = MIRBuilder.buildInstr(Opcode)
                 .addDef(ResVReg)
                 .addUse(GR->getSPIRVTypeID(ResType));
  if (!IsBool) {
    if (Const->getOpcode() == TargetOpcode::G_CONSTANT)
      addNumImm(ConstOp.getCImm()->getValue(), MIB);
    else
      addNumImm(ConstOp.getFPImm()->getValueAPF().bitcastToAPInt(), MIB);
  }
  return constrainRegOperands(MIB);
}

static bool genOpenCLOpGroup(MachineIRBuilder &MIRBuilder, StringRef name,
    unsigned opcode, Register resVReg, const SPIRVType *resType,
    const SmallVectorImpl<Register> &OrigArgs, SPIRVGlobalRegistry *GR) {
  using namespace SPIRV;
  const bool isElect = opcode == OpGroupNonUniformElect;
  const bool isAllOrAny = opcode == OpGroupAll || opcode == OpGroupAny
                          || opcode == OpGroupNonUniformAll
                          || opcode == OpGroupNonUniformAny;
  const bool isAllEqual = opcode == OpGroupNonUniformAllEqual;
  const bool isBallot = opcode == OpGroupNonUniformBallot;
  const bool isInverseBallot = opcode == OpGroupNonUniformInverseBallot;
  const bool isBallotBitExtract = opcode == OpGroupNonUniformBallotBitExtract;
  const bool isBallotFindBit = opcode == OpGroupNonUniformBallotFindLSB
                               || opcode == OpGroupNonUniformBallotFindMSB;
  const bool isLogical = opcode == OpGroupNonUniformLogicalAnd
                         || opcode == OpGroupNonUniformLogicalOr
                         || opcode == OpGroupNonUniformLogicalXor;
  const bool noGroupOperation = isElect || isAllOrAny || isAllEqual
                                || isBallot || isInverseBallot
                                || isBallotBitExtract || isBallotFindBit
                                || opcode == OpGroupNonUniformShuffle
                                || opcode == OpGroupNonUniformShuffleXor
                                || opcode == OpGroupNonUniformShuffleUp
                                || opcode == OpGroupNonUniformShuffleDown
                                || opcode == OpGroupBroadcast
                                || opcode == OpGroupNonUniformBroadcast
                                || opcode == OpGroupNonUniformBroadcastFirst;
  const bool hasBoolArg = (isAllOrAny && !isAllEqual) || isBallot || isLogical;
  const auto MRI = MIRBuilder.getMRI();
  Register arg0;
  if (hasBoolArg) {
    Register constReg = OrigArgs[0];
    auto argInstr = getDefInstrMaybeConstant(constReg, MRI);
    // TODO: support non-constant bool values
    assert(argInstr->getOpcode() == TargetOpcode::G_CONSTANT
           && "Only constant bool value args are supported");
    if (GR->getSPIRVTypeForVReg(OrigArgs[0])->getOpcode() != OpTypeBool) {
      auto boolTy = GR->getOrCreateSPIRVBoolType(MIRBuilder);
      arg0 = GR->buildConstantInt(getIConstVal(constReg, MRI),
                                  MIRBuilder, boolTy);
    }
  }
  Register groupReg = resVReg;
  SPIRVType* groupTy = resType;
  // TODO: maybe we need to check whether the result type is already boolean
  // and in this case do not insert select instruction.
  const bool hasBoolReturnTy = isElect || isAllOrAny || isAllEqual || isLogical
                               || isInverseBallot || isBallotBitExtract;
  if (hasBoolReturnTy)
    groupReg = buildScalarOrVectorBoolReg(MIRBuilder, groupTy, resType, GR);
  const auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  auto scope = name.startswith("sub_group") ? Scope::Subgroup
                                            : Scope::Workgroup;
  Register scopeReg = GR->buildConstantInt(scope, MIRBuilder, I32Ty);
  // Build OpGroup* instruction
  auto MIB = MIRBuilder.buildInstr(opcode)
                 .addDef(groupReg)
                 .addUse(GR->getSPIRVTypeID(groupTy))
                 .addUse(scopeReg);
  if (!noGroupOperation) {
    auto start = name.find("group_") + strlen("group_");
    auto end = opcode == OpGroupNonUniformBallotBitCount
               ? StringRef::npos : name.rfind('_');
    auto groupOp = GroupOperationMap.lookup(name.slice(start, end));
    MIB.addImm(groupOp);
  }
  if (OrigArgs.size() > 0) {
    MIB.addUse(arg0.isValid() ? arg0 : OrigArgs[0]);
    for (unsigned i = 1; i < OrigArgs.size(); i++) {
      MIB.addUse(OrigArgs[i]);
    }
  }
  constrainRegOperands(MIB);
  // Build select instruction
  if (hasBoolReturnTy)
    buildScalarOrVectorSelect(MIRBuilder, resVReg, groupReg, resType, GR);
  return true;
}

static bool genOpenCLExtInst(OpenCL_std::OpenCL_std extInstID,
                             MachineIRBuilder &MIRBuilder, Register ret,
                             const SPIRVType *retTy,
                             const SmallVectorImpl<Register> &args,
                             SPIRVGlobalRegistry *GR) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                 .addDef(ret)
                 .addUse(GR->getSPIRVTypeID(retTy))
                 .addImm(static_cast<uint32_t>(ExtInstSet::OpenCL_std))
                 .addImm(extInstID);
  for (const auto &arg : args) {
    MIB.addUse(arg);
  }
  return constrainRegOperands(MIB);
}

static std::string modifyBINameForLookup(StringRef nameNoArgs, SPIRVType *retTy,
    char firstArgC) {
  std::string modifiedName = nameNoArgs.str();
  if ((nameNoArgs.startswith("sub_group_")
       || nameNoArgs.startswith("work_group_")) && retTy) {
    if (nameNoArgs.contains("group_all")
        || nameNoArgs.contains("group_any")
        || nameNoArgs.startswith("sub_group_shuffle")
        || nameNoArgs.contains("group_broadcast")
        || nameNoArgs.startswith("sub_group_elect")
        || nameNoArgs.startswith("sub_group_non_uniform_all") // includes _equal
        || nameNoArgs.startswith("sub_group_non_uniform_any")
        || nameNoArgs.startswith("sub_group_non_uniform_broadcast")
        || nameNoArgs.startswith("sub_group_broadcast_first")
        || nameNoArgs.startswith("sub_group_ballot") // includes all ballot_*
        || nameNoArgs.startswith("sub_group_inverse_ballot")) {
      modifiedName = nameNoArgs.substr(nameNoArgs.find("group")).str();
    } else {
      // This mainly implements non-uniform arithmetic builtins.
      auto logicalOp = nameNoArgs.contains("logical_") ? "logical_" : "";
      auto nameSuffix = nameNoArgs.substr(nameNoArgs.rfind('_') + 1).str();
      auto nonUniform = nameNoArgs.contains("clustered_")
                        || nameNoArgs.contains("non_uniform_") ? "non_uniform_"
                                                               : "";
      char OpTyC = 0;
      if (retTy->getOpcode() == SPIRV::OpTypeFloat)
        OpTyC = 'f';
      else if (retTy->getOpcode() == SPIRV::OpTypeInt) {
        if (nameSuffix == "max" || nameSuffix == "min") {
          // Get signedness from "unsigned" argument type of the demangled name.
          OpTyC = firstArgC;
          if (OpTyC != 'u')
            OpTyC = 's';
        } else
          OpTyC = 'i';
      } else
        llvm_unreachable("Invalid OpenCL group builtin argument type");
      modifiedName = "group_" + std::string(nonUniform)
                     + std::string(logicalOp) + OpTyC + nameSuffix;
    }
  }
  return modifiedName;
}

bool llvm::generateOpenCLBuiltinCall(const StringRef demangledName,
                                     MachineIRBuilder &MIRBuilder,
                                     Register OrigRet, const Type *OrigRetTy,
                                     const SmallVectorImpl<Register> &args,
                                     SPIRVGlobalRegistry *GR) {
  LLVM_DEBUG(dbgs() << "Generating OpenCL Builtin: " << demangledName << "\n");

  SPIRVType *retTy = nullptr;
  if (OrigRetTy && !OrigRetTy->isVoidTy())
    retTy = GR->assignTypeToVReg(OrigRetTy, OrigRet, MIRBuilder);

  auto firstBraceIdx = demangledName.find_first_of('(');
  auto nameNoArgs = demangledName.substr(0, firstBraceIdx);

  auto extInstIDOpt = getOpenCL_stdFromName(nameNoArgs.str());
  if (extInstIDOpt.hasValue()) {
    auto extInstID = extInstIDOpt.getValue();
    return genOpenCLExtInst(extInstID, MIRBuilder, OrigRet, retTy, args, GR);
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

  auto extInstMatch = typeDependantExtInstMap.find(nameNoArgs.str());
  if (extInstMatch != typeDependantExtInstMap.end()) {
    char typeChar = demangledName[firstBraceIdx + 1];
    int idx = -1;
    if (typeChar == 'u')
      idx = 0;
    else if (typeChar == 'c' || typeChar == 's' || typeChar == 'i' ||
             typeChar == 'l')
      idx = 1;
    else if (typeChar == 'f' || typeChar == 'd' || typeChar == 'h')
      idx = 2;
    if (idx != -1) {
      assert(unsigned(idx) < extInstMatch->second.size());
      CL::OpenCL_std extInstId = extInstMatch->second[idx];
      return genOpenCLExtInst(extInstId, MIRBuilder, OrigRet, retTy, args, GR);
    }
  }
  // If the name is instantiated template, separate the name of the builtin and
  // the return type.
  StringRef returnTypeStr = "";
  if (nameNoArgs.find('<') && nameNoArgs.endswith(">")) {
    auto tmpStr = nameNoArgs.split('<');
    tmpStr = tmpStr.first.rsplit(' ');
    nameNoArgs = tmpStr.second;
    returnTypeStr = tmpStr.first;
  }
  // Modify the names of builtins to use smaller map of opcodes.
  auto modifiedName = modifyBINameForLookup(nameNoArgs, retTy,
                                            demangledName[firstBraceIdx + 1]);
  using namespace SPIRV;
  const unsigned opcode = OpenCLBIToOperationMap.lookup(modifiedName);
  switch (opcode) {
  case OpFOrdEqual:
  case OpFUnordNotEqual:
  case OpFOrdGreaterThan:
  case OpFOrdGreaterThanEqual:
  case OpFOrdLessThan:
  case OpFOrdLessThanEqual:
  case OpFOrdNotEqual:
  case OpOrdered:
  case OpUnordered:
  case OpIsFinite:
  case OpIsInf:
  case OpIsNan:
  case OpIsNormal:
  case OpSignBitSet:
  case OpAny:
  case OpAll:
    return genOpenCLRelational(MIRBuilder, opcode, OrigRet, retTy, args, GR);
  case OpSelectSISCond:
    return MIRBuilder.buildSelect(OrigRet, args[0], args[1], args[2]);
  case OpSpecConstant:
    return buildSpecConstant(MIRBuilder, opcode, OrigRet, retTy, args, GR);
  case OpGroupAll:
  case OpGroupAny:
  case OpGroupBroadcast:
  case OpGroupIAdd:
  case OpGroupFAdd:
  case OpGroupFMin:
  case OpGroupUMin:
  case OpGroupSMin:
  case OpGroupFMax:
  case OpGroupUMax:
  case OpGroupSMax:
  case OpGroupNonUniformElect:
  case OpGroupNonUniformAll:
  case OpGroupNonUniformAny:
  case OpGroupNonUniformAllEqual:
  case OpGroupNonUniformBroadcast:
  case OpGroupNonUniformBroadcastFirst:
  case OpGroupNonUniformBallot:
  case OpGroupNonUniformInverseBallot:
  case OpGroupNonUniformBallotBitExtract:
  case OpGroupNonUniformBallotBitCount:
  case OpGroupNonUniformBallotFindLSB:
  case OpGroupNonUniformBallotFindMSB:
  case OpGroupNonUniformShuffle:
  case OpGroupNonUniformShuffleXor:
  case OpGroupNonUniformShuffleUp:
  case OpGroupNonUniformShuffleDown:
  case OpGroupNonUniformIAdd:
  case OpGroupNonUniformFAdd:
  case OpGroupNonUniformIMul:
  case OpGroupNonUniformFMul:
  case OpGroupNonUniformSMin:
  case OpGroupNonUniformUMin:
  case OpGroupNonUniformFMin:
  case OpGroupNonUniformSMax:
  case OpGroupNonUniformUMax:
  case OpGroupNonUniformFMax:
  case OpGroupNonUniformBitwiseAnd:
  case OpGroupNonUniformBitwiseOr:
  case OpGroupNonUniformBitwiseXor:
  case OpGroupNonUniformLogicalAnd:
  case OpGroupNonUniformLogicalOr:
  case OpGroupNonUniformLogicalXor:
    return genOpenCLOpGroup(MIRBuilder, nameNoArgs, opcode, OrigRet, retTy,
                            args, GR);
  default:
    break;
  }

  const BuiltIn::BuiltIn builtInVar = OpenCLBIToBuiltInVarMap.lookup(nameNoArgs);
  switch (builtInVar) {
  case BuiltIn::SubgroupEqMask:
  case BuiltIn::SubgroupGeMask:
  case BuiltIn::SubgroupGtMask:
  case BuiltIn::SubgroupLeMask:
  case BuiltIn::SubgroupLtMask:
  case BuiltIn::GlobalLinearId: {
    unsigned BitWidth = GR->getScalarOrVectorBitWidth(retTy);
    LLT llt;
    if (retTy->getOpcode() == SPIRV::OpTypeVector)
      llt = LLT::fixed_vector(retTy->getOperand(2).getImm(), BitWidth);
    else
      llt = LLT::scalar(BitWidth);
    return buildBuiltInLoad(MIRBuilder, retTy, GR, builtInVar, llt, OrigRet);
  }
  case BuiltIn::GlobalInvocationId: {
    return genWorkgroupQuery(MIRBuilder, OrigRet, retTy, args, GR,
                             builtInVar, 0);
  }
  default:
    break;
  }

  char firstChar = nameNoArgs[0];
  switch (firstChar) {
  case 'a':
    if (nameNoArgs.startswith("atom")) {
      // Handle atom_add, atomic_add, and atomic_fetch_add etc.
      auto idx = strlen("atom_");
      if (nameNoArgs.startswith("atomic_"))
        idx = nameNoArgs.startswith("atomic_fetch_") ? strlen("atomic_fetch_")
                                                     : strlen("atomic_");
      const auto atomic = nameNoArgs.substr(idx);
      return genAtomicInstr(MIRBuilder, atomic, OrigRet, retTy, args, GR);
    }
    break;
  case 'b':
    if (nameNoArgs.startswith("barrier"))
      return genBarrier(MIRBuilder, SPIRV::OpControlBarrier, args, GR);
    break;
  case 'c':
    if (nameNoArgs.startswith("convert_")) {
      const auto convTy = demangledName.substr(strlen("convert_"));
      return genConvertInstr(MIRBuilder, convTy, OrigRet, retTy, args, GR);
    }
    break;
  case 'd':
    if (nameNoArgs.startswith("dot"))
      return genDotOrFMul(OrigRet, retTy, MIRBuilder, args, GR);
    break;
  case 'g': {
    bool local = nameNoArgs.startswith("get_local_");
    bool global = !local && nameNoArgs.startswith("get_global_");
    if (local || global) {
      auto subIdx = global ? strlen("get_global_") : strlen("get_local_");
      const auto str = nameNoArgs.substr(subIdx);
      return genGlobalLocalQuery(MIRBuilder, str, global, OrigRet, retTy, args, GR);
    } else if (nameNoArgs.startswith("get_image_")) {
      const auto imgStr = nameNoArgs.substr(strlen("get_image_"));
      return genImageQuery(MIRBuilder, imgStr, OrigRet, retTy, args, GR);
    } else if (nameNoArgs.startswith("get_group_id"))
      return genWorkgroupQuery(MIRBuilder, OrigRet, retTy, args, GR,
                               BuiltIn::WorkgroupId, 0);
    else if (nameNoArgs.startswith("get_enqueued_local_size"))
      return genWorkgroupQuery(MIRBuilder, OrigRet, retTy, args, GR,
                               BuiltIn::EnqueuedWorkgroupSize, 1);
    else if (nameNoArgs.startswith("get_num_groups"))
      return genWorkgroupQuery(MIRBuilder, OrigRet, retTy, args, GR,
                               BuiltIn::NumWorkgroups, 1);
    else if (nameNoArgs.startswith("get_work_dim"))
      // TODO
      llvm_unreachable("not implemented");
    break;
  }
  case 'r':
    if (nameNoArgs.startswith("read_image")) {
      if (demangledName.find("ocl_sampler") != StringRef::npos)
        return genSampledReadImage(MIRBuilder, OrigRet, retTy, args, GR);
      else if (demangledName.find("msaa") != StringRef::npos)
        return genReadImageMSAA(MIRBuilder, OrigRet, retTy, args, GR);
      else
        return genReadImage(MIRBuilder, OrigRet, retTy, args, GR);
    }
    break;
  case 'w':
    if (nameNoArgs.startswith("write_image"))
      return genWriteImage(MIRBuilder, args);
    else if (nameNoArgs.startswith("work_group_barrier"))
      return genBarrier(MIRBuilder, SPIRV::OpControlBarrier, args, GR);
    break;
  case '_':
    if (nameNoArgs.startswith("__translate_sampler_initializer")) {
      uint64_t bitMask = getIConstVal(args[0], MIRBuilder.getMRI());
      return buildSamplerLiteral(bitMask, OrigRet, retTy, MIRBuilder, GR);
    } else if (nameNoArgs.startswith("__spirv_SampledImage")) {
      return buildOpSampledImage(OrigRet, args[0], args[1], MIRBuilder, GR);
    } else if (nameNoArgs.startswith("__spirv_ImageSampleExplicitLod")) {
      auto rStr = nameNoArgs.substr(strlen("__spirv_ImageSampleExplicitLod"));
      // Separate return type string from "_RtypeN" suffix or instantiated
      // template name <type>.
      if (rStr.startswith("_R"))
        rStr = rStr.substr(strlen("_R"));
      else
        rStr = returnTypeStr;
      return buildOpImageSampleExplicitLod(MIRBuilder, OrigRet, rStr, args, GR);
    }
    break;
  default:
    break;
  }
  report_fatal_error("Cannot translate OpenCL built-in func: " + demangledName);
}

