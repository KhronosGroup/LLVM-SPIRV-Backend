//===-- SPIRVIRTranslator.cpp - Override IRTranslator for SPIR-V *- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Overrides GlobalISel's IRTranslator to customize default lowering behavior.
// This is mostly to stop aggregate types like Structs from getting expanded
// into scalars, and to maintain type information required for SPIR-V using the
// SPIRVGlobalRegistry before it gets discarded as LLVM IR is lowered to GMIR.
//
//===----------------------------------------------------------------------===//

#include "SPIRVIRTranslator.h"
#include "SPIRV.h"
#include "SPIRVSubtarget.h"
#include "SPIRVUtils.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IntrinsicsSPIRV.h"
#include "llvm/Target/TargetIntrinsicInfo.h"

#include <vector>

using namespace llvm;

// static bool buildOpConstantNull(const Constant &C, Register Reg,
//                                 MachineIRBuilder &MIRBuilder,
//                                 SPIRVGlobalRegistry *GR) {
//   SPIRVType *type = GR->getOrCreateSPIRVType(C.getType(), MIRBuilder);
//   auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantNull)
//                  .addDef(Reg)
//                  .addUse(GR->getSPIRVTypeID(type));
//   return GR->constrainRegOperands(MIB);
// }

// static bool isConstantCompositeConstructible(const Constant &C) {
//   if (isa<ConstantDataSequential>(C)) {
//     return true;
//   } else if (const auto *CV = dyn_cast<ConstantAggregate>(&C)) {
//     return std::all_of(CV->op_begin(), CV->op_end(), [](const Use &operand) {
//       const auto *constOp = cast_or_null<Constant>(operand);
//       return constOp && (isa<ConstantData>(operand) ||
//                          isConstantCompositeConstructible(*constOp));
//     });
//   }
//   return false;
// }

// static bool buildOpConstantComposite(const Constant &C, Register Reg,
//                                      const SmallVectorImpl<Register> &Ops,
//                                      MachineIRBuilder &MIRBuilder,
//                                      SPIRVGlobalRegistry *GR) {
//   bool areAllConst = isConstantCompositeConstructible(C);
//   SPIRVType *type = GR->getOrCreateSPIRVType(C.getType(), MIRBuilder);
//   auto MIB = MIRBuilder
//                  .buildInstr(areAllConst ? SPIRV::OpConstantComposite
//                                          : SPIRV::OpCompositeConstruct)
//                  .addDef(Reg)
//                  .addUse(GR->getSPIRVTypeID(type));
//   for (const auto &Op : Ops) {
//     MIB.addUse(Op);
//   }
//   return GR->constrainRegOperands(MIB);
// }

// bool SPIRVIRTranslator::translate(const Instruction &Inst) {
//   CurBuilder->setDebugLoc(Inst.getDebugLoc());
//   // We only emit constants into the entry block from here. To prevent jumpy
//   // debug behaviour set the line to 0.
//   if (const DebugLoc &DL = Inst.getDebugLoc())
//     EntryBuilder->setDebugLoc(
//         DebugLoc::get(0, 0, DL.getScope(), DL.getInlinedAtScope()));
//   else
//     EntryBuilder->setDebugLoc(DebugLoc());

//   if (translateInst(Inst, Inst.getOpcode()))
//     return true;

//   return IRTranslator::translate(Inst);
// }

// bool SPIRVIRTranslator::translate(const Constant &C, Register Reg) {
//   if (auto GV = dyn_cast<GlobalValue>(&C)) {
//     // Global OpVariables (possibly with constant initializers)
//     return buildGlobalValue(Reg, GV, *EntryBuilder);
//   } else if (auto CE = dyn_cast<ConstantExpr>(&C)) {
//     if (translateInst(*CE, CE->getOpcode()))
//       return true;
//     return IRTranslator::translate(C, Reg);
//   }

//   bool Res = false;
//   if (isa<ConstantPointerNull>(C) || isa<ConstantAggregateZero>(C)) {
//     // Null (needs handled here to not loose the type info)
//     Res = buildOpConstantNull(C, Reg, *EntryBuilder, GR);
//   } else if (auto CV = dyn_cast<ConstantDataSequential>(&C)) {
//     // Vectors or arrays via OpConstantComposite
//     if (CV->getNumElements() == 1)
//       return translate(*CV->getElementAsConstant(0), Reg);
//     SmallVector<Register, 4> Ops;
//     for (unsigned i = 0; i < CV->getNumElements(); ++i) {
//       Ops.push_back(getOrCreateVReg(*CV->getElementAsConstant(i)));
//     }
//     Res = buildOpConstantComposite(C, Reg, Ops, *EntryBuilder, GR);
//   } else if (auto CV = dyn_cast<ConstantAggregate>(&C)) {
//     // Structs via OpConstantComposite
//     if (C.getNumOperands() == 1)
//       Res = translate(*CV->getOperand(0), Reg);
//     else {
//       SmallVector<Register, 4> Ops;
//       for (unsigned i = 0; i < C.getNumOperands(); ++i) {
//         Ops.push_back(getOrCreateVReg(*C.getOperand(i)));
//       }
//       Res = buildOpConstantComposite(C, Reg, Ops, *EntryBuilder, GR);
//     }
//   }

//   if (!Res)
//     Res = IRTranslator::translate(C, Reg);
//   DT->add(&C, MF, Reg);
//   return Res;
// }

// bool SPIRVIRTranslator::translateInst(const User &Inst, unsigned OpCode) {
//   switch (OpCode) {
//   case Instruction::GetElementPtr:
//     return translateGetElementPtr(Inst, *CurBuilder.get());
//   case Instruction::ShuffleVector:
//     return translateShuffleVector(Inst, *CurBuilder.get());
//   case Instruction::ExtractValue:
//     return translateExtractValue(Inst, *CurBuilder.get());
//   case Instruction::InsertValue:
//     return translateInsertValue(Inst, *CurBuilder.get());
//   case Instruction::BitCast:
//     return translateBitCast(Inst, *CurBuilder.get());
//   case Instruction::Call: {
//     const CallInst &CI = cast<CallInst>(Inst);
//     auto TII = MF->getTarget().getIntrinsicInfo();
//     const Function *F = CI.getCalledFunction();

//     // FIXME: support Windows dllimport function calls.
//     if (F && F->hasDLLImportStorageClass())
//       return false;

//     if (CI.isInlineAsm())
//       return translateInlineAsm(CI, *CurBuilder);

//     Intrinsic::ID ID = Intrinsic::not_intrinsic;
//     if (F && F->isIntrinsic()) {
//       ID = F->getIntrinsicID();
//       if (TII && ID == Intrinsic::not_intrinsic)
//         ID = static_cast<Intrinsic::ID>(TII->getIntrinsicID(F));
//     }

//     if (!F || !F->isIntrinsic() || ID == Intrinsic::not_intrinsic)
//       return translateCall(CI, *CurBuilder);

//     assert(ID != Intrinsic::not_intrinsic && "unknown intrinsic");

//     switch (ID) {
//     case Intrinsic::uadd_with_overflow:
//       return translateOverflowIntrinsic(CI, TargetOpcode::G_UADDO,
//       *CurBuilder);
//     case Intrinsic::sadd_with_overflow:
//       return translateOverflowIntrinsic(CI, TargetOpcode::G_SADDO,
//       *CurBuilder);
//     case Intrinsic::usub_with_overflow:
//       return translateOverflowIntrinsic(CI, TargetOpcode::G_USUBO,
//       *CurBuilder);
//     case Intrinsic::ssub_with_overflow:
//       return translateOverflowIntrinsic(CI, TargetOpcode::G_SSUBO,
//       *CurBuilder);
//     case Intrinsic::umul_with_overflow:
//       return translateOverflowIntrinsic(CI, TargetOpcode::G_UMULO,
//       *CurBuilder);
//     case Intrinsic::smul_with_overflow:
//       return translateOverflowIntrinsic(CI, TargetOpcode::G_SMULO,
//       *CurBuilder);
//     default:
//       return false;
//     }
//   }
//   case Instruction::AtomicCmpXchg:
//     return translateAtomicCmpXchg(Inst, *CurBuilder.get());
//   default:
//     return false;
//   }
// }

// bool SPIRVIRTranslator::translateBitCast(const User &U,
//                                          MachineIRBuilder &MIRBuilder) {
//   // Maintain type info by always using G_BITCAST instead of just copying
//   vreg return IRTranslator::translateCast(TargetOpcode::G_BITCAST, U,
//   MIRBuilder);
// }

// ArrayRef<Register> SPIRVIRTranslator::getOrCreateVRegs(const Value &Val) {
//   Type *Ty = Val.getType();

//   // Ensure type definition appears before uses. This is especially important
//   // for values like OpConstant which get hoisted alongside types later
//   GR->getOrCreateSPIRVType(Ty, *EntryBuilder);
//   const auto MRI = EntryBuilder->getMRI();

//   ArrayRef<Register> ResVRegs;

//   auto VRegsIt = VMap.findVRegs(Val);
//   if (VRegsIt != VMap.vregs_end()) {
//     ResVRegs = *VRegsIt->second;
//   } else if (Ty->isVoidTy()) {
//     ResVRegs = *VMap.getVRegs(Val);
//   } else {
//     assert(Ty->isSized() && "Cannot create unsized vreg");

//     // Create entry for this type.
//     auto *NewVRegs = VMap.getVRegs(Val);
//     auto llt = getLLTForType(*Ty, *DL);
//     Register vreg = MRI->createGenericVirtualRegister(llt);
//     NewVRegs->push_back(vreg);

//     if (auto C = dyn_cast<Constant>(&Val)) {
//       bool success = translate(*C, vreg);
//       assert(success && "Could not translate constant");
//     }
//     ResVRegs = *NewVRegs;
//     assert(ResVRegs.size() == 1);

//     // Add type and name metadata
//     if (!GR->hasSPIRVTypeForVReg(ResVRegs[0])) {
//       GR->assignTypeToVReg(Ty, ResVRegs[0], *EntryBuilder);
//       if (Val.hasName())
//         buildOpName(ResVRegs[0], Val.getName(), *EntryBuilder);
//     }
//   }
//   // Make sure there's always at least 1 placeholder offset to avoid crashes
//   auto *Offsets = VMap.getOffsets(Val);
//   if (Offsets->empty()) {
//     Offsets->push_back(0);
//   }
//   return ResVRegs;
// }

static void addConstantsToTrack(MachineFunction &MF, SPIRVGlobalRegistry *GR) {
  auto &MRI = MF.getRegInfo();
  DenseMap<MachineInstr *, Register> RegsAlreadyAddedToDT;
  std::vector<MachineInstr *> ToErase, ToEraseComposites;
  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      if (MI.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
          MI.getIntrinsicID() == Intrinsic::spv_track_constant) {
        ToErase.push_back(&MI);
        auto *Const =
            cast<Constant>(cast<ConstantAsMetadata>(
                               MI.getOperand(3).getMetadata()->getOperand(0))
                               ->getValue());
        Register Reg;
        if (auto *GV = dyn_cast<GlobalValue>(Const)) {
          if (GR->find(GV, &MF, Reg) == false) {
            GR->add(GV, &MF, MI.getOperand(2).getReg());
          } else
            RegsAlreadyAddedToDT[&MI] = Reg;
        } else {
          if (GR->find(Const, &MF, Reg) == false) {
            if (auto *ConstVec = dyn_cast<ConstantDataVector>(Const)) {
              auto *BuildVec = MRI.getVRegDef(MI.getOperand(2).getReg());
              assert(BuildVec &&
                     BuildVec->getOpcode() == TargetOpcode::G_BUILD_VECTOR);
              for (unsigned i = 0; i < ConstVec->getNumElements(); ++i)
                GR->add(ConstVec->getElementAsConstant(i), &MF,
                        BuildVec->getOperand(1 + i).getReg());
            }
            GR->add(Const, &MF, MI.getOperand(2).getReg());
          } else {
            RegsAlreadyAddedToDT[&MI] = Reg;
            // This MI is unused and will be removed. If the MI uses
            // const_composite, it will be unused and should be removed too.
            assert(MI.getOperand(2).isReg() && "Reg operand is expected");
            MachineInstr *SrcMI = MRI.getVRegDef(MI.getOperand(2).getReg());
            if (SrcMI &&
                SrcMI->getOpcode() ==
                    TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
                SrcMI->getIntrinsicID() == Intrinsic::spv_const_composite)
              ToEraseComposites.push_back(SrcMI);
          }
        }
      }
    }
  }
  for (auto *MI : ToErase) {
    Register Reg = MI->getOperand(2).getReg();
    if (RegsAlreadyAddedToDT.find(MI) != RegsAlreadyAddedToDT.end())
      Reg = RegsAlreadyAddedToDT[MI];
    MRI.replaceRegWith(MI->getOperand(0).getReg(), Reg);
    MI->eraseFromParent();
  }
  for (auto *MI : ToEraseComposites)
    MI->eraseFromParent();
}

static std::map<Intrinsic::ID, unsigned> IntrsWConstsToFold = {
    {Intrinsic::spv_assign_name, 2},
    // {Intrinsic::spv_const_composite, 1}
};

static void foldConstantsIntoIntrinsics(MachineFunction &MF) {
  std::vector<MachineInstr *> ToErase;
  auto &MRI = MF.getRegInfo();
  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      if (MI.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
          IntrsWConstsToFold.count(MI.getIntrinsicID())) {
        while (MI.getOperand(MI.getNumExplicitDefs() +
                             IntrsWConstsToFold.at(MI.getIntrinsicID()))
                   .isReg()) {
          auto MOp = MI.getOperand(MI.getNumExplicitDefs() +
                                   IntrsWConstsToFold.at(MI.getIntrinsicID()));
          auto *ConstMI = MRI.getVRegDef(MOp.getReg());
          assert(ConstMI->getOpcode() == TargetOpcode::G_CONSTANT);
          MI.RemoveOperand(MI.getNumExplicitDefs() +
                           IntrsWConstsToFold.at(MI.getIntrinsicID()));
          MI.addOperand(MachineOperand::CreateImm(
              ConstMI->getOperand(1).getCImm()->getZExtValue()));
          if (MRI.use_empty(ConstMI->getOperand(0).getReg()))
            ToErase.push_back(ConstMI);
        }
      }
    }
  }
  for (auto *MI : ToErase)
    MI->eraseFromParent();
}

// Translating GV, IRTranslator sometimes generates following IR:
//   %1 = G_GLOBAL_VALUE
//   %2 = COPY %1
//   %3 = G_ADDRSPACE_CAST %2
// New registers have no SPIRVType and no register class info.
//
// Set SPIRVType for GV, propagate it from GV to other instructions,
// also set register classes.
static SPIRVType *propagateSPIRVType(MachineInstr *MI, SPIRVGlobalRegistry *GR,
                                     MachineRegisterInfo &MRI,
                                     MachineIRBuilder &MIB) {
  SPIRVType *SpirvTy = nullptr;
  assert(MI && "Machine instr is expected");
  if (MI->getOperand(0).isReg()) {
    auto Reg = MI->getOperand(0).getReg();
    SpirvTy = GR->getSPIRVTypeForVReg(Reg);
    if (!SpirvTy) {
      switch (MI->getOpcode()) {
      case TargetOpcode::G_CONSTANT: {
        MIB.setInsertPt(*MI->getParent(), MI);
        Type *Ty = MI->getOperand(1).getCImm()->getType();
        SpirvTy = GR->getOrCreateSPIRVType(Ty, MIB);
        break;
      }
      case TargetOpcode::G_GLOBAL_VALUE: {
        MIB.setInsertPt(*MI->getParent(), MI);
        Type *Ty = MI->getOperand(1).getGlobal()->getType();
        SpirvTy = GR->getOrCreateSPIRVType(Ty, MIB);
        break;
      }
      case TargetOpcode::G_TRUNC:
      case TargetOpcode::G_ADDRSPACE_CAST:
      case TargetOpcode::COPY: {
        auto Op = MI->getOperand(1);
        auto *Def = Op.isReg() ? MRI.getVRegDef(Op.getReg()) : nullptr;
        if (Def)
          SpirvTy = propagateSPIRVType(Def, GR, MRI, MIB);
        break;
      }
      default:
        break;
      }
      if (SpirvTy)
        GR->assignSPIRVTypeToVReg(SpirvTy, Reg, MIB);
      if (!MRI.getRegClassOrNull(Reg))
        MRI.setRegClass(Reg, &SPIRV::IDRegClass);
    }
  }
  return SpirvTy;
}

// Insert ASSIGN_TYPE instuction between Reg and its definition, set NewReg as
// a dst of the definition, assign SPIRVType to both registers. If SpirvTy is
// provided, use it as SPIRVType in ASSIGN_TYPE, otherwise create it from Ty.
// It's used also in SPIRVOpenCLBIFs.cpp.
Register insertAssignInstr(Register Reg, Type *Ty, SPIRVType *SpirvTy,
                           SPIRVGlobalRegistry *GR, MachineIRBuilder &MIB,
                           MachineRegisterInfo &MRI) {
  auto *Def = MRI.getVRegDef(Reg);
  assert((Ty || SpirvTy) && "Either LLVM or SPIRV type is expected.");
  MIB.setInsertPt(*Def->getParent(),
                  (Def->getNextNode() ? Def->getNextNode()->getIterator()
                                      : Def->getParent()->end()));
  auto NewReg = MRI.createGenericVirtualRegister(MRI.getType(Reg));
  if (auto *RC = MRI.getRegClassOrNull(Reg))
    MRI.setRegClass(NewReg, RC);
  SpirvTy = SpirvTy ? SpirvTy : GR->getOrCreateSPIRVType(Ty, MIB);
  GR->assignSPIRVTypeToVReg(SpirvTy, Reg, MIB);
  // This is to make it convenient for Legalizer to get the SPIRVType
  // when processing the actual MI (i.e. not pseudo one).
  GR->assignSPIRVTypeToVReg(SpirvTy, NewReg, MIB);
  auto NewMI = MIB.buildInstr(SPIRV::ASSIGN_TYPE)
                   .addDef(Reg)
                   .addUse(NewReg)
                   .addUse(GR->getSPIRVTypeID(SpirvTy));
  Def->getOperand(0).setReg(NewReg);
  constrainRegOperands(NewMI, &MIB.getMF());
  return NewReg;
}

static void generateAssignInstrs(MachineFunction &MF, SPIRVGlobalRegistry *GR) {
  MachineIRBuilder MIB(MF);
  MachineRegisterInfo &MRI = MF.getRegInfo();
  std::vector<MachineInstr *> ToDelete;

  for (MachineBasicBlock *MBB : post_order(&MF)) {
    if (MBB->empty())
      continue;

    bool ReachedBegin = false;
    for (auto MII = std::prev(MBB->end()), Begin = MBB->begin();
         !ReachedBegin;) {
      MachineInstr &MI = *MII;

      if (MI.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
          MI.getOperand(0).isIntrinsicID() &&
          MI.getOperand(0).getIntrinsicID() == Intrinsic::spv_assign_type) {
        auto Reg = MI.getOperand(1).getReg();
        auto *Ty =
            cast<ValueAsMetadata>(MI.getOperand(2).getMetadata()->getOperand(0))
                ->getType();
        auto *Def = MRI.getVRegDef(Reg);
        assert(Def && "Expecting an instruction that defines the register");
        // G_GLOBAL_VALUE already has type info.
        if (Def->getOpcode() != TargetOpcode::G_GLOBAL_VALUE)
          insertAssignInstr(Reg, Ty, nullptr, GR, MIB, MF.getRegInfo());
        ToDelete.push_back(&MI);
      } else if (MI.getOpcode() == TargetOpcode::G_CONSTANT ||
                 MI.getOpcode() == TargetOpcode::G_FCONSTANT ||
                 MI.getOpcode() == TargetOpcode::G_BUILD_VECTOR) {
        // %rc = G_CONSTANT ty Val
        // ===>
        // %cty = OpType* ty
        // %rctmp = G_CONSTANT ty Val
        // %rc = ASSIGN_TYPE %rctmp, %cty
        auto Reg = MI.getOperand(0).getReg();
        if (MRI.hasOneUse(Reg) &&
            MRI.use_instr_begin(Reg)->getOpcode() ==
                TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
            (MRI.use_instr_begin(Reg)->getIntrinsicID() ==
                 Intrinsic::spv_assign_type ||
             MRI.use_instr_begin(Reg)->getIntrinsicID() ==
                 Intrinsic::spv_assign_name))
          continue;

        auto &MRI = MF.getRegInfo();
        Type *Ty = nullptr;
        if (MI.getOpcode() == TargetOpcode::G_CONSTANT)
          Ty = MI.getOperand(1).getCImm()->getType();
        else if (MI.getOpcode() == TargetOpcode::G_FCONSTANT)
          Ty = MI.getOperand(1).getFPImm()->getType();
        else {
          assert(MI.getOpcode() == TargetOpcode::G_BUILD_VECTOR);
          Type *ElemTy = nullptr;
          auto *ElemMI = MRI.getVRegDef(MI.getOperand(1).getReg());
          assert(ElemMI);

          if (ElemMI->getOpcode() == TargetOpcode::G_CONSTANT)
            ElemTy = ElemMI->getOperand(1).getCImm()->getType();
          else if (ElemMI->getOpcode() == TargetOpcode::G_FCONSTANT)
            ElemTy = ElemMI->getOperand(1).getFPImm()->getType();
          else
            assert(0);
          Ty = VectorType::get(
              ElemTy, MI.getNumExplicitOperands() - MI.getNumExplicitDefs(),
              false);
        }
        insertAssignInstr(Reg, Ty, nullptr, GR, MIB, MRI);
      } else if (MI.getOpcode() == TargetOpcode::G_TRUNC ||
                 MI.getOpcode() == TargetOpcode::G_GLOBAL_VALUE ||
                 MI.getOpcode() == TargetOpcode::COPY ||
                 MI.getOpcode() == TargetOpcode::G_ADDRSPACE_CAST) {
        propagateSPIRVType(&MI, GR, MRI, MIB);
      }

      if (MII == Begin)
        ReachedBegin = true;
      else
        --MII;
    }
  }

  for (auto &MI : ToDelete)
    MI->eraseFromParent();
}

bool SPIRVIRTranslator::runOnMachineFunction(MachineFunction &MF) {
  // Initialize the type registry
  const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());
  GR = ST->getSPIRVGlobalRegistry();

  GR->setCurrentFunc(MF);

  // Run the regular IRTranslator
  bool Success = IRTranslator::runOnMachineFunction(MF);
  addConstantsToTrack(MF, GR);
  foldConstantsIntoIntrinsics(MF);
  generateAssignInstrs(MF, GR);

  return Success;
}
