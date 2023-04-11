//===-- SPIRVGlobalInstrRegistry.h - SPIR-V Global Instr Registry -*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===//
//
// The SPIRVGlobalInstrRegistry class is used to collect and contain information
// required for emitting SPIR-V global instructions (instructions which do not
// belong to any function and are emittied on a module level).
//
//===------------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVGLOBALINSTRREGISTRY_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVGLOBALINSTRREGISTRY_H

#include "MCTargetDesc/SPIRVBaseInfo.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {
namespace SPIRV {
/// Struct representing a global OpEntryPoint instruction for later printing.
struct OpEntryPointInst {
  const SPIRV::ExecutionModel::ExecutionModel ExecModel;
  const Function *const EntryFunction;

  OpEntryPointInst(const SPIRV::ExecutionModel::ExecutionModel ExecModel,
                   const Function *const EntryFunction)
      : ExecModel(ExecModel), EntryFunction(EntryFunction) {}
};

/// Struct representing a global OpName instruction for later printing.
struct OpNameInst {
  const Register NamedId;
  const MachineFunction *const NamedIdFunction;
  const std::string Name;

  OpNameInst(const Register NamedId, const MachineFunction *const NamedIdFunction,
             const std::string Name)
      : NamedId(NamedId), NamedIdFunction(NamedIdFunction), Name(Name) {}
};
} // namespace SPIRV

/// SPIRVGlobalInstrRegistry collects and stores information required for
/// emitting SPIR-V global instructions.
class SPIRVGlobalInstrRegistry {
private:
  SmallVector<SPIRV::OpEntryPointInst> OpEntryPointInsts;
  SmallVector<SPIRV::OpNameInst> OpNameInsts;

public:
  SPIRVGlobalInstrRegistry() {}

  /// Declare a new SPIR-V entry point.
  void addEntryPoint(const SPIRV::ExecutionModel::ExecutionModel ExecModel,
                     const Function *const EntryFunction) {
    OpEntryPointInsts.push_back(
        SPIRV::OpEntryPointInst(ExecModel, EntryFunction));
  }

  /// Get all already declared SPIR-V entry points.
  const SmallVector<SPIRV::OpEntryPointInst> &getAllEntryPoints() const {
    return OpEntryPointInsts;
  }

  void nameResultId(const Register NamedId,
                    const MachineFunction *const NamedIdFunction,
                    const std::string Name) {
    OpNameInsts.push_back(SPIRV::OpNameInst(NamedId, NamedIdFunction, Name));
  }

  const SmallVector<SPIRV::OpNameInst> &getAllOpNameInst() const {
    return OpNameInsts;
  }
};

} // namespace llvm
#endif // LLVM_LIB_TARGET_SPIRV_SPIRVGLOBALINSTRREGISTRY_H
