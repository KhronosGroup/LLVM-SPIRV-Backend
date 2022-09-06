//===-- SPIRVLowerConstExpr.cpp - Regularize LLVM IR for SPIR-V --- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass implements regularization of LLVM IR for SPIR-V.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVTargetMachine.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"

#include <list>

#define DEBUG_TYPE "spv-lower-const-expr"

using namespace llvm;

namespace llvm {
void initializeSPIRVLowerConstExprLegacyPass(PassRegistry &);
} // namespace llvm

class SPIRVLowerConstExprBase {
public:
  SPIRVLowerConstExprBase() {}
  bool runLowerConstExpr(Function &F);
};

class SPIRVLowerConstExprPass
    : public llvm::PassInfoMixin<SPIRVLowerConstExprPass>,
      public SPIRVLowerConstExprBase {
public:
  llvm::PreservedAnalyses run(Function &F) {
    return runLowerConstExpr(F) ? llvm::PreservedAnalyses::none()
                                : llvm::PreservedAnalyses::all();
  }
};

namespace {
class SPIRVLowerConstExprLegacy : public FunctionPass,
                                  public SPIRVLowerConstExprBase {
public:
  static char ID;
  SPIRVLowerConstExprLegacy() : FunctionPass(ID) {
    initializeSPIRVLowerConstExprLegacyPass(*PassRegistry::getPassRegistry());
  }
  bool runOnFunction(Function &F) override { return runLowerConstExpr(F); };
};
} // namespace

char SPIRVLowerConstExprLegacy::ID = 0;

INITIALIZE_PASS(SPIRVLowerConstExprLegacy, DEBUG_TYPE,
                "Regularize LLVM IR for SPIR-V", false, false)

/// Since SPIR-V cannot represent constant expression, constant expressions
/// in LLVM IR need to be lowered to instructions. For each function,
/// the constant expressions used by instructions of the function are replaced
/// by instructions placed in the entry block since it dominates all other BBs.
/// Each constant expression only needs to be lowered once in each function
/// and all uses of it by instructions in that function are replaced by
/// one instruction.
/// TODO: remove redundant instructions for common subexpression.
bool SPIRVLowerConstExprBase::runLowerConstExpr(Function &F) {
  LLVMContext &Ctx = F.getContext();
  std::list<Instruction *> WorkList;
  for (auto &II : instructions(F))
    WorkList.push_back(&II);

  auto FBegin = F.begin();
  while (!WorkList.empty()) {
    Instruction *II = WorkList.front();

    auto LowerOp = [&II, &FBegin, &F](Value *V) -> Value * {
      if (isa<Function>(V))
        return V;
      auto *CE = cast<ConstantExpr>(V);
      LLVM_DEBUG(dbgs() << "[lowerConstantExpressions] " << *CE);
      auto ReplInst = CE->getAsInstruction();
      auto InsPoint = II->getParent() == &*FBegin ? II : &FBegin->back();
      ReplInst->insertBefore(InsPoint);
      LLVM_DEBUG(dbgs() << " -> " << *ReplInst << '\n');
      std::vector<Instruction *> Users;
      // Do not replace use during iteration of use. Do it in another loop.
      for (auto U : CE->users()) {
        LLVM_DEBUG(dbgs() << "[lowerConstantExpressions] Use: " << *U << '\n');
        auto InstUser = dyn_cast<Instruction>(U);
        // Only replace users in scope of current function.
        if (InstUser && InstUser->getParent()->getParent() == &F)
          Users.push_back(InstUser);
      }
      for (auto &User : Users) {
        if (ReplInst->getParent() == User->getParent() &&
            User->comesBefore(ReplInst))
          ReplInst->moveBefore(User);
        User->replaceUsesOfWith(CE, ReplInst);
      }
      return ReplInst;
    };

    WorkList.pop_front();
    auto LowerConstantVec = [&II, &LowerOp, &WorkList,
                             &Ctx](ConstantVector *Vec,
                                   unsigned NumOfOp) -> Value * {
      if (std::all_of(Vec->op_begin(), Vec->op_end(), [](Value *V) {
            return isa<ConstantExpr>(V) || isa<Function>(V);
          })) {
        // Expand a vector of constexprs and construct it back with
        // series of insertelement instructions.
        std::list<Value *> OpList;
        std::transform(Vec->op_begin(), Vec->op_end(),
                       std::back_inserter(OpList),
                       [LowerOp](Value *V) { return LowerOp(V); });
        Value *Repl = nullptr;
        unsigned Idx = 0;
        auto *PhiII = dyn_cast<PHINode>(II);
        Instruction *InsPoint =
            PhiII ? &PhiII->getIncomingBlock(NumOfOp)->back() : II;
        std::list<Instruction *> ReplList;
        for (auto V : OpList) {
          if (auto *Inst = dyn_cast<Instruction>(V))
            ReplList.push_back(Inst);
          Repl = InsertElementInst::Create(
              (Repl ? Repl : UndefValue::get(Vec->getType())), V,
              ConstantInt::get(Type::getInt32Ty(Ctx), Idx++), "", InsPoint);
        }
        WorkList.splice(WorkList.begin(), ReplList);
        return Repl;
      }
      return nullptr;
    };
    for (unsigned OI = 0, OE = II->getNumOperands(); OI != OE; ++OI) {
      auto *Op = II->getOperand(OI);
      if (auto *Vec = dyn_cast<ConstantVector>(Op)) {
        Value *ReplInst = LowerConstantVec(Vec, OI);
        if (ReplInst)
          II->replaceUsesOfWith(Op, ReplInst);
      } else if (auto CE = dyn_cast<ConstantExpr>(Op)) {
        WorkList.push_front(cast<Instruction>(LowerOp(CE)));
      } else if (auto MDAsVal = dyn_cast<MetadataAsValue>(Op)) {
        auto ConstMD = dyn_cast<ConstantAsMetadata>(MDAsVal->getMetadata());
        if (!ConstMD)
          continue;
        Constant *C = ConstMD->getValue();
        Value *ReplInst = nullptr;
        if (auto *Vec = dyn_cast<ConstantVector>(C))
          ReplInst = LowerConstantVec(Vec, OI);
        if (auto *CE = dyn_cast<ConstantExpr>(C))
          ReplInst = LowerOp(CE);
        if (!ReplInst)
          continue;
        Metadata *RepMD = ValueAsMetadata::get(ReplInst);
        Value *RepMDVal = MetadataAsValue::get(Ctx, RepMD);
        II->setOperand(OI, RepMDVal);
        WorkList.push_front(cast<Instruction>(ReplInst));
      }
    }
  }
  return true;
}

FunctionPass *llvm::createSPIRVLowerConstExprLegacyPass() {
  return new SPIRVLowerConstExprLegacy();
}
