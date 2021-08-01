#include "SPIRV.h"
#include "SPIRVSubtarget.h"

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
public:
  static char ID;
  SPIRVTypeAssigner() : FunctionPass(ID) {
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

bool SPIRVTypeAssigner::runOnFunction(Function &F) {
  IRBuilder<> B(&F.getEntryBlock().front());
  std::vector<Instruction *> Worklist;
  for (auto &I : instructions(F)) {
    if (!I.getType()->isVoidTy())
      Worklist.push_back(&I);
  }
  for (auto *I : Worklist) {
    B.SetInsertPoint(I->getNextNode());
    auto *Ty = I->getType();
    auto ValueFn = Intrinsic::getDeclaration(F.getParent(),
                                             Intrinsic::spv_assign_type, {Ty});
    auto *NewF = Function::Create(
        FunctionType::get(Type::getVoidTy(F.getContext()), {Ty}),
        GlobalValue::LinkageTypes::PrivateLinkage, "assign_type",
        *F.getParent());
    auto *TyMD = MDNode::get(F.getContext(), ValueAsMetadata::getConstant(Constant::getNullValue(Ty)));
    auto *VMD = MetadataAsValue::get(
                         F.getContext(), TyMD);
    errs() << "Building assign_type w/ MDs " << TyMD << "[" << isa<MDNode>(TyMD) << "], " << VMD << "\n";
    errs() << *(B.CreateCall(ValueFn,
                 {I, VMD})) << "\n";
  }
  return true;
}

FunctionPass *llvm::createSPIRVTypeAssignerPass() {
  return new SPIRVTypeAssigner();
}
