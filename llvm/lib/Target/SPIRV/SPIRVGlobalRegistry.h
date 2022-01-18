//===-- SPIRVGlobalRegistry.h - SPIR-V Global Registry ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SPIRVGlobalRegistry is used to maintain rich type information required for
// SPIR-V even after lowering from LLVM IR to GMIR. It can convert an llvm::Type
// into an OpTypeXXX instruction, and map it to a virtual register. Also it
// builds and supports consistency of constants and global variables.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H

#include "SPIRVDuplicatesTracker.h"
#include "SPIRVEnums.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

namespace AQ = AccessQualifier;

namespace llvm {
using SPIRVType = const MachineInstr;

class SPIRVGlobalRegistry {
  // Registers holding values which have types associated with them.
  // Initialized upon VReg definition in IRTranslator.
  // Do not confuse this with DuplicatesTracker as DT maps Type* to <MF, Reg>
  // where Reg = OpType...
  // while VRegToTypeMap tracks SPIR-V type assigned to other regs (i.e. not
  // type-declaring ones)
  DenseMap<MachineFunction *, DenseMap<Register, SPIRVType *>> VRegToTypeMap;

  SPIRVGeneralDuplicatesTracker &DT;

  DenseMap<SPIRVType *, const Type *> SPIRVToLLVMType;

  using SPIRVInstrGroup = DenseMap<MachineFunction *, const MachineInstr *>;
  using VectorOfSPIRVInstrGroups = SmallVector<SPIRVInstrGroup>;
  using SpecialInstrMapTy = MapVector<unsigned int, VectorOfSPIRVInstrGroups>;

  // Builtin types may be represented in two ways while translating to SPIR-V:
  // in OpenCL form and in SPIR-V form. We have to keep only one type for such
  // pairs so check any builtin type for existance in the map before emitting
  // it to SPIR-V. The map contains a vector of SPIR-V builtin types already
  // emitted for a given type opcode.
  SpecialInstrMapTy BuiltinTypeMap;

  // Some SPIR-V types and constants have no explicit analogues in LLVM types
  // and constants. We create and store these special types and constants in
  // this map to avoid type/const duplicatoin and to hoist the instructions
  // properly to MB_TypeConstVars in GlobalTypesAndRegNumPass.
  // Currently it holds OpTypeSampledImage and OpConstantSampler instances.
  SpecialInstrMapTy SpecialTypesAndConstsMap;

  // Look for an equivalent of the newType in the map. Return the equivalent
  // if it's found, otherwise insert newType to the map and return the type.
  const MachineInstr *checkSpecialInstrMap(const MachineInstr *NewInstr,
                                           SpecialInstrMapTy &InstrMap);

  // Number of bits pointers and size_t integers require.
  const unsigned int PointerSize;

  // Add a new OpTypeXXX instruction without checking for duplicates
  SPIRVType *createSPIRVType(const Type *Type, MachineIRBuilder &MIRBuilder,
                             AQ::AccessQualifier accessQual = AQ::ReadWrite,
                             bool EmitIR = true);

  const MachineFunction *MetaMF;

  // Maps a local register to the corresponding global aliases in meta-function.
  using LocalToGlobalRegTable = std::map<Register, Register>;
  using RegisterAliasMapTy =
      std::map<const MachineFunction *, LocalToGlobalRegTable>;

  // The table contains global aliases of local registers for each machine
  // function. They are collected when local instructions are moved to global
  // level on SPIRVGlobalTypesAndRegNumPass. Then the aliases are used
  // to substitute global aliases during code emission.
  RegisterAliasMapTy RegisterAliasTable;

  // The set contains machine instructions which are necessary for correct MIR
  // but will not be finally emitted.
  DenseSet<MachineInstr *> InstrsToDelete;

public:
  void setMetaMF(const MachineFunction *MF) { MetaMF = MF; }
  const MachineFunction *getMetaMF() { return MetaMF; }

  void setRegisterAlias(const MachineFunction *MF, Register Reg,
                        Register AliasReg) {
    RegisterAliasTable[MF][Reg] = AliasReg;
  }

  Register getRegisterAlias(const MachineFunction *MF, Register Reg) {
    auto RI = RegisterAliasTable[MF].find(Reg);
    if (RI == RegisterAliasTable[MF].end()) {
      return Register(0);
    }
    return RegisterAliasTable[MF][Reg];
  }

  bool hasRegisterAlias(const MachineFunction *MF, Register Reg) {
    assert(RegisterAliasTable.find(MF) != RegisterAliasTable.end());
    return RegisterAliasTable[MF].find(Reg) != RegisterAliasTable[MF].end();
  }

  void setSkipEmission(MachineInstr *MI) { InstrsToDelete.insert(MI); }

  bool getSkipEmission(const MachineInstr *MI) {
    return InstrsToDelete.contains(MI);
  }

  SPIRVGeneralDuplicatesTracker *getDT() const { return &DT; }

  // This interface is for walking the map in GlobalTypesAndRegNumPass.
  SpecialInstrMapTy &getSpecialTypesAndConstsMap() {
    return SpecialTypesAndConstsMap;
  }

  SPIRVGlobalRegistry(SPIRVGeneralDuplicatesTracker &DT,
                      unsigned int PointerSize);

  MachineFunction *CurMF;

  // Get or create a SPIR-V type corresponding the given LLVM IR type,
  // and map it to the given VReg by creating an ASSIGN_TYPE instruction.
  SPIRVType *assignTypeToVReg(const Type *Type, Register VReg,
                              MachineIRBuilder &MIRBuilder,
                              AQ::AccessQualifier AccessQual = AQ::ReadWrite);

  // In cases where the SPIR-V type is already known, this function can be
  // used to map it to the given VReg via an ASSIGN_TYPE instruction.
  void assignSPIRVTypeToVReg(SPIRVType *Type, Register VReg,
                             MachineIRBuilder &MIRBuilder);

  // Either generate a new OpTypeXXX instruction or return an existing one
  // corresponding to the given LLVM IR type.
  // EmitIR controls if we emit GMIR or SPV constants (e.g. for array sizes)
  // because this method may be called from InstructionSelector and we don't
  // want to emit extra IR instructions there
  SPIRVType *
  getOrCreateSPIRVType(const Type *Type, MachineIRBuilder &MIRBuilder,
                       AQ::AccessQualifier accessQual = AQ::ReadWrite,
                       bool EmitIR = true);

  const Type *getTypeForSPIRVType(const SPIRVType *Ty) const {
    auto Res = SPIRVToLLVMType.find(Ty);
    assert(Res != SPIRVToLLVMType.end());
    return Res->second;
  }

  // Either generate a new OpTypeXXX instruction or return an existing one
  // corresponding to the given string containing the name of the builtin type.
  SPIRVType *getOrCreateSPIRVTypeByName(StringRef TypeStr,
                                        MachineIRBuilder &MIRBuilder);

  // Return the SPIR-V type instruction corresponding to the given VReg, or
  // nullptr if no such type instruction exists.
  SPIRVType *getSPIRVTypeForVReg(Register VReg) const;

  // Whether the given VReg has a SPIR-V type mapped to it yet.
  bool hasSPIRVTypeForVReg(Register VReg) const {
    return getSPIRVTypeForVReg(VReg) != nullptr;
  }

  // Return the VReg holding the result of the given OpTypeXXX instruction
  Register getSPIRVTypeID(const SPIRVType *SpirvType) const {
    assert(SpirvType && "Attempting to get type id for nullptr type.");
    return SpirvType->defs().begin()->getReg();
  }

  void setCurrentFunc(MachineFunction &MF) { CurMF = &MF; }

  // Whether the given VReg has an OpTypeXXX instruction mapped to it with the
  // given opcode (e.g. OpTypeFloat).
  bool isScalarOfType(Register VReg, unsigned int TypeOpcode) const;

  // Return true if the given VReg's assigned SPIR-V type is either a scalar
  // matching the given opcode, or a vector with an element type matching that
  // opcode (e.g. OpTypeBool, or OpTypeVector %x 4, where %x is OpTypeBool).
  bool isScalarOrVectorOfType(Register VReg, unsigned int TypeOpcode) const;

  // For vectors or scalars of ints or floats, return the scalar type's bitwidth
  unsigned getScalarOrVectorBitWidth(const SPIRVType *Type) const;

  // For integer vectors or scalars, return whether the integers are signed
  bool isScalarOrVectorSigned(const SPIRVType *Type) const;

  // Gets the storage class of the pointer type assigned to this vreg
  StorageClass::StorageClass getPointerStorageClass(Register VReg) const;

  // Return the number of bits SPIR-V pointers and size_t variables require.
  unsigned getPointerSize() const { return PointerSize; }

private:
  // Internal methods for creating types which are unsafe in duplications
  // check sense hence can only be called within getOrCreateSPIRVType callstack
  SPIRVType *getSamplerType(MachineIRBuilder &MIRBuilder);

  SPIRVType *getSampledImageType(SPIRVType *ImageType,
                                 MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeBool(MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeInt(uint32_t Width, MachineIRBuilder &MIRBuilder,
                          bool IsSigned = false);

  SPIRVType *getOpTypeFloat(uint32_t Width, MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeVoid(MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeVector(uint32_t NumElems, SPIRVType *ElemType,
                             MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeArray(uint32_t NumElems, SPIRVType *ElemType,
                            MachineIRBuilder &MIRBuilder, bool EmitIR = true);

  SPIRVType *getOpTypeOpaque(const StructType *Ty,
                             MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeStruct(const StructType *Ty, MachineIRBuilder &MIRBuilder,
                             bool EmitIR = true);

  SPIRVType *getOpTypePointer(StorageClass::StorageClass SC,
                              SPIRVType *ElemType,
                              MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeFunction(SPIRVType *RetType,
                               const SmallVectorImpl<SPIRVType *> &ArgTypes,
                               MachineIRBuilder &MIRBuilder);

  SPIRVType *getOpTypeImage(MachineIRBuilder &MIRBuilder,
                            SPIRVType *SampledType, Dim::Dim Dim,
                            uint32_t Depth, uint32_t Arrayed,
                            uint32_t Multisampled, uint32_t Sampled,
                            ImageFormat::ImageFormat ImageFormat,
                            AQ::AccessQualifier AccQual);

  SPIRVType *getOpTypePipe(MachineIRBuilder &MIRBuilder,
                           AQ::AccessQualifier AccQual);

  SPIRVType *getOpTypeByOpcode(MachineIRBuilder &MIRBuilder,
                               unsigned int Opcode);

  SPIRVType *handleOpenCLBuiltin(const StructType *Ty,
                                 MachineIRBuilder &MIRBuilder,
                                 AQ::AccessQualifier AccQual);

  SPIRVType *
  generateOpenCLOpaqueType(const StructType *Ty, MachineIRBuilder &MIRBuilder,
                           AQ::AccessQualifier AccQual = AQ::ReadWrite);

  SPIRVType *handleSPIRVBuiltin(const StructType *Ty,
                                MachineIRBuilder &MIRBuilder,
                                AQ::AccessQualifier AccQual);

  SPIRVType *
  generateSPIRVOpaqueType(const StructType *Ty, MachineIRBuilder &MIRBuilder,
                          AQ::AccessQualifier AccQual = AQ::ReadWrite);

public:
  Register buildConstantInt(uint64_t Val, MachineIRBuilder &MIRBuilder,
                            SPIRVType *SpvType = nullptr, bool EmitIR = true);
  Register buildConstantFP(APFloat Val, MachineIRBuilder &MIRBuilder,
                           SPIRVType *SpvType = nullptr);
  Register buildConstantIntVector(uint64_t Val, MachineIRBuilder &MIRBuilder,
                                  SPIRVType *SpvType);
  Register buildConstantSampler(Register Res, unsigned int AddrMode,
                                unsigned int Param, unsigned int FilerMode,
                                MachineIRBuilder &MIRBuilder,
                                SPIRVType *SpvType);
  Register
  buildGlobalVariable(Register Reg, SPIRVType *BaseType, StringRef Name,
                      const GlobalValue *GV, StorageClass::StorageClass Storage,
                      const MachineInstr *Init, bool IsConst, bool HasLinkageTy,
                      LinkageType::LinkageType LinkageType,
                      MachineIRBuilder &MIRBuilder, bool IsInstSelector);

  // Convenient helpers for getting types with check for duplicates.
  SPIRVType *getOrCreateSPIRVIntegerType(unsigned BitWidth,
                                         MachineIRBuilder &MIRBuilder);
  SPIRVType *getOrCreateSPIRVBoolType(MachineIRBuilder &MIRBuilder);
  SPIRVType *getOrCreateSPIRVVectorType(SPIRVType *BaseType,
                                        unsigned NumElements,
                                        MachineIRBuilder &MIRBuilder);
  SPIRVType *getOrCreateSPIRVPointerType(
      SPIRVType *BaseType, MachineIRBuilder &MIRBuilder,
      StorageClass::StorageClass SClass = StorageClass::Function);
  SPIRVType *getOrCreateSPIRVSampledImageType(SPIRVType *ImageType,
                                              MachineIRBuilder &MIRBuilder);
};
} // end namespace llvm
#endif // LLLVM_LIB_TARGET_SPIRV_SPIRVTYPEMANAGER_H
