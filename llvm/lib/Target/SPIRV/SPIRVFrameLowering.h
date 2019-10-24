//===-- SPIRVFrameLowering.h - Define frame lowering for SPIR-V -*- C++-*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class implements SPIRV-specific bits of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVFRAMELOWERING_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class SPIRVSubtarget;

class SPIRVFrameLowering : public TargetFrameLowering {
public:
  explicit SPIRVFrameLowering(const SPIRVSubtarget &sti)
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(8), 0) {}

  void emitPrologue(MachineFunction &MF,
                    MachineBasicBlock &MBB) const override {}
  void emitEpilogue(MachineFunction &MF,
                    MachineBasicBlock &MBB) const override {}

  bool hasFP(const MachineFunction &MF) const override { return false; }
};
} // namespace llvm
#endif
