//===-- SPIRVRegisterInfo.td - SPIR-V Register defs --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  Declarations that describe the SPIR-V register file.
//
//===----------------------------------------------------------------------===//

let Namespace = "SPIRV" in {
  def p0 : PtrValueType <i32, 0>;

  // Class for registers that are the result of OpTypeXXX instructions
  def TYPE0 : Register<"TYPE0">;
  def TYPE : RegisterClass<"SPIRV", [i32], 32, (add TYPE0)>;

  // Class for every other non-type ID
  def ID0 : Register<"ID0">;
  def ID : RegisterClass<"SPIRV", [i32], 32, (add ID0)>;
  def VID0 : Register<"VID0">;

  // TODO: FID register class is only needed for constants selection,
  //       consider redesigning the pattern to get rid of this
  def FID0 : Register<"FID0">;
  def FID : RegisterClass<"SPIRV", [f32], 32, (add FID0)>;

  // Scalar ID
  def SID : RegisterClass<"SPIRV", [i32, f32, p0], 32, (add ID0)>;
  // Vector ID
  def VID : RegisterClass<"SPIRV", [v2i32, v2f32], 32, (add VID0)>;
  def ANYID : RegisterClass<"SPIRV", [i32, f32, p0, v2i32, v2f32], 32, (add SID, VID)>;
}
