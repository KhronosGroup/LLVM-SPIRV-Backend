#include "SPIRV.h"
#include "SPIRVSubtarget.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <map>

// This pass fixes calls to OCL builtins that accept vector arguments
// and one of them is actually a scalar splat
// Almost all code of this pass is taken from SPIRV-LLVM translator

using namespace llvm;

namespace llvm {
void initializeSPIRVOCLRegularizerPass(PassRegistry &);
}

namespace {

struct SPIRVOCLRegularizer : public FunctionPass,
                             InstVisitor<SPIRVOCLRegularizer> {
  std::map<Function *, Function *> Old2NewFuncs;

public:
  static char ID;
  SPIRVOCLRegularizer() : FunctionPass(ID) {
    initializeSPIRVOCLRegularizerPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "SPIRV OCL Regularizer"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    FunctionPass::getAnalysisUsage(AU);
  }
  void visitCallInst(CallInst &CI);

private:
  void visitCallScalToVec(CallInst *CI, StringRef MangledName,
                          StringRef DemangledName);
};

} // namespace

char SPIRVOCLRegularizer::ID = 0;

INITIALIZE_PASS(SPIRVOCLRegularizer, "spirv-ocl-regularizer",
                "SPIRV OpenCL Regularizer", false, false)

void SPIRVOCLRegularizer::visitCallInst(CallInst &CI) {
  auto F = CI.getCalledFunction();
  if (!F)
    return;

  auto MangledName = F->getName();
  size_t n;
  int status;
  char *NameStr = itaniumDemangle(F->getName().data(), nullptr, &n, &status);
  StringRef DemangledName(NameStr);

  // TODO: add support for other builtins
  if (DemangledName.startswith("fmin") || DemangledName.startswith("fmax") ||
      DemangledName.startswith("min") || DemangledName.startswith("max"))
    visitCallScalToVec(&CI, MangledName, DemangledName);
  free(NameStr);
}

void SPIRVOCLRegularizer::visitCallScalToVec(CallInst *CI,
                                             StringRef MangledName,
                                             StringRef DemangledName) {
  // Check if all arguments have the same type - it's simple case.
  auto Uniform = true;
  auto IsArg0Vector = isa<VectorType>(CI->getOperand(0)->getType());
  for (unsigned I = 1, E = CI->arg_size(); Uniform && (I != E); ++I) {
    Uniform = isa<VectorType>(CI->getOperand(I)->getType()) == IsArg0Vector;
  }
  if (Uniform) {
    // visitCallBuiltinSimple(CI, MangledName, DemangledName);
    return;
  }

  std::vector<unsigned int> VecPos;
  std::vector<unsigned int> ScalarPos;
  VecPos.push_back(0);
  ScalarPos.push_back(1);

  auto *OldF = CI->getCalledFunction();
  Function *NewF = nullptr;

  if (!Old2NewFuncs.count(OldF)) {
    AttributeList Attrs = CI->getCalledFunction()->getAttributes();
    std::vector<Type *> ArgTypes(VecPos.size() + ScalarPos.size());
    for (auto I : VecPos)
      ArgTypes[I] = OldF->getArg(I)->getType();
    for (auto I : ScalarPos)
      ArgTypes[I] = CI->getOperand(VecPos[0])->getType();
    auto *NewFTy =
        FunctionType::get(OldF->getReturnType(), ArgTypes, OldF->isVarArg());
    NewF = Function::Create(NewFTy, OldF->getLinkage(), OldF->getName(),
                            *OldF->getParent());

    ValueToValueMapTy VMap;
    auto NewFArgIt = NewF->arg_begin();
    for (auto &Arg : OldF->args()) {
      auto ArgName = Arg.getName();
      NewFArgIt->setName(ArgName);
      VMap[&Arg] = &(*NewFArgIt++);
    }
    SmallVector<ReturnInst *, 8> Returns;

    CloneFunctionInto(NewF, OldF, VMap,
                      CloneFunctionChangeType::LocalChangesOnly, Returns);

    // NewF->takeName(OldF);
    NewF->setAttributes(Attrs);
    Old2NewFuncs[OldF] = NewF;
  } else
    NewF = Old2NewFuncs.at(OldF);
  assert(NewF);

  std::vector<Value *> Args(VecPos.size() + ScalarPos.size());
  for (auto I : VecPos) {
    Args[I] = CI->getOperand(I);
  }
  auto VecElemCount =
      cast<VectorType>(CI->getOperand(VecPos[0])->getType())->getElementCount();
  for (auto I : ScalarPos) {
    Instruction *Inst = InsertElementInst::Create(
        UndefValue::get(CI->getOperand(VecPos[0])->getType()),
        CI->getOperand(I),
        ConstantInt::get(IntegerType::get(CI->getContext(), 32), 0), "", CI);
    Value *NewVec = new ShuffleVectorInst(
        Inst, UndefValue::get(CI->getOperand(VecPos[0])->getType()),
        ConstantVector::getSplat(
            VecElemCount,
            ConstantInt::get(IntegerType::get(CI->getContext(), 32), 0)),
        "", CI);

    CI->setOperand(I, NewVec);
  }
  CI->replaceUsesOfWith(OldF, NewF);
  CI->mutateFunctionType(NewF->getFunctionType());
}

bool SPIRVOCLRegularizer::runOnFunction(Function &F) {
  visit(F);
  for (auto &OldNew : Old2NewFuncs) {
    auto *OldF = OldNew.first;
    auto *NewF = OldNew.second;
    NewF->takeName(OldF);
    OldF->eraseFromParent();
  }
  return true;
}

FunctionPass *llvm::createSPIRVOCLRegularizerPass() {
  return new SPIRVOCLRegularizer();
}
