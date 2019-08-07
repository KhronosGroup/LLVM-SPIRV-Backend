//===--- SPIRVStrings.h ---- String Utility Functions -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The methods here are used to add these string literals as a series of 32-bit
// integer operands with the correct format, and unpack them if necessary when
// making string comparisons in compiler passes.
//
// SPIR-V requires null-terminated UTF-8 strings padded to 32-bit alignment.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVSTRINGS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVSTRINGS_H

#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <string>

// Add the given string as a series of integer operand, inserting null
// terminators and padding to make sure the operands all have 32-bit
// little-endian words
void addStringImm(const llvm::StringRef &str, llvm::MachineInstrBuilder &MIB);

// Read the series of integer operands back as a null-terminated string using
// the reverse of the logic in addStringImm
std::string getStringImm(const llvm::MachineInstr &MI, unsigned int startIndex);

// Add an OpName instruction for the given target register
void buildOpName(llvm::Register target, const llvm::StringRef &name,
                 llvm::MachineIRBuilder &MIRBuilder);

#endif
