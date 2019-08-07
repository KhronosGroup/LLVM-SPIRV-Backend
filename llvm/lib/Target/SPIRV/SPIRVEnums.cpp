//===-- SPIRVEnums.cpp - SPIR-V Enums and Related Functions -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains macros implementing the enum helper functions defined in
// SPIRVEnums.h, such as getEnumName(Enum e) and getEnumCapabilities(Enum e)
//
//===----------------------------------------------------------------------===//

#include "SPIRVEnums.h"

GEN_ENUM_IMPL(Capability)
GEN_ENUM_IMPL(SourceLanguage)
GEN_ENUM_IMPL(ExecutionModel)
GEN_ENUM_IMPL(AddressingModel)
GEN_ENUM_IMPL(MemoryModel)
GEN_ENUM_IMPL(ExecutionMode)
GEN_ENUM_IMPL(StorageClass)

// Dim must be implemented manually, as "1D" is not a valid C++ naming token
std::string getDimName(Dim::Dim dim) {
  switch (dim) {
  case Dim::DIM_1D:
    return "1D";
  case Dim::DIM_2D:
    return "2D";
  case Dim::DIM_3D:
    return "3D";
  case Dim::DIM_Cube:
    return "Cube";
  case Dim::DIM_Rect:
    return "Rect";
  case Dim::DIM_Buffer:
    return "Buffer";
  case Dim::DIM_SubpassData:
    return "SubpassData";
  default:
    return "UNKNOWN_Dim";
  }
}

GEN_ENUM_IMPL(SamplerAddressingMode)
GEN_ENUM_IMPL(SamplerFilterMode)

GEN_ENUM_IMPL(ImageFormat)
GEN_ENUM_IMPL(ImageChannelOrder)
GEN_ENUM_IMPL(ImageChannelDataType)
GEN_MASK_ENUM_IMPL(ImageOperand)

GEN_MASK_ENUM_IMPL(FPFastMathMode)
GEN_ENUM_IMPL(FPRoundingMode)

GEN_ENUM_IMPL(LinkageType)
GEN_ENUM_IMPL(AccessQualifier)
GEN_ENUM_IMPL(FunctionParameterAttribute)

GEN_ENUM_IMPL(Decoration)
GEN_ENUM_IMPL(BuiltIn)

GEN_MASK_ENUM_IMPL(SelectionControl)
GEN_MASK_ENUM_IMPL(LoopControl)
GEN_MASK_ENUM_IMPL(FunctionControl)

GEN_MASK_ENUM_IMPL(MemorySemantics)
GEN_MASK_ENUM_IMPL(MemoryOperand)

GEN_ENUM_IMPL(Scope)
GEN_ENUM_IMPL(GroupOperation)

GEN_ENUM_IMPL(KernelEnqueueFlags)
GEN_MASK_ENUM_IMPL(KernelProfilingInfo)

namespace MS = MemorySemantics;
namespace SC = StorageClass;
MS::MemorySemantics getMemSemanticsForStorageClass(SC::StorageClass sc) {
  switch (sc) {
  case SC::StorageBuffer:
  case SC::Uniform:
    return MS::UniformMemory;
  case SC::Workgroup:
    return MS::WorkgroupMemory;
  case SC::CrossWorkgroup:
    return MS::CrossWorkgroupMemory;
  case SC::AtomicCounter:
    return MS::AtomicCounterMemory;
  case SC::Image:
    return MS::ImageMemory;
  default:
    return MS::None;
  }
}

DEF_BUILTIN_LINK_STR_FUNC_BODY()

GEN_EXTENSION_IMPL(Extension)
