//===-- SPIRVEnums.h - SPIR-V Enums and Related Functions -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines macros to generate all SPIR-V enum types.
// The macros also define helper functions for each enum such as
// getEnumName(Enum e) and getEnumCapabilities(Enum e) which can be
// auto-generated in SPIRVEnums.cpp and used when printing SPIR-V in
// textual form, or checking for required extensions, versions etc.
//
// TODO Auto-genrate this from SPIR-V header files
//
// If the names of any enums change in this file, SPIRVEnums.td and
// InstPrinter/SPIRVInstPrinter.h must also be updated, as the name of the enums
// is used to select the correct assembly printing methods.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_ENUMS_H
#define LLVM_LIB_TARGET_SPIRV_ENUMS_H

#include "SPIRVExtensions.h"
#include <string>
#include <vector>

// Macros to define an enum and the functions to return its name and its
// required capabilities, extensions, and SPIR-V versions

#define LIST(...) __VA_ARGS__

#define MAKE_ENUM(Enum, Var, Val, Caps, Exts, MinVer, MaxVer) Var = Val,

#define MAKE_NAME_CASE(Enum, Var, Val, Caps, Exts, MinVer, MaxVer)             \
  case Enum::Var:                                                              \
    return #Var;

#define MAKE_MASK_ENUM_NAME_CASE(Enum, Var, Val, Caps, Exts, MinVer, MaxVer)   \
  if (e == Enum::Var) {                                                        \
    return #Var;                                                               \
  } else if ((Enum::Var != 0) && (e & Enum::Var)) {                            \
    nameString += sep + #Var;                                                  \
    sep = "|";                                                                 \
  }

#define MAKE_BUILTIN_LINK_CASE(Enum, Var, Val, Caps, Exts, MinVer, MaxVer)     \
  case Enum::Var:                                                              \
    return "__spirv_BuiltIn" #Var;

#define DEF_ENUM(EnumName, DefEnumCommand)                                     \
  namespace EnumName {                                                         \
  enum EnumName : uint32_t { DefEnumCommand(EnumName, MAKE_ENUM) };            \
  }

#define DEF_NAME_FUNC_HEADER(EnumName)                                         \
  std::string get##EnumName##Name(EnumName::EnumName e);

// Use this for enums that can only take a single value
#define DEF_NAME_FUNC_BODY(EnumName, DefEnumCommand)                           \
  std::string get##EnumName##Name(EnumName::EnumName e) {                      \
    switch (e) { DefEnumCommand(EnumName, MAKE_NAME_CASE) }                    \
    return "UNKNOWN_ENUM";                                                     \
  }

// Use this for bitmasks that can take multiple values e.g. DontInline|Const
#define DEF_MASK_NAME_FUNC_BODY(EnumName, DefEnumCommand)                      \
  std::string get##EnumName##Name(EnumName::EnumName e) {                      \
    std::string nameString = "";                                               \
    std::string sep = "";                                                      \
    DefEnumCommand(EnumName, MAKE_MASK_ENUM_NAME_CASE);                        \
    return nameString;                                                         \
  }

#define DEF_BUILTIN_LINK_STR_FUNC_BODY()                                       \
  const char *getLinkStrForBuiltIn(BuiltIn::BuiltIn builtIn) {                 \
    switch (builtIn) { DEF_BuiltIn(BuiltIn, MAKE_BUILTIN_LINK_CASE) }          \
    return "UNKNOWN_BUILTIN";                                                  \
  }

#define GEN_ENUM_HEADER(EnumName)                                              \
  DEF_ENUM(EnumName, DEF_##EnumName)                                           \
  DEF_NAME_FUNC_HEADER(EnumName)                                               \

// Use this for enums that can only take a single value
#define GEN_ENUM_IMPL(EnumName)                                                \
  DEF_NAME_FUNC_BODY(EnumName, DEF_##EnumName)                                 \

// Use this for bitmasks that can take multiple values e.g. DontInline|Const
#define GEN_MASK_ENUM_IMPL(EnumName)                                           \
  DEF_MASK_NAME_FUNC_BODY(EnumName, DEF_##EnumName)                            \

#define GEN_INSTR_PRINTER_IMPL(EnumName)                                       \
  void SPIRVInstPrinter::print##EnumName(const MCInst *MI, unsigned OpNo,      \
                                         raw_ostream &O) {                     \
    if (OpNo < MI->getNumOperands()) {                                         \
      EnumName::EnumName e =                                                   \
          static_cast<EnumName::EnumName>(MI->getOperand(OpNo).getImm());      \
      O << get##EnumName##Name(e);                                             \
    }                                                                          \
  }

//===----------------------------------------------------------------------===//
// The actual enum definitions are added below
//
// Call GEN_ENUM_HEADER here in the header.
//
// Call GEN_ENUM_IMPL or GEN_MASK_ENUM_IMPL in SPIRVEnums.cpp, depending on
// whether the enum can only take a single value, or whether it can be a bitmask
// of multiple values e.g. FunctionControl which can be DontInline|Const.
//
// Call GEN_INSTR_PRINTER_IMPL in SPIRVInstPrinter.cpp, and ensure the
// corresponding operand printing function declaration is in SPIRVInstPrinter.h,
// with the correct enum name in SPIRVEnums.td
//
// Syntax for each line is:
//    X(N, Name, IdNum, Capabilities, Extensions, MinVer, MaxVer) \
// Capabilities and Extensions must be either empty brackets {}, a single
// bracketed capability e.g. {Matrix}, or if multiple capabilities are required,
// use the LIST macro:
//    LIST({SampledBuffer, Matrix})
// MinVer and MaxVer are 32 bit integers in the format 0|Major|Minor|0 e.g
// SPIR-V v1.3 = 0x00010300. Using 0 means unspecified.
//
// Each enum def must fit on a  single line, so additional macros are sometimes
// used for defining capabilities with long names.
//===----------------------------------------------------------------------===//

#define SK(Suff) SPV_KHR_##Suff

#define ANUIE(Pref) Pref##ArrayNonUniformIndexingEXT
#define SBBA(Bits) StorageBuffer##Bits##BitAccess
#define SB8BA(Pref) Pref##StorageBuffer8BitAccess
#define ADIE(Pref) Pref##ArrayDynamicIndexingEXT
#define SNUE ShaderNonUniformEXT
#define VAR_PTR_SB VariablePointersStorageBuffer

#define DRAW_PARAMS SPV_KHR_shader_draw_parameters
#define SPV_16_BIT SPV_KHR_16bit_storeage
#define SPV_VAR_PTR SPV_KHR_variable_pointers
#define SPV_PDC SPV_KHR_post_depth_coverage
#define SPV_FLT_CTRL SPV_KHR_float_controls

#define DEF_Capability(N, X)                                                   \
  X(N, Matrix, 0, {}, {}, 0, 0)                                                \
  X(N, Shader, 1, {Matrix}, {}, 0, 0)                                          \
  X(N, Geometry, 2, {Shader}, {}, 0, 0)                                        \
  X(N, Tessellation, 3, {Shader}, {}, 0, 0)                                    \
  X(N, Addresses, 4, {}, {}, 0, 0)                                             \
  X(N, Linkage, 5, {}, {}, 0, 0)                                               \
  X(N, Kernel, 6, {}, {}, 0, 0)                                                \
  X(N, Vector16, 7, {Kernel}, {}, 0, 0)                                        \
  X(N, Float16Buffer, 8, {Kernel}, {}, 0, 0)                                   \
  X(N, Float16, 9, {}, {}, 0, 0)                                               \
  X(N, Float64, 10, {}, {}, 0, 0)                                              \
  X(N, Int64, 11, {}, {}, 0, 0)                                                \
  X(N, Int64Atomics, 12, {Int64}, {}, 0, 0)                                    \
  X(N, ImageBasic, 13, {Kernel}, {}, 0, 0)                                     \
  X(N, ImageReadWrite, 14, {ImageBasic}, {}, 0, 0)                             \
  X(N, ImageMipmap, 15, {ImageBasic}, {}, 0, 0)                                \
  X(N, Pipes, 17, {Kernel}, {}, 0, 0)                                          \
  X(N, Groups, 18, {}, {}, 0, 0)                                               \
  X(N, DeviceEnqueue, 19, {}, {}, 0, 0)                                        \
  X(N, LiteralSampler, 20, {Kernel}, {}, 0, 0)                                 \
  X(N, AtomicStorage, 21, {Shader}, {}, 0, 0)                                  \
  X(N, Int16, 22, {}, {}, 0, 0)                                                \
  X(N, TessellationPointSize, 23, {Tessellation}, {}, 0, 0)                    \
  X(N, GeometryPointSize, 24, {Geometry}, {}, 0, 0)                            \
  X(N, ImageGatherExtended, 25, {Shader}, {}, 0, 0)                            \
  X(N, StorageImageMultisample, 27, {Shader}, {}, 0, 0)                        \
  X(N, UniformBufferArrayDynamicIndexing, 28, {Shader}, {}, 0, 0)              \
  X(N, SampledImageArrayDymnamicIndexing, 29, {Shader}, {}, 0, 0)              \
  X(N, ClipDistance, 32, {Shader}, {}, 0, 0)                                   \
  X(N, CullDistance, 33, {Shader}, {}, 0, 0)                                   \
  X(N, ImageCubeArray, 34, {SampledCubeArray}, {}, 0, 0)                       \
  X(N, SampleRateShading, 35, {Shader}, {}, 0, 0)                              \
  X(N, ImageRect, 36, {SampledRect}, {}, 0, 0)                                 \
  X(N, SampledRect, 37, {Shader}, {}, 0, 0)                                    \
  X(N, GenericPointer, 38, {Addresses}, {}, 0, 0)                              \
  X(N, Int8, 39, {}, {}, 0, 0)                                                 \
  X(N, InputAttachment, 40, {Shader}, {}, 0, 0)                                \
  X(N, SparseResidency, 41, {Shader}, {}, 0, 0)                                \
  X(N, MinLod, 42, {Shader}, {}, 0, 0)                                         \
  X(N, Sampled1D, 43, {}, {}, 0, 0)                                            \
  X(N, Image1D, 44, {Sampled1D}, {}, 0, 0)                                     \
  X(N, SampledCubeArray, 45, {Shader}, {}, 0, 0)                               \
  X(N, SampledBuffer, 46, {}, {}, 0, 0)                                        \
  X(N, ImageBuffer, 47, {SampledBuffer}, {}, 0, 0)                             \
  X(N, ImageMSArray, 48, {Shader}, {}, 0, 0)                                   \
  X(N, StorageImageExtendedFormats, 49, {Shader}, {}, 0, 0)                    \
  X(N, ImageQuery, 50, {Shader}, {}, 0, 0)                                     \
  X(N, DerivativeControl, 51, {Shader}, {}, 0, 0)                              \
  X(N, InterpolationFunction, 52, {Shader}, {}, 0, 0)                          \
  X(N, TransformFeedback, 53, {Shader}, {}, 0, 0)                              \
  X(N, GeometryStreams, 54, {Geometry}, {}, 0, 0)                              \
  X(N, StorageImageReadWithoutFormat, 55, {Shader}, {}, 0, 0)                  \
  X(N, StorageImageWriteWithoutFormat, 56, {Shader}, {}, 0, 0)                 \
  X(N, MultiViewport, 57, {Geometry}, {}, 0, 0)                                \
  X(N, SubgroupDispatch, 58, {DeviceEnqueue}, {}, 0x10100, 0)                  \
  X(N, NamedBarrier, 59, {Kernel}, {}, 0x10100, 0)                             \
  X(N, PipeStorage, 60, {Pipes}, {}, 0x10100, 0)                               \
  X(N, GroupNonUniform, 61, {}, {}, 0x10300, 0)                                \
  X(N, GroupNonUniformVote, 62, {GroupNonUniform}, {}, 0x10300, 0)             \
  X(N, GroupNonUniformArithmetic, 63, {GroupNonUniform}, {}, 0x10300, 0)       \
  X(N, GroupNonUniformBallot, 64, {GroupNonUniform}, {}, 0x10300, 0)           \
  X(N, GroupNonUniformShuffle, 65, {GroupNonUniform}, {}, 0x10300, 0)          \
  X(N, GroupNonUniformShuffleRelative, 66, {GroupNonUniform}, {}, 0x10300, 0)  \
  X(N, GroupNonUniformClustered, 67, {GroupNonUniform}, {}, 0x10300, 0)        \
  X(N, GroupNonUniformQuad, 68, {GroupNonUniform}, {}, 0x10300, 0)             \
  X(N, SubgroupBallotKHR, 4423, {}, {SPV_KHR_shader_ballot}, 0, 0)             \
  X(N, DrawParameters, 4427, {Shader}, {DRAW_PARAMS}, 0x10300, 0)              \
  X(N, SubgroupVoteKHR, 4431, {}, {SPV_KHR_subgroup_vote}, 0, 0)               \
  X(N, SBBA(16), 4433, {}, {SPV_16_BIT}, 0x10300, 0)                           \
  X(N, StorageUniform16, 4434, {SBBA(16)}, {SPV_16_BIT}, 0x10300, 0)           \
  X(N, StoragePushConstant16, 4435, {}, {SPV_16_BIT}, 0x10300, 0)              \
  X(N, StorageInputOutput16, 4436, {}, {SPV_16_BIT}, 0x10300, 0)               \
  X(N, DeviceGroup, 4437, {}, {SPV_KHR_device_group}, 0x10300, 0)              \
  X(N, MultiView, 4439, {Shader}, {SPV_KHR_multiview}, 0x10300, 0)             \
  X(N, VAR_PTR_SB, 4441, {Shader}, {SPV_VAR_PTR}, 0x10300, 0)                  \
  X(N, VariablePointers, 4442, {VAR_PTR_SB}, {SPV_VAR_PTR}, 0x10300, 0)        \
  X(N, AtomicStorageOps, 4445, {}, {SPV_KHR_shader_atomic_counter_ops}, 0, 0)  \
  X(N, SampleMaskPostDepthCoverage, 4447, {}, {SPV_PDC}, 0, 0)                 \
  X(N, StorageBuffer8BitAccess, 4448, {}, {SPV_KHR_8bit_storage}, 0, 0)        \
  X(N, SB8BA(UniformAnd), 4449, {SBBA(8)}, {SPV_KHR_8bit_storage}, 0, 0)       \
  X(N, StoragePushConstant8, 4450, {}, {SPV_KHR_8bit_storage}, 0, 0)           \
  X(N, DenormPreserve, 4464, {}, {SPV_FLT_CTRL}, 0x10400, 0)                   \
  X(N, DenormFlushToZero, 4465, {}, {SPV_FLT_CTRL}, 0x10400, 0)                \
  X(N, SignedZeroInfNanPreserve, 4466, {}, {SPV_FLT_CTRL}, 0x10400, 0)         \
  X(N, RoundingModeRTE, 4467, {}, {SPV_FLT_CTRL}, 0x10400, 0)                  \
  X(N, RoundingModeRTZ, 4468, {}, {SPV_FLT_CTRL}, 0x10400, 0)                  \
  X(N, Float16ImageAMD, 5008, {Shader}, {}, 0, 0)                              \
  X(N, ImageGatherBiasLodAMD, 5009, {Shader}, {}, 0, 0)                        \
  X(N, FragmentMaskAMD, 5010, {Shader}, {}, 0, 0)                              \
  X(N, StencilExportEXT, 5013, {Shader}, {}, 0, 0)                             \
  X(N, ImageReadWriteLodAMD, 5015, {Shader}, {}, 0, 0)                         \
  X(N, SampleMaskOverrideCoverageNV, 5249, {SampleRateShading}, {}, 0, 0)      \
  X(N, GeometryShaderPassthroughNV, 5251, {Geometry}, {}, 0, 0)                \
  X(N, ShaderViewportIndexLayerEXT, 5254, {MultiViewport}, {}, 0, 0)           \
  X(N, ShaderViewportMaskNV, 5255, {ShaderViewportIndexLayerEXT}, {}, 0, 0)    \
  X(N, ShaderStereoViewNV, 5259, {ShaderViewportMaskNV}, {}, 0, 0)             \
  X(N, PerViewAttributesNV, 5260, {MultiView}, {}, 0, 0)                       \
  X(N, FragmentFullyCoveredEXT, 5265, {Shader}, {}, 0, 0)                      \
  X(N, MeshShadingNV, 5266, {Shader}, {}, 0, 0)                                \
  X(N, ShaderNonUniformEXT, 5301, {Shader}, {}, 0, 0)                          \
  X(N, RuntimeDescriptorArrayEXT, 5302, {Shader}, {}, 0, 0)                    \
  X(N, ADIE(InputAttachment), 5303, {InputAttachment}, {}, 0, 0)               \
  X(N, ADIE(UniformTexelBuffer), 5304, {SampledBuffer}, {}, 0, 0)              \
  X(N, ADIE(StorageTexelBuffer), 5305, {ImageBuffer}, {}, 0, 0)                \
  X(N, ANUIE(UniformBuffer), 5306, {ShaderNonUniformEXT}, {}, 0, 0)            \
  X(N, ANUIE(SampledImage), 5307, {ShaderNonUniformEXT}, {}, 0, 0)             \
  X(N, ANUIE(StorageBuffer), 5308, {ShaderNonUniformEXT}, {}, 0, 0)            \
  X(N, ANUIE(StorageImage), 5309, {ShaderNonUniformEXT}, {}, 0, 0)             \
  X(N, ANUIE(InputAttachment), 5310, LIST({InputAttachment, SNUE}), {}, 0, 0)  \
  X(N, ANUIE(UniformTexelBuffer), 5311, LIST({SampledBuffer, SNUE}), {}, 0, 0) \
  X(N, ANUIE(StorageTexelBuffer), 5312, LIST({ImageBuffer, SNUE}), {}, 0, 0)   \
  X(N, RayTracingNV, 5340, {Shader}, {}, 0, 0)                                 \
  X(N, SubgroupShuffleINTEL, 5568, {}, {}, 0, 0)                               \
  X(N, SubgroupBufferBlockIOINTEL, 5569, {}, {}, 0, 0)                         \
  X(N, SubgroupImageBlockIOINTEL, 5570, {}, {}, 0, 0)                          \
  X(N, SubgroupImageMediaBlockIOINTEL, 5579, {}, {}, 0, 0)                     \
  X(N, SubgroupAvcMotionEstimationINTEL, 5696, {}, {}, 0, 0)                   \
  X(N, SubgroupAvcMotionEstimationIntraINTEL, 5697, {}, {}, 0, 0)              \
  X(N, SubgroupAvcMotionEstimationChromaINTEL, 5698, {}, {}, 0, 0)             \
  X(N, GroupNonUniformPartitionedNV, 5297, {}, {}, 0, 0)                       \
  X(N, VulkanMemoryModelKHR, 5345, {}, {}, 0, 0)                               \
  X(N, VulkanMemoryModelDeviceScopeKHR, 5346, {}, {}, 0, 0)                    \
  X(N, ImageFootprintNV, 5282, {}, {}, 0, 0)                                   \
  X(N, FragmentBarycentricNV, 5284, {}, {}, 0, 0)                              \
  X(N, ComputeDerivativeGroupQuadsNV, 5288, {}, {}, 0, 0)                      \
  X(N, ComputeDerivativeGroupLinearNV, 5350, {}, {}, 0, 0)                     \
  X(N, FragmentDensityEXT, 5291, {Shader}, {}, 0, 0)                           \
  X(N, PhysicalStorageBufferAddressesEXT, 5347, {Shader}, {}, 0, 0)            \
  X(N, CooperativeMatrixNV, 5357, {Shader}, {}, 0, 0)
GEN_ENUM_HEADER(Capability)

#define DEF_SourceLanguage(N, X)                                               \
  X(N, Unknown, 0, {}, {}, 0, 0)                                               \
  X(N, ESSL, 1, {}, {}, 0, 0)                                                  \
  X(N, GLSL, 2, {}, {}, 0, 0)                                                  \
  X(N, OpenCL_C, 3, {}, {}, 0, 0)                                              \
  X(N, OpenCL_CPP, 4, {}, {}, 0, 0)                                            \
  X(N, HLSL, 5, {}, {}, 0, 0)
GEN_ENUM_HEADER(SourceLanguage)

#define PSB(Suff) PhysicalStorageBuffer##Suff
#define DEF_AddressingModel(N, X)                                              \
  X(N, Logical, 0, {}, {}, 0, 0)                                               \
  X(N, Physical32, 1, {Addresses}, {}, 0, 0)                                   \
  X(N, Physical64, 2, {Addresses}, {}, 0, 0)                                   \
  X(N, PSB(64EXT), 5348, {PSB(AddressesEXT)}, {}, 0, 0)
GEN_ENUM_HEADER(AddressingModel)

#define DEF_ExecutionModel(N, X)                                               \
  X(N, Vertex, 0, {Shader}, {}, 0, 0)                                          \
  X(N, TessellationControl, 1, {Tessellation}, {}, 0, 0)                       \
  X(N, TessellationEvaluation, 2, {Tessellation}, {}, 0, 0)                    \
  X(N, Geometry, 3, {Geometry}, {}, 0, 0)                                      \
  X(N, Fragment, 4, {Shader}, {}, 0, 0)                                        \
  X(N, GLCompute, 5, {Shader}, {}, 0, 0)                                       \
  X(N, Kernel, 6, {Kernel}, {}, 0, 0)                                          \
  X(N, TaskNV, 5267, {MeshShadingNV}, {}, 0, 0)                                \
  X(N, MeshNV, 5268, {MeshShadingNV}, {}, 0, 0)                                \
  X(N, RayGenerationNV, 5313, {RayTracingNV}, {}, 0, 0)                        \
  X(N, IntersectionNV, 5314, {RayTracingNV}, {}, 0, 0)                         \
  X(N, AnyHitNV, 5315, {RayTracingNV}, {}, 0, 0)                               \
  X(N, ClosestHitNV, 5316, {RayTracingNV}, {}, 0, 0)                           \
  X(N, MissNV, 5317, {RayTracingNV}, {}, 0, 0)                                 \
  X(N, CallableNV, 5318, {RayTracingNV}, {}, 0, 0)
GEN_ENUM_HEADER(ExecutionModel)

#define DEF_MemoryModel(N, X)                                                  \
  X(N, Simple, 0, {Shader}, {}, 0, 0)                                          \
  X(N, GLSL450, 1, {Shader}, {}, 0, 0)                                         \
  X(N, OpenCL, 2, {Kernel}, {}, 0, 0)                                          \
  X(N, VulkanKHR, 3, {VulkanMemoryModelKHR}, {}, 0, 0)
GEN_ENUM_HEADER(MemoryModel)

#define MSNV MeshShadingNV
#define DG1(Suff) DerivativeGroup##Suff
#define DG2(Pref, Suff) Pref##DerivativeGroup##Suff
#define DEF_ExecutionMode(N, X)                                                \
  X(N, Invocations, 0, {Geometry}, {}, 0, 0)                                   \
  X(N, SpacingEqual, 1, {Tessellation}, {}, 0, 0)                              \
  X(N, SpacingFractionalEven, 2, {Tessellation}, {}, 0, 0)                     \
  X(N, SpacingFractionalOdd, 3, {Tessellation}, {}, 0, 0)                      \
  X(N, VertexOrderCw, 4, {Tessellation}, {}, 0, 0)                             \
  X(N, VertexOrderCcw, 5, {Tessellation}, {}, 0, 0)                            \
  X(N, PixelCenterInteger, 6, {Shader}, {}, 0, 0)                              \
  X(N, OriginUpperLeft, 7, {Shader}, {}, 0, 0)                                 \
  X(N, OriginLowerLeft, 8, {Shader}, {}, 0, 0)                                 \
  X(N, EarlyFragmentTests, 9, {Shader}, {}, 0, 0)                              \
  X(N, PointMode, 10, {Tessellation}, {}, 0, 0)                                \
  X(N, Xfb, 11, {TransformFeedback}, {}, 0, 0)                                 \
  X(N, DepthReplacing, 12, {Shader}, {}, 0, 0)                                 \
  X(N, DepthGreater, 14, {Shader}, {}, 0, 0)                                   \
  X(N, DepthLess, 15, {Shader}, {}, 0, 0)                                      \
  X(N, DepthUnchanged, 16, {Shader}, {}, 0, 0)                                 \
  X(N, LocalSize, 17, {}, {}, 0, 0)                                            \
  X(N, LocalSizeHint, 18, {Kernel}, {}, 0, 0)                                  \
  X(N, InputPoints, 19, {Geometry}, {}, 0, 0)                                  \
  X(N, InputLines, 20, {Geometry}, {}, 0, 0)                                   \
  X(N, InputLinesAdjacency, 21, {Geometry}, {}, 0, 0)                          \
  X(N, Triangles, 22, LIST({Geometry, Tessellation}), {}, 0, 0)                \
  X(N, InputTrianglesAdjacency, 23, {Geometry}, {}, 0, 0)                      \
  X(N, Quads, 24, {Tessellation}, {}, 0, 0)                                    \
  X(N, Isolines, 25, {Tessellation}, {}, 0, 0)                                 \
  X(N, OutputVertices, 26, LIST({Geometry, Tessellation, MSNV}), {}, 0, 0)     \
  X(N, OutputPoints, 27, LIST({Geometry, MeshShadingNV}), {}, 0, 0)            \
  X(N, OutputLineStrip, 28, {Geometry}, {}, 0, 0)                              \
  X(N, OutputTriangleStrip, 29, {Geometry}, {}, 0, 0)                          \
  X(N, VecTypeHint, 30, {Kernel}, {}, 0, 0)                                    \
  X(N, ContractionOff, 31, {Kernel}, {}, 0, 0)                                 \
  X(N, Initializer, 33, {Kernel}, {}, 0, 0)                                    \
  X(N, Finalizer, 34, {Kernel}, {}, 0, 0)                                      \
  X(N, SubgroupSize, 35, {SubgroupDispatch}, {}, 0, 0)                         \
  X(N, SubgroupsPerWorkgroup, 36, {SubgroupDispatch}, {}, 0, 0)                \
  X(N, SubgroupsPerWorkgroupId, 37, {SubgroupDispatch}, {}, 0, 0)              \
  X(N, LocalSizeId, 38, {}, {}, 0, 0)                                          \
  X(N, LocalSizeHintId, 39, {Kernel}, {}, 0, 0)                                \
  X(N, PostDepthCoverage, 4446, {SampleMaskPostDepthCoverage}, {}, 0, 0)       \
  X(N, DenormPreserve, 4459, {DenormPreserve}, {}, 0, 0)                       \
  X(N, DenormFlushToZero, 4460, {DenormFlushToZero}, {}, 0, 0)                 \
  X(N, SignedZeroInfNanPreserve, 4461, {SignedZeroInfNanPreserve}, {}, 0, 0)   \
  X(N, RoundingModeRTE, 4462, {RoundingModeRTE}, {}, 0, 0)                     \
  X(N, RoundingModeRTZ, 4463, {RoundingModeRTZ}, {}, 0, 0)                     \
  X(N, StencilRefReplacingEXT, 5027, {StencilExportEXT}, {}, 0, 0)             \
  X(N, OutputLinesNV, 5269, {MeshShadingNV}, {}, 0, 0)                         \
  X(N, DG1(QuadsNV), 5289, {DG2(Compute, QuadsNV)}, {}, 0, 0)                  \
  X(N, DG1(LinearNV), 5290, {DG2(Compute, LinearNV)}, {}, 0, 0)                \
  X(N, OutputTrianglesNV, 5298, {MeshShadingNV}, {}, 0, 0)
GEN_ENUM_HEADER(ExecutionMode)

#define DEF_StorageClass(N, X)                                                 \
  X(N, UniformConstant, 0, {}, {}, 0, 0)                                       \
  X(N, Input, 1, {}, {}, 0, 0)                                                 \
  X(N, Uniform, 2, {Shader}, {}, 0, 0)                                         \
  X(N, Output, 3, {Shader}, {}, 0, 0)                                          \
  X(N, Workgroup, 4, {}, {}, 0, 0)                                             \
  X(N, CrossWorkgroup, 5, {}, {}, 0, 0)                                        \
  X(N, Private, 6, {Shader}, {}, 0, 0)                                         \
  X(N, Function, 7, {}, {}, 0, 0)                                              \
  X(N, Generic, 8, {GenericPointer}, {}, 0, 0)                                 \
  X(N, PushConstant, 9, {Shader}, {}, 0, 0)                                    \
  X(N, AtomicCounter, 10, {AtomicStorage}, {}, 0, 0)                           \
  X(N, Image, 11, {}, {}, 0, 0)                                                \
  X(N, StorageBuffer, 12, {Shader}, {}, 0, 0)                                  \
  X(N, CallableDataNV, 5328, {RayTracingNV}, {}, 0, 0)                         \
  X(N, IncomingCallableDataNV, 5329, {RayTracingNV}, {}, 0, 0)                 \
  X(N, RayPayloadNV, 5338, {RayTracingNV}, {}, 0, 0)                           \
  X(N, HitAttributeNV, 5339, {RayTracingNV}, {}, 0, 0)                         \
  X(N, IncomingRayPayloadNV, 5342, {RayTracingNV}, {}, 0, 0)                   \
  X(N, ShaderRecordBufferNV, 5343, {RayTracingNV}, {}, 0, 0)                   \
  X(N, PSB(EXT), 5349, {PSB(AddressesEXT)}, {}, 0, 0)
GEN_ENUM_HEADER(StorageClass)

// Need to manually do the getDimName() function, as "1D" is not a valid token
// so it can't be auto-generated with these macros

#define DEF_Dim(N, X)                                                          \
  X(N, DIM_1D, 0, LIST({Sampled1D, Image1D}), {}, 0, 0)                        \
  X(N, DIM_2D, 1, LIST({Shader, Kernel, ImageMSArray}), {}, 0, 0)              \
  X(N, DIM_3D, 2, {}, {}, 0, 0)                                                \
  X(N, DIM_Cube, 3, LIST({Shader, ImageCubeArray}), {}, 0, 0)                  \
  X(N, DIM_Rect, 4, LIST({SampledRect, ImageRect}), {}, 0, 0)                  \
  X(N, DIM_Buffer, 5, LIST({SampledBuffer, ImageBuffer}), {}, 0, 0)            \
  X(N, DIM_SubpassData, 6, {InputAttachment}, {}, 0, 0)
GEN_ENUM_HEADER(Dim)

#define DEF_SamplerAddressingMode(N, X)                                        \
  X(N, None, 0, {Kernel}, {}, 0, 0)                                            \
  X(N, ClampToEdge, 1, {Kernel}, {}, 0, 0)                                     \
  X(N, Clamp, 2, {Kernel}, {}, 0, 0)                                           \
  X(N, Repeat, 3, {Kernel}, {}, 0, 0)                                          \
  X(N, RepeatMirrored, 4, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(SamplerAddressingMode)

#define DEF_SamplerFilterMode(N, X)                                            \
  X(N, Nearest, 0, {Kernel}, {}, 0, 0)                                         \
  X(N, Linear, 1, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(SamplerFilterMode)

#define DEF_ImageFormat(N, X)                                                  \
  X(N, Unknown, 0, {}, {}, 0, 0)                                               \
  X(N, Rgba32f, 1, {Shader}, {}, 0, 0)                                         \
  X(N, Rgba16f, 2, {Shader}, {}, 0, 0)                                         \
  X(N, R32f, 3, {Shader}, {}, 0, 0)                                            \
  X(N, Rgba8, 4, {Shader}, {}, 0, 0)                                           \
  X(N, Rgba8Snorm, 5, {Shader}, {}, 0, 0)                                      \
  X(N, Rg32f, 6, {StorageImageExtendedFormats}, {}, 0, 0)                      \
  X(N, Rg16f, 7, {StorageImageExtendedFormats}, {}, 0, 0)                      \
  X(N, R11fG11fB10f, 8, {StorageImageExtendedFormats}, {}, 0, 0)               \
  X(N, R16f, 9, {StorageImageExtendedFormats}, {}, 0, 0)                       \
  X(N, Rgba16, 10, {StorageImageExtendedFormats}, {}, 0, 0)                    \
  X(N, Rgb10A2, 11, {StorageImageExtendedFormats}, {}, 0, 0)                   \
  X(N, Rg16, 12, {StorageImageExtendedFormats}, {}, 0, 0)                      \
  X(N, Rg8, 13, {StorageImageExtendedFormats}, {}, 0, 0)                       \
  X(N, R16, 14, {StorageImageExtendedFormats}, {}, 0, 0)                       \
  X(N, R8, 15, {StorageImageExtendedFormats}, {}, 0, 0)                        \
  X(N, Rgba16Snorm, 16, {StorageImageExtendedFormats}, {}, 0, 0)               \
  X(N, Rg16Snorm, 17, {StorageImageExtendedFormats}, {}, 0, 0)                 \
  X(N, Rg8Snorm, 18, {StorageImageExtendedFormats}, {}, 0, 0)                  \
  X(N, R16Snorm, 19, {StorageImageExtendedFormats}, {}, 0, 0)                  \
  X(N, R8Snorm, 20, {StorageImageExtendedFormats}, {}, 0, 0)                   \
  X(N, Rgba32i, 21, {Shader}, {}, 0, 0)                                        \
  X(N, Rgba16i, 22, {Shader}, {}, 0, 0)                                        \
  X(N, Rgba8i, 23, {Shader}, {}, 0, 0)                                         \
  X(N, R32i, 24, {Shader}, {}, 0, 0)                                           \
  X(N, Rg32i, 25, {StorageImageExtendedFormats}, {}, 0, 0)                     \
  X(N, Rg16i, 26, {StorageImageExtendedFormats}, {}, 0, 0)                     \
  X(N, Rg8i, 27, {StorageImageExtendedFormats}, {}, 0, 0)                      \
  X(N, R16i, 28, {StorageImageExtendedFormats}, {}, 0, 0)                      \
  X(N, R8i, 29, {StorageImageExtendedFormats}, {}, 0, 0)                       \
  X(N, Rgba32ui, 30, {Shader}, {}, 0, 0)                                       \
  X(N, Rgba16ui, 31, {Shader}, {}, 0, 0)                                       \
  X(N, Rgba8ui, 32, {Shader}, {}, 0, 0)                                        \
  X(N, R32ui, 33, {Shader}, {}, 0, 0)                                          \
  X(N, Rgb10a2ui, 34, {StorageImageExtendedFormats}, {}, 0, 0)                 \
  X(N, Rg32ui, 35, {StorageImageExtendedFormats}, {}, 0, 0)                    \
  X(N, Rg16ui, 36, {StorageImageExtendedFormats}, {}, 0, 0)                    \
  X(N, Rg8ui, 37, {StorageImageExtendedFormats}, {}, 0, 0)                     \
  X(N, R16ui, 38, {StorageImageExtendedFormats}, {}, 0, 0)                     \
  X(N, R8ui, 39, {StorageImageExtendedFormats}, {}, 0, 0)
GEN_ENUM_HEADER(ImageFormat)

#define DEF_ImageChannelOrder(N, X)                                            \
  X(N, R, 0, {Kernel}, {}, 0, 0)                                               \
  X(N, A, 1, {Kernel}, {}, 0, 0)                                               \
  X(N, RG, 2, {Kernel}, {}, 0, 0)                                              \
  X(N, RA, 3, {Kernel}, {}, 0, 0)                                              \
  X(N, RGB, 4, {Kernel}, {}, 0, 0)                                             \
  X(N, RGBA, 5, {Kernel}, {}, 0, 0)                                            \
  X(N, BGRA, 6, {Kernel}, {}, 0, 0)                                            \
  X(N, ARGB, 7, {Kernel}, {}, 0, 0)                                            \
  X(N, Intensity, 8, {Kernel}, {}, 0, 0)                                       \
  X(N, Luminance, 9, {Kernel}, {}, 0, 0)                                       \
  X(N, Rx, 10, {Kernel}, {}, 0, 0)                                             \
  X(N, RGx, 11, {Kernel}, {}, 0, 0)                                            \
  X(N, RGBx, 12, {Kernel}, {}, 0, 0)                                           \
  X(N, Depth, 13, {Kernel}, {}, 0, 0)                                          \
  X(N, DepthStencil, 14, {Kernel}, {}, 0, 0)                                   \
  X(N, sRGB, 15, {Kernel}, {}, 0, 0)                                           \
  X(N, sRGBx, 16, {Kernel}, {}, 0, 0)                                          \
  X(N, sRGBA, 17, {Kernel}, {}, 0, 0)                                          \
  X(N, sBGRA, 18, {Kernel}, {}, 0, 0)                                          \
  X(N, ABGR, 19, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(ImageChannelOrder)

#define DEF_ImageChannelDataType(N, X)                                         \
  X(N, SnormInt8, 0, {}, {}, 0, 0)                                             \
  X(N, SnormInt16, 1, {}, {}, 0, 0)                                            \
  X(N, UnormInt8, 2, {Kernel}, {}, 0, 0)                                       \
  X(N, UnormInt16, 3, {Kernel}, {}, 0, 0)                                      \
  X(N, UnormShort565, 4, {Kernel}, {}, 0, 0)                                   \
  X(N, UnormShort555, 5, {Kernel}, {}, 0, 0)                                   \
  X(N, UnormInt101010, 6, {Kernel}, {}, 0, 0)                                  \
  X(N, SignedInt8, 7, {Kernel}, {}, 0, 0)                                      \
  X(N, SignedInt16, 8, {Kernel}, {}, 0, 0)                                     \
  X(N, SignedInt32, 9, {Kernel}, {}, 0, 0)                                     \
  X(N, UnsignedInt8, 10, {Kernel}, {}, 0, 0)                                   \
  X(N, UnsignedInt16, 11, {Kernel}, {}, 0, 0)                                  \
  X(N, UnsigendInt32, 12, {Kernel}, {}, 0, 0)                                  \
  X(N, HalfFloat, 13, {Kernel}, {}, 0, 0)                                      \
  X(N, Float, 14, {Kernel}, {}, 0, 0)                                          \
  X(N, UnormInt24, 15, {Kernel}, {}, 0, 0)                                     \
  X(N, UnormInt101010_2, 16, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(ImageChannelDataType)

#define DEF_ImageOperand(N, X)                                                 \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, Bias, 0x1, {Shader}, {}, 0, 0)                                          \
  X(N, Lod, 0x2, {}, {}, 0, 0)                                                 \
  X(N, Grad, 0x4, {}, {}, 0, 0)                                                \
  X(N, ConstOffset, 0x8, {}, {}, 0, 0)                                         \
  X(N, Offset, 0x10, {ImageGatherExtended}, {}, 0, 0)                          \
  X(N, ConstOffsets, 0x20, {ImageGatherExtended}, {}, 0, 0)                    \
  X(N, Sample, 0x40, {}, {}, 0, 0)                                             \
  X(N, MinLod, 0x80, {MinLod}, {}, 0, 0)                                       \
  X(N, MakeTexelAvailableKHR, 0x100, {VulkanMemoryModelKHR}, {}, 0, 0)         \
  X(N, MakeTexelVisibleKHR, 0x200, {VulkanMemoryModelKHR}, {}, 0, 0)           \
  X(N, NonPrivateTexelKHR, 0x400, {VulkanMemoryModelKHR}, {}, 0, 0)            \
  X(N, VolatileTexelKHR, 0x800, {VulkanMemoryModelKHR}, {}, 0, 0)              \
  X(N, SignExtend, 0x1000, {}, {}, 0, 0)                                       \
  X(N, ZeroExtend, 0x2000, {}, {}, 0, 0)
GEN_ENUM_HEADER(ImageOperand)

#define DEF_FPFastMathMode(N, X)                                               \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, NotNaN, 0x1, {Kernel}, {}, 0, 0)                                        \
  X(N, NotInf, 0x2, {Kernel}, {}, 0, 0)                                        \
  X(N, NSZ, 0x4, {Kernel}, {}, 0, 0)                                           \
  X(N, AllowRecip, 0x8, {Kernel}, {}, 0, 0)                                    \
  X(N, Fast, 0x10, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(FPFastMathMode)

#define DEF_FPRoundingMode(N, X)                                               \
  X(N, RTE, 0, {}, {}, 0, 0)                                                   \
  X(N, RTZ, 1, {}, {}, 0, 0)                                                   \
  X(N, RTP, 2, {}, {}, 0, 0)                                                   \
  X(N, RTN, 3, {}, {}, 0, 0)
GEN_ENUM_HEADER(FPRoundingMode)

#define DEF_LinkageType(N, X)                                                  \
  X(N, Export, 0, {Linkage}, {}, 0, 0)                                         \
  X(N, Import, 1, {Linkage}, {}, 0, 0)
GEN_ENUM_HEADER(LinkageType)

#define DEF_AccessQualifier(N, X)                                              \
  X(N, ReadOnly, 0, {Kernel}, {}, 0, 0)                                        \
  X(N, WriteOnly, 1, {Kernel}, {}, 0, 0)                                       \
  X(N, ReadWrite, 2, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(AccessQualifier)

#define DEF_FunctionParameterAttribute(N, X)                                   \
  X(N, Zext, 0, {Kernel}, {}, 0, 0)                                            \
  X(N, Sext, 1, {Kernel}, {}, 0, 0)                                            \
  X(N, ByVal, 2, {Kernel}, {}, 0, 0)                                           \
  X(N, Sret, 3, {Kernel}, {}, 0, 0)                                            \
  X(N, NoAlias, 4, {Kernel}, {}, 0, 0)                                         \
  X(N, NoCapture, 5, {Kernel}, {}, 0, 0)                                       \
  X(N, NoWrite, 6, {Kernel}, {}, 0, 0)                                         \
  X(N, NoReadWrite, 7, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(FunctionParameterAttribute)

#define DEF_Decoration(N, X)                                                   \
  X(N, RelaxedPrecision, 0, {Shader}, {}, 0, 0)                                \
  X(N, SpecId, 1, LIST({Shader, Kernel}), {}, 0, 0)                            \
  X(N, Block, 2, {Shader}, {}, 0, 0)                                           \
  X(N, BufferBlock, 3, {Shader}, {}, 0, 0)                                     \
  X(N, RowMajor, 4, {Matrix}, {}, 0, 0)                                        \
  X(N, ColMajor, 5, {Matrix}, {}, 0, 0)                                        \
  X(N, ArrayStride, 6, {Shader}, {}, 0, 0)                                     \
  X(N, MatrixStride, 7, {Matrix}, {}, 0, 0)                                    \
  X(N, GLSLShared, 8, {Shader}, {}, 0, 0)                                      \
  X(N, GLSLPacked, 9, {Shader}, {}, 0, 0)                                      \
  X(N, CPacked, 10, {Kernel}, {}, 0, 0)                                        \
  X(N, BuiltIn, 11, {}, {}, 0, 0)                                              \
  X(N, NoPerspective, 13, {Shader}, {}, 0, 0)                                  \
  X(N, Flat, 14, {Shader}, {}, 0, 0)                                           \
  X(N, Patch, 15, {Tessellation}, {}, 0, 0)                                    \
  X(N, Centroid, 16, {Shader}, {}, 0, 0)                                       \
  X(N, Sample, 17, {SampleRateShading}, {}, 0, 0)                              \
  X(N, Invariant, 18, {Shader}, {}, 0, 0)                                      \
  X(N, Restrict, 19, {}, {}, 0, 0)                                             \
  X(N, Aliased, 20, {}, {}, 0, 0)                                              \
  X(N, Volatile, 21, {}, {}, 0, 0)                                             \
  X(N, Constant, 22, {Kernel}, {}, 0, 0)                                       \
  X(N, Coherent, 23, {}, {}, 0, 0)                                             \
  X(N, NonWritable, 24, {}, {}, 0, 0)                                          \
  X(N, NonReadable, 25, {}, {}, 0, 0)                                          \
  X(N, Uniform, 26, {Shader}, {}, 0, 0)                                        \
  X(N, UniformId, 27, {Shader}, {}, 0, 0)                                      \
  X(N, SaturatedConversion, 28, {Kernel}, {}, 0, 0)                            \
  X(N, Stream, 29, {GeometryStreams}, {}, 0, 0)                                \
  X(N, Location, 30, {Shader}, {}, 0, 0)                                       \
  X(N, Component, 31, {Shader}, {}, 0, 0)                                      \
  X(N, Index, 32, {Shader}, {}, 0, 0)                                          \
  X(N, Binding, 33, {Shader}, {}, 0, 0)                                        \
  X(N, DescriptorSet, 34, {Shader}, {}, 0, 0)                                  \
  X(N, Offset, 35, {Shader}, {}, 0, 0)                                         \
  X(N, XfbBuffer, 36, {TransformFeedback}, {}, 0, 0)                           \
  X(N, XfbStride, 37, {TransformFeedback}, {}, 0, 0)                           \
  X(N, FuncParamAttr, 38, {Kernel}, {}, 0, 0)                                  \
  X(N, FPRoundingMode, 39, {}, {}, 0, 0)                                       \
  X(N, FPFastMathMode, 40, {Kernel}, {}, 0, 0)                                 \
  X(N, LinkageAttributes, 41, {Linkage}, {}, 0, 0)                             \
  X(N, NoContraction, 42, {Shader}, {}, 0, 0)                                  \
  X(N, InputAttachmentIndex, 43, {InputAttachment}, {}, 0, 0)                  \
  X(N, Alignment, 44, {Kernel}, {}, 0, 0)                                      \
  X(N, MaxByteOffset, 45, {Addresses}, {}, 0, 0)                               \
  X(N, AlignmentId, 46, {Kernel}, {}, 0, 0)                                    \
  X(N, MaxByteOffsetId, 47, {Addresses}, {}, 0, 0)                             \
  X(N, NoSignedWrap, 4469, {}, {SK(no_integer_wrap_decoration)}, 0x10400, 0)   \
  X(N, NoUnsignedWrap, 4470, {}, {SK(no_integer_wrap_decoration)}, 0x10400, 0) \
  X(N, ExplicitInterpAMD, 4999, {}, {}, 0, 0)                                  \
  X(N, OverrideCoverageNV, 5248, {SampleMaskOverrideCoverageNV}, {}, 0, 0)     \
  X(N, PassthroughNV, 5250, {GeometryShaderPassthroughNV}, {}, 0, 0)           \
  X(N, ViewportRelativeNV, 5252, {ShaderViewportMaskNV}, {}, 0, 0)             \
  X(N, SecondaryViewportRelativeNV, 5256, {ShaderStereoViewNV}, {}, 0, 0)      \
  X(N, PerPrimitiveNV, 5271, {MeshShadingNV}, {}, 0, 0)                        \
  X(N, PerViewNV, 5272, {MeshShadingNV}, {}, 0, 0)                             \
  X(N, PerVertexNV, 5273, {FragmentBarycentricNV}, {}, 0, 0)                   \
  X(N, NonUniformEXT, 5300, {ShaderNonUniformEXT}, {}, 0, 0)                   \
  X(N, CountBuffer, 5634, {}, {}, 0, 0)                                        \
  X(N, UserSemantic, 5635, {}, {}, 0, 0)                                       \
  X(N, RestrictPointerEXT, 5355, {PSB(AddressesEXT)}, {}, 0, 0)                \
  X(N, AliasedPointerEXT, 5356, {PSB(AddressesEXT)}, {}, 0, 0)
GEN_ENUM_HEADER(Decoration)

#define SBK SubgroupBallotKHR
#define PVANV PerViewAttributesNV
#define GNU GroupNonUniform
#define MSNV MeshShadingNV
#define DEF_BuiltIn(N, X)                                                      \
  X(N, Position, 0, {Shader}, {}, 0, 0)                                        \
  X(N, PointSize, 1, {Shader}, {}, 0, 0)                                       \
  X(N, ClipDistance, 3, {ClipDistance}, {}, 0, 0)                              \
  X(N, CullDistance, 4, {CullDistance}, {}, 0, 0)                              \
  X(N, VertexId, 5, {Shader}, {}, 0, 0)                                        \
  X(N, InstanceId, 6, {Shader}, {}, 0, 0)                                      \
  X(N, PrimitiveId, 7, LIST({Geometry, Tessellation, RayTracingNV}), {}, 0, 0) \
  X(N, InvocationId, 8, LIST({Geometry, Tessellation}), {}, 0, 0)              \
  X(N, Layer, 9, {Geometry}, {}, 0, 0)                                         \
  X(N, ViewportIndex, 10, {MultiViewport}, {}, 0, 0)                           \
  X(N, TessLevelOuter, 11, {Tessellation}, {}, 0, 0)                           \
  X(N, TessLevelInner, 12, {Tessellation}, {}, 0, 0)                           \
  X(N, TessCoord, 13, {Tessellation}, {}, 0, 0)                                \
  X(N, PatchVertices, 14, {Tessellation}, {}, 0, 0)                            \
  X(N, FragCoord, 15, {Shader}, {}, 0, 0)                                      \
  X(N, PointCoord, 16, {Shader}, {}, 0, 0)                                     \
  X(N, FrontFacing, 17, {Shader}, {}, 0, 0)                                    \
  X(N, SampleId, 18, {SampleRateShading}, {}, 0, 0)                            \
  X(N, SamplePosition, 19, {SampleRateShading}, {}, 0, 0)                      \
  X(N, SampleMask, 20, {Shader}, {}, 0, 0)                                     \
  X(N, FragDepth, 22, {Shader}, {}, 0, 0)                                      \
  X(N, HelperInvocation, 23, {Shader}, {}, 0, 0)                               \
  X(N, NumWorkGroups, 24, {}, {}, 0, 0)                                        \
  X(N, WorkgroupSize, 25, {}, {}, 0, 0)                                        \
  X(N, WorkgroupId, 26, {}, {}, 0, 0)                                          \
  X(N, LocalInvocationId, 27, {}, {}, 0, 0)                                    \
  X(N, GlobalInvocationId, 28, {}, {}, 0, 0)                                   \
  X(N, LocalInvocationIndex, 29, {}, {}, 0, 0)                                 \
  X(N, WorkDim, 30, {Kernel}, {}, 0, 0)                                        \
  X(N, GlobalSize, 31, {Kernel}, {}, 0, 0)                                     \
  X(N, EnqueuedWorkgroupSize, 32, {Kernel}, {}, 0, 0)                          \
  X(N, GlobalOffset, 33, {Kernel}, {}, 0, 0)                                   \
  X(N, GlobalLinearId, 34, {Kernel}, {}, 0, 0)                                 \
  X(N, SubgroupSize, 36, LIST({Kernel, GroupNonUniform, SBK}), {}, 0, 0)       \
  X(N, SubgroupMaxSize, 37, {Kernel}, {}, 0, 0)                                \
  X(N, NumSubgroups, 38, LIST({Kernel, GroupNonUniform}), {}, 0, 0)            \
  X(N, NumEnqueuedSubgroups, 39, {Kernel}, {}, 0, 0)                           \
  X(N, SubgroupId, 40, LIST({Kernel, GroupNonUniform}), {}, 0, 0)              \
  X(N, SubgroupLocalInvocationId, 41, LIST({Kernel, GNU, SBK}), {}, 0, 0)      \
  X(N, VertexIndex, 42, {Shader}, {}, 0, 0)                                    \
  X(N, InstanceIndex, 43, {Shader}, {}, 0, 0)                                  \
  X(N, SubgroupEqMask, 4416, LIST({SBK, GroupNonUniformBallot}), {}, 0, 0)     \
  X(N, SubgroupGeMask, 4417, LIST({SBK, GroupNonUniformBallot}), {}, 0, 0)     \
  X(N, SubgroupGtMask, 4418, LIST({SBK, GroupNonUniformBallot}), {}, 0, 0)     \
  X(N, SubgroupLeMask, 4419, LIST({SBK, GroupNonUniformBallot}), {}, 0, 0)     \
  X(N, SubgroupLtMask, 4420, LIST({SBK, GroupNonUniformBallot}), {}, 0, 0)     \
  X(N, BaseVertex, 4424, {DrawParameters}, {}, 0, 0)                           \
  X(N, BaseInstance, 4425, {DrawParameters}, {}, 0, 0)                         \
  X(N, DrawIndex, 4426, LIST({DrawParameters, MeshShadingNV}), {}, 0, 0)       \
  X(N, DeviceIndex, 4438, {DeviceGroup}, {}, 0, 0)                             \
  X(N, ViewIndex, 4440, {MultiView}, {}, 0, 0)                                 \
  X(N, BaryCoordNoPerspAMD, 4492, {}, {}, 0, 0)                                \
  X(N, BaryCoordNoPerspCentroidAMD, 4493, {}, {}, 0, 0)                        \
  X(N, BaryCoordNoPerspSampleAMD, 4494, {}, {}, 0, 0)                          \
  X(N, BaryCoordSmoothAMD, 4495, {}, {}, 0, 0)                                 \
  X(N, BaryCoordSmoothCentroid, 4496, {}, {}, 0, 0)                            \
  X(N, BaryCoordSmoothSample, 4497, {}, {}, 0, 0)                              \
  X(N, BaryCoordPullModel, 4498, {}, {}, 0, 0)                                 \
  X(N, FragStencilRefEXT, 5014, {StencilExportEXT}, {}, 0, 0)                  \
  X(N, ViewportMaskNV, 5253, LIST({ShaderViewportMaskNV, MSNV}), {}, 0, 0)     \
  X(N, SecondaryPositionNV, 5257, {ShaderStereoViewNV}, {}, 0, 0)              \
  X(N, SecondaryViewportMaskNV, 5258, {ShaderStereoViewNV}, {}, 0, 0)          \
  X(N, PositionPerViewNV, 5261, LIST({PVANV, MeshShadingNV}), {}, 0, 0)        \
  X(N, ViewportMaskPerViewNV, 5262, LIST({PVANV, MeshShadingNV}), {}, 0, 0)    \
  X(N, FullyCoveredEXT, 5264, {FragmentFullyCoveredEXT}, {}, 0, 0)             \
  X(N, TaskCountNV, 5274, {MeshShadingNV}, {}, 0, 0)                           \
  X(N, PrimitiveCountNV, 5275, {MeshShadingNV}, {}, 0, 0)                      \
  X(N, PrimitiveIndicesNV, 5276, {MeshShadingNV}, {}, 0, 0)                    \
  X(N, ClipDistancePerViewNV, 5277, {MeshShadingNV}, {}, 0, 0)                 \
  X(N, CullDistancePerViewNV, 5278, {MeshShadingNV}, {}, 0, 0)                 \
  X(N, LayerPerViewNV, 5279, {MeshShadingNV}, {}, 0, 0)                        \
  X(N, MeshViewCountNV, 5280, {MeshShadingNV}, {}, 0, 0)                       \
  X(N, MeshViewIndices, 5281, {MeshShadingNV}, {}, 0, 0)                       \
  X(N, BaryCoordNV, 5286, {FragmentBarycentricNV}, {}, 0, 0)                   \
  X(N, BaryCoordNoPerspNV, 5287, {FragmentBarycentricNV}, {}, 0, 0)            \
  X(N, FragSizeEXT, 5292, {FragmentDensityEXT}, {}, 0, 0)                      \
  X(N, FragInvocationCountEXT, 5293, {FragmentDensityEXT}, {}, 0, 0)           \
  X(N, LaunchIdNV, 5319, {RayTracingNV}, {}, 0, 0)                             \
  X(N, LaunchSizeNV, 5320, {RayTracingNV}, {}, 0, 0)                           \
  X(N, WorldRayOriginNV, 5321, {RayTracingNV}, {}, 0, 0)                       \
  X(N, WorldRayDirectionNV, 5322, {RayTracingNV}, {}, 0, 0)                    \
  X(N, ObjectRayOriginNV, 5323, {RayTracingNV}, {}, 0, 0)                      \
  X(N, ObjectRayDirectionNV, 5324, {RayTracingNV}, {}, 0, 0)                   \
  X(N, RayTminNV, 5325, {RayTracingNV}, {}, 0, 0)                              \
  X(N, RayTmaxNV, 5326, {RayTracingNV}, {}, 0, 0)                              \
  X(N, InstanceCustomIndexNV, 5327, {RayTracingNV}, {}, 0, 0)                  \
  X(N, ObjectToWorldNV, 5330, {RayTracingNV}, {}, 0, 0)                        \
  X(N, WorldToObjectNV, 5331, {RayTracingNV}, {}, 0, 0)                        \
  X(N, HitTNV, 5332, {RayTracingNV}, {}, 0, 0)                                 \
  X(N, HitKindNV, 5333, {RayTracingNV}, {}, 0, 0)                              \
  X(N, IncomingRayFlagsNV, 5351, {RayTracingNV}, {}, 0, 0)
GEN_ENUM_HEADER(BuiltIn)

#define DEF_SelectionControl(N, X)                                             \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, Flatten, 0x1, {}, {}, 0, 0)                                             \
  X(N, DontFlatten, 0x2, {}, {}, 0, 0)
GEN_ENUM_HEADER(SelectionControl)

#define DEF_LoopControl(N, X)                                                  \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, Unroll, 0x1, {}, {}, 0, 0)                                              \
  X(N, DontUnroll, 0x2, {}, {}, 0, 0)                                          \
  X(N, DependencyInfinite, 0x4, {}, {}, 0, 0)                                  \
  X(N, DependencyLength, 0x8, {}, {}, 0, 0)                                    \
  X(N, MinIterations, 0x10, {}, {}, 0, 0)                                      \
  X(N, MaxIterations, 0x20, {}, {}, 0, 0)                                      \
  X(N, IterationMultiple, 0x40, {}, {}, 0, 0)                                  \
  X(N, PeelCount, 0x80, {}, {}, 0, 0)                                          \
  X(N, PartialCount, 0x100, {}, {}, 0, 0)
GEN_ENUM_HEADER(LoopControl)

#define DEF_FunctionControl(N, X)                                              \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, Inline, 0x1, {}, {}, 0, 0)                                              \
  X(N, DontInline, 0x2, {}, {}, 0, 0)                                          \
  X(N, Pure, 0x4, {}, {}, 0, 0)                                                \
  X(N, Const, 0x8, {}, {}, 0, 0)
GEN_ENUM_HEADER(FunctionControl)

#define DEF_MemorySemantics(N, X)                                              \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, Acquire, 0x2, {}, {}, 0, 0)                                             \
  X(N, Release, 0x4, {}, {}, 0, 0)                                             \
  X(N, AcquireRelease, 0x8, {}, {}, 0, 0)                                      \
  X(N, SequentiallyConsistent, 0x10, {}, {}, 0, 0)                             \
  X(N, UniformMemory, 0x40, {Shader}, {}, 0, 0)                                \
  X(N, SubgroupMemory, 0x80, {}, {}, 0, 0)                                     \
  X(N, WorkgroupMemory, 0x100, {}, {}, 0, 0)                                   \
  X(N, CrossWorkgroupMemory, 0x200, {}, {}, 0, 0)                              \
  X(N, AtomicCounterMemory, 0x400, {AtomicStorage}, {}, 0, 0)                  \
  X(N, ImageMemory, 0x800, {}, {}, 0, 0)                                       \
  X(N, OutputMemoryKHR, 0x1000, {VulkanMemoryModelKHR}, {}, 0, 0)              \
  X(N, MakeAvailableKHR, 0x2000, {VulkanMemoryModelKHR}, {}, 0, 0)             \
  X(N, MakeVisibleKHR, 0x4000, {VulkanMemoryModelKHR}, {}, 0, 0)
GEN_ENUM_HEADER(MemorySemantics)

#define DEF_MemoryOperand(N, X)                                                \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, Volatile, 0x1, {}, {}, 0, 0)                                            \
  X(N, Aligned, 0x2, {}, {}, 0, 0)                                             \
  X(N, Nontemporal, 0x4, {}, {}, 0, 0)                                         \
  X(N, MakePointerAvailableKHR, 0x8, {VulkanMemoryModelKHR}, {}, 0, 0)         \
  X(N, MakePointerVisibleKHR, 0x10, {VulkanMemoryModelKHR}, {}, 0, 0)          \
  X(N, NonPrivatePointerKHR, 0x20, {VulkanMemoryModelKHR}, {}, 0, 0)
GEN_ENUM_HEADER(MemoryOperand)

#define DEF_Scope(N, X)                                                        \
  X(N, CrossDevice, 0, {}, {}, 0, 0)                                           \
  X(N, Device, 1, {}, {}, 0, 0)                                                \
  X(N, Workgroup, 2, {}, {}, 0, 0)                                             \
  X(N, Subgroup, 3, {}, {}, 0, 0)                                              \
  X(N, Invocation, 4, {}, {}, 0, 0)                                            \
  X(N, QueueFamilyKHR, 5, {VulkanMemoryModelKHR}, {}, 0, 0)
GEN_ENUM_HEADER(Scope)

#define GNUA GroupNonUniformArithmetic
#define GNUB GroupNonUniformBallot
#define GNUPNV GroupNonUniformPartitionedNV
#define DEF_GroupOperation(N, X)                                               \
  X(N, Reduce, 0, LIST({Kernel, GNUA, GNUB}), {}, 0, 0)                        \
  X(N, InclusiveScan, 1, LIST({Kernel, GNUA, GNUB}), {}, 0, 0)                 \
  X(N, ExclusiveScan, 2, LIST({Kernel, GNUA, GNUB}), {}, 0, 0)                 \
  X(N, ClusteredReduce, 3, {GroupNonUniformClustered}, {}, 0, 0)               \
  X(N, PartitionedReduceNV, 6, {GroupNonUniformPartitionedNV}, {}, 0, 0)       \
  X(N, PartitionedInclusiveScanNV, 7, {GNUPNV}, {}, 0, 0)                      \
  X(N, PartitionedExclusiveScanNV, 8, {GNUPNV}, {}, 0, 0)
GEN_ENUM_HEADER(GroupOperation)

#define DEF_KernelEnqueueFlags(N, X)                                           \
  X(N, NoWait, 0, {Kernel}, {}, 0, 0)                                          \
  X(N, WaitKernel, 1, {Kernel}, {}, 0, 0)                                      \
  X(N, WaitWorkGroup, 2, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(KernelEnqueueFlags)

#define DEF_KernelProfilingInfo(N, X)                                          \
  X(N, None, 0x0, {}, {}, 0, 0)                                                \
  X(N, CmdExecTime, 0x1, {Kernel}, {}, 0, 0)
GEN_ENUM_HEADER(KernelProfilingInfo)

MemorySemantics::MemorySemantics
getMemSemanticsForStorageClass(StorageClass::StorageClass sc);

const char *getLinkStrForBuiltIn(BuiltIn::BuiltIn builtIn);

#endif
