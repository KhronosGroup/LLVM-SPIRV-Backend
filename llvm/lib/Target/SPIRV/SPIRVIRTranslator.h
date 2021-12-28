//===-- SPIRVIRTranslator.h - Override IRTranslator for SPIR-V -*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Overrides GlobalISel's IRTranslator to customize default lowering behavior.
// This is mostly to stop aggregate types like Structs from getting expanded
// into scalars, and to maintain type information required for SPIR-V using the
// SPIRVTypeRegistry before it gets discarded as LLVM IR is lowered to GMIR.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVIRTRANSLATOR_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVIRTRANSLATOR_H

#include "SPIRVGlobalRegistry.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"

namespace llvm {
class SPIRVIRTranslator : public IRTranslator {
private:
  SPIRVTypeRegistry *TR;
  SPIRVGeneralDuplicatesTracker *DT;

  // Generate OpVariables with linkage data and their initializers if necessary
  //   bool buildGlobalValue(Register Reg, const GlobalValue *GV,
  //                         MachineIRBuilder &MIRBuilder);

  // protected:
  //   // Whenever a VReg gets created, it needs type info, and also name debug
  //   info
  //   // emitted before the LLVM IR value gets discarded
  //   // ArrayRef<Register> getOrCreateVRegs(const Value &Val) override;

  //   bool translate(const Instruction &Inst) override;

  //   // Override to translate aggregate constants and null types
  //   bool translate(const Constant &C, Register Reg) override;

  //   bool translateInst(const User &U, unsigned OpCode);

  //   // Translate to OpAccessChain etc. instead of flattening indices
  //   bool translateGetElementPtr(const User &U, MachineIRBuilder &MIRBuilder);

  //   // Translate to OpVectorShuffle instead of turning indices into const
  //   vector. bool translateShuffleVector(const User &U, MachineIRBuilder
  //   &MIRBuilder);

  //   // Translate to OpCompositeExtract instead of flattening indices
  //   bool translateExtractValue(const User &U, MachineIRBuilder &MIRBuilder);

  //   // Translate to OpCompositeInsert instead of flattening indices
  //   bool translateInsertValue(const User &U, MachineIRBuilder &MIRBuilder);

  //   // Override to ensure an explicit Bitcast is always emitted
  //   bool translateBitCast(const User &U, MachineIRBuilder &MIRBuilder);

  //   // Override to define a single struct reg rather than 2 separate regs
  //   bool translateOverflowIntrinsic(const CallInst &CI, unsigned Op,
  //                                   MachineIRBuilder &MIRBuilder);

  //   // Override to define a single struct reg rather than 2 separate regs
  //   bool translateAtomicCmpXchg(const User &U, MachineIRBuilder &MIRBuilder);

public:
  // Initialize the type registry before calling parent function
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // end namespace llvm
#endif
