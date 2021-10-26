#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVTARGETTRANSFORMINFO_H

#include "SPIRV.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"

namespace llvm {
class SPIRVTTIImpl : public BasicTTIImplBase<SPIRVTTIImpl> {
  using BaseT = BasicTTIImplBase<SPIRVTTIImpl>;
  using TTI = TargetTransformInfo;

  friend BaseT;

  const SPIRVSubtarget *ST;
  const TargetLoweringBase *TLI;

  const TargetSubtargetInfo *getST() const { return ST; }
  const TargetLoweringBase *getTLI() const { return TLI; }

public:
  explicit SPIRVTTIImpl(const SPIRVTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getParent()->getDataLayout()),
        ST(static_cast<const SPIRVSubtarget *>(TM->getSubtargetImpl(F))),
        TLI(ST->getTargetLowering()) {}
};

} // namespace llvm

#endif
