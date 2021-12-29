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

#include "SPIRVUtils.h"
#include "SPIRV.h"
#include "SPIRVStringReader.h"

using namespace llvm;

// Add the given string as a series of integer operands, inserting null
// terminators and padding to make sure the operands all have 32-bit
// little-endian words
void addStringImm(const StringRef &Str, MachineInstrBuilder &MIB) {
  // Get length including padding and null terminator
  const size_t Len = Str.size() + 1;
  const size_t PaddedLen = (Len % 4 == 0) ? Len : Len + (4 - (Len % 4));
  for (unsigned i = 0; i < PaddedLen; i += 4) {
    uint32_t Word = 0u; // Build up this 32-bit word from 4 8-bit chars
    for (unsigned WordIndex = 0; WordIndex < 4; ++WordIndex) {
      unsigned StrIndex = i + WordIndex;
      uint8_t CharToAdd = 0;      // Initilize char as padding/null
      if (StrIndex < (Len - 1)) { // If it's within the string, get a real char
        CharToAdd = Str[StrIndex];
      }
      Word |= (CharToAdd << (WordIndex * 8));
    }
    MIB.addImm(Word); // Add an operand for the 32-bits of chars or padding
  }
}

void addStringImm(const StringRef &Str, IRBuilder<> &B,
                  std::vector<Value *> &Args) {
  // Get length including padding and null terminator
  const size_t Len = Str.size() + 1;
  const size_t PaddedLen = (Len % 4 == 0) ? Len : Len + (4 - (Len % 4));
  for (unsigned i = 0; i < PaddedLen; i += 4) {
    uint32_t Word = 0u; // Build up this 32-bit word from 4 8-bit chars
    for (unsigned WordIndex = 0; WordIndex < 4; ++WordIndex) {
      unsigned StrIndex = i + WordIndex;
      uint8_t CharToAdd = 0;      // Initilize char as padding/null
      if (StrIndex < (Len - 1)) { // If it's within the string, get a real char
        CharToAdd = Str[StrIndex];
      }
      Word |= (CharToAdd << (WordIndex * 8));
    }
    Args.push_back(
        B.getInt32(Word)); // Add an operand for the 32-bits of chars or padding
  }
}

// Read the series of integer operands back as a null-terminated string using
// the reverse of the logic in addStringImm
std::string getStringImm(const MachineInstr &MI, unsigned int StartIndex) {
  return getSPIRVStringOperand(MI, StartIndex);
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
void buildOpName(Register Target, const StringRef &Name,
                 MachineIRBuilder &MIRBuilder) {
  if (!Name.empty()) {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpName).addUse(Target);
    addStringImm(Name, MIB);
  }
}

void buildOpDecorate(Register Reg, MachineIRBuilder &MIRBuilder,
                     Decoration::Decoration Dec,
                     const std::vector<uint32_t> &DecArgs, StringRef StrImm) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpDecorate).addUse(Reg).addImm(Dec);
  if (!StrImm.empty())
    addStringImm(StrImm, MIB);
  for (const auto &DecArg : DecArgs)
    MIB.addImm(DecArg);
}
