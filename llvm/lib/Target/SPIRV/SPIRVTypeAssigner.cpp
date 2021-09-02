#include "SPIRV.h"
#include "SPIRVTargetMachine.h"

#include "llvm/IR/IntrinsicsSPIRV.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

using namespace llvm;

namespace llvm {
void initializeSPIRVTypeAssignerPass(PassRegistry &);
}

namespace {

class SPIRVTypeAssigner : public FunctionPass {
  SPIRVTargetMachine *TM = nullptr;
public:
  static char ID;
  SPIRVTypeAssigner() : FunctionPass(ID) {
    initializeSPIRVTypeAssignerPass(*PassRegistry::getPassRegistry());
  }
  SPIRVTypeAssigner(SPIRVTargetMachine *_TM) : FunctionPass(ID), TM(_TM) {
    initializeSPIRVTypeAssignerPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &MF) override;

  StringRef getPassName() const override {
    return "SPIRV Types Assigner";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    FunctionPass::getAnalysisUsage(AU);
  }
};

} // namespace

char SPIRVTypeAssigner::ID = 0;

INITIALIZE_PASS(SPIRVTypeAssigner, "spirv-type-assigner",
                "SPIRV Type Assigner", false, false)

static bool isMemInstrToReplace(Instruction *I) {
  return isa<StoreInst>(I) || isa<LoadInst>(I) || isa<InsertValueInst>(I) ||
         isa<ExtractValueInst>(I);
}

static void replaceMemInstrUses(Instruction *Old, Instruction *New) {
  errs() << "Replacing " << *Old << " with " << *New << "\n";
  while (!Old->user_empty()) {
    auto *U = Old->user_back();
    errs() << "U: " << *U << "\n";
    assert(isMemInstrToReplace(cast<Instruction>(U)));
    U->replaceUsesOfWith(Old, New);
  }
  Old->eraseFromParent();
}

bool SPIRVTypeAssigner::runOnFunction(Function &F) {
  IRBuilder<> B(&F.getEntryBlock().front());
  std::vector<Instruction *> Worklist;
  for (auto &I : instructions(F)) {
    if (!I.getType()->isVoidTy() || isa<StoreInst>(&I))
      Worklist.push_back(&I);
  }
  // set the function and args types
  // B.SetInsertPoint(&F.front().front());
  // auto TyFn = Intrinsic::getDeclaration(
  //     F.getParent(), Intrinsic::spv_assign_type, {F.getReturnType()});
  // auto *TyMD = MDNode::get(
  //     F.getContext(), ValueAsMetadata::getConstant(Constant::getNullValue(Ty)));
  // auto *VMD = MetadataAsValue::get(F.getContext(), TyMD);
  // errs() << *(B.CreateCall(TyFn, {&F, VMD})) << "\n";
  // F.dump();
  for (auto *I : Worklist) {
    B.SetInsertPoint(I->getNextNode());
    Instruction *NewIntr = nullptr;
    if (auto *Gep = dyn_cast<GetElementPtrInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F.getParent(), Intrinsic::spv_gep,
          {Gep->getType(), Gep->getOperand(0)->getType()});
      std::vector<Value *> Args;
      Args.push_back(B.getInt1(Gep->isInBounds()));
      for (auto &Op : Gep->operands())
        Args.push_back(Op);
      auto *NewGep = B.CreateCall(IntrFn, {Args});
      Gep->replaceAllUsesWith(NewGep);
      Gep->eraseFromParent();
      I = NewGep;
    } else if (isa<StoreInst>(I) && cast<PointerType>(I->getOperand(1)->getType())->getElementType()->isAggregateType()) {
      auto *SI = cast<StoreInst>(I);
      auto *IntrFn = Intrinsic::getDeclaration(
          F.getParent(), Intrinsic::spv_store,
          {SI->getPointerOperand()->getType()});
      const auto *TLI = TM->getSubtargetImpl()->getTargetLowering();
      MachineMemOperand::Flags Flags = TLI->getStoreMemOperandFlags(*SI, F.getParent()->getDataLayout());
      I = B.CreateCall(IntrFn, {SI->getValueOperand(), SI->getPointerOperand(), B.getInt16(Flags)});
      SI->eraseFromParent();
    } else if (isa<LoadInst>(I) && I->getType()->isAggregateType()) {
      auto *LI = cast<LoadInst>(I);
      auto *IntrFn = Intrinsic::getDeclaration(
          F.getParent(), Intrinsic::spv_load,
          {LI->getOperand(0)->getType()});
      const auto *TLI = TM->getSubtargetImpl()->getTargetLowering();
      MachineMemOperand::Flags Flags = TLI->getLoadMemOperandFlags(*LI, F.getParent()->getDataLayout());
      NewIntr = B.CreateCall(IntrFn, {LI->getPointerOperand(), B.getInt16(Flags)});
    } else if (auto *EVI = dyn_cast<ExtractValueInst>(I)) {
      errs() << "Looking at EVI " << *EVI << "\n";
      auto *IntrFn = Intrinsic::getDeclaration(
          F.getParent(), Intrinsic::spv_extractv,
          {EVI->getType()});
      std::vector<Value *> Args;
      for (auto &Op : EVI->operands()) {
        Args.push_back(Op);
      }
      for (auto &Op : EVI->indices()) {
        Args.push_back(B.getInt32(Op));
      }
      auto *NewEVI = B.CreateCall(IntrFn, {Args});
      EVI->replaceAllUsesWith(NewEVI);
      EVI->eraseFromParent();
      I = NewEVI;
    } else if (auto *IVI = dyn_cast<InsertValueInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F.getParent(), Intrinsic::spv_insertv,
          {IVI->getInsertedValueOperand()->getType()});
      std::vector<Value *> Args;
      for (auto &Op : IVI->operands())
        Args.push_back(Op);
      for (auto &Op : IVI->indices()) {
        Args.push_back(B.getInt32(Op));
      }
      NewIntr = B.CreateCall(IntrFn, {Args});
    }

    if (NewIntr) {
      replaceMemInstrUses(I, NewIntr);
      I = NewIntr;
    }

    if (I->getType()->isVoidTy())
      continue;
    B.SetInsertPoint(I->getNextNode());
    auto *Ty = I->getType();
    auto *TyFn = Intrinsic::getDeclaration(F.getParent(),
                                             Intrinsic::spv_assign_type, {Ty});
    auto *TyMD = MDNode::get(F.getContext(), ValueAsMetadata::getConstant(Constant::getNullValue(Ty)));
    auto *VMD = MetadataAsValue::get(
                         F.getContext(), TyMD);
    B.CreateCall(TyFn, {I, VMD});
    for (const auto &Op : I->operands()) {
      if (isa<ConstantPointerNull>(
              Op)) { // || isa<ConstantAggregateZero>(Op)) {
        B.SetInsertPoint(I->getPrevNode());
        auto *CTy = Op->getType();
        auto *CTyFn = Intrinsic::getDeclaration(
            F.getParent(), Intrinsic::spv_assign_type, {CTy});
        auto *CTyMD =
            MDNode::get(F.getContext(), ValueAsMetadata::getConstant(Op));
        auto *CVMD = MetadataAsValue::get(F.getContext(), CTyMD);
        B.CreateCall(CTyFn, {Op, CVMD});
      }
    }
    // errs() << *Call << "\n";
  }
  return true;
}

FunctionPass *llvm::createSPIRVTypeAssignerPass(SPIRVTargetMachine *TM) {
  return new SPIRVTypeAssigner(TM);
}
