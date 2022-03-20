//===-- SPIRVEmitIntrinsics.cpp - emit SPIRV intrinsics ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The pass emits SPIRV intrinsics keeping essential high-level information for
// the translation of LLVM IR to SPIR-V.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVUtils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/IntrinsicsSPIRV.h"

#include <queue>
#include <unordered_map>

// This pass performs the following transformation on LLVM IR level required
// for the following translation to SPIR-V:
// - replaces direct usages of aggregate constants with target-specific
//   intrinsics;
// - replaces aggregates-related instructions (extract/insert, ld/st, etc)
//   with a target-specific intrinsics;
// - emits intrinsics for the global variable initializers since IRTranslator
//   doesn't handle them and it's not very convenient to translate them
//   ourselves;
// - emits intrinsics to keep track of the string names assigned to the values;
// - emits intrinsics to keep track of constants (this is necessary to have an
//   LLVM IR constant after the IRTranslation is completed) for their further
//   deduplication;
// - emits intrinsics to keep track of original LLVM types of the values
//   to be able to emit proper SPIR-V types eventually.
//
// TODO: consider removing spv.track.constant in favor of spv.assign.type

using namespace llvm;

namespace llvm {
void initializeSPIRVEmitIntrinsicsPass(PassRegistry &);
}

namespace {
class SPIRVEmitIntrinsics
    : public FunctionPass,
      public InstVisitor<SPIRVEmitIntrinsics, Instruction *> {
  SPIRVTargetMachine *TM = nullptr;
  IRBuilder<> *IRB = nullptr;
  Function *F = nullptr;
  bool TrackConstants = true;
  std::unordered_map<Instruction *, Constant *> AggrConsts;
  void preprocessCompositeConstants();
  CallInst *buildIntrCall(Intrinsic::ID IntrID, ArrayRef<Type *> Types,
                          ArrayRef<Value *> Args) {
    auto *IntrFn = Intrinsic::getDeclaration(F->getParent(), IntrID, Types);
    return IRB->CreateCall(IntrFn, Args);
  }
  CallInst *buildIntrWithMD(Intrinsic::ID IntrID, ArrayRef<Type *> Types,
                            Value *Arg, Value *Arg2) {
    ConstantAsMetadata *CM = ValueAsMetadata::getConstant(Arg);
    MDTuple *TyMD = MDNode::get(F->getContext(), CM);
    MetadataAsValue *VMD = MetadataAsValue::get(F->getContext(), TyMD);
    return buildIntrCall(IntrID, {Types}, {Arg2, VMD});
  }
  void replaceMemInstrUses(Instruction *Old, Instruction *New);
  void processInstrAfterVisit(Instruction *I);
  void insertAssignTypeIntrs(Instruction *I);
  void processGlobalValue(GlobalVariable &GV);

public:
  static char ID;
  SPIRVEmitIntrinsics() : FunctionPass(ID) {
    initializeSPIRVEmitIntrinsicsPass(*PassRegistry::getPassRegistry());
  }
  SPIRVEmitIntrinsics(SPIRVTargetMachine *_TM) : FunctionPass(ID), TM(_TM) {
    initializeSPIRVEmitIntrinsicsPass(*PassRegistry::getPassRegistry());
  }
  Instruction *visitInstruction(Instruction &I) { return &I; }
  Instruction *visitSwitchInst(SwitchInst &I);
  Instruction *visitGetElementPtrInst(GetElementPtrInst &I);
  Instruction *visitBitCastInst(BitCastInst &I);
  Instruction *visitInsertElementInst(InsertElementInst &I);
  Instruction *visitExtractElementInst(ExtractElementInst &I);
  Instruction *visitInsertValueInst(InsertValueInst &I);
  Instruction *visitExtractValueInst(ExtractValueInst &I);
  Instruction *visitLoadInst(LoadInst &I);
  Instruction *visitStoreInst(StoreInst &I);
  Instruction *visitAllocaInst(AllocaInst &I);
  bool runOnFunction(Function &F) override;
};
} // namespace

char SPIRVEmitIntrinsics::ID = 0;

INITIALIZE_PASS(SPIRVEmitIntrinsics, "emit-intrinsics", "SPIRV emit intrinsics",
                false, false)

static inline bool isAssignTypeInstr(const Instruction *I) {
  return isa<IntrinsicInst>(I) &&
         cast<IntrinsicInst>(I)->getIntrinsicID() == Intrinsic::spv_assign_type;
}

static bool isMemInstrToReplace(Instruction *I) {
  return isa<StoreInst>(I) || isa<LoadInst>(I) || isa<InsertValueInst>(I) ||
         isa<ExtractValueInst>(I);
}

static bool isAggrToReplace(const Value *V) {
  return isa<ConstantAggregate>(V) || isa<ConstantDataArray>(V) ||
         // isa<ConstantDataVector>(V) ||
         (isa<ConstantAggregateZero>(V) && !V->getType()->isVectorTy());
}

static void setInsertPointSkippingPhis(IRBuilder<> &B, Instruction *I) {
  while (isa<PHINode>(I))
    I = I->getNextNode();
  B.SetInsertPoint(I);
}

static bool requireAssignType(Instruction *I) {
  IntrinsicInst *Intr = dyn_cast<IntrinsicInst>(I);
  if (Intr) {
    switch (Intr->getIntrinsicID()) {
    case Intrinsic::invariant_start:
    case Intrinsic::invariant_end:
      return false;
    }
  }
  return true;
}

void SPIRVEmitIntrinsics::replaceMemInstrUses(Instruction *Old,
                                              Instruction *New) {
  while (!Old->user_empty()) {
    auto *U = Old->user_back();
    if (isMemInstrToReplace(U) || isa<ReturnInst>(U)) {
      U->replaceUsesOfWith(Old, New);
    } else if (isAssignTypeInstr(U)) {
      IRB->SetInsertPoint(U);
      std::vector<Value *> Args = {New, U->getOperand(1)};
      buildIntrCall(Intrinsic::spv_assign_type, {New->getType()}, Args);
      U->eraseFromParent();
    } else {
      llvm_unreachable("illegal aggregate intrinsic user");
    }
  }
  Old->eraseFromParent();
}

void SPIRVEmitIntrinsics::preprocessCompositeConstants() {
  std::queue<Instruction *> Worklist;
  for (auto &I : instructions(F))
    Worklist.push(&I);

  while (!Worklist.empty()) {
    auto *I = Worklist.front();
    assert(I);
    bool KeepInst = false;
    for (const auto &Op : I->operands()) {
      auto BuildCompositeIntrinsic = [&KeepInst, &Worklist, &I, &Op,
                                      this](Constant *AggrC,
                                            std::vector<Value *> &Args) {
        IRB->SetInsertPoint(I);
        auto *CCI = buildIntrCall(Intrinsic::spv_const_composite, {}, {Args});
        Worklist.push(CCI);
        I->replaceUsesOfWith(Op, CCI);
        KeepInst = true;
        AggrConsts[CCI] = AggrC;
      };

      if (auto *AggrC = dyn_cast<ConstantAggregate>(Op)) {
        std::vector<Value *> Args;
        for (auto &AggrOp : AggrC->operands())
          Args.push_back(AggrOp);
        BuildCompositeIntrinsic(AggrC, Args);
        // } else if (auto *AggrC = dyn_cast<ConstantDataVector>(Op)) {
        //   std::vector<Value *> Args;
        //   for (int i = 0; i < AggrC->getNumElements(); ++i)
        //     Args.push_back(AggrC->getElementAsConstant(i));
        //   BuildCompositeIntrinsic(AggrC, Args);
      } else if (auto *AggrC = dyn_cast<ConstantDataArray>(Op)) {
        std::vector<Value *> Args;
        for (unsigned i = 0; i < AggrC->getNumElements(); ++i)
          Args.push_back(AggrC->getElementAsConstant(i));
        BuildCompositeIntrinsic(AggrC, Args);
      } else if (isa<ConstantAggregateZero>(Op) &&
                 !Op->getType()->isVectorTy()) {
        auto *AggrC = cast<ConstantAggregateZero>(Op);
        std::vector<Value *> Args;
        for (auto &AggrOp : AggrC->operands())
          Args.push_back(AggrOp);
        BuildCompositeIntrinsic(AggrC, Args);
      }
    }
    if (!KeepInst)
      Worklist.pop();
  }
}

Instruction *SPIRVEmitIntrinsics::visitSwitchInst(SwitchInst &I) {
  std::vector<Value *> Args;
  for (auto &Op : I.operands())
    if (Op.get()->getType()->isSized())
      Args.push_back(Op);
  buildIntrCall(Intrinsic::spv_switch, {I.getOperand(0)->getType()}, {Args});
  return &I;
}

Instruction *SPIRVEmitIntrinsics::visitGetElementPtrInst(GetElementPtrInst &I) {
  std::vector<Type *> Types = {I.getType(), I.getOperand(0)->getType()};
  std::vector<Value *> Args;
  Args.push_back(IRB->getInt1(I.isInBounds()));
  for (auto &Op : I.operands())
    Args.push_back(Op);
  auto *NewI = buildIntrCall(Intrinsic::spv_gep, {Types}, {Args});
  I.replaceAllUsesWith(NewI);
  I.eraseFromParent();
  return NewI;
}

Instruction *SPIRVEmitIntrinsics::visitBitCastInst(BitCastInst &I) {
  std::vector<Type *> Types = {I.getType(), I.getOperand(0)->getType()};
  std::vector<Value *> Args;
  for (auto &Op : I.operands())
    Args.push_back(Op);
  auto *NewI = buildIntrCall(Intrinsic::spv_bitcast, {Types}, {Args});
  std::string InstName = I.hasName() ? I.getName().str() : "";
  I.replaceAllUsesWith(NewI);
  I.eraseFromParent();
  NewI->setName(InstName);
  return NewI;
}

Instruction *SPIRVEmitIntrinsics::visitInsertElementInst(InsertElementInst &I) {
  std::vector<Type *> Types = {I.getType(), I.getOperand(0)->getType(),
                               I.getOperand(1)->getType(),
                               I.getOperand(2)->getType()};
  std::vector<Value *> Args;
  for (auto &Op : I.operands())
    Args.push_back(Op);
  auto *NewI = buildIntrCall(Intrinsic::spv_insertelt, {Types}, {Args});
  std::string InstName = I.hasName() ? I.getName().str() : "";
  I.replaceAllUsesWith(NewI);
  I.eraseFromParent();
  NewI->setName(InstName);
  return NewI;
}

Instruction *
SPIRVEmitIntrinsics::visitExtractElementInst(ExtractElementInst &I) {
  std::vector<Type *> Types = {I.getType(), I.getVectorOperandType(),
                               I.getIndexOperand()->getType()};
  std::vector<Value *> Args;
  Args.push_back(I.getVectorOperand());
  Args.push_back(I.getIndexOperand());
  auto *NewI = buildIntrCall(Intrinsic::spv_extractelt, {Types}, {Args});
  std::string InstName = I.hasName() ? I.getName().str() : "";
  I.replaceAllUsesWith(NewI);
  I.eraseFromParent();
  NewI->setName(InstName);
  return NewI;
}

Instruction *SPIRVEmitIntrinsics::visitInsertValueInst(InsertValueInst &I) {
  std::vector<Type *> Types = {I.getInsertedValueOperand()->getType()};
  std::vector<Value *> Args;
  for (auto &Op : I.operands())
    if (isa<UndefValue>(Op))
      Args.push_back(UndefValue::get(IRB->getInt32Ty()));
    else
      Args.push_back(Op);
  for (auto &Op : I.indices())
    Args.push_back(IRB->getInt32(Op));
  Instruction *NewI = buildIntrCall(Intrinsic::spv_insertv, {Types}, {Args});
  replaceMemInstrUses(&I, NewI);
  return NewI;
}

Instruction *SPIRVEmitIntrinsics::visitExtractValueInst(ExtractValueInst &I) {
  std::vector<Value *> Args;
  for (auto &Op : I.operands())
    Args.push_back(Op);
  for (auto &Op : I.indices())
    Args.push_back(IRB->getInt32(Op));
  auto *NewI = buildIntrCall(Intrinsic::spv_extractv, {I.getType()}, {Args});
  I.replaceAllUsesWith(NewI);
  I.eraseFromParent();
  return NewI;
}

Instruction *SPIRVEmitIntrinsics::visitLoadInst(LoadInst &I) {
  if (!I.getType()->isAggregateType())
    return &I;
  TrackConstants = false;
  const auto *TLI = TM->getSubtargetImpl()->getTargetLowering();
  MachineMemOperand::Flags Flags =
      TLI->getLoadMemOperandFlags(I, F->getParent()->getDataLayout());
  std::vector<Type *> Types = {I.getOperand(0)->getType()};
  std::vector<Value *> Args = {I.getPointerOperand(), IRB->getInt16(Flags),
                               IRB->getInt8(I.getAlignment())};
  auto *NewI = buildIntrCall(Intrinsic::spv_load, {Types}, {Args});
  replaceMemInstrUses(&I, NewI);
  return NewI;
}

Instruction *SPIRVEmitIntrinsics::visitStoreInst(StoreInst &I) {
  PointerType *PTy = cast<PointerType>(I.getOperand(1)->getType());
  if (!PTy->getPointerElementType()->isAggregateType())
    return &I;
  TrackConstants = false;
  const auto *TLI = TM->getSubtargetImpl()->getTargetLowering();
  MachineMemOperand::Flags Flags =
      TLI->getStoreMemOperandFlags(I, F->getParent()->getDataLayout());
  std::vector<Type *> Types = {I.getPointerOperand()->getType()};
  std::vector<Value *> Args = {I.getValueOperand(), I.getPointerOperand(),
                               IRB->getInt16(Flags),
                               IRB->getInt8(I.getAlignment())};
  auto *NewI = buildIntrCall(Intrinsic::spv_store, {Types}, {Args});
  I.eraseFromParent();
  return NewI;
}

Instruction *SPIRVEmitIntrinsics::visitAllocaInst(AllocaInst &I) {
  TrackConstants = false;
  return &I;
}

void SPIRVEmitIntrinsics::processGlobalValue(GlobalVariable &GV) {
  if (GV.hasInitializer() && !isa<UndefValue>(GV.getInitializer())) {
    Constant *Init = GV.getInitializer();
    Type *Ty = isAggrToReplace(Init) ? IRB->getInt32Ty() : Init->getType();
    Constant *Const = isAggrToReplace(Init) ? IRB->getInt32(1) : Init;
    std::vector<Type *> Types = {GV.getType(), Ty};
    std::vector<Value *> Args = {&GV, Const};
    auto *InitInst = buildIntrCall(Intrinsic::spv_init_global, Types, Args);
    InitInst->setArgOperand(1, Init);
  }
  if ((!GV.hasInitializer() || isa<UndefValue>(GV.getInitializer())) &&
      GV.getNumUses() == 0) {
    buildIntrCall(Intrinsic::spv_unref_global, GV.getType(), &GV);
  }
}

void SPIRVEmitIntrinsics::insertAssignTypeIntrs(Instruction *I) {
  Type *Ty = I->getType();
  if (!Ty->isVoidTy() && requireAssignType(I)) {
    setInsertPointSkippingPhis(*IRB, I->getNextNode());
    Type *TypeToAssign = Ty;
    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      if (II->getIntrinsicID() == Intrinsic::spv_const_composite)
        TypeToAssign = AggrConsts.at(II)->getType();
    }
    Constant *Const = Constant::getNullValue(TypeToAssign);
    buildIntrWithMD(Intrinsic::spv_assign_type, {Ty}, Const, I);
  }
  for (const auto &Op : I->operands()) {
    if (isa<ConstantPointerNull>(Op) || isa<UndefValue>(Op)) {
      IRB->SetInsertPoint(I);
      buildIntrWithMD(Intrinsic::spv_assign_type, {Op->getType()}, Op, Op);
    }
  }
}

void SPIRVEmitIntrinsics::processInstrAfterVisit(Instruction *I) {
  auto *II = dyn_cast<IntrinsicInst>(I);
  if (II && II->getIntrinsicID() == Intrinsic::spv_const_composite &&
      TrackConstants) {
    IRB->SetInsertPoint(I->getNextNode());
    Type *Ty = IRB->getInt32Ty();
    auto *NewOp = buildIntrWithMD(Intrinsic::spv_track_constant, {Ty, Ty},
                                  AggrConsts.at(I), I);
    I->replaceAllUsesWith(NewOp);
    NewOp->setArgOperand(0, I);
  }
  for (const auto &Op : I->operands()) {
    if ((isa<ConstantAggregateZero>(Op) && Op->getType()->isVectorTy()) ||
        isa<PHINode>(I) || isa<SwitchInst>(I))
      TrackConstants = false;
    if (isa<ConstantData>(Op) && TrackConstants) {
      unsigned OpNo = Op.getOperandNo();
      if (II && ((II->getIntrinsicID() == Intrinsic::spv_gep && OpNo == 0) ||
                 (II->paramHasAttr(OpNo, Attribute::ImmArg))))
        continue;
      IRB->SetInsertPoint(I);
      auto *NewOp = buildIntrWithMD(Intrinsic::spv_track_constant,
                                    {Op->getType(), Op->getType()}, Op, Op);
      I->setOperand(OpNo, NewOp);
    }
  }
  if (I->hasName()) {
    setInsertPointSkippingPhis(*IRB, I->getNextNode());
    std::vector<Value *> Args = {I};
    addStringImm(I->getName(), *IRB, Args);
    buildIntrCall(Intrinsic::spv_assign_name, {I->getType()}, Args);
  }
}

bool SPIRVEmitIntrinsics::runOnFunction(Function &Func) {
  if (Func.isDeclaration())
    return false;
  F = &Func;
  IRB = new IRBuilder<>(Func.getContext());
  AggrConsts.clear();

  IRB->SetInsertPoint(&Func.getEntryBlock().front());

  for (auto &GV : Func.getParent()->globals())
    processGlobalValue(GV);

  preprocessCompositeConstants();
  std::vector<Instruction *> Worklist;
  for (auto &I : instructions(Func))
    Worklist.push_back(&I);

  for (auto &I : Worklist)
    insertAssignTypeIntrs(I);

  for (auto *I : Worklist) {
    TrackConstants = true;
    if (!I->getType()->isVoidTy() || isa<StoreInst>(I))
      IRB->SetInsertPoint(I->getNextNode());
    I = visit(*I);
    processInstrAfterVisit(I);
  }
  return true;
}

FunctionPass *llvm::createSPIRVEmitIntrinsicsPass(SPIRVTargetMachine *TM) {
  return new SPIRVEmitIntrinsics(TM);
}
