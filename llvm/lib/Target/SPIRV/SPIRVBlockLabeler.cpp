//===-- SPIRVBlockLabeler.cpp - SPIR-V Block Labeling Pass Impl -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implementation of SPIRVBlockLabeler, which ensures all basic blocks
// start with an OpLabel. All MBB literals in OpBranchConditional, OpBranch,
// and OpPhi, are also fixed to use the virtual registers defined by OpLabel.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVInstrInfo.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVUtils.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-block-label"

namespace {
class SPIRVBlockLabeler : public MachineFunctionPass {
public:
  static char ID;
  SPIRVBlockLabeler() : MachineFunctionPass(ID) {
    initializeSPIRVBlockLabelerPass(*PassRegistry::getPassRegistry());
  }
  // Add OpLabels and terminators. Fix any instructions with MBB references
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace

// Return the instruction position for an OpLabel within MBB. At this stage,
// all hoistable global instrs will still be function-local, so we must skip
// past them plus the initial OpFunction and its parameters.
static MachineInstr *setLabelPos(MachineBasicBlock &MBB,
                                 const SPIRVInstrInfo *TII) {
  for (auto &I : MBB) {
    unsigned o = I.getOpcode();
    if (o == SPIRV::OpFunction || o == SPIRV::OpFunctionParameter ||
        TII->isHeaderInstr(I))
      continue;
    return &I;
  }
  llvm_unreachable("Cannot find OpLabel position");
}

// Build an OpLabel at the start of the block (ignoring hoistable instrs).
static Register buildLabel(MachineBasicBlock &MBB, const SPIRVInstrInfo *TII) {
  MachineInstr *MI = setLabelPos(MBB, TII);
  DebugLoc DL = MI->getDebugLoc();
  MachineRegisterInfo &MRI = MBB.getParent()->getRegInfo();
  Register LabelID = MRI.createVirtualRegister(&SPIRV::IDRegClass);
  BuildMI(*MI->getParent(), MI, DL, TII->get(SPIRV::OpLabel)).addDef(LabelID);
  if (!MBB.getName().empty()) {
    auto MIB = BuildMI(*MI->getParent(), MI, DL, TII->get(SPIRV::OpName))
                   .addUse(LabelID);
    addStringImm(MBB.getName(), MIB);
  }
  return LabelID;
}

// A unique identifier for MBBs for use as a map key
using MBB_ID = int;

static Register getLabelIDForMBB(const std::map<MBB_ID, Register> &BBNumMap,
                                 MBB_ID BBNum) {
  auto f = BBNumMap.find(BBNum);
  if (f != BBNumMap.end())
    return f->second;
  errs() << "MBB Num: = " << BBNum << "\n";
  llvm_unreachable("MBB number not in MBB to label map.");
}

static MBB_ID getMBBID(const MachineBasicBlock &MBB) {
  return MBB.getNumber(); // Use MBB number as a unique identifier
}

static MBB_ID getMBBID(const MachineOperand &Op) {
  return getMBBID(*Op.getMBB());
}

// Add OpLabels and terminators. Fix any instructions with MBB references.
bool SPIRVBlockLabeler::runOnMachineFunction(MachineFunction &MF) {
  const SPIRVInstrInfo *TII =
      static_cast<const SPIRVInstrInfo *>(MF.getSubtarget().getInstrInfo());

  std::map<MBB_ID, Register> BBNumToLabelMap;
  SmallVector<MachineInstr *, 4> Branches, CondBranches, Phis;

  for (MachineBasicBlock &MBB : MF) {
    // Add the missing OpLabel in the right place
    auto LabelID = buildLabel(MBB, TII);
    BBNumToLabelMap.insert({getMBBID(MBB), LabelID});
    // Record all instructions that need to refer to a MBB label ID
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpBranch) {
        Branches.push_back(&MI);
      } else if (MI.getOpcode() == SPIRV::OpBranchConditional) {
        assert(MI.getNumOperands() == 3 &&
               "3 operands are expected in conditional branch");
        CondBranches.push_back(&MI);
      } else if (MI.getOpcode() == SPIRV::OpPhi) {
        Phis.push_back(&MI);
      }
    }
    assert((MBB.getLastNonDebugInstr()->isTerminator() || !MBB.getNextNode()) &&
           "Missed terminator in MBB");
  }

  // Replace MBB references with label IDs in OpBranch instructions
  for (const auto &MI : Branches) {
    DebugLoc DL = MI->getDebugLoc();
    auto BBNum = getMBBID(MI->getOperand(0));
    Register BBID = getLabelIDForMBB(BBNumToLabelMap, BBNum);
    BuildMI(*MI->getParent(), MI, DL, TII->get(SPIRV::OpBranch)).addUse(BBID);
    MI->eraseFromParent();
  }

  // Replace MBB references with label IDs in OpBranchConditional instructions
  for (const auto &MI : CondBranches) {
    DebugLoc DL = MI->getDebugLoc();
    auto BBNum0 = getMBBID(MI->getOperand(1));
    auto BBNum1 = getMBBID(MI->getOperand(2));
    BuildMI(*MI->getParent(), MI, DL, TII->get(SPIRV::OpBranchConditional))
        .addUse(MI->getOperand(0).getReg())
        .addUse(getLabelIDForMBB(BBNumToLabelMap, BBNum0))
        .addUse(getLabelIDForMBB(BBNumToLabelMap, BBNum1));
    MI->eraseFromParent();
  }

  // Replace MBB references with label IDs in OpPhi instructions
  for (const auto &MI : Phis) {
    DebugLoc DL = MI->getDebugLoc();
    auto MIB = BuildMI(*MI->getParent(), MI, DL, TII->get(SPIRV::OpPhi))
                   .addDef(MI->getOperand(0).getReg())
                   .addUse(MI->getOperand(1).getReg());

    // PHIs require def, type, and list of (val, MBB) pairs
    const unsigned NumOps = MI->getNumOperands();
    assert(((NumOps % 2) == 0) && "Require even num operands for PHI instrs");
    for (unsigned i = 2; i < NumOps; i += 2) {
      MIB.addUse(MI->getOperand(i).getReg());
      auto BBNum = getMBBID(MI->getOperand(i + 1));
      MIB.addUse(getLabelIDForMBB(BBNumToLabelMap, BBNum));
    }
    MI->eraseFromParent();
  }

  // Add OpFunctionEnd at the end of the last MBB
  MachineInstr &MI = *MF.back().getLastNonDebugInstr();
  DebugLoc DL = MI.getDebugLoc();
  BuildMI(&MF.back(), DL, TII->get(SPIRV::OpFunctionEnd));
  return true;
}

INITIALIZE_PASS(SPIRVBlockLabeler, DEBUG_TYPE, "SPIRV label blocks", false,
                false)

char SPIRVBlockLabeler::ID = 0;

FunctionPass *llvm::createSPIRVBlockLabelerPass() {
  return new SPIRVBlockLabeler();
}
