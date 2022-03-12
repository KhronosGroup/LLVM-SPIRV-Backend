#include "SPIRV.h"
#include "SPIRVUtils.h"
#include "SPIRVTargetMachine.h"

#include "llvm/IR/IntrinsicsSPIRV.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <queue>
#include <unordered_set>

// This pass performs the following legalization on LLVM IR level
// required for the following translation to SPIR-V:
// - modifies function signatures containing aggregate arguments
//   and/or return value
// - replaces direct usages of aggregate constants with target-specific
//   intrinsics
// - replaces aggregates-related instructions (extract/insert, ld/st, etc)
//   with a target-specific intrinsics
// - emits intrinsics for the global variable initializers since IRTranslator
//   doesn't handle them and it's not very convenient to translate them
//   ourselves
// - emits intrinsics to keep track of the string names assigned to the values
// - emits intrinsics to keep track of constants (this is necessary to have an
//   LLVM IR constant after the IRTranslation is completed) for their further
//   deduplication
// - emits intrinsics to keep track of original LLVM types of the values
//   to be able to emit proper SPIR-V types eventually
//
// NOTE: this pass is a module-level one due to the necessity to modify
// GVs/functions
// TODO: consider splitting this pass into separate ones
// TODO: consider removing spv.track.constant in favor of spv.assign.type

using namespace llvm;

namespace llvm {
void initializeSPIRVPreTranslationLegalizerPass(PassRegistry &);
}

namespace {

class SPIRVPreTranslationLegalizer : public ModulePass {
  SPIRVTargetMachine *TM = nullptr;

  std::unordered_map<Instruction *, Constant *> AggrConsts;

  Function *processFunctionSignature(Function *F);
  void preprocessCompositeConstants(IRBuilder<> &B, Function *F);

public:
  static char ID;
  SPIRVPreTranslationLegalizer() : ModulePass(ID) {
    initializeSPIRVPreTranslationLegalizerPass(
        *PassRegistry::getPassRegistry());
  }
  SPIRVPreTranslationLegalizer(SPIRVTargetMachine *_TM)
      : ModulePass(ID), TM(_TM) {
    initializeSPIRVPreTranslationLegalizerPass(
        *PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function *F);
  bool runOnModule(Module &M) override;

  StringRef getPassName() const override { return "SPIRV Pre-GISel Legalizer"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    ModulePass::getAnalysisUsage(AU);
  }
};

} // namespace

char SPIRVPreTranslationLegalizer::ID = 0;

INITIALIZE_PASS(SPIRVPreTranslationLegalizer, "pre-gisel-legalizer",
                "SPIRV Pre-GISel Legalizer", false, false)

static inline bool isAssignTypeInstr(const Instruction *I) {
  return isa<IntrinsicInst>(I) &&
         cast<IntrinsicInst>(I)->getIntrinsicID() == Intrinsic::spv_assign_type;
}

static bool isMemInstrToReplace(Instruction *I) {
  return isa<StoreInst>(I) || isa<LoadInst>(I) || isa<InsertValueInst>(I) ||
         isa<ExtractValueInst>(I);
}

static void replaceMemInstrUses(Instruction *Old, Instruction *New, IRBuilder<> &B) {
  while (!Old->user_empty()) {
    auto *U = Old->user_back();
    if(isMemInstrToReplace(U) || isa<ReturnInst>(U))
      U->replaceUsesOfWith(Old, New);
    else if (isAssignTypeInstr(U)) {
      auto *TyFn = Intrinsic::getDeclaration(Old->getFunction()->getParent(),
                                             Intrinsic::spv_assign_type, {New->getType()});
      B.SetInsertPoint(U);
      B.CreateCall(TyFn, {New, U->getOperand(1)});

      U->eraseFromParent();
    } else
      llvm_unreachable("illegal aggregate intrinsic user");
  }
  Old->eraseFromParent();
}

Function *
SPIRVPreTranslationLegalizer::processFunctionSignature(Function *F) {
  IRBuilder<> B(F->getContext());

  bool IsRetAggr = F->getReturnType()->isAggregateType();
  bool HasAggrArg =
      std::any_of(F->arg_begin(), F->arg_end(), [](Argument &Arg) {
        return Arg.getType()->isAggregateType();
      });
  bool DoClone = IsRetAggr || HasAggrArg;
  if (!DoClone)
    return F;
  SmallVector<std::pair<int, Type *>, 4> ChangedTypes;
  auto *RetType = IsRetAggr ? B.getInt32Ty() : F->getReturnType();
  if (IsRetAggr)
    ChangedTypes.push_back(std::pair<int, Type *>(-1, F->getReturnType()));
  SmallVector<Type *, 4> ArgTypes;
  for (const auto &Arg : F->args()) {
    if (Arg.getType()->isAggregateType() || Arg.getType()->isVectorTy()) {
      ArgTypes.push_back(B.getInt32Ty());
      ChangedTypes.push_back(
          std::pair<int, Type *>(Arg.getArgNo(), Arg.getType()));
    } else
      ArgTypes.push_back(Arg.getType());
  }
  auto *NewFTy =
      FunctionType::get(RetType, ArgTypes, F->getFunctionType()->isVarArg());
  auto *NewF =
      Function::Create(NewFTy, F->getLinkage(), F->getName(), *F->getParent());

  ValueToValueMapTy VMap;
  auto NewFArgIt = NewF->arg_begin();
  for (auto &Arg : F->args()) {
    auto ArgName = Arg.getName();
    NewFArgIt->setName(ArgName);
    VMap[&Arg] = &(*NewFArgIt++);
  }
  SmallVector<ReturnInst *, 8> Returns;

  CloneFunctionInto(NewF, F, VMap, CloneFunctionChangeType::LocalChangesOnly,
                    Returns);
  NewF->takeName(F);

  auto *FuncMD = F->getParent()->getOrInsertNamedMetadata("spv.cloned_funcs");
  SmallVector<Metadata *, 2> MDArgs;
  MDArgs.push_back(MDString::get(B.getContext(), NewF->getName()));
  for (auto &ChangedTyP : ChangedTypes)
    MDArgs.push_back(MDNode::get(
        B.getContext(),
        {ConstantAsMetadata::get(B.getInt32(ChangedTyP.first)),
         ValueAsMetadata::get(Constant::getNullValue(ChangedTyP.second))}));
  auto *ThisFuncMD = MDNode::get(B.getContext(), MDArgs);
  FuncMD->addOperand(ThisFuncMD);

  for (auto *U : F->users()) {
    if (auto *CI = dyn_cast<CallInst>(U))
      CI->mutateFunctionType(NewF->getFunctionType());
    U->replaceUsesOfWith(F, NewF);
  }

  return NewF;
}

static bool isAggrToReplace(const Value *V) {
  return isa<ConstantAggregate>(V) || isa<ConstantDataArray>(V)
         // || isa<ConstantDataVector>(V)
         || (isa<ConstantAggregateZero>(V) && !V->getType()->isVectorTy());
}

static void setInsertPointSkippingPhis(IRBuilder<> &B, Instruction *I) {
  while (isa<PHINode>(I))
    I = I->getNextNode();
  B.SetInsertPoint(I);
}

static bool requireAssignType(Instruction* I) {
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

void SPIRVPreTranslationLegalizer::preprocessCompositeConstants(IRBuilder<> &B,
                                                                Function *F) {
  std::queue<Instruction *> Worklist;
  for (auto &I : instructions(F))
    Worklist.push(&I);

  while (!Worklist.empty()) {
    auto *I = Worklist.front();
    assert(I);
    bool KeepInst = false;
    for (const auto &Op : I->operands()) {
      auto BuildCompositeIntrinsic = [&KeepInst, &B, &Worklist, &I, &Op,
                                      this](Constant *AggrC,
                                            std::vector<Value *> &Args) {
        auto *IntrFn = Intrinsic::getDeclaration(
            I->getFunction()->getParent(), Intrinsic::spv_const_composite, {});
        B.SetInsertPoint(I);
        auto *ConstCompI = B.CreateCall(IntrFn, {Args});
        Worklist.push(ConstCompI);

        I->replaceUsesOfWith(Op, ConstCompI);
        KeepInst = true;
        AggrConsts[ConstCompI] = AggrC;
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

bool SPIRVPreTranslationLegalizer::runOnModule(Module &M) {
  std::vector<Function *> FuncsWorklist;
  bool Changed = false;
  for (auto &F : M)
    FuncsWorklist.push_back(&F);

  for (auto *Func : FuncsWorklist) {
    auto *F = processFunctionSignature(Func);

    bool CreatedNewF = F != Func;

    if (Func->isDeclaration()) {
      Changed |= CreatedNewF;
      continue;
    }

    if (CreatedNewF)
      Func->eraseFromParent();
  }

  for (auto &F : M)
    Changed |= runOnFunction(&F);

  return Changed;
}

bool SPIRVPreTranslationLegalizer::runOnFunction(Function *F) {
  if (F->isDeclaration())
    return false;
  IRBuilder<> B(F->getContext());
  AggrConsts.clear();

  B.SetInsertPoint(&F->getEntryBlock().front());

  for (auto &GV : F->getParent()->globals()) {
    if (GV.hasInitializer() && !isa<UndefValue>(GV.getInitializer())) {
      auto *Init = GV.getInitializer();
      Type *Ty = isAggrToReplace(Init) ? B.getInt32Ty() : Init->getType();
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_init_global, {GV.getType(), Ty});
      auto *InitInst = B.CreateCall(
          IntrFn, {&GV, isAggrToReplace(Init) ? B.getInt32(1) : Init});
      InitInst->setArgOperand(1, Init);
    }
    if ((!GV.hasInitializer() || isa<UndefValue>(GV.getInitializer())) &&
        GV.getNumUses() == 0) {
      auto *CTyFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_unref_global, GV.getType());
      B.CreateCall(CTyFn, &GV);
    }
  }

  preprocessCompositeConstants(B, F);
  std::vector<Instruction *> Worklist;
  for (auto &I : instructions(F))
    Worklist.push_back(&I);

  for (auto &I : Worklist) {
    auto *Ty = I->getType();
    if (!Ty->isVoidTy() && requireAssignType(I)) {
      setInsertPointSkippingPhis(B, I->getNextNode());
      auto *TyFn = Intrinsic::getDeclaration(F->getParent(),
                                             Intrinsic::spv_assign_type, {Ty});
      auto *TypeToAssign = Ty;
      if (auto *II = dyn_cast<IntrinsicInst>(I)) {
        if (II->getIntrinsicID() == Intrinsic::spv_const_composite)
          TypeToAssign = AggrConsts.at(II)->getType();
      }
      auto *TyMD = MDNode::get(
          F->getContext(),
          ValueAsMetadata::getConstant(Constant::getNullValue(TypeToAssign)));
      auto *VMD = MetadataAsValue::get(F->getContext(), TyMD);
      B.CreateCall(TyFn, {I, VMD});
    }
    for (const auto &Op : I->operands()) {
      if (isa<ConstantPointerNull>(Op) || isa<UndefValue>(Op)) {
        B.SetInsertPoint(I);
        auto *CTy = Op->getType();
        auto *CTyFn = Intrinsic::getDeclaration(
            F->getParent(), Intrinsic::spv_assign_type, {CTy});
        auto *CTyMD =
            MDNode::get(F->getContext(), ValueAsMetadata::getConstant(Op));
        auto *CVMD = MetadataAsValue::get(F->getContext(), CTyMD);
        B.CreateCall(CTyFn, {Op, CVMD});
      }
    }
  }

  for (auto *I : Worklist) {
    bool TrackConstants = true;
    if (!I->getType()->isVoidTy() || isa<StoreInst>(I))
      B.SetInsertPoint(I->getNextNode());
    Instruction *NewIntr = nullptr;
    if (auto *Gep = dyn_cast<GetElementPtrInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_gep,
          {Gep->getType(), Gep->getOperand(0)->getType()});
      std::vector<Value *> Args;
      Args.push_back(B.getInt1(Gep->isInBounds()));
      for (auto &Op : Gep->operands())
        Args.push_back(Op);
      auto *NewGep = B.CreateCall(IntrFn, {Args});
      Gep->replaceAllUsesWith(NewGep);
      Gep->eraseFromParent();
      I = NewGep;
    } else if (isa<StoreInst>(I) &&
               cast<PointerType>(I->getOperand(1)->getType())
                   ->getElementType()
                   ->isAggregateType()) {
      TrackConstants = false;
      auto *SI = cast<StoreInst>(I);
      auto *IntrFn =
          Intrinsic::getDeclaration(F->getParent(), Intrinsic::spv_store,
                                    {SI->getPointerOperand()->getType()});
      const auto *TLI = TM->getSubtargetImpl()->getTargetLowering();
      MachineMemOperand::Flags Flags =
          TLI->getStoreMemOperandFlags(*SI, F->getParent()->getDataLayout());
      I = B.CreateCall(IntrFn,
                       {SI->getValueOperand(), SI->getPointerOperand(),
                        B.getInt16(Flags), B.getInt8(SI->getAlignment())});
      SI->eraseFromParent();
    } else if (isa<LoadInst>(I) && I->getType()->isAggregateType()) {
      TrackConstants = false;
      auto *LI = cast<LoadInst>(I);
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_load, {LI->getOperand(0)->getType()});
      const auto *TLI = TM->getSubtargetImpl()->getTargetLowering();
      MachineMemOperand::Flags Flags =
          TLI->getLoadMemOperandFlags(*LI, F->getParent()->getDataLayout());
      NewIntr =
          B.CreateCall(IntrFn, {LI->getPointerOperand(), B.getInt16(Flags),
                                B.getInt8(LI->getAlignment())});
    } else if (auto *EVI = dyn_cast<ExtractValueInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_extractv, {EVI->getType()});
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
          F->getParent(), Intrinsic::spv_insertv,
          {IVI->getInsertedValueOperand()->getType()});
      std::vector<Value *> Args;
      for (auto &Op : IVI->operands()) {
        if (isa<UndefValue>(Op))
          Args.push_back(UndefValue::get(B.getInt32Ty()));
        else
          Args.push_back(Op);
      }
      for (auto &Op : IVI->indices()) {
        Args.push_back(B.getInt32(Op));
      }
      NewIntr = B.CreateCall(IntrFn, {Args});
    } else if (auto *EEI = dyn_cast<ExtractElementInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_extractelt, {EEI->getType(), EEI->getVectorOperandType(), EEI->getIndexOperand()->getType()});
      std::vector<Value *> Args;
      Args.push_back(EEI->getVectorOperand());
      Args.push_back(EEI->getIndexOperand());
      auto *NewEEI = B.CreateCall(IntrFn, {Args});
      std::string InstName = I->hasName() ? I->getName().str() : "";
      EEI->replaceAllUsesWith(NewEEI);
      EEI->eraseFromParent();
      I = NewEEI;
      I->setName(InstName);
    } else if (auto *IEI = dyn_cast<InsertElementInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_insertelt,
          {IEI->getType(), IEI->getOperand(0)->getType(), IEI->getOperand(1)->getType(), IEI->getOperand(2)->getType()});
      std::vector<Value *> Args;
      for (auto &Op : IEI->operands())
        Args.push_back(Op);
      auto *NewIEI = B.CreateCall(IntrFn, {Args});
      std::string InstName = I->hasName() ? I->getName().str() : "";
      IEI->replaceAllUsesWith(NewIEI);
      IEI->eraseFromParent();
      I = NewIEI;
      I->setName(InstName);
    } else if (auto *BCI = dyn_cast<BitCastInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_bitcast,
          {BCI->getType(), BCI->getOperand(0)->getType()});
      std::vector<Value *> Args;
      for (auto &Op : BCI->operands())
        Args.push_back(Op);
      auto *NewBCI = B.CreateCall(IntrFn, {Args});
      std::string InstName = I->hasName() ? I->getName().str() : "";
      BCI->replaceAllUsesWith(NewBCI);
      BCI->eraseFromParent();
      I = NewBCI;
      I->setName(InstName);
    } else if (auto *SI = dyn_cast<SwitchInst>(I)) {
      auto *IntrFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_switch,
          {SI->getOperand(0)->getType()});
      std::vector<Value *> Args;
      for (auto &Op : SI->operands())
        if (Op.get()->getType()->isSized())
          Args.push_back(Op);
      B.CreateCall(IntrFn, {Args});
    } else if (isa<AllocaInst>(I))
      TrackConstants = false;

    if (NewIntr) {
      replaceMemInstrUses(I, NewIntr, B);
      I = NewIntr;
    }

    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      if (II->getIntrinsicID() == Intrinsic::spv_const_composite) {
        if (TrackConstants) {
          auto *Const = AggrConsts.at(I);
          B.SetInsertPoint(I->getNextNode());
          auto *CTyFn = Intrinsic::getDeclaration(
              F->getParent(), Intrinsic::spv_track_constant,
              {B.getInt32Ty(), B.getInt32Ty()});
          auto *CTyMD =
              MDNode::get(F->getContext(), ValueAsMetadata::getConstant(Const));
          auto *CVMD = MetadataAsValue::get(F->getContext(), CTyMD);
          auto *NewOp = B.CreateCall(CTyFn, {I, CVMD});
          I->replaceAllUsesWith(NewOp);
          NewOp->setArgOperand(0, I);
        }
      }
    }
    for (const auto &Op : I->operands()) {
      if ((isa<ConstantAggregateZero>(Op) && Op->getType()->isVectorTy()) ||
           isa<PHINode>(I) || isa<SwitchInst>(I))
        TrackConstants = false;
      if (isa<ConstantData>(Op) && TrackConstants) {
        auto OpNo = Op.getOperandNo();
        if (isa<IntrinsicInst>(I) &&
            ((cast<IntrinsicInst>(I)->getIntrinsicID() == Intrinsic::spv_gep &&
              OpNo == 0) ||
             cast<CallBase>(I)->paramHasAttr(OpNo, Attribute::ImmArg)))
          continue;
        B.SetInsertPoint(I);
        auto *CTy = Op->getType();
        auto *CTyFn = Intrinsic::getDeclaration(
            F->getParent(), Intrinsic::spv_track_constant, {CTy, CTy});
        auto *CTyMD =
            MDNode::get(F->getContext(), ValueAsMetadata::getConstant(Op));
        auto *CVMD = MetadataAsValue::get(F->getContext(), CTyMD);
        auto *NewOp = B.CreateCall(CTyFn, {Op, CVMD});
        I->setOperand(OpNo, NewOp);
      }
    }
    if (I->hasName()) {
      setInsertPointSkippingPhis(B, I->getNextNode());
      auto *NameFn = Intrinsic::getDeclaration(
          F->getParent(), Intrinsic::spv_assign_name, {I->getType()});
      std::vector<Value *> Args = {I};
      addStringImm(I->getName(), B, Args);
      B.CreateCall(NameFn, Args);
    }
  }
  return true;
}

ModulePass *
llvm::createSPIRVPreTranslationLegalizerPass(SPIRVTargetMachine *TM) {
  return new SPIRVPreTranslationLegalizer(TM);
}
