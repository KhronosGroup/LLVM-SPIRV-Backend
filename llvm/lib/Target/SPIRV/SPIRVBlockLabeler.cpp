//===-- SPIRVBlockLabeler.cpp - SPIR-V Block Labeling Pass Impl -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implementation of SPIRVBlockLabeler, which ensures all basic blocks
// start with an OpLabel, and ends with a suitable terminator (OpBranch is
// inserted if necessary).
//
// All MBB literals in OpBranchConditional, OpBranch, and OpPhi, are also fixed
// to use the virtual registers defined by OpLabel.
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

// Initialize MIRBuilder with the instruction position for an OpLabel within
// MBB. At this stage, all hoistable global instrs will still be function-local,
// so we must skip past them plus the initial OpFunction and its parameters.
static void setLabelPos(MachineBasicBlock &MBB, MachineIRBuilder &MIRBuilder) {
  const auto TII = static_cast<const SPIRVInstrInfo *>(&MIRBuilder.getTII());
  MIRBuilder.setMBB(MBB);
  for (auto &I : MBB) {
    unsigned o = I.getOpcode();
    if (o == SPIRV::OpFunction || o == SPIRV::OpFunctionParameter ||
        TII->isHeaderInstr(I)) {
      continue;
    }
    MIRBuilder.setInstr(I);
    break;
  }
}

// Build an OpLabel at the start of the block (ignoring hoistable instrs)
static Register buildLabel(MachineBasicBlock &MBB,
                           MachineIRBuilder &MIRBuilder) {
  setLabelPos(MBB, MIRBuilder);
  auto LabelID = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  MIRBuilder.buildInstr(SPIRV::OpLabel).addDef(LabelID);
  buildOpName(LabelID, MBB.getName(), MIRBuilder);
  return LabelID;
}

// Remove one instruction, and insert another in its place
static void replaceInstr(MachineInstr *ToReplace, MachineInstrBuilder &NewInstr,
                         MachineIRBuilder &MIRBuilder) {
  MIRBuilder.setMBB(*ToReplace->getParent());
  MIRBuilder.setInstr(*ToReplace);
  MIRBuilder.insertInstr(NewInstr);
  ToReplace->removeFromParent();
}

// A unique identifier for MBBs for use as a map key
using MBB_ID = int;

static Register getLabelIDForMBB(const std::map<MBB_ID, Register> &BBNumMap,
                                 MBB_ID BBNum) {
  auto f = BBNumMap.find(BBNum);
  if (f != BBNumMap.end()) {
    return f->second;
  } else {
    errs() << "MBB Num: = " << BBNum << "\n";
    llvm_unreachable("MBB number not in MBB to label map.");
  }
}

static MBB_ID getMBBID(const MachineBasicBlock &MBB) {
  return MBB.getNumber(); // Use MBB number as a unique identifier
}

static MBB_ID getMBBID(const MachineOperand &Op) {
  return getMBBID(*Op.getMBB());
}

// Add OpLabels and terminators. Fix any instructions with MBB references
bool SPIRVBlockLabeler::runOnMachineFunction(MachineFunction &MF) {
  MachineIRBuilder MIRBuilder;
  MIRBuilder.setMF(MF);

  std::map<MBB_ID, Register> BBNumToLabelMap;
  SmallVector<MachineInstr *, 4> Branches, CondBranches, Phis;

  for (MachineBasicBlock &MBB : MF) {
    // Add the missing OpLabel in the right place
    auto LabelID = buildLabel(MBB, MIRBuilder);
    BBNumToLabelMap.insert({getMBBID(MBB), LabelID});
    // Record all instructions that need to refer to a MBB label ID
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpBranch) {
        Branches.push_back(&MI);
      } else if (MI.getOpcode() == SPIRV::OpBranchConditional) {
        // If previous optimizations generated conditionals with implicit
        // fallthrough, add the missing explicit "else" to it valid SPIR-V
        if (MI.getNumOperands() < 3) {
          MI.addOperand(MachineOperand::CreateMBB(MBB.getNextNode()));
        }
        CondBranches.push_back(&MI);
      } else if (MI.getOpcode() == SPIRV::OpPhi) {
        Phis.push_back(&MI);
      }
    }

    // Add an unconditional branch if the block has no explicit terminator
    // and is not the last one.
    if (!MBB.getLastNonDebugInstr()->isTerminator() && MBB.getNextNode()) {
      MIRBuilder.setMBB(MBB); // Insert at end of block
      auto MIB =
          MIRBuilder.buildInstr(SPIRV::OpBranch).addMBB(MBB.getNextNode());
      Branches.push_back(MIB);
    }
  }

  // Replace MBB references with label IDs in OpBranch instructions
  for (const auto &Branch : Branches) {
    auto BBNum = getMBBID(Branch->getOperand(0));
    Register BBID = getLabelIDForMBB(BBNumToLabelMap, BBNum);
    auto MIB = MIRBuilder.buildInstrNoInsert(SPIRV::OpBranch).addUse(BBID);
    replaceInstr(Branch, MIB, MIRBuilder);
  }

  // Replace MBB references with label IDs in OpBranchConditional instructions
  for (const auto &Branch : CondBranches) {
    auto BBNum0 = getMBBID(Branch->getOperand(1));
    auto BBNum1 = getMBBID(Branch->getOperand(2));
    auto MIB = MIRBuilder.buildInstrNoInsert(SPIRV::OpBranchConditional)
                   .addUse(Branch->getOperand(0).getReg())
                   .addUse(getLabelIDForMBB(BBNumToLabelMap, BBNum0))
                   .addUse(getLabelIDForMBB(BBNumToLabelMap, BBNum1));
    replaceInstr(Branch, MIB, MIRBuilder);
  }

  // Replace MBB references with label IDs in OpPhi instructions
  for (const auto &Phi : Phis) {
    auto MIB = MIRBuilder.buildInstrNoInsert(SPIRV::OpPhi)
                   .addDef(Phi->getOperand(0).getReg())
                   .addUse(Phi->getOperand(1).getReg());

    // PHIs require def, type, and list of (val, MBB) pairs
    const unsigned NumOps = Phi->getNumOperands();
    assert(((NumOps % 2) == 0) && "Require even num operands for PHI instrs");
    for (unsigned i = 2; i < NumOps; i += 2) {
      MIB.addUse(Phi->getOperand(i).getReg());
      auto BBNum = getMBBID(Phi->getOperand(i + 1));
      MIB.addUse(getLabelIDForMBB(BBNumToLabelMap, BBNum));
    }
    replaceInstr(Phi, MIB, MIRBuilder);
  }

  // Add OpFunctionEnd at the end of the last MBB
  MIRBuilder.setMBB(MF.back());
  MIRBuilder.buildInstr(SPIRV::OpFunctionEnd);
  return true;
}

INITIALIZE_PASS(SPIRVBlockLabeler, DEBUG_TYPE, "SPIRV label blocks", false,
                false)

char SPIRVBlockLabeler::ID = 0;

FunctionPass *llvm::createSPIRVBlockLabelerPass() {
  return new SPIRVBlockLabeler();
}
