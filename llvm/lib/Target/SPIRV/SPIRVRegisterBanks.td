//===-- SPIRVRegisterBanks.td - Describe SPIR-V RegBanks ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Add two placeholder register banks - one for type ids from OpTypeXXX
// instructions, and one for every other variable's ID. These shouldn't
// be used though as RegisterBankSelection is disabled.
def IDRegBank : RegisterBank<"IDBank", [ID]>;
def TYPERegBank : RegisterBank<"TYPEBank", [TYPE]>;
