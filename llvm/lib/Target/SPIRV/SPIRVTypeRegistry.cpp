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
// Passes requiring type info can reconstruct the vreg->OpTypeXXX mapping
// by calling rebuildTypeTablesForFunction(MF) at the start. They must also
// call reset() when the function ends to avoid maintaining invalid state
// between function passes.
//
// Type info from this class can only be used before it gets stripped out by the
// InstructionSelector stage. All type info is function-local until the final
// SPIRVGlobalTypesAndRegNums pass hoists it globally and deduplicates it all.
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

SPIRVTypeRegistry::SPIRVTypeRegistry(unsigned int pointerSize)
    : pointerSize(pointerSize) {}

void SPIRVTypeRegistry::rebuildTypeTablesForFunction(MachineFunction &MF) {
  const auto TII = MF.getSubtarget().getInstrInfo();
  const auto spvTII = static_cast<const SPIRVInstrInfo *>(TII);
  for (const auto &MBB : MF) {
    for (const auto &MI : MBB) {
      if (MI.getOpcode() == SPIRV::ASSIGN_TYPE) {
        Register typeVReg = MI.getOperand(0).getReg();
        Register idVReg = MI.getOperand(1).getReg();

        SPIRVType *type = VRegToTypeMap[typeVReg];
        VRegToTypeMap[idVReg] = type;
      } else if (spvTII->isTypeDeclInstr(MI)) {
        VRegToTypeMap.insert({getSPIRVTypeID(&MI), &MI});
        getExistingTypesForOpcode(MI.getOpcode())->push_back(&MI);
      }
    }
  }
}

std::vector<SPIRVType *> *
SPIRVTypeRegistry::getExistingTypesForOpcode(unsigned opcode) {
  auto found = OpcodeToSPIRVTypeMap.find(opcode);
  if (found == OpcodeToSPIRVTypeMap.end()) {
    auto types = new std::vector<SPIRVType *>;
    OpcodeToSPIRVTypeMap.insert({opcode, types});
    return types;
  } else {
    return found->second;
  }
}

void SPIRVTypeRegistry::reset() {
  VRegToTypeMap.shrink_and_clear();
  TypeToSPIRVTypeMap.shrink_and_clear();
  for (const auto &t : OpcodeToSPIRVTypeMap) {
    delete t.second;
  }
  OpcodeToSPIRVTypeMap.shrink_and_clear();
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
  VRegToTypeMap[VReg] = spirvType;
  auto MIB = MIRBuilder.buildInstr(SPIRV::ASSIGN_TYPE)
                 .addUse(getSPIRVTypeID(spirvType))
                 .addUse(VReg);
  constrainRegOperands(MIB);
}

static Register createTypeVReg(MachineIRBuilder &MIRBuilder) {
  auto &MRI = MIRBuilder.getMF().getRegInfo();
  return MRI.createVirtualRegister(&SPIRV::TYPERegClass);
}

SPIRVType *SPIRVTypeRegistry::getOpTypeBool(MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeBool);
  if (tys->empty()) {
    tys->push_back(MIRBuilder.buildInstr(SPIRV::OpTypeBool)
                       .addDef(createTypeVReg(MIRBuilder)));
  }
  return tys->at(0);
}

SPIRVType *SPIRVTypeRegistry::getOpTypeInt(uint32_t width,
                                          MachineIRBuilder &MIRBuilder,
                                          bool isSigned) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeInt);
  for (const auto &ty : *tys) {
    if (ty->getOperand(1).getImm() == width &&
        ty->getOperand(2).getImm() == isSigned) {
      return ty;
    }
  }
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeInt)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(width)
                 .addImm(isSigned ? 1 : 0);
  tys->push_back(MIB);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeFloat(uint32_t width,
                                            MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeFloat);
  for (const auto &ty : *tys) {
    if (ty->getOperand(1).getImm() == width)
      return ty;
  }
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFloat)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(width);
  tys->push_back(MIB);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeVoid(MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeVoid);
  if (tys->empty()) {
    tys->push_back(MIRBuilder.buildInstr(SPIRV::OpTypeVoid)
                       .addDef(createTypeVReg(MIRBuilder)));
  }
  return tys->at(0);
}

SPIRVType *SPIRVTypeRegistry::getOpTypeVector(uint32_t numElems,
                                             SPIRVType *elemType,
                                             MachineIRBuilder &MIRBuilder) {
  using namespace SPIRV;
  auto tys = getExistingTypesForOpcode(OpTypeVector);
  for (const auto &ty : *tys) {
    if (ty->getOperand(2).getImm() == numElems &&
        ty->getOperand(1).getReg() == getSPIRVTypeID(elemType)) {
      return ty;
    }
  }
  auto eleOpc = elemType->getOpcode();
  if (eleOpc != OpTypeInt && eleOpc != OpTypeFloat && eleOpc != OpTypeBool) {
    errs() << *elemType;
    report_fatal_error("Invalid vector element type");
  }
  auto MIB = MIRBuilder.buildInstr(OpTypeVector)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(elemType))
                 .addImm(numElems);
  tys->push_back(MIB);
  return MIB;
}

static Register buildConstantI32(uint32_t val, MachineIRBuilder &MIRBuilder,
                                 SPIRVTypeRegistry *TR) {
  auto &MF = MIRBuilder.getMF();
  Register res = MF.getRegInfo().createGenericVirtualRegister(LLT::scalar(32));
  auto IntTy = IntegerType::getInt32Ty(MF.getFunction().getContext());
  const auto ConstInt = ConstantInt::get(IntTy, val);
  TR->assignTypeToVReg(IntTy, res, MIRBuilder);
  MIRBuilder.buildConstant(res, *ConstInt);
  return res;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeArray(uint32_t numElems,
                                            SPIRVType *elemType,
                                            MachineIRBuilder &MIRBuilder) {
  const auto MRI = MIRBuilder.getMRI();
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeArray);
  for (const auto &ty : *tys) {
    if (ty->getOperand(1).getReg() == getSPIRVTypeID(elemType)) {
      auto cInst = MRI->getVRegDef(ty->getOperand(2).getReg());
      auto cVal = cInst->getOperand(1).getCImm()->getValue().getZExtValue();
      if (cVal == numElems) {
        return ty;
      }
    }
  }
  if (elemType->getOpcode() == SPIRV::OpTypeVoid) {
    errs() << *elemType;
    report_fatal_error("Invalid array element type");
  }
  Register numElementsVReg = buildConstantI32(numElems, MIRBuilder, this);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeArray)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(elemType))
                 .addUse(numElementsVReg);
  tys->push_back(MIB);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeOpaque(const StringRef name,
                                             MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeOpaque);
  for (const auto &ty : *tys) {
    if (name.equals(getStringImm(*ty, 1)))
      return ty;
  }
  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeOpaque).addDef(resVReg);
  addStringImm(name, MIB);
  buildOpName(resVReg, name, MIRBuilder);
  tys->push_back(MIB);
  return MIB;
}

SPIRVType *
SPIRVTypeRegistry::getOpTypeStruct(const SmallVectorImpl<SPIRVType *> &elems,
                                  MachineIRBuilder &MIRBuilder,
                                  StringRef name) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeStruct);
  const unsigned numExpectedOps = elems.size() + 1;
  for (const auto &ty : *tys) {
    if (ty->getNumOperands() == numExpectedOps) {
      bool allMatch = true;
      for (unsigned i = 1; i < numExpectedOps; ++i) {
        auto elem = elems[i - 1];
        auto op = ty->getOperand(i).getReg();
        if (getSPIRVTypeID(elem) != op) {
          allMatch = false;
          break;
        }
      }
      if (allMatch)
        return ty;
    }
  }

  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeStruct).addDef(resVReg);
  for (const auto &elementType : elems) {
    if (elementType->getOpcode() == SPIRV::OpTypeVoid) {
      errs() << elementType;
      report_fatal_error("Invalid struct element type");
    }
    MIB.addUse(getSPIRVTypeID(elementType));
  }
  buildOpName(resVReg, name, MIRBuilder);
  tys->push_back(MIB);
  return MIB;
}

static bool isOpenCLBuiltinType(const StructType *stype) {
  return stype->isOpaque() && stype->hasName() &&
         stype->getName().startswith("opencl.");
}

static SPIRVType *handleOpenCLBuiltin(const StringRef name,
                                      MachineIRBuilder &MIRBuilder,
                                      SPIRVTypeRegistry *TR,
                                      AQ::AccessQualifier aq) {
  unsigned numStartingVRegs = MIRBuilder.getMRI()->getNumVirtRegs();
  auto resTy = generateOpenCLOpaqueType(name, MIRBuilder, TR, aq);
  if (numStartingVRegs < MIRBuilder.getMRI()->getNumVirtRegs()) {
    buildOpName(TR->getSPIRVTypeID(resTy), name, MIRBuilder);
  }
  return resTy;
}

SPIRVType *SPIRVTypeRegistry::getOpTypePointer(StorageClass::StorageClass sc,
                                              SPIRVType *elemType,
                                              MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypePointer);
  for (const auto &ty : *tys) {
    if (ty->getOperand(1).getImm() == sc &&
        ty->getOperand(2).getReg() == getSPIRVTypeID(elemType)) {
      return ty;
    }
  }
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypePointer)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addImm(sc)
                 .addUse(getSPIRVTypeID(elemType));
  tys->push_back(MIB);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::getOpTypeFunction(
    SPIRVType *retType, const SmallVectorImpl<SPIRVType *> &argTypes,
    MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeFunction);
  const unsigned numExpectedOps = argTypes.size() + 2;
  for (const auto &ty : *tys) {
    if (ty->getNumOperands() == numExpectedOps &&
        ty->getOperand(1).getReg() == getSPIRVTypeID(retType)) {
      bool allMatch = true;
      for (unsigned i = 2; i < numExpectedOps; ++i) {
        auto arg = argTypes[i - 2];
        auto op = ty->getOperand(i).getReg();
        if (getSPIRVTypeID(arg) != op) {
          allMatch = false;
          break;
        }
      }
      if (allMatch)
        return ty;
    }
  }
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeFunction)
                 .addDef(createTypeVReg(MIRBuilder))
                 .addUse(getSPIRVTypeID(retType));
  for (const auto &argType : argTypes) {
    MIB.addUse(getSPIRVTypeID(argType));
  }
  tys->push_back(MIB);
  return MIB;
}

SPIRVType *SPIRVTypeRegistry::createSPIRVType(const Type *Ty,
                                             MachineIRBuilder &MIRBuilder,
                                             AQ::AccessQualifier aq) {
  if (auto itype = dyn_cast<IntegerType>(Ty)) {
    const unsigned int width = itype->getBitWidth();
    return width == 1 ? getOpTypeBool(MIRBuilder)
                      : getOpTypeInt(width, MIRBuilder, false);
  } else if (Ty->isFloatingPointTy()) {
    return getOpTypeFloat(Ty->getPrimitiveSizeInBits(), MIRBuilder);
  } else if (Ty->isVoidTy()) {
    return getOpTypeVoid(MIRBuilder);
  } else if (Ty->isVectorTy()) {
    auto el = getOrCreateSPIRVType(Ty->getVectorElementType(), MIRBuilder);
    return getOpTypeVector(Ty->getVectorNumElements(), el, MIRBuilder);
  } else if (Ty->isArrayTy()) {
    auto *el = getOrCreateSPIRVType(Ty->getArrayElementType(), MIRBuilder);
    return getOpTypeArray(Ty->getArrayNumElements(), el, MIRBuilder);
  } else if (auto stype = dyn_cast<StructType>(Ty)) {
    const StringRef name = stype->hasName() ? stype->getName() : "";
    if (isOpenCLBuiltinType(stype)) {
      return handleOpenCLBuiltin(name, MIRBuilder, this, aq);
    } else if (stype->isOpaque()) {
      return getOpTypeOpaque(name, MIRBuilder);
    } else {
      SmallVector<SPIRVType *, 8> elementTypes;
      for (const auto &t : stype->elements()) {
        elementTypes.push_back(getOrCreateSPIRVType(t, MIRBuilder));
      }
      return getOpTypeStruct(elementTypes, MIRBuilder, name);
    }
  } else if (auto ftype = dyn_cast<FunctionType>(Ty)) {
    SPIRVType *retTy = getOrCreateSPIRVType(ftype->getReturnType(), MIRBuilder);
    SmallVector<SPIRVType *, 4> paramTypes;
    for (const auto &t : ftype->params()) {
      paramTypes.push_back(getOrCreateSPIRVType(t, MIRBuilder));
    }
    return getOpTypeFunction(retTy, paramTypes, MIRBuilder);
  } else if (const auto &ptype = dyn_cast<PointerType>(Ty)) {
    Type *elemType = ptype->getPointerElementType();

    // Some OpenCL builtins like image2d_t are passed in as pointers, but
    // should be treated as custom types like OpTypeImage
    if (auto stype = dyn_cast<StructType>(elemType)) {
      if (isOpenCLBuiltinType(stype)) {
        const StringRef name = stype->hasName() ? stype->getName() : "";
        return handleOpenCLBuiltin(name, MIRBuilder, this, aq);
      }
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

SPIRVType *SPIRVTypeRegistry::getSPIRVTypeForVReg(Register VReg) {
  auto t = VRegToTypeMap.find(VReg);
  return t == VRegToTypeMap.end() ? nullptr : t->second;
}

SPIRVType *
SPIRVTypeRegistry::getOrCreateSPIRVType(const Type *type,
                                       MachineIRBuilder &MIRBuilder,
                                       AQ::AccessQualifier accessQual) {

  auto SPIRVTypeIt = TypeToSPIRVTypeMap.find(type);
  if (SPIRVTypeIt != TypeToSPIRVTypeMap.end()) {
    return SPIRVTypeIt->second;
  } else {
    SPIRVType *spirvType = createSPIRVType(type, MIRBuilder, accessQual);
    VRegToTypeMap[getSPIRVTypeID(spirvType)] = spirvType;
    TypeToSPIRVTypeMap.insert({type, spirvType});
    return spirvType;
  }
}

SPIRVType *SPIRVTypeRegistry::getPtrUIntType(MachineIRBuilder &MIRBuilder) {
  return getOpTypeInt(pointerSize, MIRBuilder, false);
}

SPIRVType *SPIRVTypeRegistry::getPairStruct(SPIRVType *elem0, SPIRVType *elem1,
                                           MachineIRBuilder &MIRBuilder) {
  SmallVector<SPIRVType *, 2> elems = {elem0, elem1};
  return getOpTypeStruct(elems, MIRBuilder);
}

bool SPIRVTypeRegistry::isScalarOfType(Register vreg, unsigned int typeOpcode) {
  SPIRVType *type = getSPIRVTypeForVReg(vreg);
  assert(type && "isScalarOfType vreg has no type assigned");
  return type->getOpcode() == typeOpcode;
}

bool SPIRVTypeRegistry::isScalarOrVectorOfType(Register vreg,
                                              unsigned int typeOpcode) {
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

unsigned SPIRVTypeRegistry::getScalarOrVectorBitWidth(const SPIRVType *type) {
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

bool SPIRVTypeRegistry::isScalarOrVectorSigned(const SPIRVType *type) {
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
SPIRVTypeRegistry::getPointerStorageClass(Register vreg) {
  SPIRVType *type = getSPIRVTypeForVReg(vreg);
  if (type && type->getOpcode() == SPIRV::OpTypePointer) {
    auto scOp = type->getOperand(1).getImm();
    return static_cast<StorageClass::StorageClass>(scOp);
  }
  llvm_unreachable("Attempting to get storage class of non-pointer type.");
}

SPIRVType *SPIRVTypeRegistry::getGenericPtrType(SPIRVType *origPtrType,
                                               MachineIRBuilder &MIRBuilder) {
  using namespace StorageClass;
  if (origPtrType && origPtrType->getOpcode() == SPIRV::OpTypePointer) {
    auto scOp = origPtrType->getOperand(1).getImm();
    if (scOp == Generic) {
      return origPtrType;
    } else {
      auto elemTy = getSPIRVTypeForVReg(origPtrType->getOperand(2).getReg());
      return getOpTypePointer(Generic, elemTy, MIRBuilder);
    }
  }
  llvm_unreachable("Attempting to get generic equivalent of non-pointer type.");
}

SPIRVType *SPIRVTypeRegistry::getOpTypeImage(
    MachineIRBuilder &MIRBuilder, SPIRVType *sampledType, Dim::Dim dim,
    uint32_t depth, uint32_t arrayed, uint32_t multisampled, uint32_t sampled,
    ImageFormat::ImageFormat imageFormat, AQ::AccessQualifier accessQualifier) {

  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeImage);
  for (const auto &ty : *tys) {
    if (ty->getOperand(1).getReg() == getSPIRVTypeID(sampledType) &&
        ty->getOperand(2).getImm() == dim &&
        ty->getOperand(8).getImm() == accessQualifier &&
        ty->getOperand(3).getImm() == arrayed &&
        ty->getOperand(6).getImm() == sampled &&
        ty->getOperand(5).getImm() == multisampled &&
        ty->getOperand(3).getImm() == depth &&
        ty->getOperand(7).getImm() == imageFormat) {
      return ty;
    }
  }
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
  tys->push_back(MIB);
  return MIB;
}


SPIRVType *SPIRVTypeRegistry::getSamplerType(MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeSampler);
  if (tys->empty()) {
    Register resVReg = createTypeVReg(MIRBuilder);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampler).addDef(resVReg);
    constrainRegOperands(MIB);
    tys->push_back(MIB);
  }
  return tys->at(0);
}

SPIRVType *SPIRVTypeRegistry::getSampledImageType(SPIRVType *imageType,
                                                 MachineIRBuilder &MIRBuilder) {
  auto tys = getExistingTypesForOpcode(SPIRV::OpTypeSampledImage);
  for (const auto &ty : *tys) {
    if (ty->getOperand(1).getReg() == getSPIRVTypeID(imageType)) {
      return ty;
    }
  }
  Register resVReg = createTypeVReg(MIRBuilder);
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpTypeSampledImage)
                 .addDef(resVReg)
                 .addUse(getSPIRVTypeID(imageType));
  constrainRegOperands(MIB);
  tys->push_back(MIB);
  return MIB;
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
    return 5;
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
  case 5:
    return StorageClass::Input;
  default:
    llvm_unreachable("Unknown address space");
  }
}

bool SPIRVTypeRegistry::constrainRegOperands(MachineInstrBuilder &MIB) const {
  // A utility method to avoid having these TII, TRI, RBI lines everywhere
  const auto &subtarget = MIB->getMF()->getSubtarget();
  const TargetInstrInfo *TII = subtarget.getInstrInfo();
  const TargetRegisterInfo *TRI = subtarget.getRegisterInfo();
  const RegisterBankInfo *RBI = subtarget.getRegBankInfo();

  return constrainSelectedInstRegOperands(*MIB, *TII, *TRI, *RBI);
}
