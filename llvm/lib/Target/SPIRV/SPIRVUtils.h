//===--- SPIRVUtils.h ---- SPIR-V Utility Functions -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains miscellaneous utility functions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVUTILS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVUTILS_H

#include "SPIRVSymbolicOperands.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include <string>

// Add the given string as a series of integer operand, inserting null
// terminators and padding to make sure the operands all have 32-bit
// little-endian words.
void addStringImm(const llvm::StringRef &Str, llvm::MCInst &Inst);
void addStringImm(const llvm::StringRef &Str, llvm::MachineInstrBuilder &MIB);
void addStringImm(const llvm::StringRef &Str, llvm::IRBuilder<> &B,
                  std::vector<llvm::Value *> &Args);

// Read the series of integer operands back as a null-terminated string using
// the reverse of the logic in addStringImm.
std::string getStringImm(const llvm::MachineInstr &MI, unsigned int StartIndex);

// Add the given numerical immediate to MIB.
void addNumImm(const llvm::APInt &Imm, llvm::MachineInstrBuilder &MIB,
               bool IsFloat = false);

// Add an OpName instruction for the given target register.
void buildOpName(llvm::Register Target, const llvm::StringRef &Name,
                 llvm::MachineIRBuilder &MIRBuilder);

// Add an OpDecorate instruction for the given Reg.
void buildOpDecorate(llvm::Register Reg, llvm::MachineIRBuilder &MIRBuilder,
                     Decoration::Decoration Dec,
                     const std::vector<uint32_t> &DecArgs,
                     llvm::StringRef StrImm = "");

// Convert a SPIR-V storage class to the corresponding LLVM IR address space.
unsigned int storageClassToAddressSpace(StorageClass::StorageClass SC);

// Convert an LLVM IR address space to a SPIR-V storage class.
StorageClass::StorageClass addressSpaceToStorageClass(unsigned int AddrSpace);

// Utility method to constrain an instruction's operands to the correct
// register classes, and return true if this worked.
// TODO: get rid of using this function
bool constrainRegOperands(llvm::MachineInstrBuilder &MIB,
                          llvm::MachineFunction *MF = nullptr);

MemorySemantics::MemorySemantics
getMemSemanticsForStorageClass(StorageClass::StorageClass sc);

// Find def instruction for the given ConstReg, walking through
// spv_track_constant and ASSIGN_TYPE instructions. Updates ConstReg by def
// of OpConstant instruction.
llvm::MachineInstr *
getDefInstrMaybeConstant(llvm::Register &ConstReg,
                         const llvm::MachineRegisterInfo *MRI);

// Get constant integer value of the given ConstReg.
uint64_t getIConstVal(llvm::Register ConstReg,
                      const llvm::MachineRegisterInfo *MRI);
#endif // LLVM_LIB_TARGET_SPIRV_SPIRVUTILS_H
