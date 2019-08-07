//===- SPIRVRegisterBankInfo.cpp ------------------------------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the targeting of the RegisterBankInfo class for SPIR-V.
//
//===----------------------------------------------------------------------===//

#include "SPIRVRegisterBankInfo.h"
#include "SPIRVRegisterInfo.h"
#include "llvm/CodeGen/GlobalISel/RegisterBank.h"

#define GET_REGINFO_ENUM
#include "SPIRVGenRegisterInfo.inc"

#define GET_TARGET_REGBANK_IMPL
#include "SPIRVGenRegisterBank.inc"

// This file will be TableGen'ed at some point.
#include "SPIRVGenRegisterBankInfo.def"

using namespace llvm;

const RegisterBank &SPIRVRegisterBankInfo::getRegBankFromRegClass(
    const TargetRegisterClass &RC) const {
  switch (RC.getID()) {
  case SPIRV::TYPERegClassID:
    return SPIRV::TYPERegBank;
  case SPIRV::IDRegClassID:
  case SPIRV::ANYRegClassID:
    return SPIRV::IDRegBank;
  }
  llvm_unreachable("Unknown register class");
}
