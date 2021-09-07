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

#include "SPIRVEnums.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"

#include <map>

namespace llvm {

template <typename T> class SPIRVDuplicatesTracker {
  using StorageKeyTy = const T *;
  using StorageValueTy = Register;
  using StorageTy =
      MapVector<StorageKeyTy, DenseMap<MachineFunction *, StorageValueTy>>;

protected:
  StorageTy Storage;

public:
  SPIRVDuplicatesTracker() {}

  void add(const T *V, MachineFunction *MF, Register R) {
    assert (find(V, MF, R) == false && "DT: record already exists");
    Storage[V][MF] = R;
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
};

using SPIRVTypeTracker = SPIRVDuplicatesTracker<Type>;
using SPIRVConstantTracker = SPIRVDuplicatesTracker<Constant>;
using SPIRVGlobalValueTracker = SPIRVDuplicatesTracker<GlobalValue>;
using SPIRVFuncDeclsTracker = SPIRVDuplicatesTracker<Function>;

class SPIRVGeneralDuplicatesTracker {
  SPIRVTypeTracker TT;
  SPIRVConstantTracker CT;
  SPIRVGlobalValueTracker GT;
  SPIRVFuncDeclsTracker FT;

public:
  void add(const Type *T, MachineFunction *MF, Register R) { TT.add(T, MF, R); }

  void add(const Constant *C, MachineFunction *MF, Register R) {
    CT.add(C, MF, R);
  }

  void add(const GlobalValue *GV, MachineFunction *MF, Register R) {
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

  bool find(const GlobalValue *GV, MachineFunction *MF, Register R) {
    return GT.find(GV, MF, R);
  }

  bool find(const Function *F, MachineFunction *MF, Register R) {
    return FT.find(F, MF, R);
  }

  // NOTE:
  // T *Arg = nullptr is added as compiler fails to infer T for specializations
  // based just on templated return type
  template <typename T> const SPIRVDuplicatesTracker<T> *get(T *Arg = nullptr);
};

} // end namespace llvm
#endif
