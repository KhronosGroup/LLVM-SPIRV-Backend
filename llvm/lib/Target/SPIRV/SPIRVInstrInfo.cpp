//===-- SPIRVInstrInfo.cpp - SPIR-V Instruction Information ------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the SPIR-V implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "SPIRVInstrInfo.h"
#include "SPIRV.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>
#include <iterator>

#define GET_INSTRINFO_CTOR_DTOR
#include "SPIRVGenInstrInfo.inc"

using namespace llvm;
using namespace SPIRV;

SPIRVInstrInfo::SPIRVInstrInfo() : SPIRVGenInstrInfo() {}

bool SPIRVInstrInfo::isConstantInstr(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case OpConstantTrue:
  case OpConstantFalse:
  case OpConstant:
  case OpConstantComposite:
  case OpConstantSampler:
  case OpConstantNull:
  case OpSpecConstantTrue:
  case OpSpecConstantFalse:
  case OpSpecConstant:
  case OpSpecConstantComposite:
  case OpSpecConstantOp:
  case OpUndef:
    return true;
  default:
    return false;
  }
}

bool SPIRVInstrInfo::isTypeDeclInstr(const MachineInstr &MI) const {
  auto &MRI = MI.getMF()->getRegInfo();
  if (MI.getNumDefs() >= 1) {
    auto defRegClass = MRI.getRegClass(MI.getOperand(0).getReg());
    return defRegClass->getID() == TYPERegClass.getID();
  } else {
    return false;
  }
}

bool SPIRVInstrInfo::isDecorationInstr(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case OpDecorate:
  case OpDecorateId:
  case OpDecorateString:
  case OpMemberDecorate:
  case OpMemberDecorateString:
    return true;
  default:
    return false;
  }
}

bool SPIRVInstrInfo::isHeaderInstr(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case OpCapability:
  case OpExtension:
  case OpExtInstImport:
  case OpMemoryModel:
  case OpEntryPoint:
  case OpExecutionMode:
  case OpExecutionModeId:
  case OpString:
  case OpSourceExtension:
  case OpSource:
  case OpSourceContinued:
  case OpName:
  case OpMemberName:
  case OpModuleProcessed:
    return true;
  default:
    return isTypeDeclInstr(MI) || isConstantInstr(MI) || isDecorationInstr(MI);
  }
}

// Analyze the branching code at the end of MBB, returning
// true if it cannot be understood (e.g. it's a switch dispatch or isn't
// implemented for a target).  Upon success, this returns false and returns
// with the following information in various cases:
//
// 1. If this block ends with no branches (it just falls through to its succ)
//    just return false, leaving TBB/FBB null.
// 2. If this block ends with only an unconditional branch, it sets TBB to be
//    the destination block.
// 3. If this block ends with a conditional branch and it falls through to a
//    successor block, it sets TBB to be the branch destination block and a
//    list of operands that evaluate the condition. These operands can be
//    passed to other TargetInstrInfo methods to create new branches.
// 4. If this block ends with a conditional branch followed by an
//    unconditional branch, it returns the 'true' destination in TBB, the
//    'false' destination in FBB, and a list of operands that evaluate the
//    condition.  These operands can be passed to other TargetInstrInfo
//    methods to create new branches.
//
// Note that removeBranch and insertBranch must be implemented to support
// cases where this method returns success.
//
// If AllowModify is true, then this routine is allowed to modify the basic
// block (e.g. delete instructions after the unconditional branch).
//
// The CFG information in MBB.Predecessors and MBB.Successors must be valid
// before calling this function.
bool SPIRVInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TBB,
                                   MachineBasicBlock *&FBB,
                                   SmallVectorImpl<MachineOperand> &Cond,
                                   bool AllowModify) const {
  TBB = nullptr;
  FBB = nullptr;
  if (MBB.empty())
    return false;
  auto MI = MBB.getLastNonDebugInstr();
  if (!MI.isValid())
    return false;
  if (MI->getOpcode() == SPIRV::OpBranch) {
    TBB = MI->getOperand(0).getMBB();
    return false;
  } else if (MI->getOpcode() == SPIRV::OpBranchConditional) {
    Cond.push_back(MI->getOperand(0));
    TBB = MI->getOperand(1).getMBB();
    if (MI->getNumOperands() == 3) {
      FBB = MI->getOperand(2).getMBB();
    }
    return false;
  } else {
    return true;
  }
}

// Remove the branching code at the end of the specific MBB.
// This is only invoked in cases where analyzeBranch returns success. It
// returns the number of instructions that were removed.
// If \p BytesRemoved is non-null, report the change in code size from the
// removed instructions.
unsigned SPIRVInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                      int *BytesRemoved) const {

  llvm_unreachable("Branch removal not supported, as MBB info not propagated "
                   "to OpPhi instructions. Try using -O0 instead.");

  using namespace SPIRV;
  auto MI = MBB.getLastNonDebugInstr();
  if (MI.isValid() &&
      (MI->getOpcode() == OpBranch || MI->getOpcode() == OpBranchConditional)) {
    MI->eraseFromParent();
    if (BytesRemoved)
      *BytesRemoved = MI->getOpcode() == OpBranch ? 8 : 16;
    return 1;
  }
  if (BytesRemoved)
    *BytesRemoved = 0;
  return 0;
}

// Insert branch code into the end of the specified MachineBasicBlock. The
// operands to this method are the same as those returned by analyzeBranch.
// This is only invoked in cases where analyzeBranch returns success. It
// returns the number of instructions inserted. If \p BytesAdded is non-null,
// report the change in code size from the added instructions.
//
// It is also invoked by tail merging to add unconditional branches in
// cases where analyzeBranch doesn't apply because there was no original
// branch to analyze.  At least this much must be implemented, else tail
// merging needs to be disabled.
//
// The CFG information in MBB.Predecessors and MBB.Successors must be valid
// before calling this function.
unsigned SPIRVInstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {

  llvm_unreachable("Branch insertion not supported, as MBB info not propagated "
                   "to OpPhi instructions. Try using -O0 instead.");

  MachineIRBuilder MIRBuilder;
  MIRBuilder.setMF(*MBB.getParent());
  MIRBuilder.setMBB(MBB);

  int bytesAdded = 0;
  int instsAdded = 0;
  if (Cond.size() > 0) { // Conditional branch (but may only have true MBB)
    assert(TBB != nullptr && "Require non-null block for conditional branch");
    assert(Cond.size() == 1 && "Require 1 condition for insertBranch");
    auto falseBlock = FBB; //? FBB : MBB.getFallThrough();
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpBranchConditional)
                   .addUse(Cond[0].getReg())
                   .addMBB(TBB);
    if (falseBlock) {
      MIB.addMBB(falseBlock);
    }
    bytesAdded = 16;
    instsAdded = 1;
  } else { // Either add unconditional branch, or add to last conditional branch
    assert(!FBB && "False block set with no conditions");
    if (TBB) {
      if (MBB.empty()) {
        MIRBuilder.buildInstr(SPIRV::OpBranch).addMBB(TBB);
        bytesAdded = 8;
      } else {
        auto MI = MBB.getLastNonDebugInstr();
        if (MI.isValid() && !MI->isKnownSentinel() &&
            MI->getOpcode() == SPIRV::OpBranchConditional) {
          if (MI->getNumOperands() == 3) {
            MI->getOperand(2).setMBB(TBB);
          } else {
            MI->addOperand(MachineOperand::CreateMBB(TBB));
          }
        } else {
          MIRBuilder.buildInstr(SPIRV::OpBranch).addMBB(TBB);
          bytesAdded = 8;
        }
      }
      instsAdded = 1;
    }
  }
  if (BytesAdded)
    *BytesAdded = bytesAdded;
  return instsAdded;
}

