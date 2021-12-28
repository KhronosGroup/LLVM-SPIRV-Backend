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
#include "SPIRVUtils.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVGlobalRegistry.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelectorImpl.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/IntrinsicsSPIRV.h"
#include "llvm/Support/Debug.h"

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
  bool spvSelect(Register ResVReg, const SPIRVType *ResType,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectGlobalValue(Register ResVReg, const MachineInstr &I,
                         MachineIRBuilder &MIRBuilder,
                         const MachineInstr *Init = nullptr) const;

  bool selectUnOpWithSrc(Register ResVReg, const SPIRVType *ResType,
                         const MachineInstr &I, Register SrcReg,
                         MachineIRBuilder &MIRBuilder, unsigned Opcode) const;
  bool selectUnOp(Register ResVReg, const SPIRVType *ResType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                  unsigned Opcode) const;

  bool selectLoad(Register ResVReg, const SPIRVType *ResType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectStore(const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectMemOperation(Register ResVReg, const MachineInstr &I,
                          MachineIRBuilder &MIRBuilder) const;

  bool selectAtomicRMW(Register ResVReg, const SPIRVType *ResType,
                       const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                       unsigned NewOpcode) const;

  bool selectAtomicCmpXchg(Register ResVReg, const SPIRVType *ResType,
                           const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                           bool WithSuccess) const;

  bool selectFence(const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectAddrSpaceCast(Register ResVReg, const SPIRVType *ResType,
                           const MachineInstr &I,
                           MachineIRBuilder &MIRBuilder) const;

#if 0
  bool selectOverflowOp(Register ResVReg, const SPIRVType *ResType,
                        const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                        unsigned NewOpcode) const;
#endif

  bool selectBitreverse(Register ResVReg, const SPIRVType *ResType,
                        const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectConstVector(Register ResVReg, const SPIRVType *ResType,
                         const MachineInstr &I,
                         MachineIRBuilder &MIRBuilder) const;

  bool selectCmp(Register ResVReg, const SPIRVType *ResType,
                 unsigned scalarTypeOpcode, unsigned comparisonOpcode,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectICmp(Register ResVReg, const SPIRVType *ResType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;
  bool selectFCmp(Register ResVReg, const SPIRVType *ResType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  void renderImm32(MachineInstrBuilder &MIB, const MachineInstr &I,
                   int OpIdx) const;
  void renderFImm32(MachineInstrBuilder &MIB, const MachineInstr &I,
                    int OpIdx) const;

  bool selectConst(Register ResVReg, const SPIRVType *ResType, const APInt &Imm,
                   MachineIRBuilder &MIRBuilder) const;

  bool selectSelect(Register ResVReg, const SPIRVType *ResType,
                    const MachineInstr &I, bool IsSigned,
                    MachineIRBuilder &MIRBuilder) const;
  bool selectIToF(Register ResVReg, const SPIRVType *ResType,
                  const MachineInstr &I, bool IsSigned,
                  MachineIRBuilder &MIRBuilder, unsigned Opcode) const;
  bool selectExt(Register ResVReg, const SPIRVType *ResType,
                 const MachineInstr &I, bool IsSigned,
                 MachineIRBuilder &MIRBuilder) const;

  bool selectTrunc(Register ResVReg, const SPIRVType *ResType,
                   const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectIntToBool(Register IntReg, Register ResVReg,
                       const SPIRVType *intTy, const SPIRVType *boolTy,
                       MachineIRBuilder &MIRBuilder) const;

  bool selectOpUndef(Register ResVReg, const SPIRVType *ResType,
                     MachineIRBuilder &MIRBuilder) const;
  bool selectIntrinsic(Register ResVReg, const SPIRVType *ResType,
                       const MachineInstr &I,
                       MachineIRBuilder &MIRBuilder) const;
  bool selectExtractVal(Register ResVReg, const SPIRVType *ResType,
                        const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;
  bool selectInsertVal(Register ResVReg, const SPIRVType *ResType,
                       const MachineInstr &I,
                       MachineIRBuilder &MIRBuilder) const;
  bool selectExtractElt(Register ResVReg, const SPIRVType *ResType,
                        const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;
  bool selectInsertElt(Register ResVReg, const SPIRVType *ResType,
                       const MachineInstr &I,
                       MachineIRBuilder &MIRBuilder) const;
  bool selectGEP(Register ResVReg, const SPIRVType *ResType,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectFrameIndex(Register ResVReg, const SPIRVType *ResType,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectBranch(const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;
  bool selectBranchCond(const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectPhi(Register ResVReg, const SPIRVType *ResType,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectExtInst(Register ResVReg, const SPIRVType *ResType,
                     const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                     CL::OpenCL_std CLInst) const;
  bool selectExtInst(Register ResVReg, const SPIRVType *ResType,
                     const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                     OpenCL_std::OpenCL_std CLInst,
                     GL::GLSL_std_450 GLInst) const;

  bool selectExtInst(Register ResVReg, const SPIRVType *ResType,
                     const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                     const ExtInstList &ExtInsts) const;

  Register buildI32Constant(uint32_t Val, MachineIRBuilder &MIRBuilder,
                            const SPIRVType *ResType = nullptr) const;

  Register buildZerosVal(const SPIRVType *ResType,
                         MachineIRBuilder &MIRBuilder) const;
  Register buildOnesVal(bool AllOnes, const SPIRVType *ResType,
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
  bool HasDefs = I.getNumDefs() > 0;
  Register ResVReg = HasDefs ? I.getOperand(0).getReg() : Register(0);
  SPIRVType *ResType = HasDefs ? TR.getSPIRVTypeForVReg(ResVReg) : nullptr;
  assert(!HasDefs || ResType || I.getOpcode() == TargetOpcode::G_GLOBAL_VALUE);
  if (spvSelect(ResVReg, ResType, I, MIRBuilder)) {
    if (HasDefs) { // Make all vregs 32 bits (for SPIR-V IDs)
      MIRBuilder.getMRI()->setType(ResVReg, LLT::scalar(32));
    }
    I.removeFromParent();
    return true;
  }
  return false;
}

bool SPIRVInstructionSelector::spvSelect(Register ResVReg,
                                         const SPIRVType *ResType,
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
    return selectConst(ResVReg, ResType, I.getOperand(1).getCImm()->getValue(),
                       MIRBuilder);
  case TargetOpcode::G_GLOBAL_VALUE:
    return selectGlobalValue(ResVReg, I, MIRBuilder);
  case TargetOpcode::G_IMPLICIT_DEF:
    return selectOpUndef(ResVReg, ResType, MIRBuilder);

  case TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS:
    return selectIntrinsic(ResVReg, ResType, I, MIRBuilder);
  case TargetOpcode::G_BITREVERSE:
    return selectBitreverse(ResVReg, ResType, I, MIRBuilder);

  case TargetOpcode::G_BUILD_VECTOR:
    return selectConstVector(ResVReg, ResType, I, MIRBuilder);

  case TargetOpcode::G_SHUFFLE_VECTOR: {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpVectorShuffle)
                   .addDef(ResVReg)
                   .addUse(TR.getSPIRVTypeID(ResType))
                   .addUse(I.getOperand(1).getReg())
                   .addUse(I.getOperand(2).getReg());
    for (auto V : I.getOperand(3).getShuffleMask())
      MIB.addImm(V);

    return MIB.constrainAllUses(TII, TRI, RBI);
  }
  case TargetOpcode::G_MEMMOVE:
  case TargetOpcode::G_MEMCPY:
    return selectMemOperation(ResVReg, I, MIRBuilder);

  case TargetOpcode::G_ICMP:
    return selectICmp(ResVReg, ResType, I, MIRBuilder);
  case TargetOpcode::G_FCMP:
    return selectFCmp(ResVReg, ResType, I, MIRBuilder);

  case TargetOpcode::G_FRAME_INDEX:
    return selectFrameIndex(ResVReg, ResType, MIRBuilder);

  case TargetOpcode::G_LOAD:
    return selectLoad(ResVReg, ResType, I, MIRBuilder);
  case TargetOpcode::G_STORE:
    return selectStore(I, MIRBuilder);

  case TargetOpcode::G_BR:
    return selectBranch(I, MIRBuilder);
  case TargetOpcode::G_BRCOND:
    return selectBranchCond(I, MIRBuilder);

  case TargetOpcode::G_PHI:
    return selectPhi(ResVReg, ResType, I, MIRBuilder);

  case TargetOpcode::G_FPTOSI:
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpConvertFToS);
  case TargetOpcode::G_FPTOUI:
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpConvertFToU);

  case TargetOpcode::G_SITOFP:
    return selectIToF(ResVReg, ResType, I, true, MIRBuilder, OpConvertSToF);
  case TargetOpcode::G_UITOFP:
    return selectIToF(ResVReg, ResType, I, false, MIRBuilder, OpConvertUToF);

  case TargetOpcode::G_CTPOP:
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpBitCount);

    // even SPIRV-LLVM translator doens't support overflow intrinsics
    // so we don't even have a reliable tests for this functionality
#if 0
  case TargetOpcode::G_UADDO:
    return selectOverflowOp(ResVReg, ResType, I, MIRBuilder, OpIAddCarry);
  case TargetOpcode::G_USUBO:
    return selectOverflowOp(ResVReg, ResType, I, MIRBuilder, OpISubBorrow);
  case TargetOpcode::G_UMULO:
    return selectOverflowOp(ResVReg, ResType, I, MIRBuilder, OpSMulExtended);
  case TargetOpcode::G_SMULO:
    return selectOverflowOp(ResVReg, ResType, I, MIRBuilder, OpUMulExtended);
#endif

  case TargetOpcode::G_SMIN:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::s_min, GL::SMin);
  case TargetOpcode::G_UMIN:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::u_min, GL::UMin);

  case TargetOpcode::G_SMAX:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::s_max, GL::SMax);
  case TargetOpcode::G_UMAX:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::u_max, GL::UMax);

  case TargetOpcode::G_FMA:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::fma, GL::Fma);

  case TargetOpcode::G_FPOW:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::pow, GL::Pow);
  case TargetOpcode::G_FPOWI:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::pown);

  case TargetOpcode::G_FEXP:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::exp, GL::Exp);
  case TargetOpcode::G_FEXP2:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::exp2, GL::Exp2);

  case TargetOpcode::G_FLOG:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::log, GL::Log);
  case TargetOpcode::G_FLOG2:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::log2, GL::Log2);
  case TargetOpcode::G_FLOG10:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::log10);

  case TargetOpcode::G_FABS:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::fabs, GL::FAbs);
  case TargetOpcode::G_ABS:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::s_abs, GL::SAbs);

  case TargetOpcode::G_FMINNUM:
  case TargetOpcode::G_FMINIMUM:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::fmin, GL::FMin);
  case TargetOpcode::G_FMAXNUM:
  case TargetOpcode::G_FMAXIMUM:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::fmax, GL::FMax);

  case TargetOpcode::G_FCOPYSIGN:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::copysign);

  case TargetOpcode::G_FCEIL:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::ceil, GL::Ceil);
  case TargetOpcode::G_FFLOOR:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::floor, GL::Floor);

  case TargetOpcode::G_FCOS:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::cos, GL::Cos);
  case TargetOpcode::G_FSIN:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::sin, GL::Sin);

  case TargetOpcode::G_FSQRT:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::sqrt, GL::Sqrt);

  case TargetOpcode::G_CTTZ:
  case TargetOpcode::G_CTTZ_ZERO_UNDEF:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::ctz);
  case TargetOpcode::G_CTLZ:
  case TargetOpcode::G_CTLZ_ZERO_UNDEF:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::clz);

  case TargetOpcode::G_INTRINSIC_ROUND:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::round, GL::Round);
  case TargetOpcode::G_INTRINSIC_ROUNDEVEN:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::rint,
                         GL::RoundEven);
  case TargetOpcode::G_INTRINSIC_TRUNC:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::trunc, GL::Trunc);
  case TargetOpcode::G_FRINT:
  case TargetOpcode::G_FNEARBYINT:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::rint,
                         GL::RoundEven);

  case TargetOpcode::G_SMULH:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::s_mul_hi);
  case TargetOpcode::G_UMULH:
    return selectExtInst(ResVReg, ResType, I, MIRBuilder, CL::u_mul_hi);

  case TargetOpcode::G_SEXT:
    return selectExt(ResVReg, ResType, I, true, MIRBuilder);
  case TargetOpcode::G_ANYEXT:
  case TargetOpcode::G_ZEXT:
    return selectExt(ResVReg, ResType, I, false, MIRBuilder);
  case TargetOpcode::G_TRUNC:
    return selectTrunc(ResVReg, ResType, I, MIRBuilder);
  case TargetOpcode::G_FPTRUNC:
  case TargetOpcode::G_FPEXT:
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpFConvert);

  case TargetOpcode::G_PTRTOINT:
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpConvertPtrToU);
  case TargetOpcode::G_INTTOPTR:
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpConvertUToPtr);
  case TargetOpcode::G_BITCAST:
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpBitcast);
  case TargetOpcode::G_ADDRSPACE_CAST:
    return selectAddrSpaceCast(ResVReg, ResType, I, MIRBuilder);

  case TargetOpcode::G_ATOMICRMW_OR:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicOr);
  case TargetOpcode::G_ATOMICRMW_ADD:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicIAdd);
  case TargetOpcode::G_ATOMICRMW_AND:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicAnd);
  case TargetOpcode::G_ATOMICRMW_MAX:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicSMax);
  case TargetOpcode::G_ATOMICRMW_MIN:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicSMin);
  case TargetOpcode::G_ATOMICRMW_SUB:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicISub);
  case TargetOpcode::G_ATOMICRMW_XOR:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicXor);
  case TargetOpcode::G_ATOMICRMW_UMAX:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicUMax);
  case TargetOpcode::G_ATOMICRMW_UMIN:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicUMin);
  case TargetOpcode::G_ATOMICRMW_XCHG:
    return selectAtomicRMW(ResVReg, ResType, I, MIRBuilder, OpAtomicExchange);

  case TargetOpcode::G_ATOMIC_CMPXCHG_WITH_SUCCESS:
    return selectAtomicCmpXchg(ResVReg, ResType, I, MIRBuilder, true);
  case TargetOpcode::G_ATOMIC_CMPXCHG:
    return selectAtomicCmpXchg(ResVReg, ResType, I, MIRBuilder, false);

  case TargetOpcode::G_FENCE:
    return selectFence(I, MIRBuilder);

  default:
    return false;
  }
}

bool SPIRVInstructionSelector::selectExtInst(Register ResVReg,
                                             const SPIRVType *ResType,
                                             const MachineInstr &I,
                                             MachineIRBuilder &MIRBuilder,
                                             CL::OpenCL_std CLInst) const {
  return selectExtInst(ResVReg, ResType, I, MIRBuilder,
                       {{ExtInstSet::OpenCL_std, CLInst}});
}

bool SPIRVInstructionSelector::selectExtInst(Register ResVReg,
                                             const SPIRVType *ResType,
                                             const MachineInstr &I,
                                             MachineIRBuilder &MIRBuilder,
                                             CL::OpenCL_std CLInst,
                                             GL::GLSL_std_450 GLInst) const {
  ExtInstList ExtInsts = {{ExtInstSet::OpenCL_std, CLInst},
                          {ExtInstSet::GLSL_std_450, GLInst}};
  return selectExtInst(ResVReg, ResType, I, MIRBuilder, ExtInsts);
}

bool SPIRVInstructionSelector::selectExtInst(Register ResVReg,
                                             const SPIRVType *ResType,
                                             const MachineInstr &I,
                                             MachineIRBuilder &MIRBuilder,
                                             const ExtInstList &Insts) const {

  for (const auto &Ex : Insts) {
    ExtInstSet Set = Ex.first;
    uint32_t Opcode = Ex.second;
    if (STI.canUseExtInstSet(Set)) {
      auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                     .addDef(ResVReg)
                     .addUse(TR.getSPIRVTypeID(ResType))
                     .addImm(static_cast<uint32_t>(Set))
                     .addImm(Opcode);
      const unsigned NumOps = I.getNumOperands();
      for (unsigned i = 1; i < NumOps; ++i) {
        MIB.add(I.getOperand(i));
      }
      return MIB.constrainAllUses(TII, TRI, RBI);
    }
  }
  return false;
}

static bool canUseNSW(unsigned Opcode) {
  using namespace SPIRV;
  switch (Opcode) {
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

static bool canUseNUW(unsigned Opcode) {
  using namespace SPIRV;
  switch (Opcode) {
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

bool SPIRVInstructionSelector::selectUnOpWithSrc(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    Register SrcReg, MachineIRBuilder &MIRBuilder, unsigned Opcode) const {
  handleIntegerWrapFlags(I, ResVReg, Opcode, STI, MIRBuilder, TR);
  return MIRBuilder.buildInstr(Opcode)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(SrcReg)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectUnOp(Register ResVReg,
                                          const SPIRVType *ResType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder,
                                          unsigned Opcode) const {
  return selectUnOpWithSrc(ResVReg, ResType, I, I.getOperand(1).getReg(),
                           MIRBuilder, Opcode);
}

static MemorySemantics::MemorySemantics getMemSemantics(AtomicOrdering Ord) {
  switch (Ord) {
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

static Scope::Scope getScope(SyncScope::ID Ord) {
  switch (Ord) {
  case SyncScope::SingleThread:
    return Scope::Invocation;
  case SyncScope::System:
    return Scope::Device;
  default:
    llvm_unreachable("Unsupported synchronization Scope ID.");
  }
}

static void addMemoryOperands(MachineMemOperand *MemOp,
                              MachineInstrBuilder &MIB) {
  uint32_t SpvMemOp = MemoryOperand::None;
  if (MemOp->isVolatile())
    SpvMemOp |= MemoryOperand::Volatile;
  if (MemOp->isNonTemporal())
    SpvMemOp |= MemoryOperand::Nontemporal;
  if (MemOp->getAlign().value())
    SpvMemOp |= MemoryOperand::Aligned;

  if (SpvMemOp != MemoryOperand::None) {
    MIB.addImm(SpvMemOp);
    if (SpvMemOp & MemoryOperand::Aligned)
      MIB.addImm(MemOp->getAlign().value());
  }
}

static void addMemoryOperands(uint64_t Flags, MachineInstrBuilder &MIB) {
  uint32_t SpvMemOp = MemoryOperand::None;
  if (Flags & MachineMemOperand::Flags::MOVolatile)
    SpvMemOp |= MemoryOperand::Volatile;
  if (Flags & MachineMemOperand::Flags::MONonTemporal)
    SpvMemOp |= MemoryOperand::Nontemporal;
  // if (MemOp->getAlign().value())
  //   SpvMemOp |= MemoryOperand::Aligned;

  if (SpvMemOp != MemoryOperand::None) {
    MIB.addImm(SpvMemOp);
    // if (SpvMemOp & MemoryOperand::Aligned)
    //   MIB.addImm(MemOp->getAlign().value());
  }
}

bool SPIRVInstructionSelector::selectLoad(Register ResVReg,
                                          const SPIRVType *ResType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder) const {
  unsigned OpOffset =
      I.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS ? 1 : 0;
  auto Ptr = I.getOperand(1 + OpOffset).getReg();
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpLoad)
                 .addDef(ResVReg)
                 .addUse(TR.getSPIRVTypeID(ResType))
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

bool SPIRVInstructionSelector::selectMemOperation(
    Register ResVReg, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
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

bool SPIRVInstructionSelector::selectAtomicRMW(Register ResVReg,
                                               const SPIRVType *ResType,
                                               const MachineInstr &I,
                                               MachineIRBuilder &MIRBuilder,
                                               unsigned NewOpcode) const {
  assert(I.hasOneMemOperand());
  auto MemOp = *I.memoperands_begin();
  auto Scope = getScope(MemOp->getSyncScopeID());
  Register ScopeReg = buildI32Constant(Scope, MIRBuilder);

  auto Ptr = I.getOperand(1).getReg();
  // Changed as it's implemented in the translator. See test/atomicrmw.ll
  // auto ScSem =
  // getMemSemanticsForStorageClass(TR.getPointerStorageClass(Ptr));

  auto MemSem = getMemSemantics(MemOp->getSuccessOrdering());
  Register MemSemReg = buildI32Constant(MemSem /*| ScSem*/, MIRBuilder);

  return MIRBuilder.buildInstr(NewOpcode)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(Ptr)
      .addUse(ScopeReg)
      .addUse(MemSemReg)
      .addUse(I.getOperand(2).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectFence(const MachineInstr &I,
                                           MachineIRBuilder &MIRBuilder) const {
  auto MemSem = getMemSemantics(AtomicOrdering(I.getOperand(0).getImm()));
  Register MemSemReg = buildI32Constant(MemSem, MIRBuilder);

  auto Scope = getScope(SyncScope::ID(I.getOperand(1).getImm()));
  Register ScopeReg = buildI32Constant(Scope, MIRBuilder);

  return MIRBuilder.buildInstr(SPIRV::OpMemoryBarrier)
      .addUse(ScopeReg)
      .addUse(MemSemReg)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectAtomicCmpXchg(Register ResVReg,
                                                   const SPIRVType *ResType,
                                                   const MachineInstr &I,
                                                   MachineIRBuilder &MIRBuilder,
                                                   bool WithSuccess) const {
  auto MRI = MIRBuilder.getMRI();
  assert(I.hasOneMemOperand());
  auto MemOp = *I.memoperands_begin();
  auto Scope = getScope(MemOp->getSyncScopeID());
  Register ScopeReg = buildI32Constant(Scope, MIRBuilder);

  auto Ptr = I.getOperand(2).getReg();
  auto Cmp = I.getOperand(3).getReg();
  auto Val = I.getOperand(4).getReg();

  auto SpvValTy = TR.getSPIRVTypeForVReg(Val);
  auto ScSem = getMemSemanticsForStorageClass(TR.getPointerStorageClass(Ptr));

  auto MemSemEq = getMemSemantics(MemOp->getSuccessOrdering()) | ScSem;
  Register MemSemEqReg = buildI32Constant(MemSemEq, MIRBuilder);

  auto MemSemNeq = getMemSemantics(MemOp->getFailureOrdering()) | ScSem;
  Register MemSemNeqReg = MemSemEq == MemSemNeq
                              ? MemSemEqReg
                              : buildI32Constant(MemSemNeq, MIRBuilder);

  auto TmpReg = WithSuccess ? MRI->createGenericVirtualRegister(LLT::scalar(32))
                            : ResVReg;
  bool Success = MIRBuilder.buildInstr(SPIRV::OpAtomicCompareExchange)
                     .addDef(TmpReg)
                     .addUse(TR.getSPIRVTypeID(SpvValTy))
                     .addUse(Ptr)
                     .addUse(ScopeReg)
                     .addUse(MemSemEqReg)
                     .addUse(MemSemNeqReg)
                     .addUse(Val)
                     .addUse(Cmp)
                     .constrainAllUses(TII, TRI, RBI);
  if (!WithSuccess) // If we just need the old Val, not {oldVal, Success}
    return Success;
  assert(ResType->getOpcode() == SPIRV::OpTypeStruct);
  auto BoolReg = MRI->createGenericVirtualRegister(LLT::scalar(1));
  Register BoolTyID = ResType->getOperand(2).getReg();
  Success &= MIRBuilder.buildInstr(SPIRV::OpIEqual)
                 .addDef(BoolReg)
                 .addUse(BoolTyID)
                 .addUse(TmpReg)
                 .addUse(Cmp)
                 .constrainAllUses(TII, TRI, RBI);
  return Success && MIRBuilder.buildInstr(SPIRV::OpCompositeConstruct)
                        .addDef(ResVReg)
                        .addUse(TR.getSPIRVTypeID(ResType))
                        .addUse(TmpReg)
                        .addUse(BoolReg)
                        .constrainAllUses(TII, TRI, RBI);
}

static bool isGenericCastablePtr(StorageClass::StorageClass Sc) {
  switch (Sc) {
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
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;
  namespace SC = StorageClass;
  auto SrcPtr = I.getOperand(1).getReg();
  auto SrcPtrTy = TR.getSPIRVTypeForVReg(SrcPtr);
  auto SrcSC = TR.getPointerStorageClass(SrcPtr);
  auto DstSC = TR.getPointerStorageClass(ResVReg);

  if (DstSC == SC::Generic && isGenericCastablePtr(SrcSC)) {
    // We're casting from an eligable pointer to Generic
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpPtrCastToGeneric);
  } else if (SrcSC == SC::Generic && isGenericCastablePtr(DstSC)) {
    // We're casting from Generic to an eligable pointer
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpGenericCastToPtr);
  } else if (isGenericCastablePtr(SrcSC) && isGenericCastablePtr(DstSC)) {
    // We're casting between 2 eligable pointers using Generic as an
    // intermediary
    auto Tmp = MIRBuilder.getMRI()->createVirtualRegister(&IDRegClass);
    auto GenericPtrTy = TR.getOrCreateSPIRVPointerType(SrcPtrTy, MIRBuilder,
                                                       StorageClass::Generic);
    bool Success = MIRBuilder.buildInstr(OpPtrCastToGeneric)
                       .addDef(Tmp)
                       .addUse(TR.getSPIRVTypeID(GenericPtrTy))
                       .addUse(SrcPtr)
                       .constrainAllUses(TII, TRI, RBI);
    return Success && MIRBuilder.buildInstr(OpGenericCastToPtr)
                          .addDef(ResVReg)
                          .addUse(TR.getSPIRVTypeID(ResType))
                          .addUse(Tmp)
                          .constrainAllUses(TII, TRI, RBI);
  } else {
    // TODO Should this case just be disallowed completely?
    // We're casting 2 other arbitrary address spaces, so have to bitcast
    return selectUnOp(ResVReg, ResType, I, MIRBuilder, OpBitcast);
  }
}

#if 0
bool SPIRVInstructionSelector::selectOverflowOp(Register ResVReg,
                                                const SPIRVType *ResType,
                                                const MachineInstr &I,
                                                MachineIRBuilder &MIRBuilder,
                                                unsigned NewOpcode) const {
  using namespace SPIRV;
  auto MRI = MIRBuilder.getMRI();

  auto elem0TyReg = ResType->getOperand(1).getReg();
  SPIRVType *elem0Ty = TR.getSPIRVTypeForVReg(elem0TyReg);
  assert(TR.isScalarOrVectorOfType(elem0TyReg, OpTypeInt));

  auto TmpReg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  auto tmpStructTy = TR.getPairStruct(elem0Ty, elem0Ty, MIRBuilder);

  bool Success = MIRBuilder.buildInstr(NewOpcode)
                     .addDef(TmpReg)
                     .addUse(TR.getSPIRVTypeID(tmpStructTy))
                     .addUse(I.getOperand(2).getReg())
                     .addUse(I.getOperand(3).getReg())
                     .constrainAllUses(TII, TRI, RBI);

  // Need to build an {int, bool} struct from the {int, int} TR. result
  auto elem0Reg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  Success &= MIRBuilder.buildInstr(OpCompositeExtract)
                 .addDef(elem0Reg)
                 .addUse(TR.getSPIRVTypeID(elem0Ty))
                 .addUse(TmpReg)
                 .addImm(0)
                 .constrainAllUses(TII, TRI, RBI);
  auto elem1Reg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  Success &= MIRBuilder.buildInstr(OpCompositeExtract)
                 .addDef(elem1Reg)
                 .addUse(TR.getSPIRVTypeID(elem0Ty))
                 .addUse(TmpReg)
                 .addImm(1)
                 .constrainAllUses(TII, TRI, RBI);
  auto BoolReg = MRI->createGenericVirtualRegister(LLT::scalar(1));
  auto boolTy = TR.getSPIRVTypeForVReg(ResType->getOperand(2).getReg());
  Success &= selectIntToBool(elem1Reg, BoolReg, elem0Ty, boolTy, MIRBuilder);
  return Success && MIRBuilder.buildInstr(OpCompositeConstruct)
                        .addDef(ResVReg)
                        .addUse(TR.getSPIRVTypeID(ResType))
                        .addUse(elem0Reg)
                        .addUse(BoolReg)
                        .constrainAllUses(TII, TRI, RBI);
}
#endif

static unsigned int getFCmpOpcode(unsigned PredNum) {
  auto Pred = static_cast<CmpInst::Predicate>(PredNum);
  switch (Pred) {
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
                       CmpInst::getPredicateName(Pred));
  }
}

static unsigned int getICmpOpcode(unsigned PredNum) {
  auto Pred = static_cast<CmpInst::Predicate>(PredNum);
  switch (Pred) {
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
                       CmpInst::getPredicateName(Pred));
  }
}

// Return <CmpOpcode, needsConvertedToInt>
static std::pair<unsigned int, bool> getPtrCmpOpcode(unsigned Pred) {
  switch (static_cast<CmpInst::Predicate>(Pred)) {
  case CmpInst::ICMP_EQ:
    return {SPIRV::OpPtrEqual, false};
  case CmpInst::ICMP_NE:
    return {SPIRV::OpPtrNotEqual, false};
  default:
    return {getICmpOpcode(Pred), true};
  }
}

// Return the logical operation, or abort if none exists
static unsigned int getBoolCmpOpcode(unsigned PredNum) {
  auto Pred = static_cast<CmpInst::Predicate>(PredNum);
  switch (Pred) {
  case CmpInst::ICMP_EQ:
    return SPIRV::OpLogicalEqual;
  case CmpInst::ICMP_NE:
    return SPIRV::OpLogicalNotEqual;
  default:
    report_fatal_error("Unknown predicate type for Bool comparison: " +
                       CmpInst::getPredicateName(Pred));
  }
}

bool SPIRVInstructionSelector::selectBitreverse(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpBitReverse)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(I.getOperand(1).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectConstVector(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
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
                 .addDef(ResVReg)
                 .addUse(TR.getSPIRVTypeID(ResType));
  for (unsigned i = I.getNumExplicitDefs(); i < I.getNumExplicitOperands(); ++i)
    MIB.addUse(I.getOperand(i).getReg());
  return MIB.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectCmp(Register ResVReg,
                                         const SPIRVType *ResType,
                                         unsigned scalarTyOpc, unsigned CmpOpc,
                                         const MachineInstr &I,
                                         MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;

  Register Cmp0 = I.getOperand(2).getReg();
  Register Cmp1 = I.getOperand(3).getReg();
  SPIRVType *Cmp0Type = TR.getSPIRVTypeForVReg(Cmp0);
  SPIRVType *Cmp1Type = TR.getSPIRVTypeForVReg(Cmp1);
  assert(Cmp0Type->getOpcode() == Cmp1Type->getOpcode());

  if (Cmp0Type->getOpcode() != OpTypePointer &&
      (!TR.isScalarOrVectorOfType(Cmp0, scalarTyOpc) ||
       !TR.isScalarOrVectorOfType(Cmp1, scalarTyOpc)))
    llvm_unreachable("Incompatible type for comparison");

  return MIRBuilder.buildInstr(CmpOpc)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(Cmp0)
      .addUse(Cmp1)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectICmp(Register ResVReg,
                                          const SPIRVType *ResType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder) const {

  auto Pred = I.getOperand(1).getPredicate();
  unsigned CmpOpc = getICmpOpcode(Pred);
  unsigned TypeOpc = SPIRV::OpTypeInt;
  bool PtrToUInt = false;

  Register CmpOperand = I.getOperand(2).getReg();
  if (TR.isScalarOfType(CmpOperand, SPIRV::OpTypePointer)) {
    if (STI.canDirectlyComparePointers()) {
      std::tie(CmpOpc, PtrToUInt) = getPtrCmpOpcode(Pred);
    } else {
      PtrToUInt = true;
    }
  } else if (TR.isScalarOrVectorOfType(CmpOperand, SPIRV::OpTypeBool)) {
    TypeOpc = SPIRV::OpTypeBool;
    CmpOpc = getBoolCmpOpcode(Pred);
  }
  return selectCmp(ResVReg, ResType, TypeOpc, CmpOpc, I, MIRBuilder);
}

void SPIRVInstructionSelector::renderFImm32(MachineInstrBuilder &MIB,
                                            const MachineInstr &I,
                                            int OpIdx) const {
  assert(I.getOpcode() == TargetOpcode::G_FCONSTANT && OpIdx == -1 &&
         "Expected G_FCONSTANT");
  const ConstantFP *FPImm = I.getOperand(1).getFPImm();
  addNumImm(FPImm->getValueAPF().bitcastToAPInt(), MIB, true);
}

void SPIRVInstructionSelector::renderImm32(MachineInstrBuilder &MIB,
                                           const MachineInstr &I,
                                           int OpIdx) const {
  assert(I.getOpcode() == TargetOpcode::G_CONSTANT && OpIdx == -1 &&
         "Expected G_CONSTANT");
  addNumImm(I.getOperand(1).getCImm()->getValue(), MIB);
}

Register
SPIRVInstructionSelector::buildI32Constant(uint32_t Val,
                                           MachineIRBuilder &MIRBuilder,
                                           const SPIRVType *ResType) const {
  auto MRI = MIRBuilder.getMRI();
  auto LLVMTy =
      IntegerType::get(MIRBuilder.getMF().getFunction().getContext(), 32);
  auto SpvI32Ty =
      ResType ? ResType : TR.getOrCreateSPIRVType(LLVMTy, MIRBuilder);
  // Find a constant in DT or build a new one.
  auto ConstInt = ConstantInt::get(LLVMTy, Val);
  Register NewReg;
  if (DT.find(ConstInt, &MIRBuilder.getMF(), NewReg) == false) {
    NewReg = MRI->createGenericVirtualRegister(LLT::scalar(32));
    DT.add(ConstInt, &MIRBuilder.getMF(), NewReg);
    MachineInstr *MI;
    if (Val == 0)
      MI = MIRBuilder.buildInstr(SPIRV::OpConstantNull)
               .addDef(NewReg)
               .addUse(TR.getSPIRVTypeID(SpvI32Ty));
    else
      MI = MIRBuilder.buildInstr(SPIRV::OpConstantI)
               .addDef(NewReg)
               .addUse(TR.getSPIRVTypeID(SpvI32Ty))
               .addImm(APInt(32, Val).getZExtValue());
    constrainSelectedInstRegOperands(*MI, TII, TRI, RBI);
  }
  return NewReg;
}

bool SPIRVInstructionSelector::selectFCmp(Register ResVReg,
                                          const SPIRVType *ResType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder) const {
  unsigned int CmpOp = getFCmpOpcode(I.getOperand(1).getPredicate());
  return selectCmp(ResVReg, ResType, SPIRV::OpTypeFloat, CmpOp, I, MIRBuilder);
}

Register
SPIRVInstructionSelector::buildZerosVal(const SPIRVType *ResType,
                                        MachineIRBuilder &MIRBuilder) const {
  return buildI32Constant(0, MIRBuilder, ResType);
}

Register
SPIRVInstructionSelector::buildOnesVal(bool AllOnes, const SPIRVType *ResType,
                                       MachineIRBuilder &MIRBuilder) const {
  auto MRI = MIRBuilder.getMRI();
  unsigned BitWidth = TR.getScalarOrVectorBitWidth(ResType);
  auto One = AllOnes ? APInt::getAllOnesValue(BitWidth)
                     : APInt::getOneBitSet(BitWidth, 0);
  Register OneReg = buildI32Constant(One.getZExtValue(), MIRBuilder, ResType);
  if (ResType->getOpcode() == SPIRV::OpTypeVector) {
    const unsigned NumEles = ResType->getOperand(2).getImm();
    Register OneVec = MRI->createVirtualRegister(&SPIRV::IDRegClass);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantComposite)
                   .addDef(OneVec)
                   .addUse(TR.getSPIRVTypeID(ResType));
    for (unsigned i = 0; i < NumEles; ++i) {
      MIB.addUse(OneReg);
    }
    TR.constrainRegOperands(MIB);
    return OneVec;
  } else {
    return OneReg;
  }
}

bool SPIRVInstructionSelector::selectSelect(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    bool IsSigned, MachineIRBuilder &MIRBuilder) const {
  // To extend a bool, we need to use OpSelect between constants
  using namespace SPIRV;
  Register ZeroReg = buildZerosVal(ResType, MIRBuilder);
  Register OneReg = buildOnesVal(IsSigned, ResType, MIRBuilder);
  bool IsScalarBool = TR.isScalarOfType(I.getOperand(1).getReg(), OpTypeBool);
  return MIRBuilder.buildInstr(IsScalarBool ? OpSelectSISCond : OpSelectSIVCond)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(I.getOperand(1).getReg())
      .addUse(OneReg)
      .addUse(ZeroReg)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectIToF(Register ResVReg,
                                          const SPIRVType *ResType,
                                          const MachineInstr &I, bool IsSigned,
                                          MachineIRBuilder &MIRBuilder,
                                          unsigned Opcode) const {
  Register SrcReg = I.getOperand(1).getReg();
  // We can convert bool value directly to float type without OpConvert*ToF,
  // however the translator generates OpSelect+OpConvert*ToF, so we do the same.
  if (TR.isScalarOrVectorOfType(I.getOperand(1).getReg(), SPIRV::OpTypeBool)) {
    auto BitWidth = TR.getScalarOrVectorBitWidth(ResType);
    SPIRVType *TmpType = TR.getOrCreateSPIRVIntegerType(BitWidth, MIRBuilder);
    if (ResType->getOpcode() == SPIRV::OpTypeVector) {
      const unsigned NumElts = ResType->getOperand(2).getImm();
      TmpType = TR.getOrCreateSPIRVVectorType(TmpType, NumElts, MIRBuilder);
    }
    auto MRI = MIRBuilder.getMRI();
    SrcReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
    selectSelect(SrcReg, TmpType, I, false, MIRBuilder);
  }
  return selectUnOpWithSrc(ResVReg, ResType, I, SrcReg, MIRBuilder, Opcode);
}

bool SPIRVInstructionSelector::selectExt(Register ResVReg,
                                         const SPIRVType *ResType,
                                         const MachineInstr &I, bool IsSigned,
                                         MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;
  if (TR.isScalarOrVectorOfType(I.getOperand(1).getReg(), OpTypeBool)) {
    return selectSelect(ResVReg, ResType, I, IsSigned, MIRBuilder);
  } else {
    return selectUnOp(ResVReg, ResType, I, MIRBuilder,
                      IsSigned ? OpSConvert : OpUConvert);
  }
}

bool SPIRVInstructionSelector::selectIntToBool(
    Register IntReg, Register ResVReg, const SPIRVType *IntTy,
    const SPIRVType *BoolTy, MachineIRBuilder &MIRBuilder) const {
  // To truncate to a bool, we use OpBitwiseAnd 1 and OpINotEqual to zero
  auto MRI = MIRBuilder.getMRI();
  Register BitIntReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
  bool IsVectorTy = IntTy->getOpcode() == SPIRV::OpTypeVector;
  auto Opcode = IsVectorTy ? SPIRV::OpBitwiseAndV : SPIRV::OpBitwiseAndS;
  auto Zero = buildZerosVal(IntTy, MIRBuilder);
  auto One = buildOnesVal(false, IntTy, MIRBuilder);
  MIRBuilder.buildInstr(Opcode)
      .addDef(BitIntReg)
      .addUse(TR.getSPIRVTypeID(IntTy))
      .addUse(IntReg)
      .addUse(One)
      .constrainAllUses(TII, TRI, RBI);
  return MIRBuilder.buildInstr(SPIRV::OpINotEqual)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(BoolTy))
      .addUse(BitIntReg)
      .addUse(Zero)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectTrunc(Register ResVReg,
                                           const SPIRVType *ResType,
                                           const MachineInstr &I,
                                           MachineIRBuilder &MIRBuilder) const {
  using namespace SPIRV;
  if (TR.isScalarOrVectorOfType(ResVReg, OpTypeBool)) {
    auto IntReg = I.getOperand(1).getReg();
    auto ArgType = TR.getSPIRVTypeForVReg(IntReg);
    return selectIntToBool(IntReg, ResVReg, ArgType, ResType, MIRBuilder);
  } else {
    bool IsSigned = TR.isScalarOrVectorSigned(ResType);
    return selectUnOp(ResVReg, ResType, I, MIRBuilder,
                      IsSigned ? OpSConvert : OpUConvert);
  }
}

bool SPIRVInstructionSelector::selectConst(Register ResVReg,
                                           const SPIRVType *ResType,
                                           const APInt &Imm,
                                           MachineIRBuilder &MIRBuilder) const {
  assert(ResType->getOpcode() != SPIRV::OpTypePointer || Imm.isNullValue());
  if (ResType->getOpcode() == SPIRV::OpTypePointer && Imm.isNullValue())
    return MIRBuilder.buildInstr(SPIRV::OpConstantNull)
        .addDef(ResVReg)
        .addUse(TR.getSPIRVTypeID(ResType))
        .constrainAllUses(TII, TRI, RBI);
  else {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantI)
                   .addDef(ResVReg)
                   .addUse(TR.getSPIRVTypeID(ResType));
    // <=32-bit integers should be caught by the sdag pattern
    assert(Imm.getBitWidth() > 32);
    addNumImm(Imm, MIB);
    return MIB.constrainAllUses(TII, TRI, RBI);
  }
}

bool SPIRVInstructionSelector::selectOpUndef(
    Register ResVReg, const SPIRVType *ResType,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpUndef)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
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
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpCompositeInsert)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      // object to insert
      .addUse(I.getOperand(3).getReg())
      // composite to insert into
      .addUse(I.getOperand(2).getReg())
      // TODO: support arbitrary number of indices
      .addImm(foldImm(I.getOperand(4)))
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectExtractVal(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpCompositeExtract)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(I.getOperand(2).getReg())
      // TODO: support arbitrary number of indices
      .addImm(foldImm(I.getOperand(3)))
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectInsertElt(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  if (isImm(I.getOperand(4)))
    return selectInsertVal(ResVReg, ResType, I, MIRBuilder);
  return MIRBuilder.buildInstr(SPIRV::OpVectorInsertDynamic)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(I.getOperand(2).getReg())
      .addUse(I.getOperand(3).getReg())
      .addUse(I.getOperand(4).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectExtractElt(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  if (isImm(I.getOperand(3)))
    return selectExtractVal(ResVReg, ResType, I, MIRBuilder);
  return MIRBuilder.buildInstr(SPIRV::OpVectorExtractDynamic)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addUse(I.getOperand(2).getReg())
      .addUse(I.getOperand(3).getReg())
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectGEP(Register ResVReg,
                                         const SPIRVType *ResType,
                                         const MachineInstr &I,
                                         MachineIRBuilder &MIRBuilder) const {
  // In general we should also support OpAccessChain instrs here (i.e. not
  // PtrAccessChain) but SPIRV-LLVM Translator doesn't emit them at all and so
  // do we to stay compliant with its test and more importantly consumers
  auto Opcode = I.getOperand(2).getImm() ? SPIRV::OpInBoundsPtrAccessChain
                                         : SPIRV::OpPtrAccessChain;
  auto &Res = MIRBuilder.buildInstr(Opcode)
                  .addDef(ResVReg)
                  .addUse(TR.getSPIRVTypeID(ResType))
                  // object to get a pointer to
                  .addUse(I.getOperand(3).getReg());
  // adding indices
  for (unsigned i = 4; i < I.getNumExplicitOperands(); ++i)
    Res.addUse(I.getOperand(i).getReg());
  return Res.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectIntrinsic(
    Register ResVReg, const SPIRVType *ResType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  switch (I.getIntrinsicID()) {
  case Intrinsic::spv_load:
    return selectLoad(ResVReg, ResType, I, MIRBuilder);
    break;
  case Intrinsic::spv_store:
    return selectStore(I, MIRBuilder);
    break;
  case Intrinsic::spv_extractv:
    return selectExtractVal(ResVReg, ResType, I, MIRBuilder);
    break;
  case Intrinsic::spv_insertv:
    return selectInsertVal(ResVReg, ResType, I, MIRBuilder);
    break;
  case Intrinsic::spv_extractelt:
    return selectExtractElt(ResVReg, ResType, I, MIRBuilder);
    break;
  case Intrinsic::spv_insertelt:
    return selectInsertElt(ResVReg, ResType, I, MIRBuilder);
    break;
  case Intrinsic::spv_gep:
    return selectGEP(ResVReg, ResType, I, MIRBuilder);
    break;
  case Intrinsic::spv_unref_global:
  case Intrinsic::spv_init_global: {
    auto *MI = MIRBuilder.getMRI()->getVRegDef(I.getOperand(1).getReg());
    MachineInstr *Init =
        I.getNumExplicitOperands() > 2
            ? MIRBuilder.getMRI()->getVRegDef(I.getOperand(2).getReg())
            : nullptr;
    assert(MI);
    return selectGlobalValue(MI->getOperand(0).getReg(), *MI, MIRBuilder, Init);
  } break;
  case Intrinsic::spv_const_composite: {
    // If no values are attached, the composite is null constant.
    auto IsNull = I.getNumExplicitDefs() + 1 == I.getNumExplicitOperands();
    auto MIB = MIRBuilder
                   .buildInstr(IsNull ? SPIRV::OpConstantNull
                                      : SPIRV::OpConstantComposite)
                   .addDef(ResVReg)
                   .addUse(TR.getSPIRVTypeID(ResType));
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
    Register ResVReg, const SPIRVType *ResType,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpVariable)
      .addDef(ResVReg)
      .addUse(TR.getSPIRVTypeID(ResType))
      .addImm(StorageClass::Function)
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectBranch(
    const MachineInstr &I, MachineIRBuilder &MIRBuilder) const {
  // InstructionSelector walks backwards through the instructions. We can use
  // both a G_BR and a G_BRCOND to create an OpBranchConditional. We hit G_BR
  // first, so can generate an OpBranchConditional here. If there is no
  // G_BRCOND, we just use OpBranch for a regular unconditional branch.
  const MachineInstr *PrevI = I.getPrevNode();
  if (PrevI != nullptr && PrevI->getOpcode() == TargetOpcode::G_BRCOND) {
    return MIRBuilder.buildInstr(SPIRV::OpBranchConditional)
        .addUse(PrevI->getOperand(0).getReg())
        .addMBB(PrevI->getOperand(1).getMBB())
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
  const MachineInstr *NextI = I.getNextNode();
  // Check if this has already been successfully selected
  if (NextI != nullptr && NextI->getOpcode() == SPIRV::OpBranchConditional) {
    return true;
  } else {
    // Must be relying on implicit block fallthrough, so generate an
    // OpBranchConditional with the "next" basic block as the "false" target.
    auto NextMBBNum = I.getParent()->getNextNode()->getNumber();
    auto NextMBB = I.getMF()->getBlockNumbered(NextMBBNum);
    return MIRBuilder.buildInstr(SPIRV::OpBranchConditional)
        .addUse(I.getOperand(0).getReg())
        .addMBB(I.getOperand(1).getMBB())
        .addMBB(NextMBB)
        .constrainAllUses(TII, TRI, RBI);
  }
}

bool SPIRVInstructionSelector::selectPhi(Register ResVReg,
                                         const SPIRVType *ResType,
                                         const MachineInstr &I,
                                         MachineIRBuilder &MIRBuilder) const {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpPhi)
                 .addDef(ResVReg)
                 .addUse(TR.getSPIRVTypeID(ResType));
  const unsigned int NumOps = I.getNumOperands();
  assert((NumOps % 2 == 1) && "Require odd number of operands for G_PHI");
  for (unsigned int i = 1; i < NumOps; i += 2) {
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
  auto LnkType = (GV->isDeclaration() || GV->hasAvailableExternallyLinkage())
                     ? LinkageType::Import
                     : LinkageType::Export;

  auto Reg = TR.buildGlobalVariable(ResVReg, ResType, GlobalIdent, GV, Storage,
                                    Init, GlobalVar && GlobalVar->isConstant(),
                                    HasLnkTy, LnkType, MIRBuilder);
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
