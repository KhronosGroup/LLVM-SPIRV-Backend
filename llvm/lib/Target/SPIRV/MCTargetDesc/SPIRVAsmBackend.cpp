//===-- SPIRVAsmBackend.cpp - SPIR-V Assembler Backend ---------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SPIRVMCTargetDesc.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/EndianStream.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace {

class SPIRVAsmBackend : public MCAsmBackend {
public:
  SPIRVAsmBackend(support::endianness Endian) : MCAsmBackend(Endian) {}

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override;

  // No instruction requires relaxation
  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override {
    return false;
  }

  unsigned getNumFixupKinds() const override { return 1; }

  bool mayNeedRelaxation(const MCInst &Inst,
                         const MCSubtargetInfo &STI) const override {
    return false;
  }

  void relaxInstruction(const MCInst &Inst, const MCSubtargetInfo &STI,
                        MCInst &Res) const override {}

  bool writeNopData(raw_ostream &OS, uint64_t Count) const override {
    return false;
  }
};

} // end anonymous namespace


std::unique_ptr<MCObjectTargetWriter>
SPIRVAsmBackend::createObjectTargetWriter() const {
  return createSPIRVObjectTargetWriter();
}

MCAsmBackend *llvm::createSPIRVAsmBackend(const Target &T,
                                          const MCSubtargetInfo &STI,
                                          const MCRegisterInfo &MRI,
                                          const MCTargetOptions &) {
  return new SPIRVAsmBackend(support::little);
}

