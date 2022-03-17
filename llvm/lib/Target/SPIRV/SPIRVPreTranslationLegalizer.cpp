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

  return Changed;
}

ModulePass *
llvm::createSPIRVPreTranslationLegalizerPass() {
  return new SPIRVPreTranslationLegalizer();
}
