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
#include "SPIRVSymbolicOperands.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVUtils.h"

using namespace llvm;
SPIRVGlobalRegistry::SPIRVGlobalRegistry(unsigned int PointerSize)
    : PointerSize(PointerSize) {}

SPIRVType *SPIRVGlobalRegistry::assignTypeToVReg(const Type *Type,
                                                 Register VReg,
                                                 MachineIRBuilder &MIRBuilder,
                                                 AQ::AccessQualifier AccessQual,
                                                 bool EmitIR) {
  SPIRVType *SpirvType =
      getOrCreateSPIRVType(Type, MIRBuilder, AccessQual, EmitIR);
  assignSPIRVTypeToVReg(SpirvType, VReg, MIRBuilder);
  return SpirvType;
}

void SPIRVGlobalRegistry::assignSPIRVTypeToVReg(SPIRVType *SpirvType,
                                                Register VReg,
                                                MachineIRBuilder &MIRBuilder) {
  VRegToTypeMap[&MIRBuilder.getMF()][VReg] = SpirvType;
}

static Register createTypeVReg(MachineIRBuilder &MIRBuilder) {
  auto &MRI = MIRBuilder.getMF().getRegInfo();
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
    Register ResReg, unsigned int AddrMode, unsigned int Param,
    unsigned int FilerMode, MachineIRBuilder &MIRBuilder, SPIRVType *SpvType) {
  SPIRVType *SampTy;
  if (SpvType)
    SampTy = getOrCreateSPIRVType(getTypeForSPIRVType(SpvType), MIRBuilder);
  else
    SampTy = getOrCreateSPIRVTypeByName("opencl.sampler_t", MIRBuilder);

  auto Sampler =
      ResReg.isValid()
          ? ResReg
          : MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantSampler)
                 .addDef(Sampler)
                 .addUse(getSPIRVTypeID(SampTy))
                 .addImm(AddrMode)
                 .addImm(Param)
                 .addImm(FilerMode);
  constrainRegOperands(MIB);
  auto Res = checkSpecialInstrMap(MIB, SpecialTypesAndConstsMap);
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
    GVar = dyn_cast<const GlobalVariable>(GV);
  else {
    // If GV is not passed explicitly, use the name to find or construct
    // the global variable.
    auto *Module = MIRBuilder.getMBB().getBasicBlock()->getModule();
    GVar = Module->getGlobalVariable(Name);
    if (GVar == nullptr) {
      auto Ty = getTypeForSPIRVType(BaseType); // TODO check type
      GVar = new GlobalVariable(
          *const_cast<llvm::Module *>(Module), const_cast<Type *>(Ty), false,
          GlobalValue::ExternalLinkage, nullptr, Twine(Name));
    }
    GV = GVar;
  }
  assert(GV && "Global variable is expected");
  Register Reg;
  if (DT.find(GV, &MIRBuilder.getMF(), Reg)) {
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
  DT.add(GV, &MIRBuilder.getMF(), Reg);

  // Set to Reg the same type as ResVReg has.
  auto MRI = MIRBuilder.getMRI();
  assert(MRI->getType(ResVReg).isPointer() && "Pointer type is expected");
  if (Reg != ResVReg) {
    LLT RegLLTy = LLT::pointer(MRI->getType(ResVReg).getAddressSpace(), 32);
    MRI->setType(Reg, RegLLTy);
    assignSPIRVTypeToVReg(BaseType, Reg, MIRBuilder);
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
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeStruct).addDef(ResVReg);
  for (const auto &Elem : Ty->elements()) {
    auto ElemTy = getOrCreateSPIRVType(Elem, MIRBuilder, AQ::ReadWrite, EmitIR);
    if (ElemTy->getOpcode() == SPIRV::OpTypeVoid)
      report_fatal_error("Invalid struct element type");
    MIB.addUse(getSPIRVTypeID(ElemTy));
  }
  if (Ty->hasName())
    buildOpName(ResVReg, Ty->getName(), MIRBuilder);
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
  auto NewTy = generateOpenCLOpaqueType(Ty, MIRBuilder, AccQual);
  auto ResTy = checkSpecialInstrMap(NewTy, BuiltinTypeMap);
  if (NumStartingVRegs < MIRBuilder.getMRI()->getNumVirtRegs() &&
      ResTy == NewTy)
    buildOpName(getSPIRVTypeID(ResTy), Ty->getName(), MIRBuilder);
  return ResTy;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypePointer(StorageClass::StorageClass SC,
                                                 SPIRVType *ElemType,
                                                 MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePointer)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(SC)
                 .addUse(getSPIRVTypeID(ElemType));
  return MIB;
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

SPIRVType *SPIRVGlobalRegistry::createSPIRVType(const Type *Ty,
                                                MachineIRBuilder &MIRBuilder,
                                                AQ::AccessQualifier AccQual,
                                                bool EmitIR) {
  auto &TypeToSPIRVTypeMap = DT.get<Type>()->getAllUses();
  auto t = TypeToSPIRVTypeMap.find(Ty);
  if (t != TypeToSPIRVTypeMap.end()) {
    auto tt = t->second.find(&MIRBuilder.getMF());
    if (tt != t->second.end())
      return getSPIRVTypeForVReg(tt->second);
  }

  if (auto IType = dyn_cast<IntegerType>(Ty)) {
    const unsigned int Width = IType->getBitWidth();
    return Width == 1 ? getOpTypeBool(MIRBuilder)
                      : getOpTypeInt(Width, MIRBuilder, false);
  }
  if (Ty->isFloatingPointTy())
    return getOpTypeFloat(Ty->getPrimitiveSizeInBits(), MIRBuilder);
  if (Ty->isVoidTy())
    return getOpTypeVoid(MIRBuilder);
  if (Ty->isVectorTy()) {
    auto El = getOrCreateSPIRVType(cast<FixedVectorType>(Ty)->getElementType(),
                                   MIRBuilder);
    return getOpTypeVector(cast<FixedVectorType>(Ty)->getNumElements(), El,
                           MIRBuilder);
  }
  if (Ty->isArrayTy()) {
    auto *El = getOrCreateSPIRVType(Ty->getArrayElementType(), MIRBuilder);
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
    SPIRVType *RetTy = getOrCreateSPIRVType(FType->getReturnType(), MIRBuilder);
    SmallVector<SPIRVType *, 4> ParamTypes;
    for (const auto &t : FType->params()) {
      ParamTypes.push_back(getOrCreateSPIRVType(t, MIRBuilder));
    }
    return getOpTypeFunction(RetTy, ParamTypes, MIRBuilder);
  }
  if (auto PType = dyn_cast<PointerType>(Ty)) {
    Type *ElemType = PType->getPointerElementType();

    // Some OpenCL and SPIRV builtins like image2d_t are passed in as pointers,
    // but should be treated as custom types like OpTypeImage
    if (auto SType = dyn_cast<StructType>(ElemType)) {
      if (isOpenCLBuiltinType(SType))
        return handleOpenCLBuiltin(SType, MIRBuilder, AccQual);
      if (isSPIRVBuiltinType(SType))
        return handleSPIRVBuiltin(SType, MIRBuilder, AccQual);
    }

    // Otherwise, treat it as a regular pointer type
    auto SC = addressSpaceToStorageClass(PType->getAddressSpace());
    SPIRVType *SpvElementType =
        getOrCreateSPIRVType(ElemType, MIRBuilder, AQ::ReadWrite, EmitIR);
    return getOpTypePointer(SC, SpvElementType, MIRBuilder);
  }
  llvm_unreachable("Unable to convert LLVM type to SPIRVType");
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
    const Type *Type, MachineIRBuilder &MIRBuilder,
    AQ::AccessQualifier AccessQual, bool EmitIR) {
  Register Reg;
  if (DT.find(Type, &MIRBuilder.getMF(), Reg)) {
    return getSPIRVTypeForVReg(Reg);
  }
  SPIRVType *SpirvType = createSPIRVType(Type, MIRBuilder, AccessQual, EmitIR);
  VRegToTypeMap[&MIRBuilder.getMF()][getSPIRVTypeID(SpirvType)] = SpirvType;
  SPIRVToLLVMType[SpirvType] = Type;
  DT.add(Type, &MIRBuilder.getMF(), getSPIRVTypeID(SpirvType));
  return SpirvType;
}

bool SPIRVGlobalRegistry::isScalarOfType(Register VReg,
                                         unsigned int TypeOpcode) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  assert(Type && "isScalarOfType VReg has no type assigned");
  return Type->getOpcode() == TypeOpcode;
}

bool SPIRVGlobalRegistry::isScalarOrVectorOfType(
    Register VReg, unsigned int TypeOpcode) const {
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
  if (Type && Type->getOpcode() == SPIRV::OpTypeVector) {
    auto EleTypeReg = Type->getOperand(1).getReg();
    Type = getSPIRVTypeForVReg(EleTypeReg);
  }
  if (Type && (Type->getOpcode() == SPIRV::OpTypeInt ||
               Type->getOpcode() == SPIRV::OpTypeFloat))
    return Type->getOperand(1).getImm();
  if (Type && Type->getOpcode() == SPIRV::OpTypeBool)
    return 1;
  llvm_unreachable("Attempting to get bit width of non-integer/float type.");
}

bool SPIRVGlobalRegistry::isScalarOrVectorSigned(const SPIRVType *Type) const {
  if (Type && Type->getOpcode() == SPIRV::OpTypeVector) {
    auto EleTypeReg = Type->getOperand(1).getReg();
    Type = getSPIRVTypeForVReg(EleTypeReg);
  }
  if (Type && Type->getOpcode() == SPIRV::OpTypeInt) {
    return Type->getOperand(2).getImm() != 0;
  }
  llvm_unreachable("Attempting to get sign of non-integer type.");
}

StorageClass::StorageClass
SPIRVGlobalRegistry::getPointerStorageClass(Register VReg) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  if (Type && Type->getOpcode() == SPIRV::OpTypePointer) {
    auto scOp = Type->getOperand(1).getImm();
    return static_cast<StorageClass::StorageClass>(scOp);
  }
  llvm_unreachable("Attempting to get storage class of non-pointer type.");
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeImage(
    MachineIRBuilder &MIRBuilder, SPIRVType *SampledType, Dim::Dim Dim,
    uint32_t Depth, uint32_t Arrayed, uint32_t Multisampled, uint32_t Sampled,
    ImageFormat::ImageFormat ImageFormat, AQ::AccessQualifier AccessQual) {
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
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getSamplerType(MachineIRBuilder &MIRBuilder) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampler).addDef(ResVReg);
  constrainRegOperands(MIB);
  return MIB;
}

SPIRVType *SPIRVGlobalRegistry::getOpTypePipe(MachineIRBuilder &MIRBuilder,
                                              AQ::AccessQualifier AccessQual) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePipe)
                 .addDef(ResVReg)
                 .addImm(AccessQual);
  constrainRegOperands(MIB);
  return MIB;
}

SPIRVType *
SPIRVGlobalRegistry::getSampledImageType(SPIRVType *ImageType,
                                         MachineIRBuilder &MIRBuilder) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampledImage)
                 .addDef(ResVReg)
                 .addUse(getSPIRVTypeID(ImageType));
  constrainRegOperands(MIB);
  return checkSpecialInstrMap(MIB, SpecialTypesAndConstsMap);
}

SPIRVType *SPIRVGlobalRegistry::getOpTypeByOpcode(MachineIRBuilder &MIRBuilder,
                                                  unsigned int Opcode) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(Opcode).addDef(ResVReg);
  constrainRegOperands(MIB);
  return MIB;
}

const MachineInstr *
SPIRVGlobalRegistry::checkSpecialInstrMap(const MachineInstr *NewInstr,
                                          SpecialInstrMapTy &InstrMap) {
  auto t = InstrMap.find(NewInstr->getOpcode());
  if (t != InstrMap.end()) {
    for (auto InstrGroup : t->second) {
      // Each group contins identical special instructions in different MFs,
      // it's enough to check the first instruction in the group.
      const MachineInstr *Instr = InstrGroup.begin()->second;
      if (Instr->isIdenticalTo(*NewInstr,
                               MachineInstr::MICheckType::IgnoreDefs)) {
        auto tt =
            InstrGroup.find(const_cast<MachineFunction *>(NewInstr->getMF()));
        if (tt != InstrGroup.end()) {
          // The equivalent instruction was found in this MF,
          // remove new instruction and return the existing one.
          const_cast<llvm::MachineInstr *>(NewInstr)->eraseFromParent();
          return tt->second;
        }
        // No such instruction in the group, add new instruction.
        InstrGroup[const_cast<MachineFunction *>(NewInstr->getMF())] = NewInstr;
        return NewInstr;
      }
    }
  }
  // It's new instruction with no existent group, so create a group,
  // insert the instruction to the group and insert the group to the map.
  InstrMap[NewInstr->getOpcode()].push_back(SPIRVInstrGroup());
  MachineFunction *MF = const_cast<MachineFunction *>(NewInstr->getMF());
  std::pair<MachineFunction *, const MachineInstr *> Pair(MF, NewInstr);
  InstrMap[NewInstr->getOpcode()].back().insert(Pair);
  return NewInstr;
}

SPIRVType *
SPIRVGlobalRegistry::generateOpenCLOpaqueType(const StructType *Ty,
                                              MachineIRBuilder &MIRBuilder,
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
      auto Dim =
          DimC == '1' ? Dim::DIM_1D : DimC == '2' ? Dim::DIM_2D : Dim::DIM_3D;
      unsigned int Arrayed = 0;
      if (TypeName.contains("buffer"))
        Dim = Dim::DIM_Buffer;
      if (TypeName.contains("array"))
        Arrayed = 1;
      auto *VoidTy = getOrCreateSPIRVType(
          Type::getVoidTy(MIRBuilder.getMF().getFunction().getContext()),
          MIRBuilder);
      return getOpTypeImage(MIRBuilder, VoidTy, Dim, 0, Arrayed, 0, 0,
                            ImageFormat::Unknown, AccessQual);
    }
  } else if (TypeName.startswith("sampler_t")) {
    return getSamplerType(MIRBuilder);
  } else if (TypeName.startswith("pipe")) {
    if (TypeName.endswith("_ro_t"))
      AccessQual = AQ::ReadOnly;
    else if (TypeName.endswith("_wo_t"))
      AccessQual = AQ::WriteOnly;
    else if (TypeName.endswith("_rw_t"))
      AccessQual = AQ::ReadWrite;
    return getOpTypePipe(MIRBuilder, AccessQual);
  } else if (TypeName.startswith("queue"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeQueue);
  else if (TypeName.startswith("event_t"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeEvent);

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
  auto NewTy = generateSPIRVOpaqueType(Ty, MIRBuilder, AccQual);
  auto ResTy = checkSpecialInstrMap(NewTy, BuiltinTypeMap);
  if (NumStartingVRegs < MIRBuilder.getMRI()->getNumVirtRegs() &&
      NewTy == ResTy)
    buildOpName(getSPIRVTypeID(ResTy), Ty->getName(), MIRBuilder);
  return ResTy;
}

SPIRVType *
SPIRVGlobalRegistry::generateSPIRVOpaqueType(const StructType *Ty,
                                             MachineIRBuilder &MIRBuilder,
                                             AQ::AccessQualifier AccessQual) {
  const StringRef Name = Ty->getName();
  assert(Name.startswith("spirv.") && "CL types should start with 'opencl.'");
  auto TypeName = Name.substr(strlen("spirv."));

  if (TypeName.startswith("Image.")) {
    // Parse SPIRV ImageType which has following format in LLVM:
    // Image._Type_Dim_Depth_Arrayed_MS_Sampled_ImageFormat_AccessQualifier
    // e.g. %spirv.Image._void_1_0_0_0_0_0_0
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
    return getOpTypeImage(MIRBuilder, SpirvType, Dim::Dim(Ddim), Depth, Arrayed,
                          MS, Sampled, ImageFormat::ImageFormat(ImageFormat),
                          AQ::AccessQualifier(AccQual));
  } else if (TypeName.startswith("SampledImage.")) {
    // Find corresponding Image type.
    auto Literals = TypeName.substr(strlen("SampledImage."));
    auto &Ctx = MIRBuilder.getMF().getFunction().getContext();
    Type *ImgTy =
        StructType::getTypeByName(Ctx, "spirv.Image." + Literals.str());
    SPIRVType *SpirvImageType = getOrCreateSPIRVType(ImgTy, MIRBuilder);
    return getOrCreateSPIRVSampledImageType(SpirvImageType, MIRBuilder);
  } else if (TypeName.startswith("DeviceEvent"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeDeviceEvent);
  else if (TypeName.startswith("Sampler"))
    return getSamplerType(MIRBuilder);
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
    return getOpTypePipe(MIRBuilder, AccessQual);
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

SPIRVType *
SPIRVGlobalRegistry::getOrCreateSPIRVBoolType(MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      IntegerType::get(MIRBuilder.getMF().getFunction().getContext(), 1),
      MIRBuilder);
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVVectorType(
    SPIRVType *BaseType, unsigned NumElements, MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      FixedVectorType::get(const_cast<Type *>(getTypeForSPIRVType(BaseType)),
                           NumElements),
      MIRBuilder);
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVPointerType(
    SPIRVType *BaseType, MachineIRBuilder &MIRBuilder,
    StorageClass::StorageClass SClass) {
  return getOrCreateSPIRVType(
      PointerType::get(const_cast<Type *>(getTypeForSPIRVType(BaseType)),
                       storageClassToAddressSpace(SClass)),
      MIRBuilder);
}

SPIRVType *SPIRVGlobalRegistry::getOrCreateSPIRVSampledImageType(
    SPIRVType *ImageType, MachineIRBuilder &MIRBuilder) {
  SPIRVType *Type = getSampledImageType(ImageType, MIRBuilder);
  return Type;
}
