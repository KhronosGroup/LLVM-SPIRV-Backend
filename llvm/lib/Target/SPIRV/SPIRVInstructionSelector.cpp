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

#include "SPIRVTypeRegistry.h"

#define DEBUG_TYPE "spirv-isel"

using namespace llvm;
namespace CL = OpenCL_std;
namespace GL = GLSL_std_450;

using ExtInstList = std::vector<std::pair<ExtInstSet, uint32_t>>;

namespace {

class SPIRVInstructionSelector : public InstructionSelector {
public:
  SPIRVInstructionSelector(const SPIRVTargetMachine &TM,
                           const SPIRVSubtarget &ST,
                           const SPIRVRegisterBankInfo &RBI,
                           SPIRVTypeRegistry &TR);

  // Common selection code. Instruction-specific selection occurs in spvSelect()
  bool select(MachineInstr &I) override;
  static const char *getName() { return DEBUG_TYPE; }

private:
  // tblgen-erated 'select' implementation, used as the initial selector for
  // the patterns that don't require complex C++.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;

  // All instruction-specific selection that didn't happen in "select()".
  // Is basically a large Switch/Case delegating to all other select method.
  bool spvSelect(Register resVReg, const SPIRVType *resType,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

  bool selectBinOp(Register resVReg, const SPIRVType *resType,
                   const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                   unsigned newOpcode) const;
  bool selectUnOp(Register resVReg, const SPIRVType *resType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                  unsigned newOpcode) const;

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

  bool selectOverflowOp(Register resVReg, const SPIRVType *resType,
                        const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                        unsigned newOpcode) const;

  bool selectCmp(Register resVReg, const SPIRVType *resType,
                 unsigned scalarTypeOpcode, unsigned comparisonOpcode,
                 const MachineInstr &I, MachineIRBuilder &MIRBuilder,
                 bool convertToUInt = false) const;

  bool selectICmp(Register resVReg, const SPIRVType *resType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;
  bool selectFCmp(Register resVReg, const SPIRVType *resType,
                  const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

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

  bool selectVectorExtract(Register resVReg, const SPIRVType *resType,
                           const MachineInstr &I,
                           MachineIRBuilder &MIRBuilder) const;
  bool selectVectorInsert(Register resVReg, const SPIRVType *resType,
                          const MachineInstr &I,
                          MachineIRBuilder &MIRBuilder) const;

  bool selectFrameIndex(Register resVReg, const SPIRVType *resType,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectBranch(const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;
  bool selectBranchCond(const MachineInstr &I,
                        MachineIRBuilder &MIRBuilder) const;

  bool selectSelect(Register resVReg, const SPIRVType *resType,
                    const MachineInstr &I, MachineIRBuilder &MIRBuilder) const;

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

  Register buildI32Constant(uint32_t val, MachineIRBuilder &MIRBuilder) const;

  Register buildZerosVal(const SPIRVType *resType,
                         MachineIRBuilder &MIRBuilder) const;
  Register buildOnesVal(bool allOnes, const SPIRVType *resType,
                        MachineIRBuilder &MIRBuilder) const;

  Register buildConvertPtrToUInt(Register targetReg,
                                 MachineIRBuilder &MIRBuilder) const;

  const SPIRVTargetMachine &TM;
  const SPIRVSubtarget &ST;
  const SPIRVInstrInfo &TII;
  const SPIRVRegisterInfo &TRI;
  const SPIRVRegisterBankInfo &RBI;
  SPIRVTypeRegistry &TR;
};

} // end anonymous namespace

SPIRVInstructionSelector::SPIRVInstructionSelector(
    const SPIRVTargetMachine &TM, const SPIRVSubtarget &ST,
    const SPIRVRegisterBankInfo &RBI, SPIRVTypeRegistry &TR)
    : InstructionSelector(), TM(TM), ST(ST), TII(*ST.getInstrInfo()),
      TRI(*ST.getRegisterInfo()), RBI(RBI), TR(TR) {}

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

  // If tablegen patterns get used later, call selectImpl to match them first
  // if (selectImpl(I, CoverageInfo))
  // return true;

  // If it's not a GMIR instruction, we've selected it already.
  if (!isPreISelGenericOpcode(Opcode)) {
    if (Opcode == SPIRV::ASSIGN_TYPE) { // These pseudos aren't needed any more
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

  const unsigned Opcode = I.getOpcode();
  switch (Opcode) {
  case TargetOpcode::G_FCONSTANT: {
    const ConstantFP *imm = I.getOperand(1).getFPImm();
    auto apint = imm->getValueAPF().bitcastToAPInt();
    return selectConst(resVReg, resType, apint, MIRBuilder);
  }
  case TargetOpcode::G_CONSTANT: {
    auto apint = I.getOperand(1).getCImm()->getValue();
    return selectConst(resVReg, resType, apint, MIRBuilder);
  }

  case TargetOpcode::G_IMPLICIT_DEF:
    return selectOpUndef(resVReg, resType, MIRBuilder);

  case TargetOpcode::G_INSERT_VECTOR_ELT:
    return selectVectorInsert(resVReg, resType, I, MIRBuilder);
  case TargetOpcode::G_EXTRACT_VECTOR_ELT:
    return selectVectorExtract(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_ICMP:
    return selectICmp(resVReg, resType, I, MIRBuilder);
  case TargetOpcode::G_FCMP:
    return selectFCmp(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_FRAME_INDEX:
    return selectFrameIndex(resVReg, resType, MIRBuilder);

  case TargetOpcode::G_BR:
    return selectBranch(I, MIRBuilder);
  case TargetOpcode::G_BRCOND:
    return selectBranchCond(I, MIRBuilder);

  case TargetOpcode::G_SELECT:
    return selectSelect(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_PHI:
    return selectPhi(resVReg, resType, I, MIRBuilder);

  case TargetOpcode::G_ADD:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpIAdd);
  case TargetOpcode::G_FADD:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpFAdd);

  case TargetOpcode::G_SUB:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpISub);
  case TargetOpcode::G_FSUB:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpFSub);

  case TargetOpcode::G_MUL:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpIMul);
  case TargetOpcode::G_FMUL:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpFMul);

  case TargetOpcode::G_UDIV:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpUDiv);
  case TargetOpcode::G_SDIV:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpSDiv);
  case TargetOpcode::G_FDIV:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpFDiv);

  case TargetOpcode::G_SREM:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpSRem);
  case TargetOpcode::G_FREM:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpFRem);
  case TargetOpcode::G_UREM:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpUMod);

  case TargetOpcode::G_FNEG:
    return selectUnOp(resVReg, resType, I, MIRBuilder, OpFNegate);

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

  case TargetOpcode::G_UADDO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpIAddCarry);
  case TargetOpcode::G_USUBO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpISubBorrow);
  case TargetOpcode::G_UMULO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpSMulExtended);
  case TargetOpcode::G_SMULO:
    return selectOverflowOp(resVReg, resType, I, MIRBuilder, OpUMulExtended);

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

  case TargetOpcode::G_AND: {
    bool isBool = TR.isScalarOrVectorOfType(resVReg, OpTypeBool);
    return selectBinOp(resVReg, resType, I, MIRBuilder,
                       isBool ? OpLogicalAnd : OpBitwiseAnd);
  }
  case TargetOpcode::G_OR: {
    bool isBool = TR.isScalarOrVectorOfType(resVReg, OpTypeBool);
    return selectBinOp(resVReg, resType, I, MIRBuilder,
                       isBool ? OpLogicalOr : OpBitwiseOr);
  }
  case TargetOpcode::G_XOR: {
    bool isBool = TR.isScalarOrVectorOfType(resVReg, OpTypeBool);
    return selectBinOp(resVReg, resType, I, MIRBuilder,
                       isBool ? OpLogicalNotEqual : OpBitwiseXor);
  }

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

  case TargetOpcode::G_SHL:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpShiftLeftLogical);
  case TargetOpcode::G_LSHR:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpShiftRightLogical);
  case TargetOpcode::G_ASHR:
    return selectBinOp(resVReg, resType, I, MIRBuilder, OpShiftRightArithmetic);

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
    if (ST.canUseExtInstSet(set)) {
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

static bool canUseFastMathFlags(unsigned opCode) {
  using namespace SPIRV;
  switch (opCode) {
  case OpFAdd:
  case OpFSub:
  case OpFMul:
  case OpFDiv:
  case OpFRem:
  case OpFMod:
    return true;
  default:
    return false;
  }
}

static bool canUseNSW(unsigned opCode) {
  using namespace SPIRV;
  switch (opCode) {
  case OpIAdd:
  case OpISub:
  case OpIMul:
  case OpShiftLeftLogical:
  case OpSNegate:
    return true;
  default:
    return false;
  }
}

static bool canUseNUW(unsigned opCode) {
  using namespace SPIRV;
  switch (opCode) {
  case OpIAdd:
  case OpISub:
  case OpIMul:
    return true;
  default:
    return false;
  }
}

static void decorate(Register target, Decoration::Decoration dec,
                     MachineIRBuilder &MIRBuilder) {
  MIRBuilder.buildInstr(SPIRV::OpDecorate).addUse(target).addImm(dec);
}
static void decorate(Register target, Decoration::Decoration dec, uint32_t imm,
                     MachineIRBuilder &MIRBuilder) {
  MIRBuilder.buildInstr(SPIRV::OpDecorate)
      .addUse(target)
      .addImm(dec)
      .addImm(imm);
}

static uint32_t getFastMathFlags(const MachineInstr &I) {
  uint32_t flags = FPFastMathMode::None;
  if (I.getFlag(MachineInstr::MIFlag::FmNoNans)) {
    flags |= FPFastMathMode::NotNaN;
  }
  if (I.getFlag(MachineInstr::MIFlag::FmNoInfs)) {
    flags |= FPFastMathMode::NotInf;
  }
  if (I.getFlag(MachineInstr::MIFlag::FmNsz)) {
    flags |= FPFastMathMode::NSZ;
  }
  if (I.getFlag(MachineInstr::MIFlag::FmArcp)) {
    flags |= FPFastMathMode::AllowRecip;
  }
  if (I.getFlag(MachineInstr::MIFlag::FmReassoc)) {
    flags |= FPFastMathMode::Fast;
  }
  return flags;
}

static void handleIntegerWrapFlags(const MachineInstr &I, Register target,
                                   unsigned newOpcode, const SPIRVSubtarget &ST,
                                   MachineIRBuilder &MIRBuilder) {
  if (I.getFlag(MachineInstr::MIFlag::NoSWrap)) {
    if (canUseNSW(newOpcode) &&
        canUseDecoration(Decoration::NoSignedWrap, ST)) {
      decorate(target, Decoration::NoSignedWrap, MIRBuilder);
    }
  }
  if (I.getFlag(MachineInstr::MIFlag::NoUWrap)) {
    if (canUseNUW(newOpcode) &&
        canUseDecoration(Decoration::NoUnsignedWrap, ST)) {
      decorate(target, Decoration::NoUnsignedWrap, MIRBuilder);
    }
  }
}

bool SPIRVInstructionSelector::selectBinOp(Register resVReg,
                                           const SPIRVType *resType,
                                           const MachineInstr &I,
                                           MachineIRBuilder &MIRBuilder,
                                           unsigned newOpcode) const {
  bool success = MIRBuilder.buildInstr(newOpcode)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(I.getOperand(1).getReg())
      .addUse(I.getOperand(2).getReg())
      .constrainAllUses(TII, TRI, RBI);

  if (!success)
    return false;

  if (canUseFastMathFlags(newOpcode)) {
    auto fmFlags = getFastMathFlags(I);
    if (fmFlags != FPFastMathMode::None) {
      decorate(resVReg, Decoration::FPFastMathMode, fmFlags, MIRBuilder);
    }
  }
  handleIntegerWrapFlags(I, resVReg, newOpcode, ST, MIRBuilder);
  return true;
}

bool SPIRVInstructionSelector::selectUnOp(Register resVReg,
                                          const SPIRVType *resType,
                                          const MachineInstr &I,
                                          MachineIRBuilder &MIRBuilder,
                                          unsigned newOpcode) const {
  handleIntegerWrapFlags(I, resVReg, newOpcode, ST, MIRBuilder);
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
  auto scSem = getMemSemanticsForStorageClass(TR.getPointerStorageClass(ptr));

  auto memSem = getMemSemantics(memOp->getOrdering());
  Register memSemReg = buildI32Constant(memSem | scSem, MIRBuilder);

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

  auto memSemEq = getMemSemantics(memOp->getOrdering()) | scSem;
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
    auto genericPtrTy = TR.getGenericPtrType(srcPtrTy, MIRBuilder);
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

Register SPIRVInstructionSelector::buildConvertPtrToUInt(
    Register targetReg, MachineIRBuilder &MIRBuilder) const {
  auto MRI = MIRBuilder.getMRI();
  SPIRVType *ptrUIntType = TR.getPtrUIntType(MIRBuilder);
  Register resVReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
  MIRBuilder.buildInstr(SPIRV::OpConvertPtrToU)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(ptrUIntType))
      .addUse(targetReg);
  return targetReg;
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

  if (cmp0Type->getOpcode() == OpTypePointer) {
    if (convertToUInt) {
      cmp0 = buildConvertPtrToUInt(cmp0, MIRBuilder);
      cmp1 = buildConvertPtrToUInt(cmp1, MIRBuilder);
    }
  } else if (!TR.isScalarOrVectorOfType(cmp0, scalarTyOpc) ||
             !TR.isScalarOrVectorOfType(cmp1, scalarTyOpc)) {
    llvm_unreachable("Incompatible type for comparison");
  }

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
    if (ST.canDirectlyComparePointers()) {
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

bool SPIRVInstructionSelector::selectConst(Register res, const SPIRVType *resTy,
                                           const APInt &imm,
                                           MachineIRBuilder &MIRBuilder) const {

  unsigned int OpCode = SPIRV::OpConstant;
  const auto bitwidth = imm.getBitWidth();

  if (bitwidth == 1) {
    OpCode = imm.isOneValue() ? SPIRV::OpConstantTrue : SPIRV::OpConstantFalse;
  }

  Register typeID = TR.getSPIRVTypeID(resTy);
  auto MIB = MIRBuilder.buildInstr(OpCode).addDef(res).addUse(typeID);

  switch (bitwidth) {
  case 1:
    break; // Already handled
  case 8:
  case 16:
  case 32:
    MIB.addImm(imm.getZExtValue());
    break;
  case 64: {
    uint64_t fullImm = imm.getZExtValue();
    uint32_t lowBits = fullImm & 0xffffffff;
    uint32_t highBits = (fullImm >> 32) & 0xffffffff;
    MIB.addImm(lowBits).addImm(highBits);
    break;
  }
  default:
    errs() << "Bitwidth = " << bitwidth;
    report_fatal_error("Unsupported constant bitwidth");
  }
  return constrainSelectedInstRegOperands(*MIB, TII, TRI, RBI);
}

Register
SPIRVInstructionSelector::buildI32Constant(uint32_t val,
                                           MachineIRBuilder &MIRBuilder) const {
  auto MRI = MIRBuilder.getMRI();
  auto spvI32Ty = TR.getOpTypeInt(32, MIRBuilder);
  Register newReg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  selectConst(newReg, spvI32Ty, APInt(32, val), MIRBuilder);
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
  auto MRI = MIRBuilder.getMRI();
  Register zeroReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
  MIRBuilder.buildInstr(SPIRV::OpConstantNull)
      .addDef(zeroReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .constrainAllUses(TII, TRI, RBI);
  return zeroReg;
}

Register
SPIRVInstructionSelector::buildOnesVal(bool allOnes, const SPIRVType *resType,
                                       MachineIRBuilder &MIRBuilder) const {
  auto MRI = MIRBuilder.getMRI();
  Register oneReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
  unsigned bitWidth = TR.getScalarOrVectorBitWidth(resType);
  auto one = allOnes ? APInt::getAllOnesValue(bitWidth)
                     : APInt::getOneBitSet(bitWidth, 0);
  selectConst(oneReg, resType, one, MIRBuilder);
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
    return MIRBuilder.buildInstr(OpSelect)
        .addDef(resVReg)
        .addUse(TR.getSPIRVTypeID(resType))
        .addUse(oneReg)
        .addUse(zeroReg)
        .addUse(I.getOperand(1).getReg())
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

bool SPIRVInstructionSelector::selectOpUndef(
    Register resVReg, const SPIRVType *resType,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpUndef)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectVectorExtract(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  Register base = I.getOperand(1).getReg();
  Register idx = I.getOperand(2).getReg();

  MachineInstr *idxInst = MIRBuilder.getMRI()->getVRegDef(idx);
  bool isConst = idxInst->getOpcode() == TargetOpcode::G_CONSTANT;

  auto Op = isConst ? SPIRV::OpCompositeExtract : SPIRV::OpVectorExtractDynamic;
  auto MIB = MIRBuilder.buildInstr(Op)
                 .addDef(resVReg)
                 .addUse(TR.getSPIRVTypeID(resType))
                 .addUse(base);
  if (isConst) {
    auto idxImm = idxInst->getOperand(1).getCImm()->getZExtValue();
    MIB.addImm(idxImm);
  } else {
    MIB.addUse(idx);
  }
  return MIB.constrainAllUses(TII, TRI, RBI);
}

bool SPIRVInstructionSelector::selectVectorInsert(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {

  Register base = I.getOperand(1).getReg();
  Register valToInsert = I.getOperand(2).getReg();
  Register idx = I.getOperand(3).getReg();

  MachineInstr *idxInst = MIRBuilder.getMRI()->getVRegDef(idx);
  bool isConst = idxInst->getOpcode() == TargetOpcode::G_CONSTANT;

  auto Op = isConst ? SPIRV::OpCompositeInsert : SPIRV::OpVectorInsertDynamic;
  auto resTyID = TR.getSPIRVTypeID(resType);
  auto MIB = MIRBuilder.buildInstr(Op).addDef(resVReg).addUse(resTyID);
  if (isConst) {
    auto idxImm = idxInst->getOperand(1).getCImm()->getZExtValue();
    MIB.addUse(valToInsert).addUse(base).addImm(idxImm);
  } else {
    MIB.addUse(base).addUse(valToInsert).addUse(idx);
  }
  return MIB.constrainAllUses(TII, TRI, RBI);
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

bool SPIRVInstructionSelector::selectSelect(
    Register resVReg, const SPIRVType *resType, const MachineInstr &I,
    MachineIRBuilder &MIRBuilder) const {
  return MIRBuilder.buildInstr(SPIRV::OpSelect)
      .addDef(resVReg)
      .addUse(TR.getSPIRVTypeID(resType))
      .addUse(I.getOperand(1).getReg())
      .addUse(I.getOperand(2).getReg())
      .addUse(I.getOperand(3).getReg())
      .constrainAllUses(TII, TRI, RBI);
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

namespace llvm {
InstructionSelector *
createSPIRVInstructionSelector(const SPIRVTargetMachine &TM,
                               const SPIRVSubtarget &Subtarget,
                               const SPIRVRegisterBankInfo &RBI) {
  return new SPIRVInstructionSelector(TM, Subtarget, RBI,
                                      *Subtarget.getSPIRVTypeRegistry());
}
} // namespace llvm
