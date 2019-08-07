//===-- SPIRVBlockLabeler.cpp - SPIR-V Block Labeling Pass Impl -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implementation of SPIRVBlockLabeler, which ensures ensures all basic blocks
// start with an OpLabel, and ends with a suitable terminator (OpBranch is
// inserted if necessary).
//
// All MBB literals in OpBranchConditional, OpBranch, and OpPhi, are also fixed
// to use the virtual registers defined by OpLabel.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVRegisterInfo.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassSupport.h"

#include "SPIRVInstrInfo.h"
#include "SPIRVStrings.h"

using namespace llvm;
using namespace SPIRV;

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
    if (o == OpFunction || o == OpFunctionParameter || TII->isHeaderInstr(I)) {
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
  auto labelID = MIRBuilder.getMRI()->createVirtualRegister(&IDRegClass);
  MIRBuilder.buildInstr(OpLabel).addDef(labelID);
  buildOpName(labelID, MBB.getName(), MIRBuilder);
  return labelID;
}

// Remove one instruction, and insert another in its place
static void replaceInstr(MachineInstr *toReplace, MachineInstrBuilder &newInstr,
                         MachineIRBuilder &MIRBuilder) {
  MIRBuilder.setMBB(*toReplace->getParent());
  MIRBuilder.setInstr(*toReplace);
  MIRBuilder.insertInstr(newInstr);
  toReplace->removeFromParent();
}

// A unique identifier for MBBs for use as a map key
using MBB_ID = int;

static Register getLabelIDForMBB(const std::map<MBB_ID, Register> &bbNumMap,
                                 MBB_ID bbNum) {
  auto f = bbNumMap.find(bbNum);
  if (f != bbNumMap.end()) {
    return f->second;
  } else {
    errs() << "MBB Num: = " << bbNum << "\n";
    llvm_unreachable("MBB number not in MBB to label map.");
  }
}

static MBB_ID getMBBID(const MachineBasicBlock &MBB) {
  return MBB.getNumber(); // Use MBB number as a unique identifier
}

static MBB_ID getMBBID(const MachineOperand &op) {
  return getMBBID(*op.getMBB());
}

// Add OpLabels and terminators. Fix any instructions with MBB references
bool SPIRVBlockLabeler::runOnMachineFunction(MachineFunction &MF) {
  MachineIRBuilder MIRBuilder;
  MIRBuilder.setMF(MF);

  std::map<MBB_ID, Register> bbNumToLabelMap;
  SmallVector<MachineInstr *, 4> branches, condBranches, phis;

  for (MachineBasicBlock &MBB : MF) {
    // Add the missing OpLabel in the right place
    auto labelID = buildLabel(MBB, MIRBuilder);
    bbNumToLabelMap.insert({getMBBID(MBB), labelID});
    // Record all instructions that need to refer to a MBB label ID
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == OpBranch) {
        branches.push_back(&MI);
      } else if (MI.getOpcode() == OpBranchConditional) {
        // If previous optimizations generated conditionals with implicit
        // fallthrough, add the missing explicit "else" to it valid SPIR-V
        if (MI.getNumOperands() < 3) {
          MI.addOperand(MachineOperand::CreateMBB(MBB.getNextNode()));
        }
        condBranches.push_back(&MI);
      } else if (MI.getOpcode() == OpPhi) {
        phis.push_back(&MI);
      }
    }

    // Add an unconditional branch if the block has no explicit terminator
    if (!MBB.getLastNonDebugInstr()->isTerminator()) {
      MIRBuilder.setMBB(MBB); // Insert at end of block
      auto MIB = MIRBuilder.buildInstr(OpBranch).addMBB(MBB.getNextNode());
      branches.push_back(MIB);
    }
  }

  // Replace MBB references with label IDs in OpBranch instructions
  for (const auto &branch : branches) {
    auto bbNum = getMBBID(branch->getOperand(0));
    Register bbID = getLabelIDForMBB(bbNumToLabelMap, bbNum);
    auto MIB = MIRBuilder.buildInstrNoInsert(OpBranch).addUse(bbID);
    replaceInstr(branch, MIB, MIRBuilder);
  }

  // Replace MBB references with label IDs in OpBranchConditional instructions
  for (const auto &branch : condBranches) {
    auto bbNum0 = getMBBID(branch->getOperand(1));
    auto bbNum1 = getMBBID(branch->getOperand(2));
    auto MIB = MIRBuilder.buildInstrNoInsert(OpBranchConditional)
                   .addUse(branch->getOperand(0).getReg())
                   .addUse(getLabelIDForMBB(bbNumToLabelMap, bbNum0))
                   .addUse(getLabelIDForMBB(bbNumToLabelMap, bbNum1));
    replaceInstr(branch, MIB, MIRBuilder);
  }

  // Replace MBB references with label IDs in OpPhi instructions
  for (const auto &phi : phis) {
    auto MIB = MIRBuilder.buildInstrNoInsert(OpPhi)
                   .addDef(phi->getOperand(0).getReg())
                   .addUse(phi->getOperand(1).getReg());

    // PHIs require def, type, and list of (val, MBB) pairs
    const unsigned numOps = phi->getNumOperands();
    assert(((numOps % 2) == 0) && "Require even num operands for PHI instrs");
    for (unsigned i = 2; i < numOps; i += 2) {
      MIB.addUse(phi->getOperand(i).getReg());
      auto bbNum = getMBBID(phi->getOperand(i + 1));
      MIB.addUse(getLabelIDForMBB(bbNumToLabelMap, bbNum));
    }
    replaceInstr(phi, MIB, MIRBuilder);
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
