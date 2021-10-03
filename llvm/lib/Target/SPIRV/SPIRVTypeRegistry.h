//===-- SPIRVTypeRegistry.h - SPIR-V Type Registry --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SPIRVTypeRegistry is used to maintain rich type information required for
// SPIR-V even after lowering from LLVM IR to GMIR. It can convert an llvm::Type
// into an OpTypeXXX instruction, and map it to a virtual register using and
// ASSIGN_TYPE pseudo instruction.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H

#include "SPIRVEnums.h"
#include "SPIRVDuplicatesTracker.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

#include <unordered_set>

namespace AQ = AccessQualifier;

const std::unordered_set<unsigned>& getTypeFoldingSupportingOpcs();
bool isTypeFoldingSupported(unsigned Opcode);

namespace llvm {
using SPIRVType = const MachineInstr;

class SPIRVTypeRegistry {
  // Registers holding values which have types associated with them.
  // Initialized upon VReg definition in IRTranslator.
  // Do not confuse this with DuplicatesTracker as DT maps Type* to <MF, Reg>
  // where Reg = OpType...
  // while VRegToTypeMap tracks SPIR-V type assigned to other regs (i.e. not type-declaring ones)
  DenseMap<MachineFunction *, DenseMap<Register, SPIRVType *>> VRegToTypeMap;

  SPIRVGeneralDuplicatesTracker &DT;

  DenseMap<SPIRVType *, const Type *> SPIRVToLLVMType;

  // Builtin types may be represented in two ways while translating to SPIR-V:
  // in OpenCL form and in SPIR-V form. We have to keep only one type for such
  // pairs so check any builtin type for existance in the map before emitting
  // it to SPIR-V. The map contains a vector of SPIR-V builtin types already
  // emitted for a given type opcode.
  DenseMap<const MachineFunction *,
           DenseMap<unsigned int, SmallVector<SPIRVType*>>> BuiltinTypeMap;

  // Look for an equivalent of the newType in the map. Return the equivalent
  // if it's found, otherwise insert newType to the map and return the type.
  SPIRVType *checkBuiltinTypeMap(SPIRVType *newType);

  // Number of bits pointers and size_t integers require.
  const unsigned int pointerSize;

  // Add a new OpTypeXXX instruction without checking for duplicates
  SPIRVType *createSPIRVType(const Type *type, MachineIRBuilder &MIRBuilder,
                             AQ::AccessQualifier accessQual = AQ::ReadWrite);

public:
  SPIRVTypeRegistry(SPIRVGeneralDuplicatesTracker &DT, unsigned int pointerSize);

  MachineFunction *CurMF;

  void generateAssignInstrs(MachineFunction &MF);

  // Get or create a SPIR-V type corresponding the given LLVM IR type,
  // and map it to the given VReg by creating an ASSIGN_TYPE instruction.
  SPIRVType *assignTypeToVReg(const Type *type, Register VReg,
                              MachineIRBuilder &MIRBuilder,
                              AQ::AccessQualifier accessQual = AQ::ReadWrite);

  // In cases where the SPIR-V type is already known, this function can be
  // used to map it to the given VReg via an ASSIGN_TYPE instruction.
  void assignSPIRVTypeToVReg(SPIRVType *type, Register VReg,
                             MachineIRBuilder &MIRBuilder);

  // Either generate a new OpTypeXXX instruction or return an existing one
  // corresponding to the given LLVM IR type.
  SPIRVType *
  getOrCreateSPIRVType(const Type *type, MachineIRBuilder &MIRBuilder,
                       AQ::AccessQualifier accessQual = AQ ::ReadWrite);

  const Type *getTypeForSPIRVType(const SPIRVType *Ty) const {
    auto Res = SPIRVToLLVMType.find(Ty);
    assert(Res != SPIRVToLLVMType.end());
    return Res->second;
  }

  // Return the SPIR-V type instruction corresponding to the given VReg, or
  // nullptr if no such type instruction exists.
  SPIRVType *getSPIRVTypeForVReg(Register VReg) const;

  // Whether the given VReg has a SPIR-V type mapped to it yet.
  bool hasSPIRVTypeForVReg(Register VReg) const {
    return getSPIRVTypeForVReg(VReg) != nullptr;
  }

  // Return the VReg holding the result of the given OpTypeXXX instruction
  Register getSPIRVTypeID(const SPIRVType *spirvType) const {
    assert(spirvType && "Attempting to get type id for nullptr type.");
    return spirvType->defs().begin()->getReg();
  }

  void setCurrentFunc(MachineFunction &MF) { CurMF = &MF; }

  // Whether the given VReg has an OpTypeXXX instruction mapped to it with the
  // given opcode (e.g. OpTypeFloat).
  bool isScalarOfType(Register vreg, unsigned int typeOpcode) const;

  // Return true if the given VReg's assigned SPIR-V type is either a scalar
  // matching the given opcode, or a vector with an element type matching that
  // opcode (e.g. OpTypeBool, or OpTypeVector %x 4, where %x is OpTypeBool).
  bool isScalarOrVectorOfType(Register vreg, unsigned int typeOpcode) const;

  // For vectors or scalars of ints or floats, return the scalar type's bitwidth
  unsigned getScalarOrVectorBitWidth(const SPIRVType *type) const;

  // For integer vectors or scalars, return whether the integers are signed
  bool isScalarOrVectorSigned(const SPIRVType *type) const;

  // Gets the storage class of the pointer type assigned to this vreg
  StorageClass::StorageClass getPointerStorageClass(Register vreg) const;

  // Return the number of bits SPIR-V pointers and size_t variables require.
  unsigned getPointerSize() const { return pointerSize; }

private:
  // Internal methods for creating types which are unsafe in duplications
  // check sense hence can only be called within getOrCreateSPIRVType callstack
  SPIRVType *getSamplerType(MachineIRBuilder &MIRBuilder);

  SPIRVType *getSampledImageType(SPIRVType *imageType,
                                 MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeBool(MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeInt(uint32_t width, MachineIRBuilder &MIRBuilder,
                          bool isSigned = false);

  SPIRVType *getOpTypeFloat(uint32_t width, MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeVoid(MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeVector(uint32_t numElems, SPIRVType *elemType,
                             MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeArray(uint32_t numElems, SPIRVType *elemType,
                            MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeOpaque(const StructType *Ty,
                             MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeStruct(const StructType *Ty,
                             MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypePointer(StorageClass::StorageClass sc,
                              SPIRVType *elemType,
                              MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeFunction(SPIRVType *retType,
                               const SmallVectorImpl<SPIRVType *> &argTypes,
                               MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeImage(MachineIRBuilder &MIRBuilder,
                            SPIRVType *sampledType, Dim::Dim dim,
                            uint32_t depth, uint32_t arrayed,
                            uint32_t multisampled, uint32_t sampled,
                            ImageFormat::ImageFormat imageFormat,
                            AQ::AccessQualifier accessQualifier);

  SPIRVType *getOpTypePipe(MachineIRBuilder &MIRBuilder,
                           AQ::AccessQualifier accessQualifier);

  SPIRVType *getOpTypeByOpcode(MachineIRBuilder &MIRBuilder,
                               unsigned int opcode);

  SPIRVType *handleOpenCLBuiltin(const StructType *Ty,
                                 MachineIRBuilder &MIRBuilder,
                                 AQ::AccessQualifier aq);

  SPIRVType *generateOpenCLOpaqueType(const StructType *Ty,
                                      MachineIRBuilder &MIRBuilder,
                                      AQ::AccessQualifier aq = AQ::ReadWrite);

  SPIRVType *handleSPIRVBuiltin(const StructType *Ty,
                                MachineIRBuilder &MIRBuilder,
                                AQ::AccessQualifier aq);

  SPIRVType *generateSPIRVOpaqueType(const StructType *Ty,
                                     MachineIRBuilder &MIRBuilder,
                                     AQ::AccessQualifier aq = AQ::ReadWrite);

  Register buildConstantI32(uint32_t val, MachineIRBuilder &MIRBuilder,
                            SPIRVTypeRegistry *TR);
public:
  // convenient helpers for getting types
  // w/ check for duplicates
  SPIRVType *getOrCreateSPIRVIntegerType(unsigned BitWidth,
                                         MachineIRBuilder &MIRBuilder);
  SPIRVType *getOrCreateSPIRVBoolType(MachineIRBuilder &MIRBuilder);
  SPIRVType *getOrCreateSPIRVVectorType(SPIRVType *BaseType,
                                        unsigned NumElements,
                                        MachineIRBuilder &MIRBuilder);
  SPIRVType *getOrCreateSPIRVPointerType(
      SPIRVType *BaseType, MachineIRBuilder &MIRBuilder,
      StorageClass::StorageClass SClass = StorageClass::Function);

  // Convert a SPIR-V storage class to the corresponding LLVM IR address space.
  unsigned int StorageClassToAddressSpace(StorageClass::StorageClass sc);

  // Convert an LLVM IR address space to a SPIR-V storage class.
  StorageClass::StorageClass
  addressSpaceToStorageClass(unsigned int addressSpace);

  // Utility method to constrain an instruction's operands to the correct
  // register classes, and return true if this worked.
  bool constrainRegOperands(MachineInstrBuilder &MIB, MachineFunction *MF = nullptr) const;
};
} // end namespace llvm
#endif // LLLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H
