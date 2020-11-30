//===-- SPIRVRegisterInfo.td - SPIR-V Register defs --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//  Declarations that describe the SPIR-V register file
//===----------------------------------------------------------------------===//

let Namespace = "SPIRV" in {

  // All registers are for 32-bit identifiers, so have a single dummy register

  // Class for registers that are the result of OpTypeXXX instructions
  def TYPE0 : Register<"TYPE0">;
  def TYPE : RegisterClass<"SPIRV", [i32], 32, (add TYPE0)>;

  // Class for every other non-type ID
  def ID0 : Register<"ID0">;
  def ID : RegisterClass<"SPIRV", [i32], 32, (add ID0)>;
  def FID0 : Register<"FID0">;
  def FID : RegisterClass<"SPIRV", [f32], 32, (add FID0)>;

  def ANYID : RegisterClass<"SPIRV", [i32, f32], 32, (add ID, FID)>;

  // A few instructions like OpName can take ids from both type and non-type
  // instructions, so we need a super-class to allow for both to count as valid
  // arguments for these instructions.
  def ANY : RegisterClass<"SPIRV", [i32], 32, (add TYPE, ID, FID)>;
}