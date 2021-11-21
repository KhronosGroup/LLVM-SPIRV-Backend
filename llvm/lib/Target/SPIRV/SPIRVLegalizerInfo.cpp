//===- SPIRVLegalizerInfo.cpp --- SPIR-V Legalization Rules ------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the targeting of the Machinelegalizer class for SPIR-V.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVLegalizerInfo.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTypeRegistry.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include <algorithm>
#include <cassert>

using namespace llvm;
using namespace LegalizeActions;
using namespace LegalityPredicates;

SPIRVLegalizerInfo::SPIRVLegalizerInfo(const SPIRVSubtarget &ST) {
  using namespace TargetOpcode;

  this->ST = &ST;
  TR = ST.getSPIRVTypeRegistry();

  const LLT s1 = LLT::scalar(1);
  const LLT s8 = LLT::scalar(8);
  const LLT s16 = LLT::scalar(16);
  const LLT s32 = LLT::scalar(32);
  const LLT s64 = LLT::scalar(64);

  const LLT v16s64 = LLT::fixed_vector(16, 64);
  const LLT v16s32 = LLT::fixed_vector(16, 32);
  const LLT v16s16 = LLT::fixed_vector(16, 16);
  const LLT v16s8 = LLT::fixed_vector(16, 8);
  const LLT v16s1 = LLT::fixed_vector(16, 1);

  const LLT v8s64 = LLT::fixed_vector(8, 64);
  const LLT v8s32 = LLT::fixed_vector(8, 32);
  const LLT v8s16 = LLT::fixed_vector(8, 16);
  const LLT v8s8 = LLT::fixed_vector(8, 8);
  const LLT v8s1 = LLT::fixed_vector(8, 1);

  const LLT v4s64 = LLT::fixed_vector(4, 64);
  const LLT v4s32 = LLT::fixed_vector(4, 32);
  const LLT v4s16 = LLT::fixed_vector(4, 16);
  const LLT v4s8 = LLT::fixed_vector(4, 8);
  const LLT v4s1 = LLT::fixed_vector(4, 1);

  const LLT v3s64 = LLT::fixed_vector(3, 64);
  const LLT v3s32 = LLT::fixed_vector(3, 32);
  const LLT v3s16 = LLT::fixed_vector(3, 16);
  const LLT v3s8 = LLT::fixed_vector(3, 8);
  const LLT v3s1 = LLT::fixed_vector(3, 1);

  const LLT v2s64 = LLT::fixed_vector(2, 64);
  const LLT v2s32 = LLT::fixed_vector(2, 32);
  const LLT v2s16 = LLT::fixed_vector(2, 16);
  const LLT v2s8 = LLT::fixed_vector(2, 8);
  const LLT v2s1 = LLT::fixed_vector(2, 1);

  const unsigned PSize = ST.getPointerSize();
  const LLT p0 = LLT::pointer(0, PSize); // Function
  const LLT p1 = LLT::pointer(1, PSize); // CrossWorkgroup
  const LLT p2 = LLT::pointer(2, PSize); // UniformConstant
  const LLT p3 = LLT::pointer(3, PSize); // Workgroup
  const LLT p4 = LLT::pointer(4, PSize); // Generic
  const LLT p5 = LLT::pointer(5, PSize); // Input

  const LLT PInt = LLT::scalar(PSize);
  // TODO: remove copy-pasting here
  // by using concatenation in some way
  auto allPtrsScalarsAndVectors = {
      p0,    p1,    p2,    p3,    p4,    p5,    s1,     s8,     s16,
      s32,   s64,   v2s1,  v2s8,  v2s16, v2s32, v2s64,  v3s1,   v3s8,
      v3s16, v3s32, v3s64, v4s1,  v4s8,  v4s16, v4s32,  v4s64,  v8s1,
      v8s8,  v8s16, v8s32, v8s64, v16s1, v16s8, v16s16, v16s32, v16s64};

  auto allScalarsAndVectors = {
      s1,   s8,   s16,   s32,   s64,   v2s1,  v2s8,  v2s16,  v2s32,  v2s64,
      v3s1, v3s8, v3s16, v3s32, v3s64, v4s1,  v4s8,  v4s16,  v4s32,  v4s64,
      v8s1, v8s8, v8s16, v8s32, v8s64, v16s1, v16s8, v16s16, v16s32, v16s64};

  auto allVectors = {v2s1,  v2s8,   v2s16,  v2s32, v2s64, v3s1,  v3s8,
                     v3s16, v3s32,  v3s64,  v4s1,  v4s8,  v4s16, v4s32,
                     v4s64, v8s1,   v8s8,   v8s16, v8s32, v8s64, v16s1,
                     v16s8, v16s16, v16s32, v16s64};

  auto allIntScalarsAndVectors = {s8,    s16,   s32,   s64,    v2s8,   v2s16,
                                  v2s32, v2s64, v3s8,  v3s16,  v3s32,  v3s64,
                                  v4s8,  v4s16, v4s32, v4s64,  v8s8,   v8s16,
                                  v8s32, v8s64, v16s8, v16s16, v16s32, v16s64};

  auto allBoolScalarsAndVectors = {s1, v2s1, v3s1, v4s1, v8s1, v16s1};

  auto allScalars = {s1, s8, s16, s32, s64};
  auto allIntScalars = {s8, s16, s32, s64};

  auto allFloatScalars = {s16, s32, s64};
  auto allFloatScalarsAndVectors = {
      s16,   s32,   s64,   v2s16, v2s32, v2s64, v3s16,  v3s32,  v3s64,
      v4s16, v4s32, v4s64, v8s16, v8s32, v8s64, v16s16, v16s32, v16s64};

  auto allFloatAndIntScalars = allIntScalars;

  auto allPtrs = {p0, p1, p2, p3, p4, p5};
  auto allWritablePtrs = {p0, p1, p3, p4};

  auto allValues = {p0,    p1,     p2,     p3,    p4,    p5,    s1,    s8,
                    s16,   s32,    s64,    v2s1,  v2s8,  v2s16, v2s32, v2s64,
                    v3s1,  v3s8,   v3s16,  v3s32, v3s64, v4s1,  v4s8,  v4s16,
                    v4s32, v4s64,  v8s1,   v8s8,  v8s16, v8s32, v8s64, v16s1,
                    v16s8, v16s16, v16s32, v16s64};

  for (auto Opc: getTypeFoldingSupportingOpcs())
    getActionDefinitionsBuilder(Opc).custom();

  getActionDefinitionsBuilder(G_GLOBAL_VALUE).alwaysLegal();

  // TODO: add proper rules for vectors legalization
  getActionDefinitionsBuilder(
      {G_BUILD_VECTOR, G_SHUFFLE_VECTOR})
      .alwaysLegal();

  getActionDefinitionsBuilder(G_ADDRSPACE_CAST)
      .legalForCartesianProduct(allPtrs, allPtrs);

  getActionDefinitionsBuilder({G_LOAD, G_STORE})
      .legalIf(typeInSet(1, allPtrs));

  // getActionDefinitionsBuilder(
  //     {G_ADD, G_SUB, G_MUL, G_SDIV, G_UDIV, G_SREM, G_UREM})
  //     .legalFor(allIntScalarsAndVectors);

  // getActionDefinitionsBuilder({G_SHL, G_LSHR, G_ASHR})
  //     .legalForCartesianProduct(allIntScalarsAndVectors,
  //                               allIntScalarsAndVectors);

  // getActionDefinitionsBuilder({G_AND, G_OR, G_XOR})
  //     .legalFor(allScalarsAndVectors);

  getActionDefinitionsBuilder(G_BITREVERSE)
      .legalFor(allFloatScalarsAndVectors);

  getActionDefinitionsBuilder(G_FMA)
      .legalFor(allFloatScalarsAndVectors);

  // getActionDefinitionsBuilder({G_FADD, G_FSUB, G_FMUL, G_FDIV, G_FREM})
  //     .customFor(allFloatScalarsAndVectors);

  getActionDefinitionsBuilder({G_FPTOSI, G_FPTOUI})
      .legalForCartesianProduct(allIntScalarsAndVectors,
                                allFloatScalarsAndVectors);

  getActionDefinitionsBuilder({G_SITOFP, G_UITOFP})
      .legalForCartesianProduct(allFloatScalarsAndVectors,
                                allIntScalarsAndVectors);

  getActionDefinitionsBuilder({G_SMIN, G_SMAX, G_UMIN, G_UMAX})
      .legalFor(allIntScalarsAndVectors);

  getActionDefinitionsBuilder(G_CTPOP).legalForCartesianProduct(
      allIntScalarsAndVectors, allIntScalarsAndVectors);

  getActionDefinitionsBuilder(G_PHI).alwaysLegal();

  getActionDefinitionsBuilder(G_BITCAST).legalIf(all(
      typeInSet(0, allPtrsScalarsAndVectors),
      typeInSet(1, allPtrsScalarsAndVectors),
      LegalityPredicate(([=](const LegalityQuery &Query) {
        return Query.Types[0].getSizeInBits() == Query.Types[1].getSizeInBits();
      }))));

  // getActionDefinitionsBuilder(G_INSERT_VECTOR_ELT)
  //     .legalIf(all(typeInSet(0, allVectors), typeInSet(1, allScalars),
  //                  typeInSet(2, allIntScalars),
  //                  LegalityPredicate([=](const LegalityQuery &Query) {
  //                    return Query.Types[0].getElementType() == Query.Types[1];
  //                  })));

  // getActionDefinitionsBuilder(G_EXTRACT_VECTOR_ELT)
  //     .legalIf(all(typeInSet(0, allScalars), typeInSet(1, allVectors),
  //                  typeInSet(2, allIntScalars),
  //                  LegalityPredicate(([=](const LegalityQuery &Query) {
  //                    return Query.Types[1].getElementType() == Query.Types[0];
  //                  }))));
  // getActionDefinitionsBuilder(G_BUILD_VECTOR)
  //     .legalForCartesianProduct(allVectors, {s16, s32, s64});

  getActionDefinitionsBuilder(G_IMPLICIT_DEF).alwaysLegal();

  getActionDefinitionsBuilder(G_INTTOPTR)
      .legalForCartesianProduct(allPtrs, allIntScalars);
  getActionDefinitionsBuilder(G_PTRTOINT)
      .legalForCartesianProduct(allIntScalars, allPtrs);
  getActionDefinitionsBuilder(G_PTR_ADD).legalForCartesianProduct(
      allPtrs, allIntScalars);

  // Constants
  // getActionDefinitionsBuilder(G_CONSTANT).legalFor(allScalars);
  // getActionDefinitionsBuilder(G_FCONSTANT).legalFor(allFloatScalars);

  getActionDefinitionsBuilder(G_ICMP)
      .customIf(all(typeInSet(0, allBoolScalarsAndVectors),
                    typeInSet(1, allPtrsScalarsAndVectors),
                    LegalityPredicate([&](const LegalityQuery &Query) {
                      LLT retTy = Query.Types[0];
                      LLT cmpTy = Query.Types[1];
                      if (retTy.isVector())
                        return cmpTy.isVector() &&
                               retTy.getNumElements() == cmpTy.getNumElements();
                      else
                        // ST.canDirectlyComparePointers() for ponter args is
                        // checked in legalizeCustom().
                        return cmpTy.isScalar() || cmpTy.isPointer();
                    })));

  getActionDefinitionsBuilder(G_FCMP).legalIf(
      all(typeInSet(0, allBoolScalarsAndVectors),
          typeInSet(1, allFloatScalarsAndVectors),
          LegalityPredicate([=](const LegalityQuery &Query) {
            LLT retTy = Query.Types[0];
            LLT cmpTy = Query.Types[1];
            if (retTy.isVector()) {
              return cmpTy.isVector() &&
                     retTy.getNumElements() == cmpTy.getNumElements();
            } else {
              return cmpTy.isScalar();
            }
          })));

  getActionDefinitionsBuilder({G_ATOMICRMW_OR, G_ATOMICRMW_ADD, G_ATOMICRMW_AND,
                               G_ATOMICRMW_MAX, G_ATOMICRMW_MIN,
                               G_ATOMICRMW_SUB, G_ATOMICRMW_XOR,
                               G_ATOMICRMW_UMAX, G_ATOMICRMW_UMIN})
      .legalForCartesianProduct(allIntScalars, allWritablePtrs);

  getActionDefinitionsBuilder(G_ATOMICRMW_XCHG)
      .legalForCartesianProduct(allFloatAndIntScalars, allWritablePtrs);

  // Struct return types become a single large scalar, so cannot easily legalize
  getActionDefinitionsBuilder({G_ATOMIC_CMPXCHG, G_ATOMIC_CMPXCHG_WITH_SUCCESS})
      .alwaysLegal();
  getActionDefinitionsBuilder({G_UADDO, G_USUBO, G_SMULO, G_UMULO})
      .alwaysLegal();

  // Extensions
  getActionDefinitionsBuilder({G_TRUNC, G_ZEXT, G_SEXT, G_ANYEXT})
      .legalForCartesianProduct(allScalarsAndVectors);

  // FP conversions
  getActionDefinitionsBuilder({G_FPTRUNC, G_FPEXT})
      .legalForCartesianProduct(allFloatScalarsAndVectors);

  // Select
  // getActionDefinitionsBuilder(G_SELECT).legalIf(
  //     typeInSet(1, allBoolScalarsAndVectors));

  // Pointer-handling
  getActionDefinitionsBuilder(G_FRAME_INDEX).legalFor({p0});

  // Control-flow
  getActionDefinitionsBuilder(G_BRCOND).legalFor({s1});


  getActionDefinitionsBuilder({G_FPOW, G_FEXP, G_FEXP2, G_FLOG, G_FLOG2, G_FABS,
                               G_FMINNUM, G_FMAXNUM, G_FCEIL, G_FCOS, G_FSIN,
                               G_FSQRT, G_FFLOOR, G_FRINT, G_FNEARBYINT,
                               G_INTRINSIC_ROUND, G_INTRINSIC_TRUNC})
      .legalFor(allFloatScalarsAndVectors);

  if (ST.canUseExtInstSet(ExtInstSet::OpenCL_std)) {
    getActionDefinitionsBuilder(G_FLOG10).legalFor(allFloatScalarsAndVectors);

    getActionDefinitionsBuilder(
        {G_CTTZ, G_CTTZ_ZERO_UNDEF, G_CTLZ, G_CTLZ_ZERO_UNDEF})
        .legalForCartesianProduct(allIntScalarsAndVectors,
                                  allIntScalarsAndVectors);

    // Struct return types become a single scalar, so cannot easily legalize
    getActionDefinitionsBuilder({G_SMULH, G_UMULH}).alwaysLegal();
  }

  getLegacyLegalizerInfo().computeTables();
  verify(*ST.getInstrInfo());
}

static std::pair<Register, unsigned>
createNewIdReg(Register ValReg, unsigned Opcode, MachineRegisterInfo &MRI,
               const SPIRVTypeRegistry &TR) {
  auto NewT = LLT::scalar(32);
  auto SpvType = TR.getSPIRVTypeForVReg(ValReg);
  assert(SpvType && "VReg is expected to have SPIRV type");
  bool isFloat = SpvType->getOpcode() == SPIRV::OpTypeFloat;
  bool isVectorFloat =
      SpvType->getOpcode() == SPIRV::OpTypeVector &&
      TR.getSPIRVTypeForVReg(SpvType->getOperand(1).getReg())->getOpcode() ==
          SPIRV::OpTypeFloat;
  isFloat |= isVectorFloat;
  auto GetIdOp = isFloat ? SPIRV::GET_fID : SPIRV::GET_ID;
  auto DstClass = isFloat ? &SPIRV::fIDRegClass : &SPIRV::IDRegClass;
  if (MRI.getType(ValReg).isPointer()) {
    NewT = LLT::pointer(0, 32);
    GetIdOp = SPIRV::GET_pID;
    DstClass = &SPIRV::pIDRegClass;
  } else if (MRI.getType(ValReg).isVector()) {
    NewT = LLT::fixed_vector(2, NewT);
    GetIdOp = isFloat ? SPIRV::GET_vfID : SPIRV::GET_vID;
    DstClass = isFloat ? &SPIRV::vfIDRegClass : &SPIRV::vIDRegClass;
  }
  auto IdReg = MRI.createGenericVirtualRegister(NewT);
  MRI.setRegClass(IdReg, DstClass);
  return {IdReg, GetIdOp};
}

bool SPIRVLegalizerInfo::legalizeCustom(LegalizerHelper &Helper,
                                        MachineInstr &MI) const {
  auto Opc = MI.getOpcode();
  if (!isTypeFoldingSupported(Opc)) {
    assert(Opc == TargetOpcode::G_ICMP);
    auto &MRI = MI.getMF()->getRegInfo();
    // Add missed SPIRV type to the VReg
    // TODO: move SPIRV type detection to one place 
    auto resVReg = MI.getOperand(0).getReg();
    auto resType = TR->getSPIRVTypeForVReg(resVReg);
    if (!resType) {
      LLT Ty = MRI.getType(resVReg);
      LLT BaseTy = Ty.isVector() ? Ty.getElementType() : Ty;
      Type* LLVMTy = IntegerType::get(MI.getMF()->getFunction().getContext(), BaseTy.getSizeInBits());
      if (Ty.isVector())
        LLVMTy = FixedVectorType::get(LLVMTy, Ty.getNumElements());
      auto *spirvType = TR->getOrCreateSPIRVType(LLVMTy, Helper.MIRBuilder);
      TR->assignSPIRVTypeToVReg(spirvType, resVReg, Helper.MIRBuilder);
    }
    auto &Op0 = MI.getOperand(2);
    auto &Op1 = MI.getOperand(3);
    if (!ST->canDirectlyComparePointers() &&
        MRI.getType(Op0.getReg()).isPointer() &&
        MRI.getType(Op1.getReg()).isPointer()) {
      auto ConvT = LLT::scalar(ST->getPointerSize());
      auto ConvReg0 = MRI.createGenericVirtualRegister(ConvT);
      auto ConvReg1 = MRI.createGenericVirtualRegister(ConvT);
      auto *SpirvType = TR->getOrCreateSPIRVType(
              IntegerType::get(MI.getMF()->getFunction().getContext(),
                               ST->getPointerSize()),
          Helper.MIRBuilder);
      TR->assignSPIRVTypeToVReg(SpirvType, ConvReg0, Helper.MIRBuilder);
      TR->assignSPIRVTypeToVReg(SpirvType, ConvReg1, Helper.MIRBuilder);
      Helper.MIRBuilder.buildInstr(TargetOpcode::G_PTRTOINT)
          .addDef(ConvReg0)
          .addUse(Op0.getReg());
      Helper.MIRBuilder.buildInstr(TargetOpcode::G_PTRTOINT)
          .addDef(ConvReg1)
          .addUse(Op1.getReg());
      Op0.setReg(ConvReg0);
      Op1.setReg(ConvReg1);
    }
    return true;
  }
  auto &MRI = MI.getMF()->getRegInfo();
  assert(MI.getNumDefs() > 0 && MRI.hasOneUse(MI.getOperand(0).getReg()));
  MachineInstr &AssignTypeInst =
      *(MRI.use_instr_begin(MI.getOperand(0).getReg()));
  auto NewReg = createNewIdReg(MI.getOperand(0).getReg(), Opc, MRI, *TR).first;
  AssignTypeInst.getOperand(1).setReg(NewReg);
  // MRI.setRegClass(AssignTypeInst.getOperand(0).getReg(),
  // MRI.getRegClass(NewReg));
  MI.getOperand(0).setReg(NewReg);
  for (auto &Op : MI.operands()) {
    if (!Op.isReg() || Op.isDef())
      continue;
    // if (Ids.count(&Op) > 0) {
    //   Op.setReg(Ids.at(&Op));
    //   // NewI.addUse(Ids.at(&Op));
    // } else {
    auto IdOpInfo = createNewIdReg(Op.getReg(), Opc, MRI, *TR);
    Helper.MIRBuilder.buildInstr(IdOpInfo.second)
        .addDef(IdOpInfo.first)
        .addUse(Op.getReg());
    // Ids[&Op] = IdOpInfo.first;
    Op.setReg(IdOpInfo.first);
    // }
  }
  return true;
}
