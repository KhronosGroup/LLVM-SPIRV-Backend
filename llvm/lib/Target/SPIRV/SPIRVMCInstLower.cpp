//=- SPIRVMCInstLower.cpp - Convert SPIR-V MachineInstr to MCInst -*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower SPIR-V MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "SPIRVMCInstLower.h"
#include "SPIRV.h"
#include "SPIRVSubtarget.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

// Defined in SPIRVAsmPrinter.cpp
extern Register getOrCreateMBBRegister(const MachineBasicBlock &MBB,
                                       SPIRVGlobalRegistry *GR);

void SPIRVMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI,
                             SPIRVGlobalRegistry *GR) const {
  OutMI.setOpcode(MI->getOpcode());
  const MachineFunction *MF = MI->getMF();
  bool IsMetaFunc = GR->getMetaMF() == MF;
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    // At this stage, SPIR-V should only have Register and Immediate operands
    MCOperand MCOp;
    switch (MO.getType()) {
    default:
      MI->print(errs());
      llvm_unreachable("unknown operand type");
    case MachineOperand::MO_MachineBasicBlock:
      MCOp = MCOperand::createReg(getOrCreateMBBRegister(*MO.getMBB(), GR));
      break;
    case MachineOperand::MO_Register: {
      Register NewReg = GR->getRegisterAlias(MF, MO.getReg());
      bool IsOldReg = IsMetaFunc || !NewReg.isValid();
      MCOp = MCOperand::createReg(IsOldReg ? MO.getReg() : NewReg);
      break;
    }
    case MachineOperand::MO_Immediate:
      MCOp = MCOperand::createImm(MO.getImm());
      break;
    case MachineOperand::MO_FPImmediate:
      MCOp = MCOperand::createDFPImm(
          MO.getFPImm()->getValueAPF().convertToFloat());
      break;
    }

    OutMI.addOperand(MCOp);
  }
}
