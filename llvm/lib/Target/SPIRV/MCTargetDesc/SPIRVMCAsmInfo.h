//===-- SPIRVMCAsmInfo.h - SPIR-V asm properties --------------*- C++ -*--====//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the SPIRVMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_MCTARGETDESC_SPIRVMCASMINFO_H
#define LLVM_LIB_TARGET_SPIRV_MCTARGETDESC_SPIRVMCASMINFO_H

#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class SPIRVMCAsmInfo : public MCAsmInfo {
public:
  explicit SPIRVMCAsmInfo(const Triple &TT, const MCTargetOptions &Options) {
    IsLittleEndian = true;

    HasSingleParameterDotFile = false;
    HasDotTypeDotSizeDirective = false;

    MinInstAlignment = 4;

    CodePointerSize = 4;
    CommentString = ";";
    HasFunctionAlignment = false;
  }

  bool shouldOmitSectionDirective(StringRef SectionName) const override { return true; }
};
} // namespace llvm

#endif
