//===-- SPIRVGlobalRegistry.cpp - SPIR-V Global Registry --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the SPIRVGlobalRegistry class,
// which is used to maintain rich type information required for SPIR-V even
// after lowering from LLVM IR to GMIR. It can convert an llvm::Type into
// an OpTypeXXX instruction, and map it to a virtual register. Also it builds
// and supports consistency of constants and global variables.
//
//===----------------------------------------------------------------------===//

#include "SPIRVGlobalRegistry.h"
#include "SPIRV.h"
#include "SPIRVOpenCLBIFs.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVUtils.h"

using namespace llvm;
SPIRVGlobalRegistry::SPIRVGlobalRegistry(unsigned PointerSize)
    : PointerSize(PointerSize) {}

SPIRVType *SPIRVGlobalRegistry::assignIntTypeToVReg(unsigned BitWidth,
                                                    Register VReg,
                                                    MachineInstr &I,
                                                    const SPIRVInstrInfo &TII) {
  SPIRVType *SpirvType = getOrCreateSPIRVIntegerType(BitWidth, I, TII);
  assignSPIRVTypeToVReg(SpirvType, VReg, *CurMF);
  return SpirvType;
}

SPIRVType *SPIRVGlobalRegistry::assignVectTypeToVReg(
    SPIRVType *BaseType, unsigned NumElements, Register VReg, MachineInstr &I,
    const SPIRVInstrInfo &TII) {
  SPIRVType *SpirvType =
      getOrCreateSPIRVVectorType(BaseType, NumElements, I, TII);
  assignSPIRVTypeToVReg(SpirvType, VReg, *CurMF);
  return SpirvType;
}

SPIRVType *SPIRVGlobalRegistry::assignTypeToVReg(const Type *Type,
                                                 Register VReg,
                                                 MachineIRBuilder &MIRBuilder,
                                                 AQ::AccessQualifier AccessQual,
                                                 bool EmitIR) {
  SPIRVType *SpirvType =
      getOrCreateSPIRVType(Type, MIRBuilder, AccessQual, EmitIR);
  assignSPIRVTypeToVReg(SpirvType, VReg, MIRBuilder.getMF());
  return SpirvType;
}

void SPIRVGlobalRegistry::assignSPIRVTypeToVReg(SPIRVType *SpirvType,
                                                Register VReg,
                                                MachineFunction &MF) {
  VRegToTypeMap[&MF][VReg] = SpirvType;
}

static Register createTypeVReg(MachineIRBuilder &MIRBuilder) {
  auto &MRI = MIRBuilder.getMF().getRegInfo();
  auto Res = MRI.createGenericVirtualRegister(LLT::scalar(32));
  MRI.setRegClass(Res, &SPIRV::TYPERegClass);
  return Res;
}

static Register createTypeVReg(MachineRegisterInfo &MRI) {
  auto Res = MRI.createGenericVirtualRegister(LLT::scalar(32));
  MRI.setRegClass(Res, &SPIRV::TYPERegClass);
  return Res;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeBool(MachineIRBuilder &MIRBuilder) {
  return MIRBuilder.buildInstr(SPIRV::OpTypeBool)
      .addDef(createTypeVReg(MIRBuilder));
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeInt(uint32_t Width,
                                             MachineIRBuilder &MIRBuilder,
                                             bool IsSigned) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeInt)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(Width)
                 .addImm(IsSigned ? 1 : 0);
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeFloat(uint32_t Width,
                                               MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFloat)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(Width);
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeVoid(MachineIRBuilder &MIRBuilder) {
  return MIRBuilder.buildInstr(SPIRV::OpTypeVoid)
      .addDef(createTypeVReg(MIRBuilder));
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeVector(uint32_t NumElems,
                                                SPIRVType *ElemType,
                                                MachineIRBuilder &MIRBuilder) {
  auto EleOpc = ElemType->getOpcode();
  assert((EleOpc == SPIRV::OpTypeInt || EleOpc == SPIRV::OpTypeFloat ||
          EleOpc == SPIRV::OpTypeBool) &&
         "Invalid vector element type");

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeVector)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(ElemType))
                 .addImm(NumElems);
  return MIB;
}

std::tuple<Register, ConstantInt *, bool>
SPIRVGlobalRegistry::getOrCreateConstIntReg(uint64_t Val, SPIRVType *SpvType,
                                            MachineIRBuilder *MIRBuilder,
                                            MachineInstr *I,
                                            const SPIRVInstrInfo *TII) {
  const IntegerType *LLVMIntTy;
  if (SpvType)
    LLVMIntTy = cast<IntegerType>(getTypeForSPIRVType(SpvType));
  else
    LLVMIntTy = IntegerType::getInt32Ty(CurMF->getFunction().getContext());
  Register Res;
  bool NewInstr = false;
  // Find a constant in DT or build a new one.
  ConstantInt *CI = ConstantInt::get(const_cast<IntegerType *>(LLVMIntTy), Val);
  if (DT.find(CI, CurMF, Res) == false) {
    unsigned BitWidth = SpvType ? getScalarOrVectorBitWidth(SpvType) : 32;
    LLT LLTy = LLT::scalar(32);
    Res = CurMF->getRegInfo().createGenericVirtualRegister(LLTy);
    if (MIRBuilder)
      assignTypeToVReg(LLVMIntTy, Res, *MIRBuilder);
    else
      assignIntTypeToVReg(BitWidth, Res, *I, *TII);
    DT.add(CI, CurMF, Res);
    NewInstr = true;
  }
  return std::make_tuple(Res, CI, NewInstr);
}

Register SPIRVGlobalRegistry::getOrCreateConstInt(uint64_t Val, MachineInstr &I,
                                                  SPIRVType *SpvType,
                                                  const SPIRVInstrInfo &TII) {
  assert(SpvType);
  ConstantInt *CI;
  Register Res;
  bool New;
  std::tie(Res, CI, New) =
      getOrCreateConstIntReg(Val, SpvType, nullptr, &I, &TII);
  // If we have found Res register which is defined by the passed G_CONSTANT
  // machine instruction, a new constant instruction should be created.
  if (!New && (!I.getOperand(0).isReg() || Res != I.getOperand(0).getReg()))
    return Res;
  MachineInstrBuilder MIB;
  MachineBasicBlock &BB = *I.getParent();
  if (Val) {
    MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpConstantI))
              .addDef(Res)
              .addUse(getSPIRVTypeID(SpvType));
    addNumImm(APInt(getScalarOrVectorBitWidth(SpvType), Val), MIB);
  } else {
    MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpConstantNull))
              .addDef(Res)
              .addUse(getSPIRVTypeID(SpvType));
  }
  const auto &ST = CurMF->getSubtarget();
  constrainSelectedInstRegOperands(*MIB, *ST.getInstrInfo(),
                                   *ST.getRegisterInfo(), *ST.getRegBankInfo());
  return Res;
}

Register SPIRVGlobalRegistry::buildConstantInt(uint64_t Val,
                                               MachineIRBuilder &MIRBuilder,
                                               SPIRVType *SpvType,
                                               bool EmitIR) {
  auto &MF = MIRBuilder.getMF();
  Register Res;
  const IntegerType *LLVMIntTy;
  if (SpvType)
    LLVMIntTy = cast<IntegerType>(getTypeForSPIRVType(SpvType));
  else
    LLVMIntTy = IntegerType::getInt32Ty(MF.getFunction().getContext());
  // Find a constant in DT or build a new one.
  const auto ConstInt =
      ConstantInt::get(const_cast<IntegerType *>(LLVMIntTy), Val);
  if (DT.find(ConstInt, &MIRBuilder.getMF(), Res) == false) {
    unsigned BitWidth = SpvType ? getScalarOrVectorBitWidth(SpvType) : 32;
    LLT LLTy = LLT::scalar(EmitIR ? BitWidth : 32);
    Res = MF.getRegInfo().createGenericVirtualRegister(LLTy);
    assignTypeToVReg(LLVMIntTy, Res, MIRBuilder, AQ::ReadWrite, EmitIR);
    DT.add(ConstInt, &MIRBuilder.getMF(), Res);
    if (EmitIR) {
      MIRBuilder.buildConstant(Res, *ConstInt);
    } else {
      MachineInstrBuilder MIB;
      if (Val) {
        assert(SpvType);
        MIB = MIRBuilder.buildInstr(SPIRV::OpConstantI)
                  .addDef(Res)
                  .addUse(getSPIRVTypeID(SpvType));
        addNumImm(APInt(BitWidth, Val), MIB);
      } else {
        assert(SpvType);
        MIB = MIRBuilder.buildInstr(SPIRV::OpConstantNull)
                  .addDef(Res)
                  .addUse(getSPIRVTypeID(SpvType));
      }
      const auto &Subtarget = CurMF->getSubtarget();
      constrainSelectedInstRegOperands(*MIB, *Subtarget.getInstrInfo(),
                                       *Subtarget.getRegisterInfo(),
                                       *Subtarget.getRegBankInfo());
    }
  }
  return Res;
}

Register SPIRVGlobalRegistry::buildConstantFP(APFloat Val,
                                              MachineIRBuilder &MIRBuilder,
                                              SPIRVType *SpvType) {
  auto &MF = MIRBuilder.getMF();
  Register Res;
  const Type *LLVMFPTy;
  if (SpvType) {
    LLVMFPTy = getTypeForSPIRVType(SpvType);
    assert(LLVMFPTy->isFloatingPointTy());
  } else {
    LLVMFPTy = IntegerType::getFloatTy(MF.getFunction().getContext());
  }
  // Find a constant in DT or build a new one.
  const auto ConstFP = ConstantFP::get(LLVMFPTy->getContext(), Val);
  if (DT.find(ConstFP, &MIRBuilder.getMF(), Res) == false) {
    unsigned BitWidth = SpvType ? getScalarOrVectorBitWidth(SpvType) : 32;
    Res = MF.getRegInfo().createGenericVirtualRegister(LLT::scalar(BitWidth));
    assignTypeToVReg(LLVMFPTy, Res, MIRBuilder);
    DT.add(ConstFP, &MIRBuilder.getMF(), Res);
    MIRBuilder.buildFConstant(Res, *ConstFP);
  }
  return Res;
}

Register
SPIRVGlobalRegistry::getOrCreateConsIntVector(uint64_t Val, MachineInstr &I,
                                              SPIRVType *SpvType,
                                              const SPIRVInstrInfo &TII) {
  const Type *LLVMTy = getTypeForSPIRVType(SpvType);
  assert(LLVMTy->isVectorTy());
  const FixedVectorType *LLVMVecTy = cast<FixedVectorType>(LLVMTy);
  Type *LLVMBaseTy = LLVMVecTy->getElementType();
  // Find a constant vector in DT or build a new one.
  const auto ConstInt = ConstantInt::get(LLVMBaseTy, Val);
  auto ConstVec =
      ConstantVector::getSplat(LLVMVecTy->getElementCount(), ConstInt);
  Register Res;
  if (DT.find(ConstVec, CurMF, Res) == false) {
    unsigned BitWidth = getScalarOrVectorBitWidth(SpvType);
    SPIRVType *SpvBaseType = getOrCreateSPIRVIntegerType(BitWidth, I, TII);
    // SpvScalConst should be created before SpvVecConst to avoid undefined ID
    // error on validation.
    // TODO: can moved below once sorting of types/consts/defs is implemented.
    Register SpvScalConst;
    if (Val)
      SpvScalConst = getOrCreateConstInt(Val, I, SpvBaseType, TII);
    // TODO: maybe use bitwidth of base type.
    LLT LLTy = LLT::scalar(32);
    Register SpvVecConst =
        CurMF->getRegInfo().createGenericVirtualRegister(LLTy);
    const unsigned ElemCnt = SpvType->getOperand(2).getImm();
    assignVectTypeToVReg(SpvBaseType, ElemCnt, SpvVecConst, I, TII);
    DT.add(ConstVec, CurMF, SpvVecConst);
    MachineInstrBuilder MIB;
    MachineBasicBlock &BB = *I.getParent();
    if (Val) {
      MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpConstantComposite))
                .addDef(SpvVecConst)
                .addUse(getSPIRVTypeID(SpvType));
      for (unsigned i = 0; i < ElemCnt; ++i)
        MIB.addUse(SpvScalConst);
    } else {
      MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpConstantNull))
                .addDef(SpvVecConst)
                .addUse(getSPIRVTypeID(SpvType));
    }
    const auto &Subtarget = CurMF->getSubtarget();
    constrainSelectedInstRegOperands(*MIB, *Subtarget.getInstrInfo(),
                                     *Subtarget.getRegisterInfo(),
                                     *Subtarget.getRegBankInfo());
    return SpvVecConst;
  }
  return Res;
}

Register
SPIRVGlobalRegistry::buildConstantIntVector(uint64_t Val,
                                            MachineIRBuilder &MIRBuilder,
                                            SPIRVType *SpvType, bool EmitIR) {
  auto &MF = MIRBuilder.getMF();
  const Type *LLVMTy = getTypeForSPIRVType(SpvType);
  assert(LLVMTy->isVectorTy());
  const FixedVectorType *LLVMVecTy = cast<FixedVectorType>(LLVMTy);
  Type *LLVMBaseTy = LLVMVecTy->getElementType();
  // Find a constant vector in DT or build a new one.
  const auto ConstInt = ConstantInt::get(LLVMBaseTy, Val);
  auto ConstVec =
      ConstantVector::getSplat(LLVMVecTy->getElementCount(), ConstInt);
  Register Res;
  if (DT.find(ConstVec, &MIRBuilder.getMF(), Res) == false) {
    Register SpvScalConst;
    if (Val || EmitIR) {
      SPIRVType *SpvBaseType =
          getOrCreateSPIRVType(LLVMBaseTy, MIRBuilder, AQ::ReadWrite, EmitIR);
      SpvScalConst = buildConstantInt(Val, MIRBuilder, SpvBaseType, EmitIR);
    }
    unsigned BitWidth = getScalarOrVectorBitWidth(SpvType);
    LLT LLTy = EmitIR ? LLT::vector(LLVMVecTy->getElementCount(), BitWidth)
                      : LLT::scalar(32);
    Register SpvVecConst = MF.getRegInfo().createGenericVirtualRegister(LLTy);
    assignTypeToVReg(LLVMVecTy, SpvVecConst, MIRBuilder, AQ::ReadWrite, EmitIR);
    DT.add(ConstVec, &MIRBuilder.getMF(), SpvVecConst);
    if (EmitIR) {
      MIRBuilder.buildSplatVector(SpvVecConst, SpvScalConst);
    } else {
      MachineInstrBuilder MIB;
      if (Val) {
        MIB = MIRBuilder.buildInstr(SPIRV::OpConstantComposite)
                  .addDef(SpvVecConst)
                  .addUse(getSPIRVTypeID(SpvType));
        const unsigned ElemCnt = SpvType->getOperand(2).getImm();
        for (unsigned i = 0; i < ElemCnt; ++i)
          MIB.addUse(SpvScalConst);
      } else {
        MIB = MIRBuilder.buildInstr(SPIRV::OpConstantNull)
                  .addDef(SpvVecConst)
                  .addUse(getSPIRVTypeID(SpvType));
      }
      const auto &Subtarget = CurMF->getSubtarget();
      constrainSelectedInstRegOperands(*MIB, *Subtarget.getInstrInfo(),
                                       *Subtarget.getRegisterInfo(),
                                       *Subtarget.getRegBankInfo());
    }
    return SpvVecConst;
  }
  return Res;
}

Register SPIRVGlobalRegistry::buildConstantSampler(
    Register ResReg, unsigned AddrMode, unsigned Param, unsigned FilerMode,
    MachineIRBuilder &MIRBuilder, SPIRVType *SpvType) {
  SPIRVType *SampTy;
  if (SpvType)
    SampTy = getOrCreateSPIRVType(getTypeForSPIRVType(SpvType), MIRBuilder);
  else
    SampTy = getOrCreateSPIRVTypeByName("opencl.sampler_t", MIRBuilder);

  auto Sampler =
      ResReg.isValid()
          ? ResReg
          : MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  auto Res = MIRBuilder.buildInstr(SPIRV::OpConstantSampler)
                 .addDef(Sampler)
                 .addUse(getSPIRVTypeID(SampTy))
                 .addImm(AddrMode)
                 .addImm(Param)
                 .addImm(FilerMode);
  constrainRegOperands(Res);
  assert(Res->getOperand(0).isReg());
  return Res->getOperand(0).getReg();
}

Register SPIRVGlobalRegistry::buildGlobalVariable(
    Register ResVReg, SPIRVType *BaseType, StringRef Name,
    const GlobalValue *GV, StorageClass::StorageClass Storage,
    const MachineInstr *Init, bool IsConst, bool HasLinkageTy,
    LinkageType::LinkageType LinkageType, MachineIRBuilder &MIRBuilder,
    bool IsInstSelector) {
  const GlobalVariable *GVar = nullptr;
  if (GV)
    GVar = cast<const GlobalVariable>(GV);
  else {
    // If GV is not passed explicitly, use the name to find or construct
    // the global variable.
    Module *M = MIRBuilder.getMF().getFunction().getParent();
    GVar = M->getGlobalVariable(Name);
    if (GVar == nullptr) {
      const Type *Ty = getTypeForSPIRVType(BaseType); // TODO: check type.
      GVar = new GlobalVariable(*M, const_cast<Type *>(Ty), false,
                                GlobalValue::ExternalLinkage, nullptr,
                                Twine(Name));
    }
    GV = GVar;
  }
  Register Reg;
  if (DT.find(GVar, &MIRBuilder.getMF(), Reg)) {
    if (Reg != ResVReg)
      MIRBuilder.buildCopy(ResVReg, Reg);
    return ResVReg;
  }

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpVariable)
                 .addDef(ResVReg)
                 .addUse(getSPIRVTypeID(BaseType))
                 .addImm(Storage);

  if (Init != 0) {
    MIB.addUse(Init->getOperand(0).getReg());
  }

  // ISel may introduce a new register on this step, so we need to add it to
  // DT and correct its type avoiding fails on the next stage.
  if (IsInstSelector) {
    const auto &Subtarget = CurMF->getSubtarget();
    constrainSelectedInstRegOperands(*MIB, *Subtarget.getInstrInfo(),
                                     *Subtarget.getRegisterInfo(),
                                     *Subtarget.getRegBankInfo());
  }
  Reg = MIB->getOperand(0).getReg();
  DT.add(GVar, &MIRBuilder.getMF(), Reg);

  // Set to Reg the same type as ResVReg has.
  auto MRI = MIRBuilder.getMRI();
  assert(MRI->getType(ResVReg).isPointer() && "Pointer type is expected");
  if (Reg != ResVReg) {
    LLT RegLLTy = LLT::pointer(MRI->getType(ResVReg).getAddressSpace(), 32);
    MRI->setType(Reg, RegLLTy);
    assignSPIRVTypeToVReg(BaseType, Reg, MIRBuilder.getMF());
  }

  // If it's a global variable with name, output OpName for it.
  if (GVar && GVar->hasName())
    buildOpName(Reg, GVar->getName(), MIRBuilder);

  // Output decorations for the GV.
  // TODO: maybe move to GenerateDecorations pass.
  if (IsConst)
    buildOpDecorate(Reg, MIRBuilder, Decoration::Constant, {});

  if (GVar && GVar->getAlign().valueOrOne().value() != 1) {
    unsigned Alignment = (unsigned)GVar->getAlign().valueOrOne().value();
    buildOpDecorate(Reg, MIRBuilder, Decoration::Alignment, {Alignment});
  }

  if (HasLinkageTy)
    buildOpDecorate(Reg, MIRBuilder, Decoration::LinkageAttributes,
                    {static_cast<uint32_t>(LinkageType)}, Name);

  BuiltIn::BuiltIn BuiltInId;
  if (getSpirvBuiltInIdByName(Name, BuiltInId))
    buildOpDecorate(Reg, MIRBuilder, Decoration::BuiltIn,
                    {static_cast<uint32_t>(BuiltInId)});

  return Reg;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeArray(uint32_t NumElems,
                                               SPIRVType *ElemType,
                                               MachineIRBuilder &MIRBuilder,
                                               bool EmitIR) {
  assert((ElemType->getOpcode() != SPIRV::OpTypeVoid) &&
         "Invalid array element type");
  Register NumElementsVReg =
      buildConstantInt(NumElems, MIRBuilder, nullptr, EmitIR);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeArray)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(ElemType))
                 .addUse(NumElementsVReg);
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeOpaque(const StructType *Ty,
                                                MachineIRBuilder &MIRBuilder) {
  assert(Ty->hasName());
  const StringRef Name = Ty->hasName() ? Ty->getName() : "";
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeOpaque).addDef(ResVReg);
  addStringImm(Name, MIB);
  buildOpName(ResVReg, Name, MIRBuilder);
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeStruct(const StructType *Ty,
                                                MachineIRBuilder &MIRBuilder,
                                                bool EmitIR) {
  SmallVector<Register, 4> FieldTypes;
  for (const auto &Elem : Ty->elements()) {
    auto ElemTy = findSPIRVType(Elem, MIRBuilder);
    assert(ElemTy && ElemTy->getOpcode() != SPIRV::OpTypeVoid &&
           "Invalid struct element type");
    FieldTypes.push_back(getSPIRVTypeID(ElemTy));
  }
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeStruct).addDef(ResVReg);
  for (const auto &Ty : FieldTypes)
    MIB.addUse(Ty);
  if (Ty->hasName())
    buildOpName(ResVReg, Ty->getName(), MIRBuilder);
  if (Ty->isPacked())
    buildOpDecorate(ResVReg, MIRBuilder, Decoration::CPacked, {});
  return MIB;
}

static bool isOpenCLBuiltinType(const StructType *SType) {
  return SType->isOpaque() && SType->hasName() &&
         SType->getName().startswith("opencl.");
}

static bool isSPIRVBuiltinType(const StructType *SType) {
  return SType->isOpaque() && SType->hasName() &&
         SType->getName().startswith("spirv.");
}

SPIRVType *
SPIRVGlobalRegistry::handleOpenCLBuiltin(const StructType *Ty,
                                         MachineIRBuilder &MIRBuilder,
                                         AQ::AccessQualifier AccQual) {
  assert(Ty->hasName());
  unsigned NumStartingVRegs = MIRBuilder.getMRI()->getNumVirtRegs();
  auto NewTy = getOrCreateOpenCLOpaqueType(Ty, MIRBuilder, AccQual);
  if (NumStartingVRegs < MIRBuilder.getMRI()->getNumVirtRegs())
    buildOpName(getSPIRVTypeID(NewTy), Ty->getName(), MIRBuilder);
  return NewTy;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypePointer(StorageClass::StorageClass SC,
                                                 SPIRVType *ElemType,
                                                 MachineIRBuilder &MIRBuilder,
                                                 Register Reg) {
  if (!Reg.isValid())
    Reg = createTypeVReg(MIRBuilder);
  return MIRBuilder.buildInstr(SPIRV::OpTypePointer)
      .addDef(Reg)
      .addImm(SC)
      .addUse(getSPIRVTypeID(ElemType));
}

SPIRVType *
SPIRVGlobalRegistry::getOpTypeForwardPointer(StorageClass::StorageClass SC,
                                             MachineIRBuilder &MIRBuilder) {
  return MIRBuilder.buildInstr(SPIRV::OpTypeForwardPointer)
      .addUse(createTypeVReg(MIRBuilder))
      .addImm(SC);
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeFunction(
    SPIRVType *RetType, const SmallVectorImpl<SPIRVType *> &ArgTypes,
    MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFunction)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(RetType));
  for (const SPIRVType *ArgType : ArgTypes)
    MIB.addUse(getSPIRVTypeID(ArgType));
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::findSPIRVType(const Type *Ty,
                                              MachineIRBuilder &MIRBuilder,
                                              AQ::AccessQualifier AccQual,
                                              bool EmitIR) {
  Register Reg;
  if (DT.find(Ty, &MIRBuilder.getMF(), Reg))
    return getSPIRVTypeForVReg(Reg);
  if (ForwardPointerTypes.find(Ty) != ForwardPointerTypes.end())
    return ForwardPointerTypes[Ty];
  return restOfCreateSPIRVType(Ty, MIRBuilder, AccQual, EmitIR);
}

Register SPIRVGlobalRegistry::getSPIRVTypeID(const SPIRVType *SpirvType) const {
  assert(SpirvType && "Attempting to get type id for nullptr type.");
  if (SpirvType->getOpcode() == SPIRV::OpTypeForwardPointer)
    return SpirvType->uses().begin()->getReg();
  return SpirvType->defs().begin()->getReg();
}

SPIRVType *SPIRVGlobalRegistry::createSPIRVType(const Type *Ty,
                                                MachineIRBuilder &MIRBuilder,
                                                AQ::AccessQualifier AccQual,
                                                bool EmitIR) {
  auto &TypeToSPIRVTypeMap = DT.getTypes()->getAllUses();
  auto t = TypeToSPIRVTypeMap.find(Ty);
  if (t != TypeToSPIRVTypeMap.end()) {
    auto tt = t->second.find(&MIRBuilder.getMF());
    if (tt != t->second.end())
      return getSPIRVTypeForVReg(tt->second);
  }

  if (auto IType = dyn_cast<IntegerType>(Ty)) {
    const unsigned Width = IType->getBitWidth();
    return Width == 1 ? getOpTypeBool(MIRBuilder)
                      : getOpTypeInt(Width, MIRBuilder, false);
  }
  if (Ty->isFloatingPointTy())
    return getOpTypeFloat(Ty->getPrimitiveSizeInBits(), MIRBuilder);
  if (Ty->isVoidTy())
    return getOpTypeVoid(MIRBuilder);
  if (Ty->isVectorTy()) {
    auto El =
        findSPIRVType(cast<FixedVectorType>(Ty)->getElementType(), MIRBuilder);
    return getOpTypeVector(cast<FixedVectorType>(Ty)->getNumElements(), El,
                           MIRBuilder);
  }
  if (Ty->isArrayTy()) {
    auto *El = findSPIRVType(Ty->getArrayElementType(), MIRBuilder);
    return getOpTypeArray(Ty->getArrayNumElements(), El, MIRBuilder, EmitIR);
  }
  if (auto SType = dyn_cast<StructType>(Ty)) {
    if (isOpenCLBuiltinType(SType))
      return handleOpenCLBuiltin(SType, MIRBuilder, AccQual);
    if (isSPIRVBuiltinType(SType))
      return handleSPIRVBuiltin(SType, MIRBuilder, AccQual);
    if (SType->isOpaque())
      return getOpTypeOpaque(SType, MIRBuilder);
    return getOpTypeStruct(SType, MIRBuilder, EmitIR);
  }
  if (auto FType = dyn_cast<FunctionType>(Ty)) {
    SPIRVType *RetTy = findSPIRVType(FType->getReturnType(), MIRBuilder);
    SmallVector<SPIRVType *, 4> ParamTypes;
    for (const auto &t : FType->params()) {
      ParamTypes.push_back(findSPIRVType(t, MIRBuilder));
    }
    return getOpTypeFunction(RetTy, ParamTypes, MIRBuilder);
  }
  if (auto PType = dyn_cast<PointerType>(Ty)) {
    SPIRVType *SpvElementType;
    // At the moment, all opaque pointers correspond to i8 element type.
    // TODO: change the implementation once opaque pointers are supported
    // in the SPIR-V specification. Use getNonOpaquePointerElementType()
    // instead of getPointerElementType() once llvm is rebased to 15.
    if (PType->isOpaque()) {
      SpvElementType = getOrCreateSPIRVIntegerType(8, MIRBuilder);
    } else {
      Type *ElemType = PType->getPointerElementType();

      // Some OpenCL and SPIRV builtins like image2d_t are passed in as
      // pointers, but should be treated as custom types like OpTypeImage.
      if (auto SType = dyn_cast<StructType>(ElemType)) {
        if (isOpenCLBuiltinType(SType))
          return handleOpenCLBuiltin(SType, MIRBuilder, AccQual);
        if (isSPIRVBuiltinType(SType))
          return handleSPIRVBuiltin(SType, MIRBuilder, AccQual);
      }
      // Otherwise, treat it as a regular pointer type.
      SpvElementType =
          findSPIRVType(ElemType, MIRBuilder, AQ::ReadWrite, EmitIR);
    }
    auto SC = addressSpaceToStorageClass(PType->getAddressSpace());
    // Null pointer means we have a loop in type definitions, make and
    // return corresponding OpTypeForwardPointer.
    if (SpvElementType == nullptr) {
      if (ForwardPointerTypes.find(Ty) == ForwardPointerTypes.end())
        ForwardPointerTypes[PType] = getOpTypeForwardPointer(SC, MIRBuilder);
      return ForwardPointerTypes[PType];
    }
    Register Reg(0);
    // If we have forward pointer associated with this type, use its register
    // operand to create OpTypePointer.
    if (ForwardPointerTypes.find(PType) != ForwardPointerTypes.end())
      Reg = getSPIRVTypeID(ForwardPointerTypes[PType]);

    return getOpTypePointer(SC, SpvElementType, MIRBuilder, Reg);
  }
  llvm_unreachable("Unable to convert LLVM type to SPIRVType");
}

SPIRVType *SPIRVGlobalRegistry::restOfCreateSPIRVType(
    const Type *Ty, MachineIRBuilder &MIRBuilder,
    AQ::AccessQualifier AccessQual, bool EmitIR) {
  if (TypesInProcessing.count(Ty) && !Ty->isPointerTy())
    return nullptr;
  TypesInProcessing.insert(Ty);
  SPIRVType *SpirvType = createSPIRVType(Ty, MIRBuilder, AccessQual, EmitIR);
  TypesInProcessing.erase(Ty);
  VRegToTypeMap[&MIRBuilder.getMF()][getSPIRVTypeID(SpirvType)] = SpirvType;
  SPIRVToLLVMType[SpirvType] = Ty;
  Register Reg;
  // Do not add OpTypeForwardPointer to DT, a corresponding normal pointer type
  // will be added later.
  if (SpirvType->getOpcode() != SPIRV::OpTypeForwardPointer &&
      !DT.find(Ty, &MIRBuilder.getMF(), Reg))
    DT.add(Ty, &MIRBuilder.getMF(), getSPIRVTypeID(SpirvType));

  return SpirvType;
}

SPIRVType *SPIRVGlobalRegistry::getSPIRVTypeForVReg(Register VReg) const {
  auto t = VRegToTypeMap.find(CurMF);
  if (t != VRegToTypeMap.end()) {
    auto tt = t->second.find(VReg);
    if (tt != t->second.end())
      return tt->second;
  }
  return nullptr;
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVType(
    const Type *Ty, MachineIRBuilder &MIRBuilder,
    AQ::AccessQualifier AccessQual, bool EmitIR) {
  Register Reg;
  if (DT.find(Ty, &MIRBuilder.getMF(), Reg))
    return getSPIRVTypeForVReg(Reg);
  TypesInProcessing.clear();
  SPIRVType *STy = restOfCreateSPIRVType(Ty, MIRBuilder, AccessQual, EmitIR);
  // Create normal pointer types for the corresponding OpTypeForwardPointers.
  for (auto &CU : ForwardPointerTypes) {
    const Type *Ty2 = CU.first;
    SPIRVType *STy2 = CU.second;
    if (DT.find(Ty2, &MIRBuilder.getMF(), Reg))
      STy2 = getSPIRVTypeForVReg(Reg);
    else
      STy2 = restOfCreateSPIRVType(Ty2, MIRBuilder, AccessQual, EmitIR);
    if (Ty == Ty2)
      STy = STy2;
  }
  ForwardPointerTypes.clear();
  return STy;
}

bool SPIRVGlobalRegistry::isScalarOfType(Register VReg,
                                         unsigned TypeOpcode) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  assert(Type && "isScalarOfType VReg has no type assigned");
  return Type->getOpcode() == TypeOpcode;
}

bool SPIRVGlobalRegistry::isScalarOrVectorOfType(Register VReg,
                                                 unsigned TypeOpcode) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  assert(Type && "isScalarOrVectorOfType VReg has no type assigned");
  if (Type->getOpcode() == TypeOpcode)
    return true;
  if (Type->getOpcode() == SPIRV::OpTypeVector) {
    Register ScalarTypeVReg = Type->getOperand(1).getReg();
    SPIRVType *ScalarType = getSPIRVTypeForVReg(ScalarTypeVReg);
    return ScalarType->getOpcode() == TypeOpcode;
  }
  return false;
}

unsigned
SPIRVGlobalRegistry::getScalarOrVectorBitWidth(const SPIRVType *Type) const {
  assert(Type && "Invalid Type pointer");
  if (Type->getOpcode() == SPIRV::OpTypeVector) {
    auto EleTypeReg = Type->getOperand(1).getReg();
    Type = getSPIRVTypeForVReg(EleTypeReg);
  }
  if (Type->getOpcode() == SPIRV::OpTypeInt ||
      Type->getOpcode() == SPIRV::OpTypeFloat)
    return Type->getOperand(1).getImm();
  if (Type->getOpcode() == SPIRV::OpTypeBool)
    return 1;
  llvm_unreachable("Attempting to get bit width of non-integer/float type.");
}

bool SPIRVGlobalRegistry::isScalarOrVectorSigned(const SPIRVType *Type) const {
  assert(Type && "Invalid Type pointer");
  if (Type->getOpcode() == SPIRV::OpTypeVector) {
    auto EleTypeReg = Type->getOperand(1).getReg();
    Type = getSPIRVTypeForVReg(EleTypeReg);
  }
  if (Type->getOpcode() == SPIRV::OpTypeInt)
    return Type->getOperand(2).getImm() != 0;
  llvm_unreachable("Attempting to get sign of non-integer type.");
}

StorageClass::StorageClass
SPIRVGlobalRegistry::getPointerStorageClass(Register VReg) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  assert(Type && Type->getOpcode() == SPIRV::OpTypePointer &&
         Type->getOperand(1).isImm() && "Pointer type is expected");
  return static_cast<StorageClass::StorageClass>(Type->getOperand(1).getImm());
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateOpTypeImage(
    MachineIRBuilder &MIRBuilder, SPIRVType *SampledType, Dim::Dim Dim,
    uint32_t Depth, uint32_t Arrayed, uint32_t Multisampled, uint32_t Sampled,
    ImageFormat::ImageFormat ImageFormat, AQ::AccessQualifier AccessQual) {
  SPIRV::ImageTypeDescriptor TD(SPIRVToLLVMType.lookup(SampledType), Dim, Depth,
                                Arrayed, Multisampled, Sampled, ImageFormat,
                                AccessQual);
  if (auto *Res = checkSpecialInstr(TD, MIRBuilder))
    return Res;
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeImage)
                 .addDef(ResVReg)
                 .addUse(getSPIRVTypeID(SampledType))
                 .addImm(Dim)
                 .addImm(Depth)   // Depth (whether or not it is a Depth image)
                 .addImm(Arrayed) // Arrayed
                 .addImm(Multisampled) // Multisampled (0 = only single-sample)
                 .addImm(Sampled)      // Sampled (0 = usage known at runtime)
                 .addImm(ImageFormat)
                 .addImm(AccessQual);
  DT.add(TD, &MIRBuilder.getMF(), ResVReg);
  return MIB;
}

SPIRVType *
SPIRVGlobalRegistry::getOrCreateOpTypeSampler(MachineIRBuilder &MIRBuilder) {
  SPIRV::SamplerTypeDescriptor TD;
  if (auto *Res = checkSpecialInstr(TD, MIRBuilder))
    return Res;
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampler).addDef(ResVReg);
  constrainRegOperands(MIB);
  DT.add(TD, &MIRBuilder.getMF(), ResVReg);
  return MIB;
}

SPIRVType *
SPIRVGlobalRegistry::getOrCreateOpTypePipe(MachineIRBuilder &MIRBuilder,
                                           AQ::AccessQualifier AccessQual) {
  SPIRV::PipeTypeDescriptor TD(AccessQual);
  if (auto *Res = checkSpecialInstr(TD, MIRBuilder))
    return Res;
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePipe)
                 .addDef(ResVReg)
                 .addImm(AccessQual);
  constrainRegOperands(MIB);
  DT.add(TD, &MIRBuilder.getMF(), ResVReg);
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateOpTypeSampledImage(
    SPIRVType *ImageType, MachineIRBuilder &MIRBuilder) {
  SPIRV::SampledImageTypeDescriptor TD(
      SPIRVToLLVMType.lookup(MIRBuilder.getMF().getRegInfo().getVRegDef(
          ImageType->getOperand(1).getReg())),
      ImageType);
  if (auto *Res = checkSpecialInstr(TD, MIRBuilder))
    return Res;
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampledImage)
                 .addDef(ResVReg)
                 .addUse(getSPIRVTypeID(ImageType));
  constrainRegOperands(MIB);
  DT.add(TD, &MIRBuilder.getMF(), ResVReg);

  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeByOpcode(MachineIRBuilder &MIRBuilder,
                                                  unsigned Opcode) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(Opcode).addDef(ResVReg);
  constrainRegOperands(MIB);
  return MIB;
}

const MachineInstr *
SPIRVGlobalRegistry::checkSpecialInstr(const SPIRV::SpecialTypeDescriptor &TD,
                                       MachineIRBuilder &MIRBuilder) {
  Register Reg;
  if (DT.find(TD, &MIRBuilder.getMF(), Reg))
    return MIRBuilder.getMF().getRegInfo().getUniqueVRegDef(Reg);
  return nullptr;
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateOpenCLOpaqueType(
    const StructType *Ty, MachineIRBuilder &MIRBuilder,
    AQ::AccessQualifier AccessQual) {
  const StringRef Name = Ty->getName();
  assert(Name.startswith("opencl.") && "CL types should start with 'opencl.'");
  auto TypeName = Name.substr(strlen("opencl."));

  if (TypeName.startswith("image")) {
    // default is read only
    AccessQual = AQ::ReadOnly;
    if (TypeName.endswith("_ro_t"))
      AccessQual = AQ::ReadOnly;
    else if (TypeName.endswith("_wo_t"))
      AccessQual = AQ::WriteOnly;
    else if (TypeName.endswith("_rw_t"))
      AccessQual = AQ::ReadWrite;
    char DimC = TypeName[strlen("image")];
    if (DimC >= '1' && DimC <= '3') {
      auto Dim = DimC == '1'   ? Dim::DIM_1D
                 : DimC == '2' ? Dim::DIM_2D
                               : Dim::DIM_3D;
      unsigned Arrayed = 0;
      unsigned Depth = 0;
      if (TypeName.contains("buffer"))
        Dim = Dim::DIM_Buffer;
      if (TypeName.contains("array"))
        Arrayed = 1;
      if (TypeName.contains("depth"))
        Depth = 1;
      auto *VoidTy = getOrCreateSPIRVType(
          Type::getVoidTy(MIRBuilder.getMF().getFunction().getContext()),
          MIRBuilder);
      return getOrCreateOpTypeImage(MIRBuilder, VoidTy, Dim, Depth, Arrayed, 0,
                                    0, ImageFormat::Unknown, AccessQual);
    }
  } else if (TypeName.startswith("clk_event_t")) {
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeDeviceEvent);
  } else if (TypeName.startswith("sampler_t")) {
    return getOrCreateOpTypeSampler(MIRBuilder);
  } else if (TypeName.startswith("pipe")) {
    if (TypeName.endswith("_ro_t"))
      AccessQual = AQ::ReadOnly;
    else if (TypeName.endswith("_wo_t"))
      AccessQual = AQ::WriteOnly;
    else if (TypeName.endswith("_rw_t"))
      AccessQual = AQ::ReadWrite;
    return getOrCreateOpTypePipe(MIRBuilder, AccessQual);
  } else if (TypeName.startswith("queue"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeQueue);
  else if (TypeName.startswith("event_t"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeEvent);
  else if (TypeName.startswith("reserve_id_t"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeReserveId);

  report_fatal_error("Cannot generate OpenCL type: " + Name);
}

SPIRVType *
SPIRVGlobalRegistry::getOrCreateSPIRVTypeByName(StringRef TypeStr,
                                                MachineIRBuilder &MIRBuilder) {
  unsigned VecElts = 0;
  auto &Ctx = MIRBuilder.getMF().getFunction().getContext();

  // Parse type name in either "typeN" or "type vector[N]" format, where
  // N is the number of elements of the vector.
  Type *Type;
  if (TypeStr.startswith("void")) {
    Type = Type::getVoidTy(Ctx);
    TypeStr = TypeStr.substr(strlen("void"));
  } else if (TypeStr.startswith("int") || TypeStr.startswith("uint")) {
    Type = Type::getInt32Ty(Ctx);
    TypeStr = TypeStr.startswith("int") ? TypeStr.substr(strlen("int"))
                                        : TypeStr.substr(strlen("uint"));
  } else if (TypeStr.startswith("float")) {
    Type = Type::getFloatTy(Ctx);
    TypeStr = TypeStr.substr(strlen("float"));
  } else if (TypeStr.startswith("half")) {
    Type = Type::getHalfTy(Ctx);
    TypeStr = TypeStr.substr(strlen("half"));
  } else if (TypeStr.startswith("opencl.sampler_t")) {
    Type = StructType::create(Ctx, "opencl.sampler_t");
  } else
    llvm_unreachable("Unable to recognize SPIRV type name.");
  if (TypeStr.startswith(" vector[")) {
    TypeStr = TypeStr.substr(strlen(" vector["));
    TypeStr = TypeStr.substr(0, TypeStr.find(']'));
  }
  TypeStr.getAsInteger(10, VecElts);
  auto SpirvTy = getOrCreateSPIRVType(Type, MIRBuilder);
  if (VecElts > 0)
    SpirvTy = getOrCreateSPIRVVectorType(SpirvTy, VecElts, MIRBuilder);
  return SpirvTy;
}

SPIRVType *
SPIRVGlobalRegistry::handleSPIRVBuiltin(const StructType *Ty,
                                        MachineIRBuilder &MIRBuilder,
                                        AQ::AccessQualifier AccQual) {
  assert(Ty->hasName());
  unsigned NumStartingVRegs = MIRBuilder.getMRI()->getNumVirtRegs();
  auto NewTy = getOrCreateSPIRVOpaqueType(Ty, MIRBuilder, AccQual);
  if (NumStartingVRegs < MIRBuilder.getMRI()->getNumVirtRegs())
    buildOpName(getSPIRVTypeID(NewTy), Ty->getName(), MIRBuilder);
  return NewTy;
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVOpaqueType(
    const StructType *Ty, MachineIRBuilder &MIRBuilder,
    AQ::AccessQualifier AccessQual) {
  const StringRef Name = Ty->getName();
  assert(Name.startswith("spirv.") && "CL types should start with 'spirv.'");
  auto TypeName = Name.substr(strlen("spirv."));

  if (TypeName.startswith("Image.")) {
    // Parse SPIRV ImageType which has following format in LLVM:
    // Image._Type_Dim_Depth_Arrayed_MS_Sampled_ImageFormat_AccessQualifier
    // e.g. %spirv.Image._void_1_0_0_0_0_0_0.
    auto TypeLiteralStr = TypeName.substr(strlen("Image."));
    SmallVector<StringRef> TypeLiterals;
    SplitString(TypeLiteralStr, TypeLiterals, "_");
    assert(TypeLiterals.size() == 8 &&
           "Wrong number of literals in Image type");
    auto *SpirvType = getOrCreateSPIRVTypeByName(TypeLiterals[0], MIRBuilder);

    unsigned Ddim = 0, Depth = 0, Arrayed = 0, MS = 0, Sampled = 0;
    unsigned ImageFormat = 0, AccQual = 0;
    if (TypeLiterals[1].getAsInteger(10, Ddim) ||
        TypeLiterals[2].getAsInteger(10, Depth) ||
        TypeLiterals[3].getAsInteger(10, Arrayed) ||
        TypeLiterals[4].getAsInteger(10, MS) ||
        TypeLiterals[5].getAsInteger(10, Sampled) ||
        TypeLiterals[6].getAsInteger(10, ImageFormat) ||
        TypeLiterals[7].getAsInteger(10, AccQual))
      llvm_unreachable("Unable to recognize Image type literals");
    return getOrCreateOpTypeImage(
        MIRBuilder, SpirvType, Dim::Dim(Ddim), Depth, Arrayed, MS, Sampled,
        ImageFormat::ImageFormat(ImageFormat), AQ::AccessQualifier(AccQual));
  } else if (TypeName.startswith("SampledImage.")) {
    // Find corresponding Image type.
    auto Literals = TypeName.substr(strlen("SampledImage."));
    auto &Ctx = MIRBuilder.getMF().getFunction().getContext();
    Type *ImgTy =
        StructType::getTypeByName(Ctx, "spirv.Image." + Literals.str());
    SPIRVType *SpirvImageType = getOrCreateSPIRVType(ImgTy, MIRBuilder);
    return getOrCreateOpTypeSampledImage(SpirvImageType, MIRBuilder);
  } else if (TypeName.startswith("DeviceEvent"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeDeviceEvent);
  else if (TypeName.startswith("Sampler"))
    return getOrCreateOpTypeSampler(MIRBuilder);
  else if (TypeName.startswith("Event"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeEvent);
  else if (TypeName.startswith("Queue"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeQueue);
  else if (TypeName.startswith("Pipe.")) {
    if (TypeName.endswith("_0"))
      AccessQual = AQ::ReadOnly;
    else if (TypeName.endswith("_1"))
      AccessQual = AQ::WriteOnly;
    else if (TypeName.endswith("_2"))
      AccessQual = AQ::ReadWrite;
    return getOrCreateOpTypePipe(MIRBuilder, AccessQual);
  } else if (TypeName.startswith("PipeStorage"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypePipeStorage);
  else if (TypeName.startswith("ReserveId"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeReserveId);

  report_fatal_error("Cannot generate SPIRV built-in type: " + Name);
}

SPIRVType *
SPIRVGlobalRegistry::getOrCreateSPIRVIntegerType(unsigned BitWidth,
                                                 MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      IntegerType::get(MIRBuilder.getMF().getFunction().getContext(), BitWidth),
      MIRBuilder);
}

SPIRVType *SPIRVGlobalRegistry::restOfCreateSPIRVType(Type *LLVMTy,
                                                      SPIRVType *SpirvType) {
  VRegToTypeMap[CurMF][getSPIRVTypeID(SpirvType)] = SpirvType;
  SPIRVToLLVMType[SpirvType] = LLVMTy;
  DT.add(LLVMTy, CurMF, getSPIRVTypeID(SpirvType));
  return SpirvType;
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVIntegerType(
    unsigned BitWidth, MachineInstr &I, const SPIRVInstrInfo &TII) {
  Type *LLVMTy = IntegerType::get(CurMF->getFunction().getContext(), BitWidth);
  Register Reg;
  if (DT.find(LLVMTy, CurMF, Reg)) {
    return getSPIRVTypeForVReg(Reg);
  }
  MachineBasicBlock &BB = *I.getParent();
  auto MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpTypeInt))
                 .addDef(createTypeVReg(CurMF->getRegInfo()))
                 .addImm(BitWidth)
                 .addImm(0);
  return restOfCreateSPIRVType(LLVMTy, MIB);
}

SPIRVType *
SPIRVGlobalRegistry::getOrCreateSPIRVBoolType(MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      IntegerType::get(MIRBuilder.getMF().getFunction().getContext(), 1),
      MIRBuilder);
}

SPIRVType *
SPIRVGlobalRegistry::getOrCreateSPIRVBoolType(MachineInstr &I,
                                              const SPIRVInstrInfo &TII) {
  Type *LLVMTy = IntegerType::get(CurMF->getFunction().getContext(), 1);
  Register Reg;
  if (DT.find(LLVMTy, CurMF, Reg)) {
    return getSPIRVTypeForVReg(Reg);
  }
  MachineBasicBlock &BB = *I.getParent();
  auto MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpTypeBool))
                 .addDef(createTypeVReg(CurMF->getRegInfo()));
  return restOfCreateSPIRVType(LLVMTy, MIB);
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVVectorType(
    SPIRVType *BaseType, unsigned NumElements, MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      FixedVectorType::get(const_cast<Type *>(getTypeForSPIRVType(BaseType)),
                           NumElements),
      MIRBuilder);
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVVectorType(
    SPIRVType *BaseType, unsigned NumElements, MachineInstr &I,
    const SPIRVInstrInfo &TII) {
  Type *LLVMTy = FixedVectorType::get(
      const_cast<Type *>(getTypeForSPIRVType(BaseType)), NumElements);
  Register Reg;
  if (DT.find(LLVMTy, CurMF, Reg)) {
    return getSPIRVTypeForVReg(Reg);
  }
  MachineBasicBlock &BB = *I.getParent();
  auto MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpTypeVector))
                 .addDef(createTypeVReg(CurMF->getRegInfo()))
                 .addUse(getSPIRVTypeID(BaseType))
                 .addImm(NumElements);
  return restOfCreateSPIRVType(LLVMTy, MIB);
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVPointerType(
    SPIRVType *BaseType, MachineIRBuilder &MIRBuilder,
    StorageClass::StorageClass SClass) {
  return getOrCreateSPIRVType(
      PointerType::get(const_cast<Type *>(getTypeForSPIRVType(BaseType)),
                       storageClassToAddressSpace(SClass)),
      MIRBuilder);
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVPointerType(
    SPIRVType *BaseType, MachineInstr &I, const SPIRVInstrInfo &TII,
    StorageClass::StorageClass SC) {
  Type *LLVMTy =
      PointerType::get(const_cast<Type *>(getTypeForSPIRVType(BaseType)),
                       storageClassToAddressSpace(SC));
  Register Reg;
  if (DT.find(LLVMTy, CurMF, Reg)) {
    return getSPIRVTypeForVReg(Reg);
  }
  MachineBasicBlock &BB = *I.getParent();
  auto MIB = BuildMI(BB, I, I.getDebugLoc(), TII.get(SPIRV::OpTypePointer))
                 .addDef(createTypeVReg(CurMF->getRegInfo()))
                 .addImm(static_cast<uint32_t>(SC))
                 .addUse(getSPIRVTypeID(BaseType));
  return restOfCreateSPIRVType(LLVMTy, MIB);
}

Register SPIRVGlobalRegistry::getOrCreateUndef(MachineInstr &I,
                                               SPIRVType *SpvType,
                                               const SPIRVInstrInfo &TII) {
  assert(SpvType);
  const Type *LLVMTy = getTypeForSPIRVType(SpvType);
  assert(LLVMTy);
  Register Res;
  // Find a constant in DT or build a new one.
  UndefValue *UV = UndefValue::get(const_cast<Type *>(LLVMTy));
  if (DT.find(UV, CurMF, Res))
    return Res;

  LLT LLTy = LLT::scalar(32);
  Res = CurMF->getRegInfo().createGenericVirtualRegister(LLTy);
  assignSPIRVTypeToVReg(SpvType, Res, *CurMF);
  DT.add(UV, CurMF, Res);

  MachineInstrBuilder MIB;
  MIB = BuildMI(*I.getParent(), I, I.getDebugLoc(), TII.get(SPIRV::OpUndef))
            .addDef(Res)
            .addUse(getSPIRVTypeID(SpvType));
  const auto &ST = CurMF->getSubtarget();
  constrainSelectedInstRegOperands(*MIB, *ST.getInstrInfo(),
                                   *ST.getRegisterInfo(), *ST.getRegBankInfo());
  return Res;
}
