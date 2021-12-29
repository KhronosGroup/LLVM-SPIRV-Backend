//===-- SPIRVTypeRegistry.cpp - SPIR-V Type Registry ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the SPIRVTypeRegistry class,
// which is used to maintain rich type information required for SPIR-V even
// after lowering from LLVM IR to GMIR. It can convert an llvm::Type into
// an OpTypeXXX instruction, and map it to a virtual register using and
// ASSIGN_TYPE pseudo instruction.
//
//
//===----------------------------------------------------------------------===//

#include "SPIRVGlobalRegistry.h"
#include "SPIRV.h"
#include "SPIRVEnums.h"
#include "SPIRVOpenCLBIFs.h"
#include "SPIRVUtils.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/IntrinsicsSPIRV.h"

using namespace llvm;

static const std::unordered_set<unsigned> TypeFoldingSupportingOpcs = {
    TargetOpcode::G_ADD,
    TargetOpcode::G_FADD,
    TargetOpcode::G_SUB,
    TargetOpcode::G_FSUB,
    TargetOpcode::G_MUL,
    TargetOpcode::G_FMUL,
    TargetOpcode::G_SDIV,
    TargetOpcode::G_UDIV,
    TargetOpcode::G_FDIV,
    TargetOpcode::G_SREM,
    TargetOpcode::G_UREM,
    TargetOpcode::G_FREM,
    TargetOpcode::G_FNEG,
    TargetOpcode::G_CONSTANT,
    TargetOpcode::G_FCONSTANT,
    TargetOpcode::G_AND,
    TargetOpcode::G_OR,
    TargetOpcode::G_XOR,
    TargetOpcode::G_SHL,
    TargetOpcode::G_ASHR,
    TargetOpcode::G_LSHR,
    TargetOpcode::G_SELECT,
    TargetOpcode::G_EXTRACT_VECTOR_ELT,
    // TargetOpcode::G_INSERT_VECTOR_ELT
};

const std::unordered_set<unsigned> &getTypeFoldingSupportingOpcs() {
  return TypeFoldingSupportingOpcs;
}

bool isTypeFoldingSupported(unsigned Opcode) {
  return TypeFoldingSupportingOpcs.count(Opcode) > 0;
}

SPIRVTypeRegistry::SPIRVTypeRegistry(SPIRVGeneralDuplicatesTracker &DT,
                                     unsigned int PointerSize)
    : DT(DT), PointerSize(PointerSize) {}

// Translating GV, IRTranslator sometimes generates following IR:
//   %1 = G_GLOBAL_VALUE
//   %2 = COPY %1
//   %3 = G_ADDRSPACE_CAST %2
// New registers have no SPIRVType and no register class info.
// Propagate SPIRVType from GV to other instructions, also set register classes.
SPIRVType *SPIRVTypeRegistry::propagateSPIRVType(MachineInstr *MI,
                                                 MachineRegisterInfo &MRI,
                                                 MachineIRBuilder &MIB) {
  SPIRVType *SpirvTy = nullptr;
  assert(MI && "Machine instr is expected");
  if (MI->getOperand(0).isReg()) {
    auto Reg = MI->getOperand(0).getReg();
    SpirvTy = getSPIRVTypeForVReg(Reg);
    if (!SpirvTy) {
      switch (MI->getOpcode()) {
      case TargetOpcode::G_CONSTANT: {
        MIB.setInsertPt(*MI->getParent(), MI);
        Type *Ty = MI->getOperand(1).getCImm()->getType();
        SpirvTy = getOrCreateSPIRVType(Ty, MIB);
        break;
      }
      case TargetOpcode::G_GLOBAL_VALUE: {
        MIB.setInsertPt(*MI->getParent(), MI);
        Type *Ty = MI->getOperand(1).getGlobal()->getType();
        SpirvTy = getOrCreateSPIRVType(Ty, MIB);
        break;
      }
      case TargetOpcode::G_TRUNC:
      case TargetOpcode::G_ADDRSPACE_CAST:
      case TargetOpcode::COPY: {
        auto Op = MI->getOperand(1);
        auto *Def = Op.isReg() ? MRI.getVRegDef(Op.getReg()) : nullptr;
        if (Def)
          SpirvTy = propagateSPIRVType(Def, MRI, MIB);
        break;
      }
      default:
        break;
      }
      if (SpirvTy)
        assignSPIRVTypeToVReg(SpirvTy, Reg, MIB);
      if (!MRI.getRegClassOrNull(Reg))
        MRI.setRegClass(Reg, &SPIRV::IDRegClass);
    }
  }
  return SpirvTy;
}

Register SPIRVTypeRegistry::insertAssignInstr(Register Reg, Type *Ty,
                                              SPIRVType *SpirvTy,
                                              MachineIRBuilder &MIB,
                                              MachineRegisterInfo &MRI) {
  auto *Def = MRI.getVRegDef(Reg);
  assert((Ty || SpirvTy) && "Either LLVM or SPIRV type is expected.");
  MIB.setInsertPt(*Def->getParent(),
                  (Def->getNextNode() ? Def->getNextNode()->getIterator()
                                      : Def->getParent()->end()));
  auto NewReg = MRI.createGenericVirtualRegister(MRI.getType(Reg));
  if (auto *RC = MRI.getRegClassOrNull(Reg))
    MRI.setRegClass(NewReg, RC);
  SpirvTy = SpirvTy ? SpirvTy : getOrCreateSPIRVType(Ty, MIB);
  assignSPIRVTypeToVReg(SpirvTy, Reg, MIB);
  // This is to make it convenient for Legalizer to get the SPIRVType
  // when processing the actual MI (i.e. not pseudo one).
  assignSPIRVTypeToVReg(SpirvTy, NewReg, MIB);
  auto NewMI = MIB.buildInstr(SPIRV::ASSIGN_TYPE)
                   .addDef(Reg)
                   .addUse(NewReg)
                   .addUse(getSPIRVTypeID(SpirvTy));
  Def->getOperand(0).setReg(NewReg);
  constrainRegOperands(NewMI, &MIB.getMF());
  return NewReg;
}

void SPIRVTypeRegistry::generateAssignInstrs(MachineFunction &MF) {
  MachineIRBuilder MIB(MF);
  MachineRegisterInfo &MRI = MF.getRegInfo();
  std::vector<MachineInstr *> ToDelete;

  for (MachineBasicBlock *MBB : post_order(&MF)) {
    if (MBB->empty())
      continue;

    bool ReachedBegin = false;
    for (auto MII = std::prev(MBB->end()), Begin = MBB->begin();
         !ReachedBegin;) {
      MachineInstr &MI = *MII;

      if (MI.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
          MI.getOperand(0).isIntrinsicID() &&
          MI.getOperand(0).getIntrinsicID() == Intrinsic::spv_assign_type) {
        auto Reg = MI.getOperand(1).getReg();
        auto *Ty =
            cast<ValueAsMetadata>(MI.getOperand(2).getMetadata()->getOperand(0))
                ->getType();
        auto *Def = MRI.getVRegDef(Reg);
        assert(Def && "Expecting an instruction that defines the register");
        // G_GLOBAL_VALUE already has type info.
        if (Def->getOpcode() != TargetOpcode::G_GLOBAL_VALUE)
          insertAssignInstr(Reg, Ty, nullptr, MIB, MF.getRegInfo());
        ToDelete.push_back(&MI);
      } else if (MI.getOpcode() == TargetOpcode::G_CONSTANT ||
                 MI.getOpcode() == TargetOpcode::G_FCONSTANT ||
                 MI.getOpcode() == TargetOpcode::G_BUILD_VECTOR) {
        // %rc = G_CONSTANT ty Val
        // ===>
        // %cty = OpType* ty
        // %rctmp = G_CONSTANT ty Val
        // %rc = ASSIGN_TYPE %rctmp, %cty
        auto Reg = MI.getOperand(0).getReg();
        if (MRI.hasOneUse(Reg) &&
            MRI.use_instr_begin(Reg)->getOpcode() ==
                TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS &&
            (MRI.use_instr_begin(Reg)->getIntrinsicID() ==
                 Intrinsic::spv_assign_type ||
             MRI.use_instr_begin(Reg)->getIntrinsicID() ==
                 Intrinsic::spv_assign_name))
          continue;

        auto &MRI = MF.getRegInfo();
        Type *Ty = nullptr;
        if (MI.getOpcode() == TargetOpcode::G_CONSTANT)
          Ty = MI.getOperand(1).getCImm()->getType();
        else if (MI.getOpcode() == TargetOpcode::G_FCONSTANT)
          Ty = MI.getOperand(1).getFPImm()->getType();
        else {
          assert(MI.getOpcode() == TargetOpcode::G_BUILD_VECTOR);
          Type *ElemTy = nullptr;
          auto *ElemMI = MRI.getVRegDef(MI.getOperand(1).getReg());
          assert(ElemMI);

          if (ElemMI->getOpcode() == TargetOpcode::G_CONSTANT)
            ElemTy = ElemMI->getOperand(1).getCImm()->getType();
          else if (ElemMI->getOpcode() == TargetOpcode::G_FCONSTANT)
            ElemTy = ElemMI->getOperand(1).getFPImm()->getType();
          else
            assert(0);
          Ty = VectorType::get(
              ElemTy, MI.getNumExplicitOperands() - MI.getNumExplicitDefs(),
              false);
        }
        insertAssignInstr(Reg, Ty, nullptr, MIB, MRI);
      } else if (MI.getOpcode() == TargetOpcode::G_TRUNC ||
                 MI.getOpcode() == TargetOpcode::G_GLOBAL_VALUE ||
                 MI.getOpcode() == TargetOpcode::COPY ||
                 MI.getOpcode() == TargetOpcode::G_ADDRSPACE_CAST) {
        propagateSPIRVType(&MI, MRI, MIB);
      }

      if (MII == Begin)
        ReachedBegin = true;
      else
        --MII;
    }
  }

  for (auto &MI : ToDelete)
    MI->eraseFromParent();
}

SPIRVType *SPIRVTypeRegistry::assignTypeToVReg(const Type *Type, Register VReg,
                                               MachineIRBuilder &MIRBuilder,
                                               AQ::AccessQualifier AccessQual) {

  SPIRVType *SpirvType = getOrCreateSPIRVType(Type, MIRBuilder, AccessQual);
  assignSPIRVTypeToVReg(SpirvType, VReg, MIRBuilder);
  return SpirvType;
}

void SPIRVTypeRegistry::assignSPIRVTypeToVReg(SPIRVType *SpirvType,
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

SPIRVType *SPIRVTypeRegistry::getOpTypeBool(MachineIRBuilder &MIRBuilder) {
  return MIRBuilder.buildInstr(SPIRV::OpTypeBool)
      .addDef(createTypeVReg(MIRBuilder));
}

SPIRVType *SPIRVTypeRegistry::getOpTypeInt(uint32_t Width,
                                           MachineIRBuilder &MIRBuilder,
                                           bool IsSigned) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeInt)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(Width)
                 .addImm(IsSigned ? 1 : 0);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeFloat(uint32_t Width,
                                             MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFloat)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(Width);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeVoid(MachineIRBuilder &MIRBuilder) {
  return MIRBuilder.buildInstr(SPIRV::OpTypeVoid)
      .addDef(createTypeVReg(MIRBuilder));
}

SPIRVType *SPIRVTypeRegistry::getOpTypeVector(uint32_t NumElems,
                                              SPIRVType *ElemType,
                                              MachineIRBuilder &MIRBuilder) {
  using namespace SPIRV;
  auto EleOpc = ElemType->getOpcode();
  if (EleOpc != OpTypeInt && EleOpc != OpTypeFloat && EleOpc != OpTypeBool)
    report_fatal_error("Invalid vector element type");

  auto MIB = MIRBuilder.buildInstr(OpTypeVector)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(ElemType))
                 .addImm(NumElems);
  return MIB;
}

Register SPIRVTypeRegistry::buildConstantInt(uint64_t Val,
                                             MachineIRBuilder &MIRBuilder,
                                             SPIRVType *SpvType, bool EmitIR) {
  auto &MF = MIRBuilder.getMF();
  Register Res;
  IntegerType *LLVMIntTy;
  if (SpvType) {
    Type *LLVMTy = const_cast<Type *>(getTypeForSPIRVType(SpvType));
    assert(LLVMTy->isIntegerTy());
    LLVMIntTy = cast<IntegerType>(LLVMTy);
  } else {
    LLVMIntTy = IntegerType::getInt32Ty(MF.getFunction().getContext());
  }
  // Find a constant in DT or build a new one.
  const auto ConstInt = ConstantInt::get(LLVMIntTy, Val);
  if (DT.find(ConstInt, &MIRBuilder.getMF(), Res) == false) {
    unsigned BitWidth = SpvType ? getScalarOrVectorBitWidth(SpvType) : 32;
    Res = MF.getRegInfo().createGenericVirtualRegister(LLT::scalar(BitWidth));
    assignTypeToVReg(LLVMIntTy, Res, MIRBuilder);
    DT.add(ConstInt, &MIRBuilder.getMF(), Res);
    if (EmitIR)
      MIRBuilder.buildConstant(Res, *ConstInt);
    else
      MIRBuilder.buildInstr(SPIRV::OpConstantI)
          .addDef(Res)
          .addImm(ConstInt->getSExtValue());
  }
  return Res;
}

Register SPIRVTypeRegistry::buildConstantFP(APFloat Val,
                                            MachineIRBuilder &MIRBuilder,
                                            SPIRVType *SpvType) {
  auto &MF = MIRBuilder.getMF();
  Register Res;
  Type *LLVMFPTy;
  if (SpvType) {
    Type *LLVMTy = const_cast<Type *>(getTypeForSPIRVType(SpvType));
    assert(LLVMTy->isFloatingPointTy());
    LLVMFPTy = LLVMTy;
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

Register SPIRVTypeRegistry::buildConstantIntVector(uint64_t Val,
                                                   MachineIRBuilder &MIRBuilder,
                                                   SPIRVType *SpvType) {
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
    unsigned BitWidth = getScalarOrVectorBitWidth(SpvType);
    LLT LLTy = LLT::vector(LLVMVecTy->getElementCount(), BitWidth);
    Register SpvVecConst =
        MIRBuilder.getMF().getRegInfo().createGenericVirtualRegister(LLTy);
    assignTypeToVReg(LLVMVecTy, SpvVecConst, MIRBuilder);
    DT.add(ConstVec, &MIRBuilder.getMF(), SpvVecConst);
    SPIRVType *SpvBaseType = getOrCreateSPIRVType(LLVMBaseTy, MIRBuilder);
    auto SpvScalConst = buildConstantInt(Val, MIRBuilder, SpvBaseType);
    MIRBuilder.buildSplatVector(SpvVecConst, SpvScalConst);
    return SpvVecConst;
  }
  return Res;
}

Register SPIRVTypeRegistry::buildConstantSampler(
    Register ResReg, unsigned int AddrMode, unsigned int Param,
    unsigned int FilerMode, MachineIRBuilder &MIRBuilder, SPIRVType *SpvType) {
  SPIRVType *SampTy =
      getOrCreateSPIRVType(getTypeForSPIRVType(SpvType), MIRBuilder);
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

Register SPIRVTypeRegistry::buildGlobalVariable(
    Register ResVReg, SPIRVType *BaseType, StringRef Name,
    const GlobalValue *GV, StorageClass::StorageClass Storage,
    const MachineInstr *Init, bool IsConst, bool HasLinkageTy,
    LinkageType::LinkageType LinkageType, MachineIRBuilder &MIRBuilder) {
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
  constrainRegOperands(MIB, CurMF);
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
                    {LinkageType}, Name);

  BuiltIn::BuiltIn BuiltInId;
  if (getSpirvBuilInIdByName(Name, BuiltInId))
    buildOpDecorate(Reg, MIRBuilder, Decoration::BuiltIn, {BuiltInId});

  return Reg;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeArray(uint32_t NumElems,
                                             SPIRVType *ElemType,
                                             MachineIRBuilder &MIRBuilder,
                                             bool EmitIR) {
  if (ElemType->getOpcode() == SPIRV::OpTypeVoid)
    report_fatal_error("Invalid array element type");
  Register NumElementsVReg =
      buildConstantInt(NumElems, MIRBuilder, nullptr, EmitIR);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeArray)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(ElemType))
                 .addUse(NumElementsVReg);
  assert(constrainRegOperands(MIB, CurMF));
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeOpaque(const StructType *Ty,
                                              MachineIRBuilder &MIRBuilder) {
  assert(Ty->hasName());
  const StringRef Name = Ty->hasName() ? Ty->getName() : "";
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeOpaque).addDef(ResVReg);
  addStringImm(Name, MIB);
  buildOpName(ResVReg, Name, MIRBuilder);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeStruct(const StructType *Ty,
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

SPIRVType *SPIRVTypeRegistry::handleOpenCLBuiltin(const StructType *Ty,
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

SPIRVType *SPIRVTypeRegistry::getOpTypePointer(StorageClass::StorageClass SC,
                                               SPIRVType *ElemType,
                                               MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePointer)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(SC)
                 .addUse(getSPIRVTypeID(ElemType));
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeFunction(
    SPIRVType *RetType, const SmallVectorImpl<SPIRVType *> &ArgTypes,
    MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFunction)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(RetType));
  for (const auto &ArgType : ArgTypes)
    MIB.addUse(getSPIRVTypeID(ArgType));
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::createSPIRVType(const Type *Ty,
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
  } else if (Ty->isFloatingPointTy())
    return getOpTypeFloat(Ty->getPrimitiveSizeInBits(), MIRBuilder);
  else if (Ty->isVoidTy())
    return getOpTypeVoid(MIRBuilder);
  else if (Ty->isVectorTy()) {
    auto El = getOrCreateSPIRVType(cast<FixedVectorType>(Ty)->getElementType(),
                                   MIRBuilder);
    return getOpTypeVector(cast<FixedVectorType>(Ty)->getNumElements(), El,
                           MIRBuilder);
  } else if (Ty->isArrayTy()) {
    auto *El = getOrCreateSPIRVType(Ty->getArrayElementType(), MIRBuilder);
    return getOpTypeArray(Ty->getArrayNumElements(), El, MIRBuilder, EmitIR);
  } else if (auto SType = dyn_cast<StructType>(Ty)) {
    if (isOpenCLBuiltinType(SType))
      return handleOpenCLBuiltin(SType, MIRBuilder, AccQual);
    else if (isSPIRVBuiltinType(SType))
      return handleSPIRVBuiltin(SType, MIRBuilder, AccQual);
    else if (SType->isOpaque())
      return getOpTypeOpaque(SType, MIRBuilder);
    else
      return getOpTypeStruct(SType, MIRBuilder, EmitIR);
  } else if (auto FType = dyn_cast<FunctionType>(Ty)) {
    SPIRVType *RetTy = getOrCreateSPIRVType(FType->getReturnType(), MIRBuilder);
    SmallVector<SPIRVType *, 4> ParamTypes;
    for (const auto &t : FType->params()) {
      ParamTypes.push_back(getOrCreateSPIRVType(t, MIRBuilder));
    }
    return getOpTypeFunction(RetTy, ParamTypes, MIRBuilder);
  } else if (const auto &PType = dyn_cast<PointerType>(Ty)) {
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
  } else
    llvm_unreachable("Unable to convert LLVM type to SPIRVType");
}

SPIRVType *SPIRVTypeRegistry::getSPIRVTypeForVReg(Register VReg) const {
  auto t = VRegToTypeMap.find(CurMF);
  if (t != VRegToTypeMap.end()) {
    auto tt = t->second.find(VReg);
    if (tt != t->second.end())
      return tt->second;
  }
  return nullptr;
}

SPIRVType *SPIRVTypeRegistry::getOrCreateSPIRVType(
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

bool SPIRVTypeRegistry::isScalarOfType(Register VReg,
                                       unsigned int TypeOpcode) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  assert(Type && "isScalarOfType VReg has no type assigned");
  return Type->getOpcode() == TypeOpcode;
}

bool SPIRVTypeRegistry::isScalarOrVectorOfType(Register VReg,
                                               unsigned int TypeOpcode) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  assert(Type && "isScalarOrVectorOfType VReg has no type assigned");
  if (Type->getOpcode() == TypeOpcode) {
    return true;
  } else if (Type->getOpcode() == SPIRV::OpTypeVector) {
    Register ScalarTypeVReg = Type->getOperand(1).getReg();
    SPIRVType *ScalarType = getSPIRVTypeForVReg(ScalarTypeVReg);
    return ScalarType->getOpcode() == TypeOpcode;
  }
  return false;
}

unsigned
SPIRVTypeRegistry::getScalarOrVectorBitWidth(const SPIRVType *Type) const {
  if (Type && Type->getOpcode() == SPIRV::OpTypeVector) {
    auto EleTypeReg = Type->getOperand(1).getReg();
    Type = getSPIRVTypeForVReg(EleTypeReg);
  }
  if (Type && (Type->getOpcode() == SPIRV::OpTypeInt ||
               Type->getOpcode() == SPIRV::OpTypeFloat)) {
    return Type->getOperand(1).getImm();
  } else if (Type && Type->getOpcode() == SPIRV::OpTypeBool) {
    return 1;
  }
  llvm_unreachable("Attempting to get bit width of non-integer/float type.");
}

bool SPIRVTypeRegistry::isScalarOrVectorSigned(const SPIRVType *Type) const {
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
SPIRVTypeRegistry::getPointerStorageClass(Register VReg) const {
  SPIRVType *Type = getSPIRVTypeForVReg(VReg);
  if (Type && Type->getOpcode() == SPIRV::OpTypePointer) {
    auto scOp = Type->getOperand(1).getImm();
    return static_cast<StorageClass::StorageClass>(scOp);
  }
  llvm_unreachable("Attempting to get storage class of non-pointer type.");
}

SPIRVType *SPIRVTypeRegistry::getOpTypeImage(
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

SPIRVType *SPIRVTypeRegistry::getSamplerType(MachineIRBuilder &MIRBuilder) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampler).addDef(ResVReg);
  constrainRegOperands(MIB);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypePipe(MachineIRBuilder &MIRBuilder,
                                            AQ::AccessQualifier AccessQual) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePipe)
                 .addDef(ResVReg)
                 .addImm(AccessQual);
  constrainRegOperands(MIB);
  return MIB;
}

SPIRVType *
SPIRVTypeRegistry::getSampledImageType(SPIRVType *ImageType,
                                       MachineIRBuilder &MIRBuilder) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampledImage)
                 .addDef(ResVReg)
                 .addUse(getSPIRVTypeID(ImageType));
  constrainRegOperands(MIB);
  return checkSpecialInstrMap(MIB, SpecialTypesAndConstsMap);
}

SPIRVType *SPIRVTypeRegistry::getOpTypeByOpcode(MachineIRBuilder &MIRBuilder,
                                                unsigned int Opcode) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(Opcode).addDef(ResVReg);
  constrainRegOperands(MIB);
  return MIB;
}

const MachineInstr *
SPIRVTypeRegistry::checkSpecialInstrMap(const MachineInstr *NewInstr,
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
  auto *NewInstrGroup = new SPIRVInstrGroup;
  (*NewInstrGroup)[const_cast<MachineFunction *>(NewInstr->getMF())] = NewInstr;
  InstrMap[NewInstr->getOpcode()].push_back(*NewInstrGroup);
  return NewInstr;
}

SPIRVType *
SPIRVTypeRegistry::generateOpenCLOpaqueType(const StructType *Ty,
                                            MachineIRBuilder &MIRBuilder,
                                            AQ::AccessQualifier AccessQual) {
  const StringRef Name = Ty->getName();
  assert(Name.startswith("opencl.") && "CL types should start with 'opencl.'");
  using namespace Dim;
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
      auto Dim = DimC == '1' ? DIM_1D : DimC == '2' ? DIM_2D : DIM_3D;
      unsigned int Arrayed = 0;
      if (TypeName.contains("buffer"))
        Dim = DIM_Buffer;
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
SPIRVTypeRegistry::getOrCreateSPIRVTypeByName(StringRef TypeStr,
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

SPIRVType *SPIRVTypeRegistry::handleSPIRVBuiltin(const StructType *Ty,
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
SPIRVTypeRegistry::generateSPIRVOpaqueType(const StructType *Ty,
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
SPIRVTypeRegistry::getOrCreateSPIRVIntegerType(unsigned BitWidth,
                                               MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      IntegerType::get(MIRBuilder.getMF().getFunction().getContext(), BitWidth),
      MIRBuilder);
}

SPIRVType *
SPIRVTypeRegistry::getOrCreateSPIRVBoolType(MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      IntegerType::get(MIRBuilder.getMF().getFunction().getContext(), 1),
      MIRBuilder);
}

SPIRVType *SPIRVTypeRegistry::getOrCreateSPIRVVectorType(
    SPIRVType *BaseType, unsigned NumElements, MachineIRBuilder &MIRBuilder) {
  return getOrCreateSPIRVType(
      FixedVectorType::get(const_cast<Type *>(getTypeForSPIRVType(BaseType)),
                           NumElements),
      MIRBuilder);
}

SPIRVType *SPIRVTypeRegistry::getOrCreateSPIRVPointerType(
    SPIRVType *BaseType, MachineIRBuilder &MIRBuilder,
    StorageClass::StorageClass SClass) {
  return getOrCreateSPIRVType(
      PointerType::get(const_cast<Type *>(getTypeForSPIRVType(BaseType)),
                       StorageClassToAddressSpace(SClass)),
      MIRBuilder);
}

SPIRVType *SPIRVTypeRegistry::getOrCreateSPIRVSampledImageType(
    SPIRVType *ImageType, MachineIRBuilder &MIRBuilder) {
  SPIRVType *Type = getSampledImageType(ImageType, MIRBuilder);
  return Type;
}

unsigned int
SPIRVTypeRegistry::StorageClassToAddressSpace(StorageClass::StorageClass Sc) {
  // TODO maybe this should be handled in the subtarget to allow for different
  // OpenCL vs Vulkan handling?
  switch (Sc) {
  case StorageClass::Function:
    return 0;
  case StorageClass::CrossWorkgroup:
    return 1;
  case StorageClass::UniformConstant:
    return 2;
  case StorageClass::Workgroup:
    return 3;
  case StorageClass::Generic:
    return 4;
  case StorageClass::Input:
    return 7;
  default:
    llvm_unreachable("Unable to get address space id");
  }
}

StorageClass::StorageClass
SPIRVTypeRegistry::addressSpaceToStorageClass(unsigned int AddressSpace) {
  // TODO maybe this should be handled in the subtarget to allow for different
  // OpenCL vs Vulkan handling?
  switch (AddressSpace) {
  case 0:
    return StorageClass::Function;
  case 1:
    return StorageClass::CrossWorkgroup;
  case 2:
    return StorageClass::UniformConstant;
  case 3:
    return StorageClass::Workgroup;
  case 4:
    return StorageClass::Generic;
  case 7:
    return StorageClass::Input;
  default:
    llvm_unreachable("Unknown address space");
  }
}

bool SPIRVTypeRegistry::constrainRegOperands(MachineInstrBuilder &MIB,
                                             MachineFunction *MF) const {
  // A utility method to avoid having these TII, TRI, RBI lines everywhere
  if (!MF)
    MF = MIB->getMF();
  const auto &Subtarget = MF->getSubtarget();
  const TargetInstrInfo *TII = Subtarget.getInstrInfo();
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  const RegisterBankInfo *RBI = Subtarget.getRegBankInfo();

  return constrainSelectedInstRegOperands(*MIB, *TII, *TRI, *RBI);
}
