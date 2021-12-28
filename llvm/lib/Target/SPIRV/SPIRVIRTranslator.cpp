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
// SPIRVTypeRegistry before it gets discarded as LLVM IR is lowered to GMIR.
//
//===----------------------------------------------------------------------===//

#include "SPIRVIRTranslator.h"

#include "SPIRV.h"
#include "SPIRVStrings.h"
#include "SPIRVSubtarget.h"

#include "llvm/IR/IntrinsicsSPIRV.h"

#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Target/TargetIntrinsicInfo.h"

#include <vector>

using namespace llvm;

// static bool buildOpConstantNull(const Constant &C, Register Reg,
//                                 MachineIRBuilder &MIRBuilder,
//                                 SPIRVTypeRegistry *TR) {
//   SPIRVType *type = TR->getOrCreateSPIRVType(C.getType(), MIRBuilder);
//   auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantNull)
//                  .addDef(Reg)
//                  .addUse(TR->getSPIRVTypeID(type));
//   return TR->constrainRegOperands(MIB);
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
//                                      SPIRVTypeRegistry *TR) {
//   bool areAllConst = isConstantCompositeConstructible(C);
//   SPIRVType *type = TR->getOrCreateSPIRVType(C.getType(), MIRBuilder);
//   auto MIB = MIRBuilder
//                  .buildInstr(areAllConst ? SPIRV::OpConstantComposite
//                                          : SPIRV::OpCompositeConstruct)
//                  .addDef(Reg)
//                  .addUse(TR->getSPIRVTypeID(type));
//   for (const auto &Op : Ops) {
//     MIB.addUse(Op);
//   }
//   return TR->constrainRegOperands(MIB);
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
//     Res = buildOpConstantNull(C, Reg, *EntryBuilder, TR);
//   } else if (auto CV = dyn_cast<ConstantDataSequential>(&C)) {
//     // Vectors or arrays via OpConstantComposite
//     if (CV->getNumElements() == 1)
//       return translate(*CV->getElementAsConstant(0), Reg);
//     SmallVector<Register, 4> Ops;
//     for (unsigned i = 0; i < CV->getNumElements(); ++i) {
//       Ops.push_back(getOrCreateVReg(*CV->getElementAsConstant(i)));
//     }
//     Res = buildOpConstantComposite(C, Reg, Ops, *EntryBuilder, TR);
//   } else if (auto CV = dyn_cast<ConstantAggregate>(&C)) {
//     // Structs via OpConstantComposite
//     if (C.getNumOperands() == 1)
//       Res = translate(*CV->getOperand(0), Reg);
//     else {
//       SmallVector<Register, 4> Ops;
//       for (unsigned i = 0; i < C.getNumOperands(); ++i) {
//         Ops.push_back(getOrCreateVReg(*C.getOperand(i)));
//       }
//       Res = buildOpConstantComposite(C, Reg, Ops, *EntryBuilder, TR);
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
//   TR->getOrCreateSPIRVType(Ty, *EntryBuilder);
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
//     if (!TR->hasSPIRVTypeForVReg(ResVRegs[0])) {
//       TR->assignTypeToVReg(Ty, ResVRegs[0], *EntryBuilder);
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

static void addConstantsToTrack(MachineFunction &MF,
                                SPIRVGeneralDuplicatesTracker *DT) {
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
          if (DT->find(GV, &MF, Reg) == false) {
            DT->add(GV, &MF, MI.getOperand(2).getReg());
          } else
            RegsAlreadyAddedToDT[&MI] = Reg;
        } else {
          if (DT->find(Const, &MF, Reg) == false) {
            if (auto *ConstVec = dyn_cast<ConstantDataVector>(Const)) {
              auto *BuildVec = MRI.getVRegDef(MI.getOperand(2).getReg());
              assert(BuildVec &&
                     BuildVec->getOpcode() == TargetOpcode::G_BUILD_VECTOR);
              for (unsigned i = 0; i < ConstVec->getNumElements(); ++i)
                DT->add(ConstVec->getElementAsConstant(i), &MF,
                        BuildVec->getOperand(1 + i).getReg());
            }
            DT->add(Const, &MF, MI.getOperand(2).getReg());
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

bool SPIRVIRTranslator::runOnMachineFunction(MachineFunction &MF) {
  // Initialize the type registry
  const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());
  TR = ST->getSPIRVTypeRegistry();
  DT = ST->getSPIRVDuplicatesTracker();

  TR->setCurrentFunc(MF);

  // Run the regular IRTranslator
  bool Success = IRTranslator::runOnMachineFunction(MF);
  addConstantsToTrack(MF, DT);
  foldConstantsIntoIntrinsics(MF);
  TR->generateAssignInstrs(MF);

  return Success;
}
