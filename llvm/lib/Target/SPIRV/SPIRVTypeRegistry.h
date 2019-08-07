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

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H

#include "SPIRVEnums.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

namespace AQ = AccessQualifier;

namespace llvm {
using SPIRVType = const MachineInstr;

class SPIRVTypeRegistry {

private:
  // Registers holding values which have types associated with them.
  // Initialized upon VReg definition in IRTranslator.
  // Reconstituted from ASSIGN_TYPE machineInstrs in subsequent passes.
  DenseMap<Register, SPIRVType *> VRegToTypeMap;

  // Maps LLVM IR types to SPIR-V types (only used in IRTranslator pass)
  DenseMap<const Type *, SPIRVType *> TypeToSPIRVTypeMap;

  // Maps OpTypeXXX opcode to a list of OpTypeXXX instrs (for deduplication).
  DenseMap<unsigned, std::vector<SPIRVType *> *> OpcodeToSPIRVTypeMap;

  // Number of bits pointers and size_t integers require.
  const unsigned int pointerSize;

  // Add a new OpTypeXXX instruction without checking for duplicates
  SPIRVType *createSPIRVType(const Type *type, MachineIRBuilder &MIRBuilder,
                             AQ::AccessQualifier accessQual = AQ::ReadWrite);

public:
  SPIRVTypeRegistry(unsigned int pointerSize);

  // Run at the start of any pass requiring this to read all ASSIGN_TYPE
  // instructions to rebuild the VReg -> Type map.
  void rebuildTypeTablesForFunction(MachineFunction &MF);

  // Erase the VReg -> Type map and any other internal state.
  // Call after every function pass using this type system.
  void reset();

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

  // Return the SPIR-V type instruction corresponding to the given VReg, or
  // nullptr if no such type instruction exists.
  SPIRVType *getSPIRVTypeForVReg(Register VReg);

  // Whether the given VReg has a SPIR-V type mapped to it yet.
  bool hasSPIRVTypeForVReg(Register VReg) {
    return getSPIRVTypeForVReg(VReg) != nullptr;
  }

  // Return the VReg holding the result of the given OpTypeXXX instruction
  Register getSPIRVTypeID(const SPIRVType *spirvType) const {
    assert(spirvType && "Attempting to get type id for nullptr type.");
    return spirvType->defs().begin()->getReg();
  }

  // Whether the given VReg has an OpTypeXXX instruction mapped to it with the
  // given opcode (e.g. OpTypeFloat).
  bool isScalarOfType(Register vreg, unsigned int typeOpcode);

  // Return true if the given VReg's assigned SPIR-V type is either a scalar
  // matching the given opcode, or a vector with an element type matching that
  // opcode (e.g. OpTypeBool, or OpTypeVector %x 4, where %x is OpTypeBool).
  bool isScalarOrVectorOfType(Register vreg, unsigned int typeOpcode);

  // For vectors or scalars of ints or floats, return the scalar type's bitwidth
  unsigned getScalarOrVectorBitWidth(const SPIRVType *type);

  // For integer vectors or scalars, return whether the integers are signed
  bool isScalarOrVectorSigned(const SPIRVType *type);

  // Gets the storage class of the pointer type assigned to this vreg
  StorageClass::StorageClass getPointerStorageClass(Register vreg);

  // Return the number of bits SPIR-V pointers and size_t variables require.
  unsigned getPointerSize() const { return pointerSize; }

  // Gets a Generic storage class pointer to the same type as the given ptr.
  SPIRVType *getGenericPtrType(SPIRVType *origPtrType,
                               MachineIRBuilder &MIRBuilder);

  // Get or create an OpTypeSampler instruction.
  SPIRVType *getSamplerType(MachineIRBuilder &MIRBuilder);

  // Get or create an OpTypeSampledImage of for the given image type.
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

  SPIRVType *getOpTypeOpaque(const StringRef name,
                             MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeStruct(const SmallVectorImpl<SPIRVType *> &elems,
                             MachineIRBuilder &MIRBuilder, StringRef name = "");

  SPIRVType *getOpTypePointer(StorageClass::StorageClass sc,
                              SPIRVType *elemType,
                              MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeFunction(SPIRVType *retType,
                               const SmallVectorImpl<SPIRVType *> &argTypes,
                               MachineIRBuilder &MIRBuilder);

  // Get or create the SPIR-V integer type that a pointer could be bitcast to.
  SPIRVType *getPtrUIntType(MachineIRBuilder &MIRBuilder);

  // Get a 2-element struct type with the given element types.
  SPIRVType *getPairStruct(SPIRVType *elem0, SPIRVType *elem1,
                           MachineIRBuilder &MIRBuilder);

  // Get or create an OpTypeImage with the given parameters.
  SPIRVType *getOpTypeImage(MachineIRBuilder &MIRBuilder,
                            SPIRVType *sampledType, Dim::Dim dim,
                            uint32_t depth, uint32_t arrayed,
                            uint32_t multisampled, uint32_t sampled,
                            ImageFormat::ImageFormat imageFormat,
                            AQ::AccessQualifier accessQualifier);

  std::vector<SPIRVType *> *getExistingTypesForOpcode(unsigned opcode);

  // Convert a SPIR-V storage class to the corresponding LLVM IR address space.
  unsigned int StorageClassToAddressSpace(StorageClass::StorageClass sc);

  // Convert an LLVM IR address space to a SPIR-V storage class.
  StorageClass::StorageClass
  addressSpaceToStorageClass(unsigned int addressSpace);

  // Utility method to constrain an instruction's operands to the correct
  // register classes, and return true if this worked.
  bool constrainRegOperands(MachineInstrBuilder &MIB) const;
};
} // end namespace llvm
#endif // LLLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H
