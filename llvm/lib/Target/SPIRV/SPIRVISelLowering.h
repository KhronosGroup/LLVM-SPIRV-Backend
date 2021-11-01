//===-- SPIRVISelLowering.h - SPIR-V DAG Lowering Interface -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that SPIR-V uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVISELLOWERING_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

#include "llvm/IR/IntrinsicsSPIRV.h"

namespace llvm {
class SPIRVSubtarget;

class SPIRVTargetLowering : public TargetLowering {
public:
  explicit SPIRVTargetLowering(const TargetMachine &TM,
                               const SPIRVSubtarget &STI)
      : TargetLowering(TM) {}

  // Stop IRTranslator breaking up FMA instrs to preserve types information
  bool isFMAFasterThanFMulAndFAdd(const MachineFunction &MF,
                                  EVT) const override {
    return true;
  }

  // FIXME: this is to prevent sexts of non-i64 vector indices
  // which are generated within general IRTranslator hence
  // type generation for it is omitted
  MVT getVectorIdxTy(const DataLayout &DL) const override {
    return MVT::getIntegerVT(32);
  }

  // Avoid fail on v3i1 argument. Maybe we need to return 1 for all types.
  unsigned getNumRegistersForCallingConv(LLVMContext &Context,
                                         CallingConv::ID CC,
                                         EVT VT) const override {
    if (VT.isVector() && VT.getVectorNumElements() == 3
        && (VT.getVectorElementType() == MVT::i1
            || VT.getVectorElementType() == MVT::i8))
      return 1;
    return getNumRegisters(Context, VT);
  }

  // Avoid fail on v3i1 argument. Maybe we need to return i32 for all types.
  MVT getRegisterTypeForCallingConv(LLVMContext &Context,
                                    CallingConv::ID CC,
                                    EVT VT) const override {
    if (VT.isVector() && VT.getVectorNumElements() == 3) {
      if (VT.getVectorElementType() == MVT::i1)
        return MVT::v4i1;
      else if (VT.getVectorElementType() == MVT::i8)
        return MVT::v4i8;
    }
    return getRegisterType(Context, VT);
  }

  bool getTgtMemIntrinsic(IntrinsicInfo &Info, const CallInst &I,
                          MachineFunction &MF,
                          unsigned Intrinsic) const override {
    unsigned AlignIdx = 3;
    switch (Intrinsic) {
    case Intrinsic::spv_load:
      AlignIdx = 2;
      LLVM_FALLTHROUGH;
    case Intrinsic::spv_store: {
      if (I.getNumOperands() >= AlignIdx + 1) {
        auto *AlignOp = cast<ConstantInt>(I.getOperand(AlignIdx));
        Info.align = Align(AlignOp->getZExtValue());
      }
      Info.flags = static_cast<MachineMemOperand::Flags>(
          cast<ConstantInt>(I.getOperand(AlignIdx - 1))->getZExtValue());
      Info.memVT = MVT::i64;
      // MVT::getVT(PtrTy->getElementType());
      return true;
      break;
    }
    default:
      break;
    }
    return false;
  }
};
} // namespace llvm

#endif
