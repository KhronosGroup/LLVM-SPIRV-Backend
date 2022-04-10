//===-- SPIRVDuplicatesTracker.h - SPIR-V Duplicates Tracker --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// General infrastructure for keeping track of the values that according to
// the SPIR-V binary layout should be global to the whole module.
// Actual hoisting happens in SPIRVGlobalTypesAndRegNum pass.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVDUPLICATESTRACKER_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVDUPLICATESTRACKER_H

#include "MCTargetDesc/SPIRVBaseInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/ADT/DenseMap.h"

namespace llvm {

class DTSortableEntry : public DenseMap<MachineFunction *, Register> {
  std::vector<DTSortableEntry *> Deps;

  struct FlagsTy {
    unsigned IsFunc : 1;
    unsigned IsGV : 1;
    // NOTE: bit-field default init is a C++20 feature
    FlagsTy() : IsFunc(0), IsGV(0) {}
  };
  FlagsTy Flags;

public:
  // common hoisting utility doesn't support
  // function because their hoisting require hoisting of params as well
  bool getIsFunc() const { return Flags.IsFunc; }
  bool getIsGV() const { return Flags.IsGV; }
  void setIsFunc(bool V) { Flags.IsFunc = V; }
  void setIsGV(bool V) { Flags.IsGV = V; }

  std::vector<DTSortableEntry *> &getDeps() { return Deps; }
};

template <typename T> class SPIRVDuplicatesTracker {

  using StorageKeyTy = const T *;
  // TODO: consider replacing MapVector with DenseMap which
  // requires topological sorting of the storage contents
  // (e.g. to handle dependencies of const composites to its
  // elements)
  // Currently we rely on the order of insertion to be correct
  using StorageTy = DenseMap<StorageKeyTy, DTSortableEntry>;

  StorageTy Storage;

public:
  SPIRVDuplicatesTracker() {}

  void add(const T *V, MachineFunction *MF, Register R) {
    Register OldReg;
    if (find(V, MF, OldReg)) {
      assert(OldReg == R);
      return;
    }
    Storage[V][MF] = R;
    if (std::is_same<Function, T>())
      Storage[V].setIsFunc(true);
    if (std::is_same<GlobalVariable, T>())
      Storage[V].setIsGV(true);
  }

  bool find(const T *V, MachineFunction *MF, Register& R) {
    auto iter = Storage.find(V);
    if (iter != Storage.end()) {
      auto Map = iter->second;
      auto iter2 = Map.find(MF);
      if (iter2 != Map.end()) {
        R = iter2->second;
        return true;
      }
    }
    return false;
  }

  const StorageTy &getAllUses() const { return Storage; }

private:
  StorageTy &getAllUses() { return Storage; }

  // The frined class needs to have access to the internal storage
  // to be able to build dependency graph, can't declare only one
  // function a 'friend' due to the incomplete declaration at this point
  // and mutual dependency problems.
  friend class SPIRVGeneralDuplicatesTracker;
};

using SPIRVTypeTracker = SPIRVDuplicatesTracker<Type>;
using SPIRVConstantTracker = SPIRVDuplicatesTracker<Constant>;
using SPIRVGlobalVariableTracker = SPIRVDuplicatesTracker<GlobalVariable>;
using SPIRVFuncDeclsTracker = SPIRVDuplicatesTracker<Function>;

class SPIRVGeneralDuplicatesTracker {
  SPIRVTypeTracker TT;
  SPIRVConstantTracker CT;
  SPIRVGlobalVariableTracker GT;
  SPIRVFuncDeclsTracker FT;

  // NOTE: using MOs instead of regs to get rid of MF dependency
  // to be able to use flat data structure
  // NOTE: replacing DenseMap with MapVector doesn't affect overall
  // correctness but makes LITs more stable, should prefer DenseMap still
  // due to significant perf difference
  using Reg2EntryTy = DenseMap<MachineOperand *, DTSortableEntry *>;

private:
  template <typename T>
  void prebuildReg2Entry(SPIRVDuplicatesTracker<T> &DT,
                         Reg2EntryTy &Reg2Entry) {
    for (auto &TPair : DT.getAllUses()) {
      for (auto &RegPair : TPair.second) {
        auto *MF = RegPair.first;
        auto R = RegPair.second;
        auto *MI = MF->getRegInfo().getVRegDef(R);
        if (!MI)
          continue;
        Reg2Entry[&MI->getOperand(0)] = &TPair.second;
      }
    }
  }

public:
  void buildDepsGraph(std::vector<DTSortableEntry *> &Graph) {
    Reg2EntryTy Reg2Entry;
    prebuildReg2Entry(TT, Reg2Entry);
    prebuildReg2Entry(CT, Reg2Entry);
    prebuildReg2Entry(GT, Reg2Entry);
    prebuildReg2Entry(FT, Reg2Entry);

    for (auto &Op2E : Reg2Entry) {
      auto *E = Op2E.second;
      Graph.push_back(E);
      for (auto &U : *E) {
        auto *MF = U.first;
        auto R = U.second;

        auto *MI = MF->getRegInfo().getVRegDef(R);
        if (!MI)
          continue;
        assert(MI && MI->getParent() && "No MachineInstr created yet");
        for (auto i = MI->getNumDefs(); i < MI->getNumOperands(); i++) {
          auto Op = MI->getOperand(i);
          if (!Op.isReg())
            continue;
          auto &RegOp = MF->getRegInfo().getVRegDef(Op.getReg())->getOperand(0);
          assert(Reg2Entry.count(&RegOp));
          E->getDeps().push_back(Reg2Entry[&RegOp]);
        }
      }
    }
  }

  void add(const Type *T, MachineFunction *MF, Register R) { TT.add(T, MF, R); }

  void add(const Constant *C, MachineFunction *MF, Register R) {
    CT.add(C, MF, R);
  }

  void add(const GlobalVariable *GV, MachineFunction *MF, Register R) {
    GT.add(GV, MF, R);
  }

  void add(const Function *F, MachineFunction *MF, Register R) {
    FT.add(F, MF, R);
  }

  bool find(const Type *T, MachineFunction *MF, Register& R) {
    return TT.find(T, MF, R);
  }

  bool find(const Constant *C, MachineFunction *MF, Register& R) {
    return CT.find(C, MF, R);
  }

  bool find(const GlobalVariable *GV, MachineFunction *MF, Register &R) {
    return GT.find(GV, MF, R);
  }

  bool find(const Function *F, MachineFunction *MF, Register& R) {
    return FT.find(F, MF, R);
  }

  // NOTE:
  // T *Arg = nullptr is added as compiler fails to infer T for specializations
  // based just on templated return type
  template <typename T> const SPIRVDuplicatesTracker<T> *get(T *Arg = nullptr);
};

} // end namespace llvm
#endif
