//===-- SPIRVModifySignatures.cpp - modify function signatures --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass modifies function signatures containing aggregate arguments
// and/or return value.
//
// NOTE: this pass is a module-level one due to the necessity to modify
// GVs/functions.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVUtils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

namespace llvm {
void initializeSPIRVPreTranslationLegalizerPass(PassRegistry &);
}

namespace {

class SPIRVPreTranslationLegalizer : public ModulePass {
  Function *processFunctionSignature(Function *F);

public:
  static char ID;
  SPIRVPreTranslationLegalizer() : ModulePass(ID) {
    initializeSPIRVPreTranslationLegalizerPass(
        *PassRegistry::getPassRegistry());
  }

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

Function *SPIRVPreTranslationLegalizer::processFunctionSignature(Function *F) {
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
    if (Arg.getType()->isAggregateType()) {
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

  for (auto *U : make_early_inc_range(F->users())) {
    if (auto *CI = dyn_cast<CallInst>(U))
      CI->mutateFunctionType(NewF->getFunctionType());
    U->replaceUsesOfWith(F, NewF);
  }
  return NewF;
}

static Function *getOrCreateFunction(Module *M, Type *RetTy,
                                     ArrayRef<Type *> ArgTypes,
                                     StringRef Name) {
  FunctionType *FT = FunctionType::get(RetTy, ArgTypes, false);
  Function *F = M->getFunction(Name);
  if (!F || F->getFunctionType() != FT) {
    auto NewF = Function::Create(FT, GlobalValue::ExternalLinkage, Name, M);
    if (F)
      NewF->setDSOLocal(F->isDSOLocal());
    F = NewF;
    F->setCallingConv(CallingConv::SPIR_FUNC);
  }
  return F;
}

// Add a wrapper around the kernel function to act as an entry point.
static void addKernelEntryPointWrapper(Module *M, Function *F) {
  F->setCallingConv(CallingConv::SPIR_FUNC);
  FunctionType *FType = F->getFunctionType();
  std::string WrapName =
      "__spirv_entry_" + static_cast<std::string>(F->getName());
  Function *WrapFn =
      getOrCreateFunction(M, F->getReturnType(), FType->params(), WrapName);

  auto *CallBB = BasicBlock::Create(M->getContext(), "", WrapFn);
  IRBuilder<> Builder(CallBB);

  Function::arg_iterator DestI = WrapFn->arg_begin();
  for (const Argument &I : F->args()) {
    DestI->setName(I.getName());
    DestI++;
  }
  SmallVector<Value *, 1> Args;
  for (Argument &I : WrapFn->args())
    Args.emplace_back(&I);

  auto *CI = CallInst::Create(F, ArrayRef<Value *>(Args), "", CallBB);
  CI->setCallingConv(F->getCallingConv());
  CI->setAttributes(F->getAttributes());

  // Copy over all the metadata excepting debug info.
  // TODO: removed some metadata from F if it's necessery.
  SmallVector<std::pair<unsigned, MDNode *>> MDs;
  F->getAllMetadata(MDs);
  WrapFn->setAttributes(F->getAttributes());
  for (auto MD = MDs.begin(), End = MDs.end(); MD != End; ++MD) {
    if (MD->first != LLVMContext::MD_dbg)
      WrapFn->addMetadata(MD->first, *MD->second);
  }
  WrapFn->setCallingConv(CallingConv::SPIR_KERNEL);
  WrapFn->setLinkage(llvm::GlobalValue::InternalLinkage);

  Builder.CreateRet(F->getReturnType()->isVoidTy() ? nullptr : CI);

  // Have to find the spir-v metadata for execution mode and transfer it to
  // the wrapper.
  auto Node = M->getNamedMetadata("spirv.ExecutionMode");
  if (Node) {
    for (unsigned i = 0; i < Node->getNumOperands(); i++) {
      MDNode *MDN = cast<MDNode>(Node->getOperand(i));
      const MDOperand &MDOp = MDN->getOperand(0);
      auto *CMeta = dyn_cast<ConstantAsMetadata>(MDOp);
      Function *MDF = dyn_cast<Function>(CMeta->getValue());
      if (MDF == F)
        MDN->replaceOperandWith(0, ValueAsMetadata::get(WrapFn));
    }
  }
}

bool SPIRVPreTranslationLegalizer::runOnModule(Module &M) {
  std::vector<Function *> FuncsWorklist;
  bool Changed = false;
  for (auto &F : M)
    FuncsWorklist.push_back(&F);

  for (auto *Func : FuncsWorklist) {
    if (isKernel(Func))
      addKernelEntryPointWrapper(&M, Func);

    auto *F = processFunctionSignature(Func);

    bool CreatedNewF = F != Func;

    if (Func->isDeclaration()) {
      Changed |= CreatedNewF;
      continue;
    }

    if (CreatedNewF)
      Func->eraseFromParent();
  }

  return Changed;
}

ModulePass *llvm::createSPIRVPreTranslationLegalizerPass() {
  return new SPIRVPreTranslationLegalizer();
}
