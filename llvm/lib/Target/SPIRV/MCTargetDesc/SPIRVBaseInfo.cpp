//===-- SPIRVBaseInfo.cpp - Top level SPIRV definitions ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation for helper mnemonic lookup functions,
// versioning/capabilities/extensions getters for symbolic/named operands used
// in various SPIR-V instructions.
//
//===----------------------------------------------------------------------===//

#include "SPIRVBaseInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

#include <string>

namespace {
using namespace llvm;
using namespace OperandCategory;
using namespace Extension;
using namespace Capability;

struct SymbolicOperand {
  ::OperandCategory::OperandCategory Category;
  uint32_t Value;
  StringRef Mnemonic;
  uint32_t MinVersion;
  uint32_t MaxVersion;
};

struct ExtensionEntry {
  ::OperandCategory::OperandCategory Category;
  uint32_t Value;
  ::Extension::Extension ReqExtension;
};

struct CapabilityEntry {
  ::OperandCategory::OperandCategory Category;
  uint32_t Value;
  ::Capability::Capability ReqCapability;
};

#define GET_SymbolicOperands_DECL
#define GET_SymbolicOperands_IMPL
#define GET_ExtensionEntries_DECL
#define GET_ExtensionEntries_IMPL
#define GET_CapabilityEntries_DECL
#define GET_CapabilityEntries_IMPL
#include "SPIRVGenTables.inc"
} // end anonymous namespace

std::string
getSymbolicOperandMnemonic(::OperandCategory::OperandCategory Category,
                           int32_t Value) {
  const SymbolicOperand *Lookup =
      lookupSymbolicOperandByCategoryAndValue(Category, Value);

  if (Lookup) {
    // Value that encodes just one enum value
    return Lookup->Mnemonic.str();
  } else if (Category == ::OperandCategory::ImageOperandOperand ||
             Category == ::OperandCategory::FPFastMathModeOperand ||
             Category == ::OperandCategory::SelectionControlOperand ||
             Category == ::OperandCategory::LoopControlOperand ||
             Category == ::OperandCategory::FunctionControlOperand ||
             Category == ::OperandCategory::MemorySemanticsOperand ||
             Category == ::OperandCategory::MemoryOperandOperand ||
             Category == ::OperandCategory::KernelProfilingInfoOperand) {
    // Value that encodes many enum values (one bit per enum value)
    std::string Name;
    std::string Separator;

    const SymbolicOperand *EnumValueInCategory =
        lookupSymbolicOperandByCategory(Category);

    while (EnumValueInCategory && EnumValueInCategory->Category == Category) {
      if ((EnumValueInCategory->Value != 0) &&
          (Value & EnumValueInCategory->Value)) {
        Name += Separator + EnumValueInCategory->Mnemonic.str();
        Separator = "|";
      }

      ++EnumValueInCategory;
    }

    return Name;
  }

  return "UNKNOWN";
}

uint32_t
getSymbolicOperandMinVersion(::OperandCategory::OperandCategory Category,
                             uint32_t Value) {
  const SymbolicOperand *Lookup =
      lookupSymbolicOperandByCategoryAndValue(Category, Value);

  if (Lookup)
    return Lookup->MinVersion;

  return 0;
}

uint32_t
getSymbolicOperandMaxVersion(::OperandCategory::OperandCategory Category,
                             uint32_t Value) {
  const SymbolicOperand *Lookup =
      lookupSymbolicOperandByCategoryAndValue(Category, Value);

  if (Lookup)
    return Lookup->MaxVersion;

  return 0;
}

std::vector<::Capability::Capability>
getSymbolicOperandCapabilities(::OperandCategory::OperandCategory Category,
                               uint32_t Value) {
  const CapabilityEntry *Capability =
      lookupCapabilityByCategoryAndValue(Category, Value);

  std::vector<::Capability::Capability> Capabilities;
  while (Capability && Capability->Category == Category &&
         Capability->Value == Value) {
    Capabilities.push_back(
        static_cast<::Capability::Capability>(Capability->ReqCapability));
    ++Capability;
  }

  return Capabilities;
}

std::vector<::Extension::Extension>
getSymbolicOperandExtensions(::OperandCategory::OperandCategory Category,
                             uint32_t Value) {
  const ExtensionEntry *Extension =
      lookupExtensionByCategoryAndValue(Category, Value);

  std::vector<::Extension::Extension> Extensions;
  while (Extension && Extension->Category == Category &&
         Extension->Value == Value) {
    Extensions.push_back(
        static_cast<::Extension::Extension>(Extension->ReqExtension));
    ++Extension;
  }

  return Extensions;
}

std::string getLinkStringForBuiltIn(::BuiltIn::BuiltIn BuiltInValue) {
  const SymbolicOperand *Lookup = lookupSymbolicOperandByCategoryAndValue(
      ::OperandCategory::BuiltInOperand, BuiltInValue);

  if (Lookup)
    return "__spirv_BuiltIn" + Lookup->Mnemonic.str();
  else
    return "UNKNOWN_BUILTIN";
}

// TODO: Remove function after new SPIRVOpenCLBIFs is merged
bool getSpirvBuiltInIdByName(llvm::StringRef Name, BuiltIn::BuiltIn &BI) {
  const std::string Prefix = "__spirv_BuiltIn";
  if (!Name.startswith(Prefix))
    return false;

  const SymbolicOperand *Lookup = lookupSymbolicOperandByCategoryAndMnemonic(
      BuiltInOperand, Name.drop_front(Prefix.length()));

  if (!Lookup)
    return false;

  BI = static_cast<BuiltIn::BuiltIn>(Lookup->Value);
  return true;
}
