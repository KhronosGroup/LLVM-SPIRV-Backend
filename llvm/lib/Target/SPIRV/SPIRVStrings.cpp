//===--- SPIRVStrings.cpp ---- String Utility Functions ---------*- C++ -*-===//
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

#include "SPIRVStrings.h"
#include "SPIRV.h"
#include "SPIRVStringReader.h"

using namespace llvm;

// Add the given string as a series of integer operands, inserting null
// terminators and padding to make sure the operands all have 32-bit
// little-endian words
void addStringImm(const StringRef &str, MachineInstrBuilder &MIB) {
  // Get length including padding and null terminator
  const size_t len = str.size() + 1;
  const size_t paddedLen = (len % 4 == 0) ? len : len + (4 - (len % 4));
  for (unsigned i = 0; i < paddedLen; i += 4) {
    uint32_t word = 0u; // Build up this 32-bit word from 4 8-bit chars
    for (unsigned wordIndex = 0; wordIndex < 4; ++wordIndex) {
      unsigned strIndex = i + wordIndex;
      uint8_t charToAdd = 0; // Initilize char as padding/null
      if (strIndex < (len - 1)) { // If it's within the string, get a real char
        charToAdd = str[strIndex];
      }
      word |= (charToAdd << (wordIndex * 8));
    }
    MIB.addImm(word); // Add an operand for the 32-bits of chars or padding
  }
}

void addStringImm(const StringRef &str, IRBuilder<> &B,
                  std::vector<Value *> &Args) {
  // Get length including padding and null terminator
  const size_t len = str.size() + 1;
  const size_t paddedLen = (len % 4 == 0) ? len : len + (4 - (len % 4));
  for (unsigned i = 0; i < paddedLen; i += 4) {
    uint32_t word = 0u; // Build up this 32-bit word from 4 8-bit chars
    for (unsigned wordIndex = 0; wordIndex < 4; ++wordIndex) {
      unsigned strIndex = i + wordIndex;
      uint8_t charToAdd = 0;      // Initilize char as padding/null
      if (strIndex < (len - 1)) { // If it's within the string, get a real char
        charToAdd = str[strIndex];
      }
      word |= (charToAdd << (wordIndex * 8));
    }
    Args.push_back(
        B.getInt32(word)); // Add an operand for the 32-bits of chars or padding
  }
}

// Read the series of integer operands back as a null-terminated string using
// the reverse of the logic in addStringImm
std::string getStringImm(const MachineInstr &MI, unsigned int startIndex) {
  return getSPIRVStringOperand(MI, startIndex);
}

void addNumImm(const APInt &Imm, MachineInstrBuilder &MIB, bool IsFloat) {
  const auto Bitwidth = Imm.getBitWidth();
  switch (Bitwidth) {
  case 1:
    break; // Already handled
  case 8:
  case 16:
  case 32:
    MIB.addImm(Imm.getZExtValue());
    break;
  case 64: {
    if (!IsFloat) {
      uint64_t FullImm = Imm.getZExtValue();
      uint32_t LowBits = FullImm & 0xffffffff;
      uint32_t HighBits = (FullImm >> 32) & 0xffffffff;
      MIB.addImm(LowBits).addImm(HighBits);
    } else
      MIB.addImm(Imm.getZExtValue());
    break;
  }
  default:
    report_fatal_error("Unsupported constant bitwidth");
  }
}

// Add an OpName instruction for the given target register
void buildOpName(Register target, const StringRef &name,
                 MachineIRBuilder &MIRBuilder) {
  if (!name.empty()) {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpName).addUse(target);
    addStringImm(name, MIB);
  }
}

void buildOpDecorate(Register Reg, MachineIRBuilder &MIRBuilder,
    Decoration::Decoration Dec, const std::vector<uint32_t> &DecArgs,
    StringRef StrImm) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpDecorate)
                 .addUse(Reg)
                 .addImm(Dec);
  if (!StrImm.empty())
    addStringImm(StrImm, MIB);
  for (const auto &DecArg : DecArgs)
    MIB.addImm(DecArg);
}
