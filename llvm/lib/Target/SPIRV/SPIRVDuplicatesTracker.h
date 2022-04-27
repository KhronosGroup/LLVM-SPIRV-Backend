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
#include "MCTargetDesc/SPIRVMCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"

#include <type_traits>

namespace llvm {

// NOTE: using MapVector instead of DenseMap because it helps getting
// everything ordered in a stable manner for a price of extra (NumKeys)*PtrSize
// memory and expensive removals which do not happen anyway
class DTSortableEntry : public MapVector<const MachineFunction *, Register> {
  SmallVector<DTSortableEntry *, 2> Deps;

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

  SmallVector<DTSortableEntry *, 2> &getDeps() { return Deps; }
};

struct SpecialTypeDescriptor {
  enum SpecialTypeKind {
    STK_Empty = 0,
    STK_Image,
    STK_SampledImage,
    STK_Sampler,
    STK_Pipe,
    STK_Last = -1
  };
  SpecialTypeKind Kind;

  unsigned Hash;
  
  SpecialTypeDescriptor() = delete;
  SpecialTypeDescriptor(SpecialTypeKind K): Kind(K) { Hash = Kind; }

  unsigned getHash() const {
    return Hash;
  }

  virtual ~SpecialTypeDescriptor() {}
};

struct ImageTypeDescriptor: public SpecialTypeDescriptor {
  union ImageAttrs {
    struct BitFlags {
      unsigned Dim : 3;
      unsigned Depth : 2;
      unsigned Arrayed : 1;
      unsigned MS : 1;
      unsigned Sampled : 2;
      unsigned ImageFormat : 6;
      unsigned AQ : 2;
    } Flags;
    unsigned Val;
  };

  ImageTypeDescriptor(const Type *SampledTy, unsigned Dim, unsigned Depth,
                      unsigned Arrayed, unsigned MS, unsigned Sampled,
                      unsigned ImageFormat, unsigned AQ = 0)
      : SpecialTypeDescriptor(SpecialTypeKind::STK_Image) {
    ImageAttrs Attrs;
    Attrs.Val = 0;
    Attrs.Flags.Dim = Dim;
    Attrs.Flags.Depth = Depth;
    Attrs.Flags.Arrayed = Arrayed;
    Attrs.Flags.MS = MS;
    Attrs.Flags.Sampled = Sampled;
    Attrs.Flags.ImageFormat = ImageFormat;
    Attrs.Flags.AQ = AQ;
    Hash = (DenseMapInfo<Type*>().getHashValue(SampledTy) & 0xffff) ^ ((Attrs.Val << 8) | Kind);
  }

  static bool classof(const SpecialTypeDescriptor *TD) {
    return TD->Kind == SpecialTypeKind::STK_Image;
  }
};

struct SampledImageTypeDescriptor: public SpecialTypeDescriptor {
  SampledImageTypeDescriptor(const Type *SampledTy, const MachineInstr *ImageTy)
      : SpecialTypeDescriptor(SpecialTypeKind::STK_SampledImage) {
    assert(ImageTy->getOpcode() == SPIRV::OpTypeImage);
    ImageTypeDescriptor TD(
        SampledTy, ImageTy->getOperand(2).getImm(),
        ImageTy->getOperand(3).getImm(), ImageTy->getOperand(4).getImm(),
        ImageTy->getOperand(5).getImm(), ImageTy->getOperand(6).getImm(),
        ImageTy->getOperand(7).getImm(), ImageTy->getOperand(8).getImm());
    Hash = TD.getHash() ^ Kind;
  }

  static bool classof(const SpecialTypeDescriptor *TD) {
    return TD->Kind == SpecialTypeKind::STK_SampledImage;
  }
};

struct SamplerTypeDescriptor: public SpecialTypeDescriptor {
  SamplerTypeDescriptor(): SpecialTypeDescriptor(SpecialTypeKind::STK_Sampler) {
    Hash = Kind;
  }

  static bool classof(const SpecialTypeDescriptor *TD) {
    return TD->Kind == SpecialTypeKind::STK_Sampler;
  }
};
struct PipeTypeDescriptor: public SpecialTypeDescriptor {

  PipeTypeDescriptor(uint8_t AQ): SpecialTypeDescriptor(SpecialTypeKind::STK_Pipe) {
    Hash = (AQ << 8) | Kind;
  }

  static bool classof(const SpecialTypeDescriptor *TD) {
    return TD->Kind == SpecialTypeKind::STK_Pipe;
  }
};

template <> struct DenseMapInfo<SpecialTypeDescriptor> {
  static inline SpecialTypeDescriptor getEmptyKey() {
    return SpecialTypeDescriptor(SpecialTypeDescriptor::STK_Empty);
  }
  static inline SpecialTypeDescriptor getTombstoneKey() {
    return SpecialTypeDescriptor(SpecialTypeDescriptor::STK_Last);
  }
  static unsigned getHashValue(SpecialTypeDescriptor Val) {
    return Val.getHash();
  }
  static bool isEqual(SpecialTypeDescriptor LHS, SpecialTypeDescriptor RHS) {
    return getHashValue(LHS) == getHashValue(RHS);
  }
};


template <typename KeyTy> class SPIRVDuplicatesTrackerBase {
public:
  // NOTE: using MapVector instead of DenseMap helps getting
  // everything ordered in a stable manner for a price of extra
  // (NumKeys)*PtrSize memory and expensive removals which don't happen anyway
  using StorageTy = MapVector<KeyTy, DTSortableEntry>;

private:
  StorageTy Storage;

public:
  SPIRVDuplicatesTrackerBase() {}

  void add(KeyTy V, const MachineFunction *MF, Register R) {
    Register OldReg;
    if (find(V, MF, OldReg)) {
      assert(OldReg == R);
      return;
    }
    Storage[V][MF] = R;
    if (std::is_same<Function,
                     typename std::remove_const<
                         typename std::remove_pointer<KeyTy>::type>::type>())
      Storage[V].setIsFunc(true);
    if (std::is_same<GlobalVariable,
                     typename std::remove_const<
                         typename std::remove_pointer<KeyTy>::type>::type>())
      Storage[V].setIsGV(true);
  }

  bool find(KeyTy V, const MachineFunction *MF, Register& R) const {
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

  // The friend class needs to have access to the internal storage
  // to be able to build dependency graph, can't declare only one
  // function a 'friend' due to the incomplete declaration at this point
  // and mutual dependency problems.
  friend class SPIRVGeneralDuplicatesTracker;
};

template<typename T>
class SPIRVDuplicatesTracker: public SPIRVDuplicatesTrackerBase<const T*> {};

template<>
class SPIRVDuplicatesTracker<SpecialTypeDescriptor>: public SPIRVDuplicatesTrackerBase<SpecialTypeDescriptor> {};

using SPIRVTypeTracker = SPIRVDuplicatesTracker<Type>;
using SPIRVConstantTracker = SPIRVDuplicatesTracker<Constant>;
using SPIRVGlobalVariableTracker = SPIRVDuplicatesTracker<GlobalVariable>;
using SPIRVFuncDeclsTracker = SPIRVDuplicatesTracker<Function>;
using SPIRVSpecialDeclsTracker = SPIRVDuplicatesTracker<SpecialTypeDescriptor>;

class SPIRVGeneralDuplicatesTracker {
  SPIRVTypeTracker TT;
  SPIRVConstantTracker CT;
  SPIRVGlobalVariableTracker GT;
  SPIRVFuncDeclsTracker FT;
  SPIRVSpecialDeclsTracker ST;

  // NOTE: using MOs instead of regs to get rid of MF dependency
  // to be able to use flat data structure
  // NOTE: replacing DenseMap with MapVector doesn't affect overall
  // correctness but makes LITs more stable, should prefer DenseMap still
  // due to significant perf difference
  using Reg2EntryTy = MapVector<MachineOperand *, DTSortableEntry *>;

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
    prebuildReg2Entry(ST, Reg2Entry);

    for (auto &Op2E : Reg2Entry) {
      auto *E = Op2E.second;
      Graph.push_back(E);
      for (auto &U : *E) {
        auto *MF = U.first;
        auto R = U.second;

        auto *MI = MF->getRegInfo().getUniqueVRegDef(R);
        if (!MI)
          continue;
        assert(MI && MI->getParent() && "No MachineInstr created yet");
        for (auto i = MI->getNumDefs(); i < MI->getNumOperands(); i++) {
          auto Op = MI->getOperand(i);
          if (!Op.isReg())
            continue;
          MachineOperand *RegOp = &MF->getRegInfo().getVRegDef(Op.getReg())->getOperand(0);
          assert((MI->getOpcode() == SPIRV::OpVariable && i == 3) || Reg2Entry.count(RegOp));
          if (Reg2Entry.count(RegOp))
            E->getDeps().push_back(Reg2Entry[RegOp]);
        }
      }
    }
  }

  void add(const Type *T, const MachineFunction *MF, Register R) { TT.add(T, MF, R); }

  void add(const Constant *C, const MachineFunction *MF, Register R) {
    CT.add(C, MF, R);
  }

  void add(const GlobalVariable *GV, const MachineFunction *MF, Register R) {
    GT.add(GV, MF, R);
  }

  void add(const Function *F, const MachineFunction *MF, Register R) {
    FT.add(F, MF, R);
  }

  void add(const SpecialTypeDescriptor &TD, const MachineFunction *MF, Register R) {
    ST.add(TD, MF, R);
  }

  bool find(const Type *T, const MachineFunction *MF, Register& R) {
    return TT.find(const_cast<Type*>(T), MF, R);
  }

  bool find(const Constant *C, const MachineFunction *MF, Register& R) {
    return CT.find(const_cast<Constant*>(C), MF, R);
  }

  bool find(const GlobalVariable *GV, const MachineFunction *MF, Register &R) {
    return GT.find(const_cast<GlobalVariable*>(GV), MF, R);
  }

  bool find(const Function *F, const MachineFunction *MF, Register& R) {
    return FT.find(const_cast<Function*>(F), MF, R);
  }

  bool find(const SpecialTypeDescriptor &TD, const MachineFunction *MF, Register& R) {
    return ST.find(TD, MF, R);
  }


  // NOTE:
  // T *Arg = nullptr is added as compiler fails to infer T for specializations
  // based just on templated return type
  template <typename T> const SPIRVDuplicatesTracker<T> *get(T *Arg = nullptr);
};

} // end namespace llvm
#endif
