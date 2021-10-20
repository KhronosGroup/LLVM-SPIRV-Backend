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

#include "SPIRVTypeRegistry.h"

#include "SPIRV.h"
#include "SPIRVTargetMachine.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/IR/DerivedTypes.h"

#include "SPIRVEnums.h"
#include "SPIRVStrings.h"
#include "SPIRVSubtarget.h"

#include "SPIRVOpenCLBIFs.h"

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
    TargetOpcode::G_INSERT_VECTOR_ELT
};

const std::unordered_set<unsigned>& getTypeFoldingSupportingOpcs() {
  return TypeFoldingSupportingOpcs;
}

bool isTypeFoldingSupported(unsigned Opcode) {
  return TypeFoldingSupportingOpcs.count(Opcode) > 0;
}

SPIRVTypeRegistry::SPIRVTypeRegistry(SPIRVGeneralDuplicatesTracker &DT,
                                     unsigned int pointerSize)
    : DT(DT), pointerSize(pointerSize) {}

void SPIRVTypeRegistry::generateAssignInstrs(MachineFunction &MF) {
  MachineIRBuilder MIB(MF);
  MachineRegisterInfo &MRI = MF.getRegInfo();
  std::vector<std::pair<Register, Register>> PostProcessList;
  for (auto P : VRegToTypeMap[&MF]) {
    auto Reg = P.first;
    if (MRI.getRegClassOrNull(Reg) == &SPIRV::TYPERegClass)
      continue;
    // MRI.setType(Reg, LLT::scalar(32));
    auto *Ty = P.second;
    auto *Def = MRI.getVRegDef(Reg);

    MIB.setInsertPt(*Def->getParent(),
                    (Def->getNextNode() ? Def->getNextNode()->getIterator()
                                        : Def->getParent()->end()));
    auto &MRI = MF.getRegInfo();
    auto NewReg = MRI.createGenericVirtualRegister(MRI.getType(Reg));
    PostProcessList.push_back({Reg, NewReg});
    if (auto *RC = MRI.getRegClassOrNull(Reg))
      MRI.setRegClass(NewReg, RC);
    auto MI = MIB.buildInstr(SPIRV::ASSIGN_TYPE)
                  .addDef(Reg)
                  .addUse(NewReg)
                  .addUse(getSPIRVTypeID(Ty));
    Def->getOperand(0).setReg(NewReg);
    constrainRegOperands(MI, &MF);
  }
  // this to make it convenient for Legalizer to get the SPIRVType
  // when processing the actual MI (ie not pseudo one)
  for (auto &P : PostProcessList)
    assignSPIRVTypeToVReg(getSPIRVTypeForVReg(P.first), P.second, MIB);
}

SPIRVType *SPIRVTypeRegistry::assignTypeToVReg(const Type *type, Register VReg,
                                              MachineIRBuilder &MIRBuilder,
                                              AQ::AccessQualifier accessQual) {

  SPIRVType *spirvType = getOrCreateSPIRVType(type, MIRBuilder, accessQual);
  assignSPIRVTypeToVReg(spirvType, VReg, MIRBuilder);
  return spirvType;
}

void SPIRVTypeRegistry::assignSPIRVTypeToVReg(SPIRVType *spirvType,
                                              Register VReg,
                                              MachineIRBuilder &MIRBuilder) {
  VRegToTypeMap[&MIRBuilder.getMF()][VReg] = spirvType;
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

SPIRVType *SPIRVTypeRegistry::getOpTypeInt(uint32_t width,
                                           MachineIRBuilder &MIRBuilder,
                                           bool isSigned) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeInt)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(width)
                 .addImm(isSigned ? 1 : 0);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeFloat(uint32_t width,
                                             MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFloat)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(width);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeVoid(MachineIRBuilder &MIRBuilder) {
  return MIRBuilder.buildInstr(SPIRV::OpTypeVoid)
      .addDef(createTypeVReg(MIRBuilder));
}

SPIRVType *SPIRVTypeRegistry::getOpTypeVector(uint32_t numElems,
                                             SPIRVType *elemType,
                                             MachineIRBuilder &MIRBuilder) {
  using namespace SPIRV;
  auto eleOpc = elemType->getOpcode();
  if (eleOpc != OpTypeInt && eleOpc != OpTypeFloat && eleOpc != OpTypeBool) {
    errs() << *elemType;
    report_fatal_error("Invalid vector element type");
  }
  auto MIB = MIRBuilder.buildInstr(OpTypeVector)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(elemType))
                 .addImm(numElems);
  return MIB;
}

Register
SPIRVTypeRegistry::buildConstantInt(uint64_t val, MachineIRBuilder &MIRBuilder,
                                    SPIRVType *spvType) {
  auto &MF = MIRBuilder.getMF();
  Register res;
  IntegerType *LLVMIntTy;
  if (spvType) {
    Type* LLVMTy = const_cast<Type*>(getTypeForSPIRVType(spvType));
    assert(LLVMTy->isIntegerTy());
    LLVMIntTy = cast<IntegerType>(LLVMTy);
  } else {
    LLVMIntTy = IntegerType::getInt32Ty(MF.getFunction().getContext());
  }
  // Find a constant in DT or build a new one.
  const auto ConstInt = ConstantInt::get(LLVMIntTy, val);
  if (DT.find(ConstInt, &MIRBuilder.getMF(), res) == false) {
    unsigned BitWidth = spvType ? getScalarOrVectorBitWidth(spvType) : 32;
    res = MF.getRegInfo().createGenericVirtualRegister(LLT::scalar(BitWidth));
    assignTypeToVReg(LLVMIntTy, res, MIRBuilder);
    DT.add(ConstInt, &MIRBuilder.getMF(), res);
    MIRBuilder.buildConstant(res, *ConstInt);
  }
  return res;
}

Register
SPIRVTypeRegistry::buildConstantFP(APFloat val, MachineIRBuilder &MIRBuilder,
                                   SPIRVType *spvType) {
  auto &MF = MIRBuilder.getMF();
  Register res;
  Type *LLVMFPTy;
  if (spvType) {
    Type* LLVMTy = const_cast<Type*>(getTypeForSPIRVType(spvType));
    assert(LLVMTy->isFloatingPointTy());
    LLVMFPTy = LLVMTy;
  } else {
    LLVMFPTy = IntegerType::getFloatTy(MF.getFunction().getContext());
  }
  // Find a constant in DT or build a new one.
  const auto ConstFP = ConstantFP::get(LLVMFPTy->getContext(), val);
  if (DT.find(ConstFP, &MIRBuilder.getMF(), res) == false) {
    unsigned BitWidth = spvType ? getScalarOrVectorBitWidth(spvType) : 32;
    res = MF.getRegInfo().createGenericVirtualRegister(LLT::scalar(BitWidth));
    assignTypeToVReg(LLVMFPTy, res, MIRBuilder);
    DT.add(ConstFP, &MIRBuilder.getMF(), res);
    MIRBuilder.buildFConstant(res, *ConstFP);
  }
  return res;
}

Register
SPIRVTypeRegistry::buildConstantSampler(Register resReg, unsigned int addrMode,
    unsigned int param, unsigned int filerMode, MachineIRBuilder &MIRBuilder,
    SPIRVType *spvType) {
  SPIRVType *sampTy =
      getOrCreateSPIRVType(getTypeForSPIRVType(spvType), MIRBuilder);
  auto sampler = resReg.isValid() ? resReg :
      MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantSampler)
                 .addDef(sampler)
                 .addUse(getSPIRVTypeID(sampTy))
                 .addImm(addrMode)
                 .addImm(param)
                 .addImm(filerMode);
  constrainRegOperands(MIB);
  auto res = checkSpecialInstrMap(MIB, SpecialTypesAndConstsMap);
  assert(res->getOperand(0).isReg());
  return res->getOperand(0).getReg();
}

SPIRVType *SPIRVTypeRegistry::getOpTypeArray(uint32_t numElems,
                                             SPIRVType *elemType,
                                             MachineIRBuilder &MIRBuilder) {
  if (elemType->getOpcode() == SPIRV::OpTypeVoid) {
    errs() << *elemType;
    report_fatal_error("Invalid array element type");
  }
  Register numElementsVReg = buildConstantInt(numElems, MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeArray)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(elemType))
                 .addUse(numElementsVReg);
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

SPIRVType *
SPIRVTypeRegistry::getOpTypeStruct(const StructType *Ty,
                                   MachineIRBuilder &MIRBuilder) {
  Register ResVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeStruct).addDef(ResVReg);
  for (const auto &Elem : Ty->elements()) {
    auto ElemTy = getOrCreateSPIRVType(Elem, MIRBuilder);
    if (ElemTy->getOpcode() == SPIRV::OpTypeVoid) {
      errs() << ElemTy;
      report_fatal_error("Invalid struct element type");
    }
    MIB.addUse(getSPIRVTypeID(ElemTy));
  }
  if (Ty->hasName())
    buildOpName(ResVReg, Ty->getName(), MIRBuilder);
  return MIB;
}

static bool isOpenCLBuiltinType(const StructType *stype) {
  return stype->isOpaque() && stype->hasName() &&
         stype->getName().startswith("opencl.");
}

static bool isSPIRVBuiltinType(const StructType *stype) {
  return stype->isOpaque() && stype->hasName() &&
         stype->getName().startswith("spirv.");
}

SPIRVType *SPIRVTypeRegistry::handleOpenCLBuiltin(const StructType *Ty,
                                                  MachineIRBuilder &MIRBuilder,
                                                  AQ::AccessQualifier aq) {
  assert(Ty->hasName());
  unsigned NumStartingVRegs = MIRBuilder.getMRI()->getNumVirtRegs();
  auto newTy = generateOpenCLOpaqueType(Ty, MIRBuilder, aq);
  auto resTy = checkSpecialInstrMap(newTy, BuiltinTypeMap);
  if (NumStartingVRegs < MIRBuilder.getMRI()->getNumVirtRegs() &&
      resTy == newTy)
    buildOpName(getSPIRVTypeID(resTy), Ty->getName(), MIRBuilder);
  return resTy;
}

SPIRVType *SPIRVTypeRegistry::getOpTypePointer(StorageClass::StorageClass sc,
                                               SPIRVType *elemType,
                                               MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePointer)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(sc)
                 .addUse(getSPIRVTypeID(elemType));
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeFunction(
    SPIRVType *retType, const SmallVectorImpl<SPIRVType *> &argTypes,
    MachineIRBuilder &MIRBuilder) {
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFunction)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(retType));
  for (const auto &argType : argTypes)
    MIB.addUse(getSPIRVTypeID(argType));
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::createSPIRVType(const Type *Ty,
                                              MachineIRBuilder &MIRBuilder,
                                              AQ::AccessQualifier aq) {
  auto &TypeToSPIRVTypeMap = DT.get<Type>()->getAllUses();
  auto t = TypeToSPIRVTypeMap.find(Ty);
  if (t != TypeToSPIRVTypeMap.end()) {
    auto tt = t->second.find(&MIRBuilder.getMF());
    if (tt != t->second.end())
      return getSPIRVTypeForVReg(tt->second);
  }

  if (auto itype = dyn_cast<IntegerType>(Ty)) {
    const unsigned int width = itype->getBitWidth();
    return width == 1 ? getOpTypeBool(MIRBuilder)
                      : getOpTypeInt(width, MIRBuilder, false);
  } else if (Ty->isFloatingPointTy())
    return getOpTypeFloat(Ty->getPrimitiveSizeInBits(), MIRBuilder);
  else if (Ty->isVoidTy())
    return getOpTypeVoid(MIRBuilder);
  else if (Ty->isVectorTy()) {
    auto el = getOrCreateSPIRVType(cast<FixedVectorType>(Ty)->getElementType(),
                                   MIRBuilder);
    return getOpTypeVector(cast<FixedVectorType>(Ty)->getNumElements(), el,
                           MIRBuilder);
  } else if (Ty->isArrayTy()) {
    auto *el = getOrCreateSPIRVType(Ty->getArrayElementType(), MIRBuilder);
    return getOpTypeArray(Ty->getArrayNumElements(), el, MIRBuilder);
  } else if (auto stype = dyn_cast<StructType>(Ty)) {
    if (isOpenCLBuiltinType(stype))
      return handleOpenCLBuiltin(stype, MIRBuilder, aq);
    else if (isSPIRVBuiltinType(stype))
      return handleSPIRVBuiltin(stype, MIRBuilder, aq);
    else if (stype->isOpaque())
      return getOpTypeOpaque(stype, MIRBuilder);
    else
      return getOpTypeStruct(stype, MIRBuilder);
  } else if (auto ftype = dyn_cast<FunctionType>(Ty)) {
    SPIRVType *retTy = getOrCreateSPIRVType(ftype->getReturnType(), MIRBuilder);
    SmallVector<SPIRVType *, 4> paramTypes;
    for (const auto &t : ftype->params()) {
      paramTypes.push_back(getOrCreateSPIRVType(t, MIRBuilder));
    }
    return getOpTypeFunction(retTy, paramTypes, MIRBuilder);
  } else if (const auto &ptype = dyn_cast<PointerType>(Ty)) {
    Type *elemType = ptype->getPointerElementType();

    // Some OpenCL and SPIRV builtins like image2d_t are passed in as pointers,
    // but should be treated as custom types like OpTypeImage
    if (auto stype = dyn_cast<StructType>(elemType)) {
      if (isOpenCLBuiltinType(stype))
        return handleOpenCLBuiltin(stype, MIRBuilder, aq);
      if (isSPIRVBuiltinType(stype))
        return handleSPIRVBuiltin(stype, MIRBuilder, aq);
    }

    // Otherwise, treat it as a regular pointer type
    auto sc = addressSpaceToStorageClass(ptype->getAddressSpace());
    SPIRVType *spvElementType = getOrCreateSPIRVType(elemType, MIRBuilder);
    return getOpTypePointer(sc, spvElementType, MIRBuilder);
  } else {
    errs() << "Converting type: " << Ty << "\n";
    llvm_unreachable("Unable to convert LLVM type to SPIRVType");
  }
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

SPIRVType *
SPIRVTypeRegistry::getOrCreateSPIRVType(const Type *type,
                                        MachineIRBuilder &MIRBuilder,
                                        AQ::AccessQualifier accessQual) {
  Register reg;
  if (DT.find(type, &MIRBuilder.getMF(), reg)) {
    return getSPIRVTypeForVReg(reg);
  }
  SPIRVType *spirvType = createSPIRVType(type, MIRBuilder, accessQual);
  VRegToTypeMap[&MIRBuilder.getMF()][getSPIRVTypeID(spirvType)] = spirvType;
  SPIRVToLLVMType[spirvType] = type;
  DT.add(type, &MIRBuilder.getMF(), getSPIRVTypeID(spirvType));
  return spirvType;
}

bool SPIRVTypeRegistry::isScalarOfType(Register vreg, unsigned int typeOpcode) const {
  SPIRVType *type = getSPIRVTypeForVReg(vreg);
  assert(type && "isScalarOfType vreg has no type assigned");
  return type->getOpcode() == typeOpcode;
}

bool SPIRVTypeRegistry::isScalarOrVectorOfType(Register vreg,
                                               unsigned int typeOpcode) const {
  SPIRVType *type = getSPIRVTypeForVReg(vreg);
  assert(type && "isScalarOrVectorOfType vreg has no type assigned");
  if (type->getOpcode() == typeOpcode) {
    return true;
  } else if (type->getOpcode() == SPIRV::OpTypeVector) {
    Register scalarTypeVReg = type->getOperand(1).getReg();
    SPIRVType *scalarType = getSPIRVTypeForVReg(scalarTypeVReg);
    return scalarType->getOpcode() == typeOpcode;
  }
  return false;
}

unsigned SPIRVTypeRegistry::getScalarOrVectorBitWidth(const SPIRVType *type) const {
  if (type && type->getOpcode() == SPIRV::OpTypeVector) {
    auto eleTypeReg = type->getOperand(1).getReg();
    type = getSPIRVTypeForVReg(eleTypeReg);
  }
  if (type && (type->getOpcode() == SPIRV::OpTypeInt ||
               type->getOpcode() == SPIRV::OpTypeFloat)) {
    return type->getOperand(1).getImm();
  }
  llvm_unreachable("Attempting to get bit width of non-integer/float type.");
}

bool SPIRVTypeRegistry::isScalarOrVectorSigned(const SPIRVType *type) const {
  if (type && type->getOpcode() == SPIRV::OpTypeVector) {
    auto eleTypeReg = type->getOperand(1).getReg();
    type = getSPIRVTypeForVReg(eleTypeReg);
  }
  if (type && type->getOpcode() == SPIRV::OpTypeInt) {
    return type->getOperand(2).getImm() != 0;
  }
  llvm_unreachable("Attempting to get sign of non-integer type.");
}

StorageClass::StorageClass
SPIRVTypeRegistry::getPointerStorageClass(Register vreg) const {
  SPIRVType *type = getSPIRVTypeForVReg(vreg);
  if (type && type->getOpcode() == SPIRV::OpTypePointer) {
    auto scOp = type->getOperand(1).getImm();
    return static_cast<StorageClass::StorageClass>(scOp);
  }
  llvm_unreachable("Attempting to get storage class of non-pointer type.");
}

SPIRVType *SPIRVTypeRegistry::getOpTypeImage(
    MachineIRBuilder &MIRBuilder, SPIRVType *sampledType, Dim::Dim dim,
    uint32_t depth, uint32_t arrayed, uint32_t multisampled, uint32_t sampled,
    ImageFormat::ImageFormat imageFormat, AQ::AccessQualifier accessQualifier) {
  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeImage)
                 .addDef(resVReg)
                 .addUse(getSPIRVTypeID(sampledType))
                 .addImm(dim)
                 .addImm(depth)   // Depth (whether or not it is a depth image)
                 .addImm(arrayed) // Arrayed
                 .addImm(multisampled) // Multisampled (0 = only single-sample)
                 .addImm(sampled)      // Sampled (0 = usage known at runtime)
                 .addImm(imageFormat)
                 .addImm(accessQualifier);
  return MIB;
}


SPIRVType *SPIRVTypeRegistry::getSamplerType(MachineIRBuilder &MIRBuilder) {
  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampler).addDef(resVReg);
  constrainRegOperands(MIB);
  return MIB;
}

SPIRVType *
SPIRVTypeRegistry::getOpTypePipe(MachineIRBuilder &MIRBuilder,
                                 AQ::AccessQualifier accessQualifier) {
  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePipe)
                 .addDef(resVReg)
                 .addImm(accessQualifier);
  constrainRegOperands(MIB);
  return MIB;
}

SPIRVType *
SPIRVTypeRegistry::getSampledImageType(SPIRVType *imageType,
                                       MachineIRBuilder &MIRBuilder) {
  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampledImage)
                 .addDef(resVReg)
                 .addUse(getSPIRVTypeID(imageType));
  constrainRegOperands(MIB);
  return checkSpecialInstrMap(MIB, SpecialTypesAndConstsMap);
}

SPIRVType *SPIRVTypeRegistry::getOpTypeByOpcode(MachineIRBuilder &MIRBuilder,
                                                unsigned int opcode) {
  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(opcode).addDef(resVReg);
  constrainRegOperands(MIB);
  return MIB;
}

const MachineInstr *SPIRVTypeRegistry::checkSpecialInstrMap(
    const MachineInstr *newInstr, SpecialInstrMapTy &instrMap) {
  auto t = instrMap.find(newInstr->getOpcode());
  if (t != instrMap.end()) {
    for (auto instrGroup : t->second) {
      // Each group contins identical special instructions in different MFs,
      // it's enough to check the first instruction in the group.
      const MachineInstr *instr = instrGroup.begin()->second;
      if (instr->isIdenticalTo(*newInstr,
                              MachineInstr::MICheckType::IgnoreDefs)) {
        auto tt = instrGroup.find(const_cast<MachineFunction *>
                                  (newInstr->getMF()));
        if (tt != instrGroup.end()) {
          // The equivalent instruction was found in this MF,
          // remove new instruction and return the existing one.
          const_cast<llvm::MachineInstr*>(newInstr)->eraseFromParent();
          return tt->second;
        }
        // No such instruction in the group, add new instruction.
        instrGroup[const_cast<MachineFunction *>(newInstr->getMF())] = newInstr;
        return newInstr;
      }
    }
  }
  // It's new instruction with no existent group, so create a group,
  // insert the instruction to the group and insert the group to the map.
  auto *newInstrGroup = new SPIRVInstrGroup;
  (*newInstrGroup)[const_cast<MachineFunction *>(newInstr->getMF())] = newInstr;
  instrMap[newInstr->getOpcode()].push_back(*newInstrGroup);
  return newInstr;
}

SPIRVType *SPIRVTypeRegistry::generateOpenCLOpaqueType(const StructType *Ty,
                                          MachineIRBuilder &MIRBuilder,
                                          AQ::AccessQualifier accessQual) {
  const StringRef Name = Ty->getName();
  assert(Name.startswith("opencl.") && "CL types should start with 'opencl.'");
  using namespace Dim;
  auto TypeName = Name.substr(strlen("opencl."));

  if (TypeName.startswith("image")) {
    // default is read only
    accessQual = AQ::ReadOnly;
    if (TypeName.endswith("_ro_t"))
      accessQual = AQ::ReadOnly;
    else if (TypeName.endswith("_wo_t"))
      accessQual = AQ::WriteOnly;
    else if (TypeName.endswith("_rw_t"))
      accessQual = AQ::ReadWrite;
    char dimC = TypeName[strlen("image")];
    if (dimC >= '1' && dimC <= '3') {
      auto dim = dimC == '1' ? DIM_1D : dimC == '2' ? DIM_2D : DIM_3D;
      unsigned int arrayed = 0;
      if (TypeName.contains("buffer"))
        dim = DIM_Buffer;
      if (TypeName.contains("array"))
        arrayed = 1;
      auto *VoidTy = getOrCreateSPIRVType(
          Type::getVoidTy(MIRBuilder.getMF().getFunction().getContext()),
          MIRBuilder);
      return getOpTypeImage(MIRBuilder, VoidTy, dim, 0, arrayed, 0, 0,
                            ImageFormat::Unknown, accessQual);
    }
  } else if (TypeName.startswith("sampler_t")) {
    return getSamplerType(MIRBuilder);
  } else if (TypeName.startswith("pipe")) {
    if (TypeName.endswith("_ro_t"))
      accessQual = AQ::ReadOnly;
    else if (TypeName.endswith("_wo_t"))
      accessQual = AQ::WriteOnly;
    else if (TypeName.endswith("_rw_t"))
      accessQual = AQ::ReadWrite;
    return getOpTypePipe(MIRBuilder, accessQual);
  } else if (TypeName.startswith("queue"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeQueue);
  else if (TypeName.startswith("event_t"))
    return getOpTypeByOpcode(MIRBuilder, SPIRV::OpTypeEvent);

  report_fatal_error("Cannot generate OpenCL type: " + Name);
}

SPIRVType *SPIRVTypeRegistry::getOrCreateSPIRVTypeByName(StringRef typeStr,
    MachineIRBuilder &MIRBuilder) {
  unsigned vecElts = 0;
  auto &ctx = MIRBuilder.getMF().getFunction().getContext();

  // Parse type name in either "typeN" or "type vector[N]" format, where
  // N is the number of elements of the vector.
  Type* type;
  if (typeStr.startswith("void")) {
    type = Type::getVoidTy(ctx);
    typeStr = typeStr.substr(strlen("void"));
  } else if (typeStr.startswith("int") || typeStr.startswith("uint")) {
    type = Type::getInt32Ty(ctx);
    typeStr = typeStr.startswith("int") ? typeStr.substr(strlen("int"))
                                        : typeStr.substr(strlen("uint"));
  } else if (typeStr.startswith("float")) {
    type = Type::getFloatTy(ctx);
    typeStr = typeStr.substr(strlen("float"));
  } else if (typeStr.startswith("half")) {
    type = Type::getHalfTy(ctx);
    typeStr = typeStr.substr(strlen("half"));
  } else
    llvm_unreachable("Unable to recognize SPIRV type name.");
  if (typeStr.startswith(" vector[")) {
    typeStr = typeStr.substr(strlen(" vector["));
    typeStr = typeStr.substr(0, typeStr.find(']'));
  }
  typeStr.getAsInteger(10, vecElts);
  auto spirvTy = getOrCreateSPIRVType(type, MIRBuilder);
  if (vecElts > 0)
    spirvTy = getOrCreateSPIRVVectorType(spirvTy, vecElts, MIRBuilder);
  return spirvTy;
}

SPIRVType *SPIRVTypeRegistry::handleSPIRVBuiltin(const StructType *Ty,
                                                  MachineIRBuilder &MIRBuilder,
                                                  AQ::AccessQualifier aq) {
  assert(Ty->hasName());
  unsigned NumStartingVRegs = MIRBuilder.getMRI()->getNumVirtRegs();
  auto newTy = generateSPIRVOpaqueType(Ty, MIRBuilder, aq);
  auto resTy = checkSpecialInstrMap(newTy, BuiltinTypeMap);
  if (NumStartingVRegs < MIRBuilder.getMRI()->getNumVirtRegs() &&
      newTy == resTy)
    buildOpName(getSPIRVTypeID(resTy), Ty->getName(), MIRBuilder);
  return resTy;
}

SPIRVType *SPIRVTypeRegistry::generateSPIRVOpaqueType(const StructType *Ty,
                                          MachineIRBuilder &MIRBuilder,
                                          AQ::AccessQualifier accessQual) {
  const StringRef Name = Ty->getName();
  assert(Name.startswith("spirv.") && "CL types should start with 'opencl.'");
  auto TypeName = Name.substr(strlen("spirv."));

  if (TypeName.startswith("Image.")) {
    // Parse SPIRV ImageType which has following format in LLVM:
    // Image._Type_Dim_Depth_Arrayed_MS_Sampled_ImageFormat_AccessQualifier
    // e.g. %spirv.Image._void_1_0_0_0_0_0_0
    auto typeLiteralStr = TypeName.substr(strlen("Image."));
    SmallVector<StringRef> typeLiterals;
    SplitString(typeLiteralStr, typeLiterals, "_");
    assert (typeLiterals.size() == 8 && "Wrong number of literals in Image type");
    auto *spirvType = getOrCreateSPIRVTypeByName(typeLiterals[0], MIRBuilder);

    unsigned ddim = 0, depth = 0, arrayed = 0, ms = 0, sampled = 0;
    unsigned imageFormat = 0, accessQualifier = 0;
    if (typeLiterals[1].getAsInteger(10, ddim) ||
        typeLiterals[2].getAsInteger(10, depth) ||
        typeLiterals[3].getAsInteger(10, arrayed) ||
        typeLiterals[4].getAsInteger(10, ms) ||
        typeLiterals[5].getAsInteger(10, sampled) ||
        typeLiterals[6].getAsInteger(10, imageFormat) ||
        typeLiterals[7].getAsInteger(10, accessQualifier))
      llvm_unreachable("Unable to recognize Image type literals");
    return getOpTypeImage(MIRBuilder, spirvType, Dim::Dim(ddim),
                          depth, arrayed, ms, sampled,
                          ImageFormat::ImageFormat(imageFormat),
                          AQ::AccessQualifier(accessQualifier));
  } else if (TypeName.startswith("SampledImage.")) {
    // Find corresponding Image type.
    auto literals = TypeName.substr(strlen("SampledImage."));
    auto &ctx = MIRBuilder.getMF().getFunction().getContext();
    Type* imgTy = StructType::getTypeByName(ctx, "spirv.Image."
                                                 + literals.str());
    SPIRVType *spirvImageType = getOrCreateSPIRVType(imgTy, MIRBuilder);
    return getOrCreateSPIRVSampledImageType(spirvImageType, MIRBuilder);
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
      accessQual = AQ::ReadOnly;
    else if (TypeName.endswith("_1"))
      accessQual = AQ::WriteOnly;
    else if (TypeName.endswith("_2"))
      accessQual = AQ::ReadWrite;
    return getOpTypePipe(MIRBuilder, accessQual);
  }
  else if (TypeName.startswith("PipeStorage"))
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
    SPIRVType *imageType, MachineIRBuilder &MIRBuilder) {
  SPIRVType *type = getSampledImageType(imageType, MIRBuilder);
  return type;
}

unsigned int
SPIRVTypeRegistry::StorageClassToAddressSpace(StorageClass::StorageClass sc) {
  // TODO maybe this should be handled in the subtarget to allow for different
  // OpenCL vs Vulkan handling?
  switch (sc) {
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
    errs() << getStorageClassName(sc) << "\n";
    llvm_unreachable("Unable to get address space id");
  }
}

StorageClass::StorageClass
SPIRVTypeRegistry::addressSpaceToStorageClass(unsigned int addressSpace) {
  // TODO maybe this should be handled in the subtarget to allow for different
  // OpenCL vs Vulkan handling?
  switch (addressSpace) {
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

bool SPIRVTypeRegistry::constrainRegOperands(MachineInstrBuilder &MIB, MachineFunction *MF) const {
  // A utility method to avoid having these TII, TRI, RBI lines everywhere
  if (!MF)
    MF = MIB->getMF();
  const auto &subtarget = MF->getSubtarget();
  const TargetInstrInfo *TII = subtarget.getInstrInfo();
  const TargetRegisterInfo *TRI = subtarget.getRegisterInfo();
  const RegisterBankInfo *RBI = subtarget.getRegBankInfo();

  return constrainSelectedInstRegOperands(*MIB, *TII, *TRI, *RBI);
}
