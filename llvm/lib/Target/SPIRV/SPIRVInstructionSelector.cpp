//===- SPIRVInstructionSelector.cpp ------------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the InstructionSelector class for SPIR-V
//
// TODO: Can some of this be moved to tablegen, or does the reliance on
// SPIRVTypeRegistry prevent this?
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVInstrInfo.h"
#include "SPIRVRegisterBankInfo.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVStrings.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVTypeRegistry.h"

#include "llvm/IR/IntrinsicsSPIRV.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelectorImpl.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "spirv-isel"

using namespace llvm;
namespace CL = OpenCL_std;
namespace GL = GLSL_std_450;

using ExtInstList = std::vector<std::pair<ExtInstSet, uint32_t>>;

namespace {

#define GET_GLOBALISEL_PREDICATE_BITSET
#include "SPIRVGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATE_BITSET

class SPIRVInstructionSelector : public InstructionSelector {
  const SPIRVSubtarget &STI;
  const SPIRVInstrInfo &TII;
  const SPIRVRegisterInfo &TRI;
  const SPIRVRegisterBankInfo &RBI;
  SPIRVTypeRegistry &TR;
  SPIRVGeneralDuplicatesTracker &DT;

public:
  SPIRVInstructionSelector(const SPIRVTargetMachine &TM,
                           const SPIRVSubtarget &ST,
                           const SPIRVRegisterBankInfo &RBI);

  // Common selection code. Instruction-specific selection occurs in spvSelect()
  bool select(MachineInstr &I) override;
  static const char *getName() { return DEBUG_TYPE; }

#define GET_GLOBALISEL_PREDICATES_DECL
#include "SPIRVGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_DECL

#define GET_GLOBALISEL_TEMPORARIES_DECL
#include "SPIRVGenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_DECL

private:
  // tblgen-erated 'select' implementation, used as the initial selector for
  // the patterns that don't require complex C++.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;

  // All instruction-specific selection that didn't happen in "select()".
  // Is basically a large Switch/Case delegating to all other select method.
  bool spvSelect(Register resVReg, const SPIRVType *resType,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectGlobalValue(Register resVReg, const MachineInstr &I,
                         MachineIRBuilder &MIRBuilder,
                         const MachineInstr *Init = nullptr) const;

  bool selectUnOp(Register resVReg, const SPIRVType *resType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                  unsigned newOpcode) const;

  bool selectLoad(Register resVReg, const SPIRVType *resType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectStore(const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectMemOperation(Register ResVReg, const MachineInstr &I,
                          MachineIRBuilder &MIRBuilder) const;

  bool selectAtomicRMW(Register resVReg, const SPIRVType *resType,
                       const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                       unsigned newOpcode) const;

  bool selectAtomicCmpXchg(Register resVReg, const SPIRVType *resType,
                           const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                           bool withSuccess) const;

  bool selectFence(const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectAddrSpaceCast(Register resVReg, const SPIRVType *resType,
                           const MachineInstr &I,
                           MachineIRBuilder &MIRBuilder) const;

#if 0
  bool selectOverflowOp(Register resVReg, const SPIRVType *resType,
                        const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                        unsigned newOpcode) const;
#endif

  bool selectBitreverse(Register resVReg, const SPIRVType *resType,
                        const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectConstVector(Register resVReg, const SPIRVType *resType,
                         const MachineInstr &I,
                         MachineIRBuilder &MIRBuilder) const;

  bool selectCmp(Register resVReg, const SPIRVType *resType,
                 unsigned scalarTypeOpcode, unsigned comparisonOpcode,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                 bool convertToUInt = false) const;

  bool selectICmp(Register resVReg, const SPIRVType *resType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;
  bool selectFCmp(Register resVReg, const SPIRVType *resType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  void renderImm32(MachineInstrBuilder &MIB, const MachineInstr &I,
                          int OpIdx) const;
  void renderFImm32(MachineInstrBuilder &MIB, const MachineInstr &I,
                          int OpIdx) const;

  bool selectConst(Register resVReg, const SPIRVType *resType, const APInt &imm,
                   MachineIRBuilder &MIRBuilder) const;

  bool selectExt(Register resVReg, const SPIRVType *resType,
                 const MachineInstr &I, bool isSigned,
                 MachineIRBuilder &MIRBuilder) const;

  bool selectTrunc(Register resVReg, const SPIRVType *resType,
                   const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectIntToBool(Register intReg, Register resVReg,
                       const SPIRVType *intTy, const SPIRVType *boolTy,
                       MachineIRBuilder &MIRBuilder) const;

  bool selectOpUndef(Register resVReg, const SPIRVType *resType,
                     MachineIRBuilder &MIRBuilder) const;
  bool selectIntrinsic(Register resVReg, const SPIRVType *resType,
                       const MachineInstr &I,
                       MachineIRBuilder &MIRBuilder) const;
  bool selectExtractVal(Register resVReg, const SPIRVType *resType,
                        const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;
  bool selectInsertVal(Register resVReg, const SPIRVType *resType,
                       const MachineInstr &I,
                       MachineIRBuilder &MIRBuilder) const;
  bool selectExtractElt(Register resVReg, const SPIRVType *resType,
                        const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;
  bool selectInsertElt(Register resVReg, const SPIRVType *resType,
                       const MachineInstr &I,
                       MachineIRBuilder &MIRBuilder) const;
  bool selectGEP(Register resVReg, const SPIRVType *resType,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectFrameIndex(Register resVReg, const SPIRVType *resType,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectBranch(const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;
  bool selectBranchCond(const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectPhi(Register resVReg, const SPIRVType *resType,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectExtInst(Register resVReg, const SPIRVType *resType,
                     const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                     CL::OpenCL_std clInst) const;
  bool selectExtInst(Register resVReg, const SPIRVType *resType,
                     const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                     OpenCL_std::OpenCL_std clInst,
                     GL::GLSL_std_450 glInst) const;

  bool selectExtInst(Register resVReg, const SPIRVType *resType,
                     const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                     const ExtInstList &extInsts) const;

  Register buildI32Constant(uint32_t val, MachineIRBuilder &MIRBuilder,
                            const SPIRVType *resType = nullptr) const;

  Register buildZerosVal(const SPIRVType *resType,
                         MachineIRBuilder &MIRBuilder) const;
  Register buildOnesVal(bool allOnes, const SPIRVType *resType,
                        MachineIRBuilder &MIRBuilder) const;
};

} // end anonymous namespace

#define GET_GLOBALISEL_IMPL
#include "SPIRVGenGlobalISel.inc"
#undef GET_GLOBALISEL_IMPL

SPIRVInstructionSelector::SPIRVInstructionSelector(
    const SPIRVTargetMachine &TM, const SPIRVSubtarget &ST,
    const SPIRVRegisterBankInfo &RBI)
    : InstructionSelector(), STI(ST), TII(*ST.getInstrInfo()),
      TRI(*ST.getRegisterInfo()), RBI(RBI), TR(*ST.getSPIRVTypeRegistry()),
      DT(*ST.getSPIRVDuplicatesTracker()),
#define GET_GLOBALISEL_PREDICATES_INIT
#include "SPIRVGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_INIT
#define GET_GLOBALISEL_TEMPORARIES_INIT
#include "SPIRVGenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_INIT
{
}

bool SPIRVInstructionSelector::select(MachineInstr &I) {
  assert(I.getParent() && "Instruction should be in a basic block!");
  assert(I.getParent()->getParent() && "Instruction should be in a function!");

  MachineBasicBlock &MBB = *I.getParent();
  MachineFunction &MF = *MBB.getParent();

  MachineIRBuilder MIRBuilder;
  MIRBuilder.setMF(MF);
  MIRBuilder.setMBB(MBB);
  MIRBuilder.setInstr(I);

  Register Opcode = I.getOpcode();

  // If it's not a GMIR instruction, we've selected it already.
  if (!isPreISelGenericOpcode(Opcode)) {
    if (Opcode == SPIRV::ASSIGN_TYPE) { // These pseudos aren't needed any more
      auto MRI = MIRBuilder.getMRI();
      auto *Def = MRI->getVRegDef(I.getOperand(1).getReg());
      if (isTypeFoldingSupported(Def->getOpcode())) {
        auto Res = selectImpl(I, *CoverageInfo);
        assert(Res || Def->getOpcode() == TargetOpcode::G_CONSTANT);
        if (Res)
          return Res;
      }
      MRI->replaceRegWith(I.getOperand(1).getReg(), I.getOperand(0).getReg());
      I.removeFromParent();
    } else if (I.getNumDefs() == 1) { // Make all vregs 32 bits (for SPIR-V IDs)
      MIRBuilder.getMRI()->setType(I.getOperand(0).getReg(), LLT::scalar(32));
    }
    return true;
  }

  if (I.getNumOperands() != I.getNumExplicitOperands()) {
    LLVM_DEBUG(errs() << "Generic instr has unexpected implicit operands\n");
    return false;
  }

  // Common code for getting return reg+type, and removing selected instr
  // from parent occurs here. Instr-specific selection happens in spvSelect().
  bool hasDefs = I.getNumDefs() > 0;
  Register resVReg = hasDefs ? I.getOperand(0).getReg() : Register(0);
  SPIRVType *resType = hasDefs ? TR.getSPIRVTypeForVReg(resVReg) : nullptr;
  assert(!hasDefs || resType || I.getOpcode() == TargetOpcode::G_GLOBAL_VALUE);
  if (spvSelect(resVReg, resType, I, MIRBuilder)) {
    if (hasDefs) { // Make all vregs 32 bits (for SPIR-V IDs)
      MIRBuilder.getMRI()->setType(resVReg, LLT::scalar(32));
    }
    I.removeFromParent();
    return true;
  }
  return false;
}

bool SPIRVInstructionSelector::spvSelect(Register resVReg,
                                         const SPIRVType *resType,
                                         const MachineInstr &I,
                                         MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;
  namespace CL = OpenCL_std;
  namespace GL = GLSL_std_450;
  assert(!isTypeFoldingSupported(I.getOpcode()) ||
         I.getOpcode() == TargetOpcode::G_CONSTANT);

  const unsigned Opcode = I.getOpcode();
  switch (Opcode) {
  case TargetOpcode::G_CONSTANT:
    return selectConst(resVReg, resType, I.getOperand(1).getCImm()->getValue(),
                       MIRBuilder);
  case TargetOpcode::G_GLOBAL_VALUE:
    return selectGlobalValue(resVReg, I, MIRBuilder);
  case TargetOpcode::G_IMPLICIT_DEF:
    return selectOpUndef(resVReg, resType, MIRBuilder);

  case TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS:
    return selectIntrinsic(resVReg, resType, I, MIRBuilder);
  case TargetOpcode::G_BITREVERSE:
    return selectBitreverse(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_BUILD_VECTOR:
    return selectConstVector(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_SHUFFLE_VECTOR: {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpVectorShuffle)
                   .addDef(resVReg)
                   .addUse(TR.getSPIRVTypeID(resType))
                   .addUse(I.getOperand(1).getReg())
                   .addUse(I.getOperand(2).getReg());
    for (auto V : I.getOperand(3).getShuffleMask())
      MIB.addImm(V);

    return MIB.constrainAllUses(TII, TRI, RBI);
  }
  case TargetOpcode::G_MEMMOVE:
  case TargetOpcode::G_MEMCPY:
    return selectMemOperation(resVReg, I, MIRBuilder);

  case TargetOpcode::G_ICMP:
    return selectICmp(resVReg, resType, I, MIRBuilder);
  case TargetOpcode::G_FCMP:
    return selectFCmp(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_FRAME_INDEX:
    return selectFrameIndex(resVReg, resType, MIRBuilder);

  case TargetOpcode::G_LOAD:
    return selectLoad(resVReg, resType, I, MIRBuilder);
  case TargetOpcode::G_STORE:
    return selectStore(I, MIRBuilder);

  case TargetOpcode::G_BR:
    return selectBranch(I, MIRBuilder);
  case TargetOpcode::G_BRCOND:
    return selectBranchCond(I, MIRBuilder);

  case TargetOpcode::G_PHI:
    return selectPhi(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_FPTOSI:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpConvertFToS);
  case TargetOpcode::G_FPTOUI:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpConvertFToU);

  case TargetOpcode::G_SITOFP:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpConvertSToF);
  case TargetOpcode::G_UITOFP:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpConvertUToF);

  case TargetOpcode::G_CTPOP:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpBitCount);

  // even SPIRV-LLVM translator doens't support overflow intrinsics
  // so we don't even have a reliable tests for this functionality
#if 0
  case TargetOpcode::G_UADDO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpIAddCarry);
  case TargetOpcode::G_USUBO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpISubBorrow);
  case TargetOpcode::G_UMULO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpSMulExtended);
  case TargetOpcode::G_SMULO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpUMulExtended);
#endif

  case TargetOpcode::G_SMIN:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::s_min, GL::SMin);
  case TargetOpcode::G_UMIN:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::u_min, GL::UMin);

  case TargetOpcode::G_SMAX:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::s_max, GL::SMax);
  case TargetOpcode::G_UMAX:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::u_max, GL::UMax);

  case TargetOpcode::G_FMA:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::fma, GL::Fma);

  case TargetOpcode::G_FPOW:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::pow, GL::Pow);

  case TargetOpcode::G_FEXP:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::exp, GL::Exp);
  case TargetOpcode::G_FEXP2:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::exp2, GL::Exp2);

  case TargetOpcode::G_FLOG:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::log, GL::Log);
  case TargetOpcode::G_FLOG2:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::log2, GL::Log2);
  case TargetOpcode::G_FLOG10:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::log10);

  case TargetOpcode::G_FABS:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::fabs, GL::FAbs);

  case TargetOpcode::G_FMINNUM:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::fmin, GL::FMin);
  case TargetOpcode::G_FMAXNUM:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::fmax, GL::FMax);

  case TargetOpcode::G_FCEIL:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::ceil, GL::Ceil);
  case TargetOpcode::G_FFLOOR:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::floor, GL::Floor);

  case TargetOpcode::G_FCOS:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::cos, GL::Cos);
  case TargetOpcode::G_FSIN:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::sin, GL::Sin);

  case TargetOpcode::G_FSQRT:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::sqrt, GL::Sqrt);

  case TargetOpcode::G_CTTZ:
  case TargetOpcode::G_CTTZ_ZERO_UNDEF:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::ctz);
  case TargetOpcode::G_CTLZ:
  case TargetOpcode::G_CTLZ_ZERO_UNDEF:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::clz);

  case TargetOpcode::G_INTRINSIC_ROUND:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::round, GL::Round);
  case TargetOpcode::G_INTRINSIC_TRUNC:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::trunc, GL::Trunc);
  case TargetOpcode::G_FRINT:
  case TargetOpcode::G_FNEARBYINT:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::rint,
                         GL::RoundEven);

  case TargetOpcode::G_SMULH:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::s_mul_hi);
  case TargetOpcode::G_UMULH:
    return selectExtInst(resVReg, resType, I, MIRBuilder, CL::u_mul_hi);

  case TargetOpcode::G_SEXT:
    return selectExt(resVReg, resType, I, true, MIRBuilder);
  case TargetOpcode::G_ANYEXT:
  case TargetOpcode::G_ZEXT:
    return selectExt(resVReg, resType, I, false, MIRBuilder);
  case TargetOpcode::G_TRUNC:
    return selectTrunc(resVReg, resType, I, MIRBuilder);
  case TargetOpcode::G_FPTRUNC:
  case TargetOpcode::G_FPEXT:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpFConvert);

  case TargetOpcode::G_PTRTOINT:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpConvertPtrToU);
  case TargetOpcode::G_INTTOPTR:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpConvertUToPtr);
  case TargetOpcode::G_BITCAST:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpBitcast);
  case TargetOpcode::G_ADDRSPACE_CAST:
    return selectAddrSpaceCast(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_ATOMICRMW_OR:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicOr);
  case TargetOpcode::G_ATOMICRMW_ADD:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicIAdd);
  case TargetOpcode::G_ATOMICRMW_AND:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicAnd);
  case TargetOpcode::G_ATOMICRMW_MAX:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicSMax);
  case TargetOpcode::G_ATOMICRMW_MIN:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicSMin);
  case TargetOpcode::G_ATOMICRMW_SUB:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicISub);
  case TargetOpcode::G_ATOMICRMW_XOR:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicXor);
  case TargetOpcode::G_ATOMICRMW_UMAX:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicUMax);
  case TargetOpcode::G_ATOMICRMW_UMIN:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicUMin);
  case TargetOpcode::G_ATOMICRMW_XCHG:
    return selectAtomicRMW(resVReg, resType, I, MIRBuilder, OpAtomicExchange);

  case TargetOpcode::G_ATOMIC_CMPXCHG_WITH_SUCCESS:
    return selectAtomicCmpXchg(resVReg, resType, I, MIRBuilder, true);
  case TargetOpcode::G_ATOMIC_CMPXCHG:
    return selectAtomicCmpXchg(resVReg, resType, I, MIRBuilder, false);

  case TargetOpcode::G_FENCE:
    return selectFence(I, MIRBuilder);

  default:
    return false;
  }
}

bool SPIRVInstructionSelector::selectExtInst(Register resVReg,
                                             const SPIRVType *resType,
                                             const MachineInstr &I,
                                             MachineIRBuilder &MIRBuilder,
                                             CL::OpenCL_std clInst) const {
  return selectExtInst(resVReg, resType, I, MIRBuilder,
                       {{ExtInstSet::OpenCL_std, clInst}});
}

bool SPIRVInstructionSelector::selectExtInst(Register resVReg,
                                             const SPIRVType *resType,
                                             const MachineInstr &I,
                                             MachineIRBuilder &MIRBuilder,
                                             CL::OpenCL_std clInst,
                                             GL::GLSL_std_450 glInst) const {
  ExtInstList extInsts = {{ExtInstSet::OpenCL_std, clInst},
                          {ExtInstSet::GLSL_std_450, glInst}};
  return selectExtInst(resVReg, resType, I, MIRBuilder, extInsts);
}

bool SPIRVInstructionSelector::selectExtInst(Register resVReg,
                                             const SPIRVType *resType,
                                             const MachineInstr &I,
                                             MachineIRBuilder &MIRBuilder,
                                             const ExtInstList &insts) const {

  for (const auto &ex : insts) {
    ExtInstSet set = ex.first;
    uint32_t opCode = ex.second;
    if (STI.canUseExtInstSet(set)) {
      auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                     .addDef(resVReg)
                     .addUse(TR.getSPIRVTypeID(resType))
                     .addImm(static_cast<uint32_t>(set))
                     .addImm(opCode);
      const unsigned numOps = I.getNumOperands();
      for (unsigned i = 1; i < numOps; ++i) {
        MIB.add(I.getOperand(i));
      }
      return MIB.constrainAllUses(TII, TRI, RBI);
    }
  }
  return false;
}

static bool canUseNSW(unsigned opCode) {
  using namespace SPIRV;
  switch (opCode) {
  case OpIAddS:
  case OpIAddV:
  case OpISubS:
  case OpISubV:
  case OpIMulS:
  case OpIMulV:
  case OpShiftLeftLogicalS:
  case OpShiftLeftLogicalV:
  case OpSNegate:
    return true;
  default:
    return false;
  }
}

static bool canUseNUW(unsigned opCode) {
  using namespace SPIRV;
  switch (opCode) {
  case OpIAddS:
  case OpIAddV:
  case OpISubS:
  case OpISubV:
  case OpIMulS:
  case OpIMulV:
    return true;
  default:
    return false;
  }
}

// TODO: move to GenerateDecorations pass
static void handleIntegerWrapFlags(const MachineInstr &I, Register Target,
                                   unsigned NewOpcode, const SPIRVSubtarget &ST,
                                   MachineIRBuilder &MIRBuilder,
                                   SPIRVTypeRegistry &TR) {
  if (I.getFlag(MachineInstr::MIFlag::NoSWrap) && canUseNSW(NewOpcode) &&
      canUseDecoration(Decoration::NoSignedWrap, ST))
    buildOpDecorate(Target, MIRBuilder, Decoration::NoSignedWrap, {});

  if (I.getFlag(MachineInstr::MIFlag::NoUWrap) && canUseNUW(NewOpcode) &&
       canUseDecoration(Decoration::NoUnsignedWrap, ST))
    buildOpDecorate(Target, MIRBuilder, Decoration::NoUnsignedWrap, {});
}

bool SPIRVInstructionSelector::selectUnOp(Register resVReg,
                                          const SPIRVType *resType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder,
                                          unsigned newOpcode) const {
  handleIntegerWrapFlags(I, resVReg, newOpcode, STI, MIRBuilder, TR);
  return MIRBuilder.buildInstr(newOpcode)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(I.getOperand(1).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

static MemorySemantics::MemorySemantics getMemSemantics(AtomicOrdering ord) {
  switch (ord) {
  case AtomicOrdering::Acquire:
    return MemorySemantics::Acquire;
  case AtomicOrdering::Release:
    return MemorySemantics::Release;
  case AtomicOrdering::AcquireRelease:
    return MemorySemantics::AcquireRelease;
  case AtomicOrdering::SequentiallyConsistent:
    return MemorySemantics::SequentiallyConsistent;
  case AtomicOrdering::Unordered:
  case AtomicOrdering::Monotonic:
  case AtomicOrdering::NotAtomic:
  default:
    return MemorySemantics::None;
  }
}

static Scope::Scope getScope(SyncScope::ID ord) {
  switch (ord) {
  case SyncScope::SingleThread:
    return Scope::Invocation;
  case SyncScope::System:
    return Scope::Device;
  default:
    llvm_unreachable("Unsupported synchronization scope ID.");
  }
}

static void addMemoryOperands(MachineMemOperand *MemOp, MachineInstrBuilder &MIB) {
  uint32_t spvMemOp = MemoryOperand::None;
  if (MemOp->isVolatile())
    spvMemOp |= MemoryOperand::Volatile;
  if (MemOp->isNonTemporal())
    spvMemOp |= MemoryOperand::Nontemporal;
  if (MemOp->getAlign().value())
    spvMemOp |= MemoryOperand::Aligned;

  if (spvMemOp != MemoryOperand::None) {
    MIB.addImm(spvMemOp);
    if (spvMemOp & MemoryOperand::Aligned)
      MIB.addImm(MemOp->getAlign().value());
  }
}

static void addMemoryOperands(uint64_t Flags, MachineInstrBuilder &MIB) {
  uint32_t spvMemOp = MemoryOperand::None;
  if (Flags & MachineMemOperand::Flags::MOVolatile)
    spvMemOp |= MemoryOperand::Volatile;
  if (Flags & MachineMemOperand::Flags::MONonTemporal)
    spvMemOp |= MemoryOperand::Nontemporal;
  // if (MemOp->getAlign().value())
  //   spvMemOp |= MemoryOperand::Aligned;

  if (spvMemOp != MemoryOperand::None) {
    MIB.addImm(spvMemOp);
    // if (spvMemOp & MemoryOperand::Aligned)
    //   MIB.addImm(MemOp->getAlign().value());
  }
}

bool SPIRVInstructionSelector::selectLoad(Register resVReg,
                                          const SPIRVType *resType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder) const {
  unsigned OpOffset =
      I.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS ? 1 : 0;
  auto Ptr = I.getOperand(1 + OpOffset).getReg();
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpLoad)
                 .addDef(resVReg)
                 .addUse(TR.getSPIRVTypeID(resType))
                 .addUse(Ptr);
  if (!I.getNumMemOperands()) {
    assert(I.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS);
    addMemoryOperands(I.getOperand(2 + OpOffset).getImm(), MIB);
  } else {
    auto MemOp = *I.memoperands_begin();
    addMemoryOperands(MemOp, MIB);
  }
  return MIB.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectStore(const MachineInstr &I,
                                           MachineIRBuilder &MIRBuilder) const {
  unsigned OpOffset =
      I.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS ? 1 : 0;
  auto StoreVal = I.getOperand(0 + OpOffset).getReg();
  auto Ptr = I.getOperand(1 + OpOffset).getReg();
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpStore).addUse(Ptr).addUse(StoreVal);
  if (!I.getNumMemOperands()) {
    assert(I.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS);
    addMemoryOperands(I.getOperand(2 + OpOffset).getImm(), MIB);
  } else {
    auto MemOp = *I.memoperands_begin();
    addMemoryOperands(MemOp, MIB);
  }
  return MIB.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectMemOperation(Register ResVReg,
    const MachineInstr &I, MachineIRBuilder &MIRBuilder) const {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpCopyMemorySized)
                 .addDef(I.getOperand(0).getReg())
                 .addUse(I.getOperand(1).getReg())
                 .addUse(I.getOperand(2).getReg());
  if (I.getNumMemOperands()) {
    auto MemOp = *I.memoperands_begin();
    addMemoryOperands(MemOp, MIB);
  }
  bool Result = MIB.constrainAllUses(TII, TRI, RBI);
  if (ResVReg != MIB->getOperand(0).getReg()) {
    MIRBuilder.buildCopy(ResVReg, MIB->getOperand(0).getReg());
  }
  return Result;
}

bool SPIRVInstructionSelector::selectAtomicRMW(Register resVReg,
                                               const SPIRVType *resType,
                                               const MachineInstr &I,
                                               MachineIRBuilder &MIRBuilder,
                                               unsigned newOpcode) const {

  assert(I.hasOneMemOperand());
  auto memOp = *I.memoperands_begin();
  auto scope = getScope(memOp->getSyncScopeID());
  Register scopeReg = buildI32Constant(scope, MIRBuilder);

  auto ptr = I.getOperand(1).getReg();
  // Changed as it's implemented in the translator. See test/atomicrmw.ll
  //auto scSem = getMemSemanticsForStorageClass(TR.getPointerStorageClass(ptr));

  auto memSem = getMemSemantics(memOp->getSuccessOrdering());
  Register memSemReg = buildI32Constant(memSem /*| scSem*/, MIRBuilder);

  return MIRBuilder.buildInstr(newOpcode)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(ptr)
      .addUse(scopeReg)
      .addUse(memSemReg)
      .addUse(I.getOperand(2).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectFence(const MachineInstr &I,
                                           MachineIRBuilder &MIRBuilder) const {
  auto memSem = getMemSemantics(AtomicOrdering(I.getOperand(0).getImm()));
  Register memSemReg = buildI32Constant(memSem, MIRBuilder);

  auto scope = getScope(SyncScope::ID(I.getOperand(1).getImm()));
  Register scopeReg = buildI32Constant(scope, MIRBuilder);

  return MIRBuilder.buildInstr(SPIRV::OpMemoryBarrier)
      .addUse(scopeReg)
      .addUse(memSemReg)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectAtomicCmpXchg(Register resVReg,
                                                   const SPIRVType *resType,
                                                   const MachineInstr &I,
                                                   MachineIRBuilder &MIRBuilder,
                                                   bool withSuccess) const {
  auto MRI = MIRBuilder.getMRI();
  assert(I.hasOneMemOperand());
  auto memOp = *I.memoperands_begin();
  auto scope = getScope(memOp->getSyncScopeID());
  Register scopeReg = buildI32Constant(scope, MIRBuilder);

  auto ptr = I.getOperand(2).getReg();
  auto cmp = I.getOperand(3).getReg();
  auto val = I.getOperand(4).getReg();

  auto spvValTy = TR.getSPIRVTypeForVReg(val);
  auto scSem = getMemSemanticsForStorageClass(TR.getPointerStorageClass(ptr));

  auto memSemEq = getMemSemantics(memOp->getSuccessOrdering()) | scSem;
  Register memSemEqReg = buildI32Constant(memSemEq, MIRBuilder);

  auto memSemNeq = getMemSemantics(memOp->getFailureOrdering()) | scSem;
  Register memSemNeqReg = memSemEq == memSemNeq
                              ? memSemEqReg
                              : buildI32Constant(memSemNeq, MIRBuilder);

  auto tmpReg = withSuccess ? MRI->createGenericVirtualRegister(LLT::scalar(32))
                            : resVReg;
  bool success = MIRBuilder.buildInstr(SPIRV::OpAtomicCompareExchange)
                     .addDef(tmpReg)
                     .addUse(TR.getSPIRVTypeID(spvValTy))
                     .addUse(ptr)
                     .addUse(scopeReg)
                     .addUse(memSemEqReg)
                     .addUse(memSemNeqReg)
                     .addUse(val)
                     .addUse(cmp)
                     .constrainAllUses(TII, TRI, RBI);
  if (!withSuccess) // If we just need the old val, not {oldVal, success}
    return success;
  assert(resType->getOpcode() == SPIRV::OpTypeStruct);
  auto boolReg = MRI->createGenericVirtualRegister(LLT::scalar(1));
  Register boolTyID = resType->getOperand(2).getReg();
  success &= MIRBuilder.buildInstr(SPIRV::OpIEqual)
                 .addDef(boolReg)
                 .addUse(boolTyID)
                 .addUse(tmpReg)
                 .addUse(cmp)
                 .constrainAllUses(TII, TRI, RBI);
  return success && MIRBuilder.buildInstr(SPIRV::OpCompositeConstruct)
                        .addDef(resVReg)
                        .addUse(TR.getSPIRVTypeID(resType))
                        .addUse(tmpReg)
                        .addUse(boolReg)
                        .constrainAllUses(TII, TRI, RBI);
}

static bool isGenericCastablePtr(StorageClass::StorageClass sc) {
  switch (sc) {
  case StorageClass::Workgroup:
  case StorageClass::CrossWorkgroup:
  case StorageClass::Function:
    return true;
  default:
    return false;
  }
}

// In SPIR-V address space casting can only happen to and from the Generic
// storage class. We can also only case Workgroup, CrossWorkgroup, or Function
// pointers to and from Generic pointers. As such, we can convert e.g. from
// Workgroup to Function by going via a Generic pointer as an intermediary. All
// other combinations can only be done by a bitcast, and are probably not safe.
bool SPIRVInstructionSelector::selectAddrSpaceCast(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;
  namespace SC = StorageClass;
  auto srcPtr = I.getOperand(1).getReg();
  auto srcPtrTy = TR.getSPIRVTypeForVReg(srcPtr);
  auto srcSC = TR.getPointerStorageClass(srcPtr);
  auto dstSC = TR.getPointerStorageClass(resVReg);

  if (dstSC == SC::Generic && isGenericCastablePtr(srcSC)) {
    // We're casting from an eligable pointer to Generic
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpPtrCastToGeneric);
  } else if (srcSC == SC::Generic && isGenericCastablePtr(dstSC)) {
    // We're casting from Generic to an eligable pointer
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpGenericCastToPtr);
  } else if (isGenericCastablePtr(srcSC) && isGenericCastablePtr(dstSC)) {
    // We're casting between 2 eligable pointers using Generic as an
    // intermediary
    auto tmp = MIRBuilder.getMRI()->createVirtualRegister(&IDRegClass);
    auto genericPtrTy = TR.getOrCreateSPIRVPointerType(srcPtrTy, MIRBuilder,
                                                       StorageClass::Generic);
    bool success = MIRBuilder.buildInstr(OpPtrCastToGeneric)
                       .addDef(tmp)
                       .addUse(TR.getSPIRVTypeID(genericPtrTy))
                       .addUse(srcPtr)
                       .constrainAllUses(TII, TRI, RBI);
    return success && MIRBuilder.buildInstr(OpGenericCastToPtr)
                          .addDef(resVReg)
                          .addUse(TR.getSPIRVTypeID(resType))
                          .addUse(tmp)
                          .constrainAllUses(TII, TRI, RBI);
  } else {
    // TODO Should this case just be disallowed completely?
    // We're casting 2 other arbitrary address spaces, so have to bitcast
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpBitcast);
  }
}

#if 0
bool SPIRVInstructionSelector::selectOverflowOp(Register resVReg,
                                                const SPIRVType *resType,
                                                const MachineInstr &I,
                                                MachineIRBuilder &MIRBuilder,
                                                unsigned newOpcode) const {
  using namespace SPIRV;
  auto MRI = MIRBuilder.getMRI();

  auto elem0TyReg = resType->getOperand(1).getReg();
  SPIRVType *elem0Ty = TR.getSPIRVTypeForVReg(elem0TyReg);
  assert(TR.isScalarOrVectorOfType(elem0TyReg, OpTypeInt));

  auto tmpReg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  auto tmpStructTy = TR.getPairStruct(elem0Ty, elem0Ty, MIRBuilder);

  bool success = MIRBuilder.buildInstr(newOpcode)
                     .addDef(tmpReg)
                     .addUse(TR.getSPIRVTypeID(tmpStructTy))
                     .addUse(I.getOperand(2).getReg())
                     .addUse(I.getOperand(3).getReg())
                     .constrainAllUses(TII, TRI, RBI);

  // Need to build an {int, bool} struct from the {int, int} TR. result
  auto elem0Reg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  success &= MIRBuilder.buildInstr(OpCompositeExtract)
                 .addDef(elem0Reg)
                 .addUse(TR.getSPIRVTypeID(elem0Ty))
                 .addUse(tmpReg)
                 .addImm(0)
                 .constrainAllUses(TII, TRI, RBI);
  auto elem1Reg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  success &= MIRBuilder.buildInstr(OpCompositeExtract)
                 .addDef(elem1Reg)
                 .addUse(TR.getSPIRVTypeID(elem0Ty))
                 .addUse(tmpReg)
                 .addImm(1)
                 .constrainAllUses(TII, TRI, RBI);
  auto boolReg = MRI->createGenericVirtualRegister(LLT::scalar(1));
  auto boolTy = TR.getSPIRVTypeForVReg(resType->getOperand(2).getReg());
  success &= selectIntToBool(elem1Reg, boolReg, elem0Ty, boolTy, MIRBuilder);
  return success && MIRBuilder.buildInstr(OpCompositeConstruct)
                        .addDef(resVReg)
                        .addUse(TR.getSPIRVTypeID(resType))
                        .addUse(elem0Reg)
                        .addUse(boolReg)
                        .constrainAllUses(TII, TRI, RBI);
}
#endif

static unsigned int getFCmpOpcode(unsigned predNum) {
  auto pred = static_cast<CmpInst::Predicate>(predNum);
  switch (pred) {
  case CmpInst::FCMP_OEQ:
    return SPIRV::OpFOrdEqual;
  case CmpInst::FCMP_OGE:
    return SPIRV::OpFOrdGreaterThanEqual;
  case CmpInst::FCMP_OGT:
    return SPIRV::OpFOrdGreaterThan;
  case CmpInst::FCMP_OLE:
    return SPIRV::OpFOrdLessThanEqual;
  case CmpInst::FCMP_OLT:
    return SPIRV::OpFOrdLessThan;
  case CmpInst::FCMP_ONE:
    return SPIRV::OpFOrdNotEqual;
  case CmpInst::FCMP_ORD:
    return SPIRV::OpOrdered;
  case CmpInst::FCMP_UEQ:
    return SPIRV::OpFUnordEqual;
  case CmpInst::FCMP_UGE:
    return SPIRV::OpFUnordGreaterThanEqual;
  case CmpInst::FCMP_UGT:
    return SPIRV::OpFUnordGreaterThan;
  case CmpInst::FCMP_ULE:
    return SPIRV::OpFUnordLessThanEqual;
  case CmpInst::FCMP_ULT:
    return SPIRV::OpFUnordLessThan;
  case CmpInst::FCMP_UNE:
    return SPIRV::OpFUnordNotEqual;
  case CmpInst::FCMP_UNO:
    return SPIRV::OpUnordered;
  default:
    report_fatal_error("Unknown predicate type for FCmp: " +
                       CmpInst::getPredicateName(pred));
  }
}

static unsigned int getICmpOpcode(unsigned predNum) {
  auto pred = static_cast<CmpInst::Predicate>(predNum);
  switch (pred) {
  case CmpInst::ICMP_EQ:
    return SPIRV::OpIEqual;
  case CmpInst::ICMP_NE:
    return SPIRV::OpINotEqual;
  case CmpInst::ICMP_SGE:
    return SPIRV::OpSGreaterThanEqual;
  case CmpInst::ICMP_SGT:
    return SPIRV::OpSGreaterThan;
  case CmpInst::ICMP_SLE:
    return SPIRV::OpSLessThanEqual;
  case CmpInst::ICMP_SLT:
    return SPIRV::OpSLessThan;
  case CmpInst::ICMP_UGE:
    return SPIRV::OpUGreaterThanEqual;
  case CmpInst::ICMP_UGT:
    return SPIRV::OpUGreaterThan;
  case CmpInst::ICMP_ULE:
    return SPIRV::OpULessThanEqual;
  case CmpInst::ICMP_ULT:
    return SPIRV::OpULessThan;
  default:
    report_fatal_error("Unknown predicate type for ICmp: " +
                       CmpInst::getPredicateName(pred));
  }
}

// Return <CmpOpcode, needsConvertedToInt>
static std::pair<unsigned int, bool> getPtrCmpOpcode(unsigned pred) {
  switch (static_cast<CmpInst::Predicate>(pred)) {
  case CmpInst::ICMP_EQ:
    return {SPIRV::OpPtrEqual, false};
  case CmpInst::ICMP_NE:
    return {SPIRV::OpPtrNotEqual, false};
  default:
    return {getICmpOpcode(pred), true};
  }
}

// Return the logical operation, or abort if none exists
static unsigned int getBoolCmpOpcode(unsigned predNum) {
  auto pred = static_cast<CmpInst::Predicate>(predNum);
  switch (pred) {
  case CmpInst::ICMP_EQ:
    return SPIRV::OpLogicalEqual;
  case CmpInst::ICMP_NE:
    return SPIRV::OpLogicalNotEqual;
  default:
    report_fatal_error("Unknown predicate type for Bool comparison: " +
                       CmpInst::getPredicateName(pred));
  }
}

bool SPIRVInstructionSelector::selectBitreverse(Register resVReg,
                                          const SPIRVType *resType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder) const {
  Register src = I.getOperand(1).getReg();
  return MIRBuilder.buildInstr(SPIRV::OpBitReverse)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(src)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectConstVector(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  // TODO: only const case is supported for now
  assert(std::all_of(
      I.operands_begin(), I.operands_end(),
      [&MIRBuilder](const MachineOperand &MO) {
        if (MO.isDef())
          return true;
        if (!MO.isReg())
          return false;
        auto *ConstTy = MIRBuilder.getMRI()->getVRegDef(MO.getReg());
        assert(ConstTy && ConstTy->getOpcode() == SPIRV::ASSIGN_TYPE &&
               ConstTy->getOperand(1).isReg());
        auto *Const =
            MIRBuilder.getMRI()->getVRegDef(ConstTy->getOperand(1).getReg());
        assert(Const);
        return (Const->getOpcode() == TargetOpcode::G_CONSTANT ||
                Const->getOpcode() == TargetOpcode::G_FCONSTANT);
      }));

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantComposite)
                 .addDef(resVReg)
                 .addUse(TR.getSPIRVTypeID(resType));
  for (unsigned i = I.getNumExplicitDefs(); i < I.getNumExplicitOperands(); ++i)
    MIB.addUse(I.getOperand(i).getReg());
  return MIB.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectCmp(Register resVReg,
                                         const SPIRVType *resType,
                                         unsigned scalarTyOpc, unsigned cmpOpc,
                                         const MachineInstr &I,
                                         MachineIRBuilder &MIRBuilder,
                                         bool convertToUInt) const {
  using namespace SPIRV;

  Register cmp0 = I.getOperand(2).getReg();
  Register cmp1 = I.getOperand(3).getReg();
  SPIRVType *cmp0Type = TR.getSPIRVTypeForVReg(cmp0);
  SPIRVType *cmp1Type = TR.getSPIRVTypeForVReg(cmp1);
  assert(cmp0Type->getOpcode() == cmp1Type->getOpcode());

  if (cmp0Type->getOpcode() != OpTypePointer &&
      (!TR.isScalarOrVectorOfType(cmp0, scalarTyOpc) ||
       !TR.isScalarOrVectorOfType(cmp1, scalarTyOpc)))
    llvm_unreachable("Incompatible type for comparison");

  return MIRBuilder.buildInstr(cmpOpc)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(cmp0)
      .addUse(cmp1)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectICmp(Register resVReg,
                                          const SPIRVType *resType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder) const {

  auto pred = I.getOperand(1).getPredicate();
  unsigned cmpOpc = getICmpOpcode(pred);
  unsigned typeOpc = SPIRV::OpTypeInt;
  bool ptrToUInt = false;

  Register cmpOperand = I.getOperand(2).getReg();
  if (TR.isScalarOfType(cmpOperand, SPIRV::OpTypePointer)) {
    if (STI.canDirectlyComparePointers()) {
      std::tie(cmpOpc, ptrToUInt) = getPtrCmpOpcode(pred);
    } else {
      ptrToUInt = true;
    }
  } else if (TR.isScalarOrVectorOfType(cmpOperand, SPIRV::OpTypeBool)) {
    typeOpc = SPIRV::OpTypeBool;
    cmpOpc = getBoolCmpOpcode(pred);
  }
  return selectCmp(resVReg, resType, typeOpc, cmpOpc, I, MIRBuilder, ptrToUInt);
}

static void transformConst(const APInt &imm, MachineInstrBuilder &MIB, bool IsFloat = false) {
  const auto bitwidth = imm.getBitWidth();
  switch (bitwidth) {
  case 1:
    break; // Already handled
  case 8:
  case 16:
  case 32:
    MIB.addImm(imm.getZExtValue());
    break;
  case 64: {
    if (!IsFloat) {
      uint64_t fullImm = imm.getZExtValue();
      uint32_t lowBits = fullImm & 0xffffffff;
      uint32_t highBits = (fullImm >> 32) & 0xffffffff;
      MIB.addImm(lowBits).addImm(highBits);
    } else
      MIB.addImm(imm.getZExtValue());
    break;
  }
  default:
    report_fatal_error("Unsupported constant bitwidth");
  }
}

void SPIRVInstructionSelector::renderFImm32(MachineInstrBuilder &MIB,
                                                   const MachineInstr &I,
                                                   int OpIdx) const {
  assert(I.getOpcode() == TargetOpcode::G_FCONSTANT && OpIdx == -1 &&
         "Expected G_FCONSTANT");
  const ConstantFP *FPImm = I.getOperand(1).getFPImm();
  transformConst(FPImm->getValueAPF().bitcastToAPInt(), MIB, true);
}

void SPIRVInstructionSelector::renderImm32(
  MachineInstrBuilder &MIB, const MachineInstr &I, int OpIdx) const {
  assert(I.getOpcode() == TargetOpcode::G_CONSTANT && OpIdx == -1 &&
         "Expected G_CONSTANT");
  transformConst(I.getOperand(1).getCImm()->getValue(), MIB);
}

Register
SPIRVInstructionSelector::buildI32Constant(uint32_t val,
    MachineIRBuilder &MIRBuilder, const SPIRVType *resType) const {
  auto MRI = MIRBuilder.getMRI();
  auto LLVMTy = IntegerType::get(MIRBuilder.getMF().getFunction().getContext(), 32);
  auto spvI32Ty = resType ? resType : TR.getOrCreateSPIRVType(LLVMTy,
                                                              MIRBuilder);
  // Find a constant in DT or build a new one.
  auto ConstInt = ConstantInt::get(LLVMTy, val);
  Register newReg;
  if (DT.find(ConstInt, &MIRBuilder.getMF(), newReg) == false) {
    newReg = MRI->createGenericVirtualRegister(LLT::scalar(32));
    DT.add(ConstInt, &MIRBuilder.getMF(), newReg);
    MachineInstr *MI;
    if (val == 0)
      MI = MIRBuilder.buildInstr(SPIRV::OpConstantNull)
                 .addDef(newReg)
                 .addUse(TR.getSPIRVTypeID(spvI32Ty));
    else
      MI = MIRBuilder.buildInstr(SPIRV::OpConstantI)
                 .addDef(newReg)
                 .addUse(TR.getSPIRVTypeID(spvI32Ty))
                 .addImm(APInt(32, val).getZExtValue());
    constrainSelectedInstRegOperands(*MI, TII, TRI, RBI);
  }
  return newReg;
}

bool SPIRVInstructionSelector::selectFCmp(Register resVReg,
                                          const SPIRVType *resType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder) const {
  unsigned int cmpOp = getFCmpOpcode(I.getOperand(1).getPredicate());
  return selectCmp(resVReg, resType, SPIRV::OpTypeFloat, cmpOp, I, MIRBuilder);
}

Register
SPIRVInstructionSelector::buildZerosVal(const SPIRVType *resType,
                                        MachineIRBuilder &MIRBuilder) const {
  return buildI32Constant(0, MIRBuilder, resType);
}

Register
SPIRVInstructionSelector::buildOnesVal(bool allOnes, const SPIRVType *resType,
                                       MachineIRBuilder &MIRBuilder) const {
  auto MRI = MIRBuilder.getMRI();
  unsigned bitWidth = TR.getScalarOrVectorBitWidth(resType);
  auto one = allOnes ? APInt::getAllOnesValue(bitWidth)
                     : APInt::getOneBitSet(bitWidth, 0);
  Register oneReg = buildI32Constant(one.getZExtValue(), MIRBuilder, resType);
  if (resType->getOpcode() == SPIRV::OpTypeVector) {
    const unsigned numEles = resType->getOperand(2).getImm();
    Register oneVec = MRI->createVirtualRegister(&SPIRV::IDRegClass);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantComposite)
                   .addDef(oneVec)
                   .addUse(TR.getSPIRVTypeID(resType));
    for (unsigned i = 0; i < numEles; ++i) {
      MIB.addUse(oneReg);
    }
    TR.constrainRegOperands(MIB);
    return oneVec;
  } else {
    return oneReg;
  }
}

bool SPIRVInstructionSelector::selectExt(Register resVReg,
                                         const SPIRVType *resType,
                                         const MachineInstr &I, bool isSigned,
                                         MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;
  if (TR.isScalarOrVectorOfType(I.getOperand(1).getReg(), OpTypeBool)) {
    // To extend a bool, we need to use OpSelect between constants
    Register zeroReg = buildZerosVal(resType, MIRBuilder);
    Register oneReg = buildOnesVal(isSigned, resType, MIRBuilder);
    return MIRBuilder
        .buildInstr(TR.isScalarOfType(I.getOperand(1).getReg(), OpTypeBool)
                        ? OpSelectSISCond
                        : OpSelectSIVCond)
        .addDef(resVReg)
        .addUse(TR.getSPIRVTypeID(resType))
        .addUse(I.getOperand(1).getReg())
        .addUse(oneReg)
        .addUse(zeroReg)
        .constrainAllUses(TII, TRI, RBI);
  } else {
    return selectUnOp(resVReg, resType, I, MIRBuilder,
                      isSigned ? OpSConvert : OpUConvert);
  }
}

bool SPIRVInstructionSelector::selectIntToBool(
    Register intReg, Register resVReg, const SPIRVType *intTy,
    const SPIRVType *boolTy, MachineIRBuilder &MIRBuilder) const {
  // To truncate to a bool, we use OpINotEqual to zero
  auto zero = buildZerosVal(intTy, MIRBuilder);
  return MIRBuilder.buildInstr(SPIRV::OpINotEqual)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(boolTy))
      .addUse(intReg)
      .addUse(zero)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectTrunc(Register resVReg,
                                           const SPIRVType *resType,
                                           const MachineInstr &I,
                                           MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;
  if (TR.isScalarOrVectorOfType(resVReg, OpTypeBool)) {
    auto intReg = I.getOperand(1).getReg();
    auto argType = TR.getSPIRVTypeForVReg(intReg);
    return selectIntToBool(intReg, resVReg, argType, resType, MIRBuilder);
  } else {
    bool isSigned = TR.isScalarOrVectorSigned(resType);
    return selectUnOp(resVReg, resType, I, MIRBuilder,
                      isSigned ? OpSConvert : OpUConvert);
  }
}

bool SPIRVInstructionSelector::selectConst(Register resVReg,
                                           const SPIRVType *resType,
                                           const APInt &imm,
                                           MachineIRBuilder &MIRBuilder) const {
  assert(resType->getOpcode() != SPIRV::OpTypePointer || imm.isNullValue());
  if (resType->getOpcode() == SPIRV::OpTypePointer && imm.isNullValue())
    return MIRBuilder.buildInstr(SPIRV::OpConstantNull)
        .addDef(resVReg)
        .addUse(TR.getSPIRVTypeID(resType))
        .constrainAllUses(TII, TRI, RBI);
  else {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantI).addDef(resVReg).addUse(TR.getSPIRVTypeID(resType));
    // <=32-bit integers should be caught by the sdag pattern
    assert(imm.getBitWidth() > 32);
    transformConst(imm, MIB);
    return MIB.constrainAllUses(TII, TRI, RBI);
  }
}

bool SPIRVInstructionSelector::selectOpUndef(
    Register resVReg, const SPIRVType *resType,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpUndef)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .constrainAllUses(TII, TRI, RBI);
}

static bool isImm(const MachineOperand &MO) {
  auto &MRI = MO.getParent()->getMF()->getRegInfo();
  auto *TypeInst = MRI.getVRegDef(MO.getReg());
  if (TypeInst->getOpcode() != SPIRV::ASSIGN_TYPE)
    return false;
  auto *ImmInst = MRI.getVRegDef(TypeInst->getOperand(1).getReg());
  return ImmInst->getOpcode() == TargetOpcode::G_CONSTANT;
}

static int64_t foldImm(const MachineOperand &MO) {
  auto &MRI = MO.getParent()->getMF()->getRegInfo();
  auto *TypeInst = MRI.getVRegDef(MO.getReg());
  auto *ImmInst = MRI.getVRegDef(TypeInst->getOperand(1).getReg());
  assert(ImmInst->getOpcode() == TargetOpcode::G_CONSTANT);
  return ImmInst->getOperand(1).getCImm()->getZExtValue();
}

bool SPIRVInstructionSelector::selectInsertVal(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpCompositeInsert)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      // object to insert
      .addUse(I.getOperand(3).getReg())
      // composite to insert into
      .addUse(I.getOperand(2).getReg())
      // TODO: support arbitrary number of indices
      .addImm(foldImm(I.getOperand(4)))
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectExtractVal(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpCompositeExtract)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(I.getOperand(2).getReg())
      // TODO: support arbitrary number of indices
      .addImm(foldImm(I.getOperand(3)))
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectInsertElt(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  if (isImm(I.getOperand(4)))
    return selectInsertVal(resVReg, resType, I, MIRBuilder);
  return MIRBuilder.buildInstr(SPIRV::OpVectorInsertDynamic)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(I.getOperand(2).getReg())
      .addUse(I.getOperand(3).getReg())
      .addUse(I.getOperand(4).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectExtractElt(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  if (isImm(I.getOperand(3)))
    return selectExtractVal(resVReg, resType, I, MIRBuilder);
  return MIRBuilder.buildInstr(SPIRV::OpVectorExtractDynamic)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(I.getOperand(2).getReg())
      .addUse(I.getOperand(3).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectGEP(Register resVReg,
                                         const SPIRVType *resType,
                                         const MachineInstr &I,
                                         MachineIRBuilder &MIRBuilder) const {
  // In general we should also support OpAccessChain instrs here (i.e. not
  // PtrAccessChain) but SPIRV-LLVM Translator doesn't emit them at all and so
  // do we to stay compliant with its test and more importantly consumers
  auto Opcode = I.getOperand(2).getImm() ? SPIRV::OpInBoundsPtrAccessChain
                                         : SPIRV::OpPtrAccessChain;
  auto &Res = MIRBuilder.buildInstr(Opcode)
                  .addDef(resVReg)
                  .addUse(TR.getSPIRVTypeID(resType))
                  // object to get a pointer to
                  .addUse(I.getOperand(3).getReg());
  // adding indices
  for (unsigned i = 4; i < I.getNumExplicitOperands(); ++i)
    Res.addUse(I.getOperand(i).getReg());
  return Res.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectIntrinsic(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  switch (I.getIntrinsicID()) {
  case Intrinsic::spv_load:
    return selectLoad(resVReg, resType, I, MIRBuilder);
    break;
  case Intrinsic::spv_store:
    return selectStore(I, MIRBuilder);
    break;
  case Intrinsic::spv_extractv:
    return selectExtractVal(resVReg, resType, I, MIRBuilder);
    break;
  case Intrinsic::spv_insertv:
    return selectInsertVal(resVReg, resType, I, MIRBuilder);
    break;
  case Intrinsic::spv_extractelt:
    return selectExtractElt(resVReg, resType, I, MIRBuilder);
    break;
  case Intrinsic::spv_insertelt:
    return selectInsertElt(resVReg, resType, I, MIRBuilder);
    break;
  case Intrinsic::spv_gep:
    return selectGEP(resVReg, resType, I, MIRBuilder);
    break;
  case Intrinsic::spv_unref_global:
  case Intrinsic::spv_init_global: {
    auto *MI = MIRBuilder.getMRI()->getVRegDef(I.getOperand(1).getReg());
    MachineInstr *Init = I.getNumExplicitOperands() > 2 ? MIRBuilder.getMRI()->getVRegDef(I.getOperand(2).getReg()) : nullptr;
    assert(MI);
    return selectGlobalValue(MI->getOperand(0).getReg(), *MI, MIRBuilder, Init);
  } break;
  case Intrinsic::spv_const_composite: {
    // If no values are attached, the composite is null constant.
    auto IsNull = I.getNumExplicitDefs() + 1 == I.getNumExplicitOperands();
    auto MIB = MIRBuilder.buildInstr(IsNull ? SPIRV::OpConstantNull
                                            : SPIRV::OpConstantComposite)
                   .addDef(resVReg)
                   .addUse(TR.getSPIRVTypeID(resType));
    // skip type MD node we already used when generated assign.type for this
    if (!IsNull) {
      for (unsigned i = I.getNumExplicitDefs() + 1;
           i < I.getNumExplicitOperands(); ++i) {
        MIB.addUse(I.getOperand(i).getReg());
      }
    }
    return MIB.constrainAllUses(TII, TRI, RBI);
  } break;
  case Intrinsic::spv_assign_name: {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpName);
    MIB.addUse(I.getOperand(I.getNumExplicitDefs() + 1).getReg());
    for (unsigned i = I.getNumExplicitDefs() + 2;
         i < I.getNumExplicitOperands(); ++i) {
      MIB.addImm(I.getOperand(i).getImm());
    }
    return MIB.constrainAllUses(TII, TRI, RBI);
  } break;
  default:
    llvm_unreachable("Intrinsic selection not implemented");
  }
  return true;
}

bool SPIRVInstructionSelector::selectFrameIndex(
    Register resVReg, const SPIRVType *resType,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpVariable)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addImm(StorageClass::Function)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectBranch(
    const MachineInstr &I, MachineIRBuilder &MIRBuilder) const {
  // InstructionSelector walks backwards through the instructions. We can use
  // both a G_BR and a G_BRCOND to create an OpBranchConditional. We hit G_BR
  // first, so can generate an OpBranchConditional here. If there is no
  // G_BRCOND, we just use OpBranch for a regular unconditional branch.
  const MachineInstr *prevI = I.getPrevNode();
  if (prevI != nullptr && prevI->getOpcode() == TargetOpcode::G_BRCOND) {
    return MIRBuilder.buildInstr(SPIRV::OpBranchConditional)
        .addUse(prevI->getOperand(0).getReg())
        .addMBB(prevI->getOperand(1).getMBB())
        .addMBB(I.getOperand(0).getMBB())
        .constrainAllUses(TII, TRI, RBI);
  } else {
    return MIRBuilder.buildInstr(SPIRV::OpBranch)
        .addMBB(I.getOperand(0).getMBB())
        .constrainAllUses(TII, TRI, RBI);
  }
}

bool SPIRVInstructionSelector::selectBranchCond(
    const MachineInstr &I, MachineIRBuilder &MIRBuilder) const {
  // InstructionSelector walks backwards through the instructions. For an
  // explicit conditional branch with no fallthrough, we use both a G_BR and a
  // G_BRCOND to create an OpBranchConditional. We should hit G_BR first, and
  // generate the OpBranchConditional in selectBranch above.
  //
  // If an OpBranchConditional has been generated, we simply return, as the work
  // is alread done. If there is no OpBranchConditional, LLVM must be relying on
  // implicit fallthrough to the next basic block, so we need to create an
  // OpBranchConditional with an explicit "false" argument pointing to the next
  // basic block that LLVM would fall through to.
  const MachineInstr *nextI = I.getNextNode();
  // Check if this has already been successfully selected
  if (nextI != nullptr && nextI->getOpcode() == SPIRV::OpBranchConditional) {
    return true;
  } else {
    // Must be relying on implicit block fallthrough, so generate an
    // OpBranchConditional with the "next" basic block as the "false" target.
    auto nextMBBNum = I.getParent()->getNextNode()->getNumber();
    auto nextMBB = I.getMF()->getBlockNumbered(nextMBBNum);
    return MIRBuilder.buildInstr(SPIRV::OpBranchConditional)
        .addUse(I.getOperand(0).getReg())
        .addMBB(I.getOperand(1).getMBB())
        .addMBB(nextMBB)
        .constrainAllUses(TII, TRI, RBI);
  }
}

bool SPIRVInstructionSelector::selectPhi(Register resVReg,
                                         const SPIRVType *resType,
                                         const MachineInstr &I,
                                         MachineIRBuilder &MIRBuilder) const {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpPhi)
                 .addDef(resVReg)
                 .addUse(TR.getSPIRVTypeID(resType));
  const unsigned int numOps = I.getNumOperands();
  assert((numOps % 2 == 1) && "Require odd number of operands for G_PHI");
  for (unsigned int i = 1; i < numOps; i += 2) {
    MIB.addUse(I.getOperand(i + 0).getReg());
    MIB.addMBB(I.getOperand(i + 1).getMBB());
  }
  return MIB.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectGlobalValue(
    Register ResVReg, const MachineInstr &I, MachineIRBuilder &MIRBuilder,
    const MachineInstr *Init) const {
  auto *GV = I.getOperand(1).getGlobal();
  SPIRVType *ResType =
      TR.getOrCreateSPIRVType(GV->getType(), MIRBuilder, AQ::ReadWrite, false);

  auto GlobalIdent = GV->getGlobalIdentifier();
  auto GlobalVar = dyn_cast<GlobalVariable>(GV);

  bool HasInit = GlobalVar && GlobalVar->hasInitializer() &&
                 !isa<UndefValue>(GlobalVar->getInitializer());
  // Skip empty declaration for GVs with initilaizers till we get the decl with
  // passed initializer.
  if (HasInit && !Init)
    return true;

  auto AddrSpace = GV->getAddressSpace();
  auto Storage = TR.addressSpaceToStorageClass(AddrSpace);
  bool HasLnkTy = GV->getLinkage() != GlobalValue::InternalLinkage &&
      Storage != StorageClass::Function;
  auto LnkType = (GV->isDeclaration() || GV->hasAvailableExternallyLinkage()) ?
                 LinkageType::Import : LinkageType::Export;

  auto Reg = TR.buildGlobalVariable(ResVReg, ResType, GlobalIdent, GV, Storage,
    Init, GlobalVar && GlobalVar->isConstant(), HasLnkTy, LnkType, MIRBuilder);
  return Reg.isValid();
}

namespace llvm {
InstructionSelector *
createSPIRVInstructionSelector(const SPIRVTargetMachine &TM,
                               const SPIRVSubtarget &Subtarget,
                               const SPIRVRegisterBankInfo &RBI) {
  return new SPIRVInstructionSelector(TM, Subtarget, RBI);
}
} // namespace llvm
