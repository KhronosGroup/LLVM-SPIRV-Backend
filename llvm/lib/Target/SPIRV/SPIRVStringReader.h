//===--- SPIRVStringReader.h ---- String Utility Functions ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SPIR-V string literals are represented as a series of 32-bit integer operands
// so the helper function below can be used to read these operands back into
// a usable string (e.g. for string name comparisons).
//
// SPIR-V requires null-terminated UTF-8 strings padded to 32-bit alignment.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVSTRINGREADER_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVSTRINGREADER_H

#include <string>

// Return a string representation of the operands from startIndex onwards.
// Templated to allow both MachineInstr and MCInst to use the same logic.
template <class InstType>
std::string getSPIRVStringOperand(const InstType &MI, unsigned int StartIndex) {
  std::string s = ""; // Iteratively append to this string

  const unsigned int NumOps = MI.getNumOperands();
  bool IsFinished = false;
  for (unsigned int i = StartIndex; i < NumOps && !IsFinished; ++i) {
    const auto &Op = MI.getOperand(i);
    if (!Op.isImm()) // Stop if we hit a register operand
      break;
    uint32_t Imm = Op.getImm(); // Each i32 word is up to 4 characters
    for (unsigned shiftAmount = 0; shiftAmount < 32; shiftAmount += 8) {
      char c = (Imm >> shiftAmount) & 0xff;
      if (c == 0) { // Stop if we hit a null-terminator character
        IsFinished = true;
        break;
      } else {
        s += c; // Otherwise, append the character to the result string
      }
    }
  }
  return s;
}

#endif
