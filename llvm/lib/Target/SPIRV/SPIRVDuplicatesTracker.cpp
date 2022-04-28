//===-- SPIRVDuplicatesTracker.cpp - SPIR-V Duplicates Tracker --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// General infrastructure for keeping track of the values that according to
// the SPIR-V binary layout should be global to the whole module.
//
//===----------------------------------------------------------------------===//

#include "SPIRVDuplicatesTracker.h"

using namespace llvm;

template <typename T>
void SPIRVGeneralDuplicatesTracker::prebuildReg2Entry(
    SPIRVDuplicatesTracker<T> &DT, SPIRVReg2EntryTy &Reg2Entry) {
  for (auto &TPair : DT.getAllUses()) {
    for (auto &RegPair : TPair.second) {
      const MachineFunction *MF = RegPair.first;
      Register R = RegPair.second;
      MachineInstr *MI = MF->getRegInfo().getVRegDef(R);
      if (!MI)
        continue;
      Reg2Entry[&MI->getOperand(0)] = &TPair.second;
    }
  }
}

void SPIRVGeneralDuplicatesTracker::buildDepsGraph(
    std::vector<SPIRV::DTSortableEntry *> &Graph) {
  SPIRVReg2EntryTy Reg2Entry;
  prebuildReg2Entry(TT, Reg2Entry);
  prebuildReg2Entry(CT, Reg2Entry);
  prebuildReg2Entry(GT, Reg2Entry);
  prebuildReg2Entry(FT, Reg2Entry);
  prebuildReg2Entry(ST, Reg2Entry);

  for (auto &Op2E : Reg2Entry) {
    auto *E = Op2E.second;
    Graph.push_back(E);
    for (auto &U : *E) {
      const MachineRegisterInfo &MRI = U.first->getRegInfo();
      MachineInstr *MI = MRI.getUniqueVRegDef(U.second);
      if (!MI)
        continue;
      assert(MI && MI->getParent() && "No MachineInstr created yet");
      for (auto i = MI->getNumDefs(); i < MI->getNumOperands(); i++) {
        MachineOperand &Op = MI->getOperand(i);
        if (!Op.isReg())
          continue;
        MachineOperand *RegOp = &MRI.getVRegDef(Op.getReg())->getOperand(0);
        assert((MI->getOpcode() == SPIRV::OpVariable && i == 3) ||
               Reg2Entry.count(RegOp));
        if (Reg2Entry.count(RegOp))
          E->getDeps().push_back(Reg2Entry[RegOp]);
      }
    }
  }
}
