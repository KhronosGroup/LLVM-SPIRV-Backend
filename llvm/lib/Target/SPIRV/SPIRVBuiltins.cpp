//===- SPIRVBuiltins.cpp - SPIR-V Built-in Functions ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements lowering builtin function calls and types using their
// demangled names and TableGen records.
//
//===----------------------------------------------------------------------===//

#include "SPIRVBuiltins.h"
#include "SPIRV.h"
#include "SPIRVUtils.h"

#include "llvm/IR/IntrinsicsSPIRV.h"

#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace llvm;
using namespace SPIRV;

#define DEBUG_TYPE "spirv-builtins"

namespace {
#define GET_InstructionSet_DECL
#define GET_BuiltinGroup_DECL
#include "SPIRVGenTables.inc"

struct DemangledBuiltin {
  StringRef Name;
  InstructionSet Set;
  BuiltinGroup Group;
  uint8_t MinNumArgs;
  uint8_t MaxNumArgs;
};

#define GET_DemangledBuiltins_DECL
#define GET_DemangledBuiltins_IMPL

struct IncomingCall {
  const std::string BuiltinName;
  const DemangledBuiltin *Builtin;

  const Register ReturnRegister;
  const SPIRVType *ReturnType;
  const SmallVectorImpl<Register> &Arguments;

  IncomingCall(const std::string BuiltinName, const DemangledBuiltin *Builtin,
               const Register ReturnRegister, const SPIRVType *ReturnType,
               const SmallVectorImpl<Register> &Arguments)
      : BuiltinName(BuiltinName), Builtin(Builtin),
        ReturnRegister(ReturnRegister), ReturnType(ReturnType),
        Arguments(Arguments) {}
};

struct ExtendedBuiltin {
  StringRef Name;
  InstructionSet Set;
  uint32_t Number;
};

#define GET_ExtendedBuiltins_DECL
#define GET_ExtendedBuiltins_IMPL

struct NativeBuiltin {
  StringRef Name;
  InstructionSet Set;
  uint32_t Opcode;
};

#define GET_NativeBuiltins_DECL
#define GET_NativeBuiltins_IMPL

struct GroupBuiltin {
  StringRef Name;
  uint32_t Opcode;
  uint32_t GroupOperation;
  bool IsElect;
  bool IsAllOrAny;
  bool IsAllEqual;
  bool IsBallot;
  bool IsInverseBallot;
  bool IsBallotBitExtract;
  bool IsBallotFindBit;
  bool IsLogical;
  bool NoGroupOperation;
  bool HasBoolArg;
};

#define GET_GroupBuiltins_DECL
#define GET_GroupBuiltins_IMPL

struct GetBuiltin {
  StringRef Name;
  InstructionSet Set;
  BuiltIn::BuiltIn Value;
};

using namespace BuiltIn;
#define GET_GetBuiltins_DECL
#define GET_GetBuiltins_IMPL

struct ImageQueryBuiltin {
  StringRef Name;
  InstructionSet Set;
  uint32_t Component;
};

#define GET_ImageQueryBuiltins_DECL
#define GET_ImageQueryBuiltins_IMPL

struct ConvertBuiltin {
  StringRef Name;
  InstructionSet Set;
  bool IsDestinationSigned;
  bool IsSaturated;
  bool IsRounded;
  ::FPRoundingMode::FPRoundingMode RoundingMode;
};

using namespace FPRoundingMode;
#define GET_ConvertBuiltins_DECL
#define GET_ConvertBuiltins_IMPL

struct VectorLoadStoreBuiltin {
  StringRef Name;
  InstructionSet Set;
  uint32_t Number;
  bool IsRounded;
  ::FPRoundingMode::FPRoundingMode RoundingMode;
};

#define GET_VectorLoadStoreBuiltins_DECL
#define GET_VectorLoadStoreBuiltins_IMPL

#define GET_CLMemoryScope_DECL
#define GET_CLSamplerAddressingMode_DECL
#define GET_CLMemoryFenceFlags_DECL
#include "SPIRVGenTables.inc"

} // end anonymous namespace

//===----------------------------------------------------------------------===//
// Misc functions for looking up builtins and veryfying requirements using
// TableGen records
//===----------------------------------------------------------------------===//

/// Looks up the demangled builtin call in the SPIRVBuiltins.td records using
/// the provided \p DemangledCall and specified \p Set.
///
/// The lookup follows the following algorithm, returning the first successful
/// match:
/// 1. Search with the plain demangled name (expecting a 1:1 match).
/// 2. Search with the prefix before or sufix after the demangled name
/// signyfying the type of the first argument.
///
/// \returns Wrapper around the demangled call and found builtin definition.
static std::unique_ptr<const IncomingCall>
lookupBuiltin(StringRef DemangledCall,
              ExternalInstructionSet::InstructionSet Set,
              Register ReturnRegister, const SPIRVType *ReturnType,
              const SmallVectorImpl<Register> &Arguments) {
  // Extract the builtin function name and types of arguments from the call
  // skeleton.
  std::string BuiltinName =
      DemangledCall.substr(0, DemangledCall.find('(')).str();

  // Check if the extracted name contains type information between angle
  // brackets. If so, the builtin is an instantiated template - needs to have
  // the information after angle brackets and return type removed.
  if (BuiltinName.find('<') && BuiltinName.back() == '>') {
    BuiltinName = BuiltinName.substr(0, BuiltinName.find('<'));
    BuiltinName = BuiltinName.substr(BuiltinName.find_last_of(" ") + 1);
  }

  // Check if the extracted name begins with "__spirv_ImageSampleExplicitLod"
  // contains return type information at the end "_R<type>", if so extract the
  // plain builtin name without the type information.
  if (StringRef(BuiltinName).contains("__spirv_ImageSampleExplicitLod") &&
      StringRef(BuiltinName).contains("_R")) {
    BuiltinName = BuiltinName.substr(0, BuiltinName.find("_R"));
  }

  std::vector<std::string> BuiltinArgumentTypes;
  {
    std::stringstream Stream(
        DemangledCall.substr(DemangledCall.find('(') + 1).drop_back(1).str());
    std::string DemangledArgument;
    while (std::getline(Stream, DemangledArgument, ',')) {
      BuiltinArgumentTypes.push_back(StringRef(DemangledArgument).trim().str());
    }
  }

  // Look up the builtin in the defined set. Start with the plain demangled
  // name, expecting a 1:1 match in the defined builtin set.
  if (const DemangledBuiltin *Builtin = lookupBuiltin(BuiltinName, Set))
    return std::make_unique<IncomingCall>(BuiltinName, Builtin, ReturnRegister,
                                          ReturnType, Arguments);

  // If the initial look up was unsuccessful and the demangled call takes at
  // least 1 argument, add a prefix or sufix signifying the type of the first
  // argument and repeat the search.
  if (BuiltinArgumentTypes.size() >= 1) {
    char FirstArgumentType = BuiltinArgumentTypes[0][0];
    // Prefix to be added to the builtin's name for lookup.
    // For example, OpenCL "abs" taking an unsigned value has a prefix "u_".
    std::string Prefix;

    switch (FirstArgumentType) {
    // Unsigned
    case 'u':
      if (Set == ExternalInstructionSet::OpenCL_std)
        Prefix = "u_";
      else if (Set == ExternalInstructionSet::GLSL_std_450)
        Prefix = "u";
      break;
    // Signed
    case 'c':
    case 's':
    case 'i':
    case 'l':
      if (Set == ExternalInstructionSet::OpenCL_std)
        Prefix = "s_";
      else if (Set == ExternalInstructionSet::GLSL_std_450)
        Prefix = "s";
      break;
    // Floating-point
    case 'f':
    case 'd':
    case 'h':
      if (Set == ExternalInstructionSet::OpenCL_std ||
          Set == ExternalInstructionSet::GLSL_std_450)
        Prefix = "f";
      break;
    }

    // If argument-type name prefix was added, look up the builtin again.
    if (!Prefix.empty()) {
      const DemangledBuiltin *Builtin;
      if ((Builtin = lookupBuiltin(Prefix + BuiltinName, Set)))
        return std::make_unique<IncomingCall>(
            BuiltinName, Builtin, ReturnRegister, ReturnType, Arguments);
    }

    // If lookup with a prefix failed, find a sufix to be added to the builtin's
    // name for lookup. For example, OpenCL "group_reduce_max" taking an
    // unsigned value has a sufix "u".
    std::string Sufix;

    switch (FirstArgumentType) {
    // Unsigned
    case 'u':
      Sufix = "u";
      break;
    // Signed
    case 'c':
    case 's':
    case 'i':
    case 'l':
      Sufix = "s";
      break;
    // Floating-point
    case 'f':
    case 'd':
    case 'h':
      Sufix = "f";
      break;
    }

    // If argument-type name sufix was added, look up the builtin again.
    if (!Prefix.empty()) {
      const DemangledBuiltin *Builtin;
      if ((Builtin = lookupBuiltin(BuiltinName + Sufix, Set)))
        return std::make_unique<IncomingCall>(
            BuiltinName, Builtin, ReturnRegister, ReturnType, Arguments);
    }
  }

  // No builtin with such name was found in the set.
  return nullptr;
}

/// Verify if the provided \p Arguments meet the requirments of the given \p
/// Builtin.
static bool verifyBuiltinArgs(const DemangledBuiltin *Builtin,
                              const SmallVectorImpl<Register> &Arguments) {
  assert(Arguments.size() >= Builtin->MinNumArgs &&
         "Too little arguments to generate the builtin");
  if (Builtin->MaxNumArgs && Arguments.size() <= Builtin->MaxNumArgs)
    LLVM_DEBUG(dbgs() << "More arguments provided than required!");

  return true;
}

//===----------------------------------------------------------------------===//
// Helper functions for building misc instructions
//===----------------------------------------------------------------------===//

/// Helper function building either a resulting scalar or vector bool register
/// depending on the expected \p ResultType.
///
/// \returns Tuple of the resulting register and its type.
static std::tuple<Register, SPIRVType *>
buildBoolRegister(MachineIRBuilder &MIRBuilder, const SPIRVType *ResultType,
                  SPIRVGlobalRegistry *GR) {
  LLT Type;
  SPIRVType *BoolType = GR->getOrCreateSPIRVBoolType(MIRBuilder);

  if (ResultType->getOpcode() == OpTypeVector) {
    unsigned VectorElements = ResultType->getOperand(2).getImm();
    BoolType =
        GR->getOrCreateSPIRVVectorType(BoolType, VectorElements, MIRBuilder);
    auto LLVMVectorType =
        cast<FixedVectorType>(GR->getTypeForSPIRVType(BoolType));
    Type = LLT::vector(LLVMVectorType->getElementCount(), 1);
  } else {
    Type = LLT::scalar(1);
  }

  Register ResultRegister =
      MIRBuilder.getMRI()->createGenericVirtualRegister(Type);
  GR->assignSPIRVTypeToVReg(BoolType, ResultRegister, MIRBuilder.getMF());
  return std::make_tuple(ResultRegister, BoolType);
}

/// Helper function for building either a vector or scalar select instruction
/// depending on the expected \p ResultType.
static bool buildSelectInst(MachineIRBuilder &MIRBuilder,
                            Register ReturnRegister, Register SourceRegister,
                            const SPIRVType *ReturnType,
                            SPIRVGlobalRegistry *GR) {
  Register TrueConst;
  Register FalseConst;
  unsigned Bits = GR->getScalarOrVectorBitWidth(ReturnType);

  if (ReturnType->getOpcode() == OpTypeVector) {
    auto AllOnes = APInt::getAllOnesValue(Bits).getZExtValue();
    TrueConst = GR->buildConstantIntVector(AllOnes, MIRBuilder, ReturnType);
    FalseConst = GR->buildConstantIntVector(0, MIRBuilder, ReturnType);
  } else {
    TrueConst = GR->buildConstantInt(1, MIRBuilder, ReturnType);
    FalseConst = GR->buildConstantInt(0, MIRBuilder, ReturnType);
  }

  return MIRBuilder.buildSelect(ReturnRegister, SourceRegister, TrueConst,
                                FalseConst);
}

/// Helper function for building a load instruction loading into the
/// \p DestinationReg.
static Register buildLoadInst(SPIRVType *BaseType, Register PtrRegister,
                              MachineIRBuilder &MIRBuilder,
                              SPIRVGlobalRegistry *GR, LLT LowLevelType,
                              Register DestinationReg = Register(0)) {
  const auto MRI = MIRBuilder.getMRI();
  if (!DestinationReg.isValid()) {
    DestinationReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
    MRI->setType(DestinationReg, LLT::scalar(32));
    GR->assignSPIRVTypeToVReg(BaseType, DestinationReg, MIRBuilder.getMF());
  }

  // TODO: consider using correct address space and alignment
  // p0 is canonical type for selection though
  MachinePointerInfo PtrInfo = MachinePointerInfo();
  MIRBuilder.buildLoad(DestinationReg, PtrRegister, PtrInfo, Align());
  return DestinationReg;
}

/// Helper function for building a load instruction for loading a builtin global
/// variable of \p BuiltinValue value.
static Register buildBuiltinVariableLoad(MachineIRBuilder &MIRBuilder,
                                         SPIRVType *VariableType,
                                         SPIRVGlobalRegistry *GR,
                                         ::BuiltIn::BuiltIn BuiltinValue,
                                         LLT LLType,
                                         Register Reg = Register(0)) {
  Register NewRegister =
      MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  MIRBuilder.getMRI()->setType(NewRegister,
                               LLT::pointer(0, GR->getPointerSize()));
  SPIRVType *PtrType = GR->getOrCreateSPIRVPointerType(VariableType, MIRBuilder,
                                                       StorageClass::Input);
  GR->assignSPIRVTypeToVReg(PtrType, NewRegister, MIRBuilder.getMF());

  // Set up the global OpVariable with the necessary builtin decorations
  Register Variable = GR->buildGlobalVariable(
      NewRegister, PtrType, getLinkStringForBuiltIn(BuiltinValue), nullptr,
      StorageClass::Input, nullptr, true, true, LinkageType::Import, MIRBuilder,
      false);

  // Load the value from the global variable
  Register LoadedRegister =
      buildLoadInst(VariableType, Variable, MIRBuilder, GR, LLType, Reg);
  MIRBuilder.getMRI()->setType(LoadedRegister, LLType);
  return LoadedRegister;
}

/// Helper external function for inserting ASSIGN_TYPE instuction between \p Reg
/// and its definition, set the new register as a destination of the definition,
/// assign SPIRVType to both registers. If SpirvTy is provided, use it as
/// SPIRVType in ASSIGN_TYPE, otherwise create it from \p Ty. Defined in
/// SPIRVIRTranslator.cpp.
extern Register insertAssignInstr(Register Reg, Type *Ty, SPIRVType *SpirvTy,
                                  SPIRVGlobalRegistry *GR,
                                  MachineIRBuilder &MIB,
                                  MachineRegisterInfo &MRI);

// TODO: Move to TableGen
static MemorySemantics::MemorySemantics
getSPIRVMemSemantics(std::memory_order MemOrder) {
  switch (MemOrder) {
  case std::memory_order::memory_order_relaxed:
    return MemorySemantics::None;
  case std::memory_order::memory_order_acquire:
    return MemorySemantics::Acquire;
  case std::memory_order::memory_order_release:
    return MemorySemantics::Release;
  case std::memory_order::memory_order_acq_rel:
    return MemorySemantics::AcquireRelease;
  case std::memory_order::memory_order_seq_cst:
    return MemorySemantics::SequentiallyConsistent;
  default:
    llvm_unreachable("Unknown CL memory scope");
  }
}

static Scope::Scope getSPIRVScope(CLMemoryScope clScope) {
  switch (clScope) {
  case memory_scope_work_item:
    return Scope::Invocation;
  case memory_scope_work_group:
    return Scope::Workgroup;
  case memory_scope_device:
    return Scope::Device;
  case memory_scope_all_svm_devices:
    return Scope::CrossDevice;
  case memory_scope_sub_group:
    return Scope::Subgroup;
  }
  llvm_unreachable("Unknown CL memory scope");
}

/// Helper function for building an atomic load instruction.
static bool buildAtomicLoadInst(const IncomingCall *Call,
                                MachineIRBuilder &MIRBuilder,
                                SPIRVGlobalRegistry *GR) {
  SPIRVType *Int32Type = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  Register PtrRegister = Call->Arguments[0];
  Register ScopeRegister;

  if (Call->Arguments.size() > 1) {
    // TODO: Insert call to __translate_ocl_memory_sccope before OpAtomicLoad
    // and the function implementation. We can use Translator's output for
    // transcoding/atomic_explicit_arguments.cl as an example.
    ScopeRegister = Call->Arguments[1];
  } else {
    Scope::Scope Scope = Scope::Device;
    ScopeRegister = GR->buildConstantInt(Scope, MIRBuilder, Int32Type);
  }

  Register MemSemanticsReg;
  if (Call->Arguments.size() > 2) {
    // TODO: Insert call to __translate_ocl_memory_order before OpAtomicLoad.
    MemSemanticsReg = Call->Arguments[2];
  } else {
    int Semantics =
        MemorySemantics::SequentiallyConsistent |
        getMemSemanticsForStorageClass(GR->getPointerStorageClass(PtrRegister));
    MemSemanticsReg = GR->buildConstantInt(Semantics, MIRBuilder, Int32Type);
  }

  auto MIB = MIRBuilder.buildInstr(OpAtomicLoad)
                 .addDef(Call->ReturnRegister)
                 .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                 .addUse(PtrRegister)
                 .addUse(ScopeRegister)
                 .addUse(MemSemanticsReg);

  return constrainRegOperands(MIB);
}

/// Helper function for building an atomic store instruction.
static bool buildAtomicStoreInst(const IncomingCall *Call,
                                 MachineIRBuilder &MIRBuilder,
                                 SPIRVGlobalRegistry *GR) {
  SPIRVType *Int32Type = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  Register ScopeRegister;
  Scope::Scope Scope = Scope::Device;
  if (!ScopeRegister.isValid())
    ScopeRegister = GR->buildConstantInt(Scope, MIRBuilder, Int32Type);

  Register PtrRegister = Call->Arguments[0];
  Register MemSemanticsReg;

  int Semantics =
      MemorySemantics::SequentiallyConsistent |
      getMemSemanticsForStorageClass(GR->getPointerStorageClass(PtrRegister));

  if (!MemSemanticsReg.isValid())
    MemSemanticsReg = GR->buildConstantInt(Semantics, MIRBuilder, Int32Type);

  auto MIB = MIRBuilder.buildInstr(OpAtomicStore)
                 .addUse(PtrRegister)
                 .addUse(ScopeRegister)
                 .addUse(MemSemanticsReg)
                 .addUse(Call->Arguments[1]);
  return constrainRegOperands(MIB);
}

/// Helper function for building an atomic compare-exchange instruction.
static bool buildAtomicCompareExchangeInst(const IncomingCall *Call,
                                           MachineIRBuilder &MIRBuilder,
                                           SPIRVGlobalRegistry *GR) {
  const DemangledBuiltin *Builtin = Call->Builtin;
  unsigned Opcode = lookupNativeBuiltin(Builtin->Name, Builtin->Set)->Opcode;

  bool isCmpxchg = Call->Builtin->Name.contains("cmpxchg");
  const auto MRI = MIRBuilder.getMRI();

  Register objectPtr = Call->Arguments[0];   // Pointer (volatile A *object)
  Register expectedArg = Call->Arguments[1]; // Comparator (C* expected)
  Register desired = Call->Arguments[2];     // Value (C desired)
  SPIRVType *spvDesiredTy = GR->getSPIRVTypeForVReg(desired);
  LLT desiredLLT = MRI->getType(desired);

  assert(GR->getSPIRVTypeForVReg(objectPtr)->getOpcode() == OpTypePointer);
  auto expectedType = GR->getSPIRVTypeForVReg(expectedArg)->getOpcode();
  assert(isCmpxchg ? expectedType == OpTypeInt : expectedType == OpTypePointer);
  assert(GR->isScalarOfType(desired, OpTypeInt));

  SPIRVType *spvObjectPtrTy = GR->getSPIRVTypeForVReg(objectPtr);
  assert(spvObjectPtrTy->getOperand(2).isReg() && "SPIRV type is expected");
  auto storageClass = static_cast<StorageClass::StorageClass>(
      spvObjectPtrTy->getOperand(1).getImm());
  auto memSemStorage = getMemSemanticsForStorageClass(storageClass);

  Register memSemEqualReg;
  Register memSemUnequalReg;
  auto memSemEqual =
      isCmpxchg ? MemorySemantics::None
                : MemorySemantics::SequentiallyConsistent | memSemStorage;
  auto memSemUnequal =
      isCmpxchg ? MemorySemantics::None
                : MemorySemantics::SequentiallyConsistent | memSemStorage;
  if (Call->Arguments.size() >= 4) {
    assert(Call->Arguments.size() >= 5 &&
           "Need 5+ args for explicit atomic cmpxchg");
    auto memOrdEq =
        static_cast<std::memory_order>(getIConstVal(Call->Arguments[3], MRI));
    auto memOrdNeq =
        static_cast<std::memory_order>(getIConstVal(Call->Arguments[4], MRI));
    memSemEqual = getSPIRVMemSemantics(memOrdEq) | memSemStorage;
    memSemUnequal = getSPIRVMemSemantics(memOrdNeq) | memSemStorage;
    if (memOrdEq == memSemEqual)
      memSemEqualReg = Call->Arguments[3];
    if (memOrdNeq == memSemEqual)
      memSemUnequalReg = Call->Arguments[4];
  }
  auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  if (!memSemEqualReg.isValid())
    memSemEqualReg = GR->buildConstantInt(memSemEqual, MIRBuilder, I32Ty);
  if (!memSemUnequalReg.isValid())
    memSemUnequalReg = GR->buildConstantInt(memSemUnequal, MIRBuilder, I32Ty);

  Register scopeReg;
  auto scope = isCmpxchg ? Scope::Workgroup : Scope::Device;
  if (Call->Arguments.size() >= 6) {
    assert(Call->Arguments.size() == 6 &&
           "Extra args for explicit atomic cmpxchg");
    auto clScope =
        static_cast<CLMemoryScope>(getIConstVal(Call->Arguments[5], MRI));
    scope = getSPIRVScope(clScope);
    if (clScope == static_cast<unsigned>(scope))
      scopeReg = Call->Arguments[5];
  }
  if (!scopeReg.isValid())
    scopeReg = GR->buildConstantInt(scope, MIRBuilder, I32Ty);

  Register expected = isCmpxchg
                          ? expectedArg
                          : buildLoadInst(spvDesiredTy, expectedArg, MIRBuilder,
                                          GR, LLT::scalar(32));
  MRI->setType(expected, desiredLLT);
  Register tmp = !isCmpxchg ? MRI->createGenericVirtualRegister(desiredLLT)
                            : Call->ReturnRegister;
  GR->assignSPIRVTypeToVReg(spvDesiredTy, tmp, MIRBuilder.getMF());

  auto MIB = MIRBuilder.buildInstr(Opcode)
                 .addDef(tmp)
                 .addUse(GR->getSPIRVTypeID(
                     GR->getOrCreateSPIRVIntegerType(32, MIRBuilder)))
                 .addUse(objectPtr)
                 .addUse(scopeReg)
                 .addUse(memSemEqualReg)
                 .addUse(memSemUnequalReg)
                 .addUse(desired)
                 .addUse(expected);
  if (!isCmpxchg) {
    MIRBuilder.buildInstr(OpStore).addUse(expectedArg).addUse(tmp);
    MIRBuilder.buildICmp(CmpInst::ICMP_EQ, Call->ReturnRegister, tmp, expected);
  }
  return constrainRegOperands(MIB);
}

/// Helper function for building an atomic load instruction.
static bool buildAtomicRMWInst(const IncomingCall *Call, unsigned Opcode,
                               MachineIRBuilder &MIRBuilder,
                               SPIRVGlobalRegistry *GR) {
  const auto MRI = MIRBuilder.getMRI();
  auto Int32Type = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);

  Register ScopeRegister;
  Scope::Scope Scope = Scope::Workgroup;
  if (Call->Arguments.size() >= 4) {
    assert(Call->Arguments.size() == 4 && "Extra args for explicit atomic RMW");
    CLMemoryScope CLScope =
        static_cast<CLMemoryScope>(getIConstVal(Call->Arguments[5], MRI));
    Scope = getSPIRVScope(CLScope);
    if (CLScope == static_cast<unsigned>(Scope))
      ScopeRegister = Call->Arguments[5];
  }
  if (!ScopeRegister.isValid())
    ScopeRegister = GR->buildConstantInt(Scope, MIRBuilder, Int32Type);

  Register PtrRegister = Call->Arguments[0];

  Register MemSemanticsReg;
  unsigned Semantics = MemorySemantics::None;
  if (Call->Arguments.size() >= 3) {
    std::memory_order Order =
        static_cast<std::memory_order>(getIConstVal(Call->Arguments[2], MRI));
    Semantics =
        getSPIRVMemSemantics(Order) |
        getMemSemanticsForStorageClass(GR->getPointerStorageClass(PtrRegister));
    if (Order == Semantics)
      MemSemanticsReg = Call->Arguments[3];
  }
  if (!MemSemanticsReg.isValid())
    MemSemanticsReg = GR->buildConstantInt(Semantics, MIRBuilder, Int32Type);

  auto MIB = MIRBuilder.buildInstr(Opcode)
                 .addDef(Call->ReturnRegister)
                 .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                 .addUse(PtrRegister)
                 .addUse(ScopeRegister)
                 .addUse(MemSemanticsReg)
                 .addUse(Call->Arguments[1]);
  return constrainRegOperands(MIB);
}

/// Helper function for building barriers, i.e., memory/control ordering
/// operations.
static bool buildBarrierInst(const IncomingCall *Call, unsigned Opcode,
                             MachineIRBuilder &MIRBuilder,
                             SPIRVGlobalRegistry *GR) {
  const auto MRI = MIRBuilder.getMRI();
  const auto Int32Type = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);

  unsigned MemFlags = getIConstVal(Call->Arguments[0], MRI);
  unsigned MemSemantics = MemorySemantics::None;

  if (MemFlags & CLK_LOCAL_MEM_FENCE)
    MemSemantics |= MemorySemantics::WorkgroupMemory;

  if (MemFlags & CLK_GLOBAL_MEM_FENCE)
    MemSemantics |= MemorySemantics::CrossWorkgroupMemory;

  if (MemFlags & CLK_IMAGE_MEM_FENCE)
    MemSemantics |= MemorySemantics::ImageMemory;

  if (Opcode == OpMemoryBarrier) {
    std::memory_order MemOrder =
        static_cast<std::memory_order>(getIConstVal(Call->Arguments[1], MRI));
    MemSemantics = getSPIRVMemSemantics(MemOrder) | MemSemantics;
  } else {
    MemSemantics |= MemorySemantics::SequentiallyConsistent;
  }

  Register MemSemanticsReg;
  if (MemFlags == MemSemantics)
    MemSemanticsReg = Call->Arguments[0];
  else
    MemSemanticsReg = GR->buildConstantInt(MemSemantics, MIRBuilder, Int32Type);

  Register ScopeReg;
  Scope::Scope Scope = Scope::Workgroup;
  Scope::Scope MemScope = Scope;
  if (Call->Arguments.size() >= 2) {
    assert(((Opcode != OpMemoryBarrier && Call->Arguments.size() == 2) ||
            (Opcode == OpMemoryBarrier && Call->Arguments.size() == 3)) &&
           "Extra args for explicitly scoped barrier");
    Register ScopeArg =
        (Opcode == OpMemoryBarrier) ? Call->Arguments[2] : Call->Arguments[1];
    CLMemoryScope CLScope =
        static_cast<CLMemoryScope>(getIConstVal(ScopeArg, MRI));
    MemScope = getSPIRVScope(CLScope);
    if (!(MemFlags & CLK_LOCAL_MEM_FENCE) || (Opcode == OpMemoryBarrier))
      Scope = MemScope;

    if (CLScope == static_cast<unsigned>(Scope))
      ScopeReg = Call->Arguments[1];
  }

  if (!ScopeReg.isValid())
    ScopeReg = GR->buildConstantInt(Scope, MIRBuilder, Int32Type);

  auto MIB = MIRBuilder.buildInstr(Opcode).addUse(ScopeReg);
  if (Opcode != OpMemoryBarrier)
    MIB.addUse(GR->buildConstantInt(MemScope, MIRBuilder, Int32Type));
  MIB.addUse(MemSemanticsReg);
  return constrainRegOperands(MIB);
}

static unsigned getNumComponentsForDim(Dim::Dim dim) {
  using namespace Dim;
  switch (dim) {
  case DIM_1D:
  case DIM_Buffer:
    return 1;
  case DIM_2D:
  case DIM_Cube:
  case DIM_Rect:
    return 2;
  case DIM_3D:
    return 3;
  default:
    llvm_unreachable("Cannot get num components for given Dim");
  }
}

/// Helper function for obtaining the number of size components.
static unsigned getNumSizeComponents(SPIRVType *imgType) {
  assert(imgType->getOpcode() == SPIRV::OpTypeImage);
  auto dim = static_cast<Dim::Dim>(imgType->getOperand(2).getImm());
  unsigned numComps = getNumComponentsForDim(dim);
  bool arrayed = imgType->getOperand(4).getImm() == 1;
  return arrayed ? numComps + 1 : numComps;
}

//===----------------------------------------------------------------------===//
// Implementation functions for each builtin group
//===----------------------------------------------------------------------===//

static bool generateExtInst(const IncomingCall *Call,
                            MachineIRBuilder &MIRBuilder,
                            SPIRVGlobalRegistry *GR) {
  // Lookup the extended instruction number in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  uint32_t Number = lookupExtendedBuiltin(Builtin->Name, Builtin->Set)->Number;

  // Build extended instruction.
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                 .addDef(Call->ReturnRegister)
                 .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                 .addImm(static_cast<uint32_t>(::InstructionSet::OpenCL_std))
                 .addImm(Number);

  for (auto Argument : Call->Arguments)
    MIB.addUse(Argument);

  return constrainRegOperands(MIB);
}

static bool generateRelationalInst(const IncomingCall *Call,
                                   MachineIRBuilder &MIRBuilder,
                                   SPIRVGlobalRegistry *GR) {
  // Lookup the instruction opcode in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  unsigned Opcode = lookupNativeBuiltin(Builtin->Name, Builtin->Set)->Opcode;

  Register CompareRegister;
  SPIRVType *RelationType;
  std::tie(CompareRegister, RelationType) =
      buildBoolRegister(MIRBuilder, Call->ReturnType, GR);

  // Build relational instruction.
  auto MIB = MIRBuilder.buildInstr(Opcode)
                 .addDef(CompareRegister)
                 .addUse(GR->getSPIRVTypeID(RelationType));

  for (auto Argument : Call->Arguments) {
    MIB.addUse(Argument);
  }

  if (!constrainRegOperands(MIB))
    return false;

  // Build select instruction.
  return buildSelectInst(MIRBuilder, Call->ReturnRegister, CompareRegister,
                         Call->ReturnType, GR);
}

static bool generateGroupInst(const IncomingCall *Call,
                              MachineIRBuilder &MIRBuilder,
                              SPIRVGlobalRegistry *GR) {
  const DemangledBuiltin *Builtin = Call->Builtin;
  const GroupBuiltin *GroupBuiltin = lookupGroupBuiltin(Builtin->Name);

  const auto MRI = MIRBuilder.getMRI();
  Register Arg0;
  if (GroupBuiltin->HasBoolArg) {
    Register ConstRegister = Call->Arguments[0];
    auto ArgInstruction = getDefInstrMaybeConstant(ConstRegister, MRI);
    // TODO: support non-constant bool values
    assert(ArgInstruction->getOpcode() == TargetOpcode::G_CONSTANT &&
           "Only constant bool value args are supported");
    if (GR->getSPIRVTypeForVReg(Call->Arguments[0])->getOpcode() !=
        OpTypeBool) {
      auto boolTy = GR->getOrCreateSPIRVBoolType(MIRBuilder);
      Arg0 = GR->buildConstantInt(getIConstVal(ConstRegister, MRI), MIRBuilder,
                                  boolTy);
    }
  }

  Register GroupResultRegister = Call->ReturnRegister;
  SPIRVType *GroupResultType = Call->ReturnType;

  // TODO: maybe we need to check whether the result type is already boolean
  // and in this case do not insert select instruction.
  const bool HasBoolReturnTy =
      GroupBuiltin->IsElect || GroupBuiltin->IsAllOrAny ||
      GroupBuiltin->IsAllEqual || GroupBuiltin->IsLogical ||
      GroupBuiltin->IsInverseBallot || GroupBuiltin->IsBallotBitExtract;

  if (HasBoolReturnTy)
    std::tie(GroupResultRegister, GroupResultType) =
        buildBoolRegister(MIRBuilder, Call->ReturnType, GR);

  const auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  auto Scope = Builtin->Name.startswith("sub_group") ? Scope::Subgroup
                                                     : Scope::Workgroup;
  Register ScopeRegister = GR->buildConstantInt(Scope, MIRBuilder, I32Ty);

  // Build work/sub group instruction.
  auto MIB = MIRBuilder.buildInstr(GroupBuiltin->Opcode)
                 .addDef(GroupResultRegister)
                 .addUse(GR->getSPIRVTypeID(GroupResultType))
                 .addUse(ScopeRegister);

  if (!GroupBuiltin->NoGroupOperation) {
    MIB.addImm(GroupBuiltin->GroupOperation);
  }

  if (Call->Arguments.size() > 0) {
    MIB.addUse(Arg0.isValid() ? Arg0 : Call->Arguments[0]);
    for (unsigned i = 1; i < Call->Arguments.size(); i++) {
      MIB.addUse(Call->Arguments[i]);
    }
  }
  constrainRegOperands(MIB);

  // Build select instruction.
  if (HasBoolReturnTy)
    buildSelectInst(MIRBuilder, Call->ReturnRegister, GroupResultRegister,
                    Call->ReturnType, GR);
  return true;
}

// These queries ask for a single size_t result for a given dimension index, e.g
// size_t get_global_id(uintt dimindex). In SPIR-V, the builtins corresonding to
// these values are all vec3 types, so we need to extract the correct index or
// return defaultVal (0 or 1 depending on the query). We also handle extending
// or tuncating in case size_t does not match the expected result type's
// bitwidth.
//
// For a constant index >= 3 we generate:
//  %res = OpConstant %SizeT 0
//
// For other indices we generate:
//  %g = OpVariable %ptr_V3_SizeT Input
//  OpDecorate %g BuiltIn XXX
//  OpDecorate %g LinkageAttributes "__spirv_BuiltInXXX"
//  OpDecorate %g Constant
//  %loadedVec = OpLoad %V3_SizeT %g
//
//  Then, if the index is constant < 3, we generate:
//    %res = OpCompositeExtract %SizeT %loadedVec idx
//  If the index is dynamic, we generate:
//    %tmp = OpVectorExtractDynamic %SizeT %loadedVec %idx
//    %cmp = OpULessThan %bool %idx %const_3
//    %res = OpSelect %SizeT %cmp %tmp %const_0
//
//  If the bitwidth of %res does not match the expected return type, we add an
//  extend or truncate.
//
static bool genWorkgroupQuery(const IncomingCall *Call,
                              MachineIRBuilder &MIRBuilder,
                              SPIRVGlobalRegistry *GR,
                              ::BuiltIn::BuiltIn BuiltinValue,
                              uint64_t DefaultValue) {
  Register IndexRegister = Call->Arguments[0];

  const unsigned ResultWidth = Call->ReturnType->getOperand(1).getImm();
  const unsigned int PointerSize = GR->getPointerSize();

  const auto PointerSizeType =
      GR->getOrCreateSPIRVIntegerType(PointerSize, MIRBuilder);

  const auto MRI = MIRBuilder.getMRI();
  auto IndexInstruction = getDefInstrMaybeConstant(IndexRegister, MRI);

  // Set up the final register to do truncation or extension on at the end.
  Register ToTruncate = Call->ReturnRegister;

  // If the index is constant, we can statically determine if it is in range
  bool IsConstantIndex =
      IndexInstruction->getOpcode() == TargetOpcode::G_CONSTANT;

  // If it's out of range (max dimension is 3), we can just return the constant
  // default value (0 or 1 depending on which query function)
  if (IsConstantIndex && getIConstVal(IndexRegister, MRI) >= 3) {
    Register defaultReg = Call->ReturnRegister;
    if (PointerSize != ResultWidth) {
      defaultReg = MRI->createGenericVirtualRegister(LLT::scalar(PointerSize));
      GR->assignSPIRVTypeToVReg(PointerSizeType, defaultReg,
                                MIRBuilder.getMF());
      ToTruncate = defaultReg;
    }
    auto NewRegister =
        GR->buildConstantInt(DefaultValue, MIRBuilder, PointerSizeType);
    MIRBuilder.buildCopy(defaultReg, NewRegister);
  } else { // If it could be in range, we need to load from the given builtin
    auto Vec3Ty =
        GR->getOrCreateSPIRVVectorType(PointerSizeType, 3, MIRBuilder);
    Register LoadedVector =
        buildBuiltinVariableLoad(MIRBuilder, Vec3Ty, GR, BuiltinValue,
                                 LLT::fixed_vector(3, PointerSize));

    // Set up the vreg to extract the result to (possibly a new temporary one)
    Register Extracted = Call->ReturnRegister;
    if (!IsConstantIndex || PointerSize != ResultWidth) {
      Extracted = MRI->createGenericVirtualRegister(LLT::scalar(PointerSize));
      GR->assignSPIRVTypeToVReg(PointerSizeType, Extracted, MIRBuilder.getMF());
    }

    // Use Intrinsic::spv_extractelt so dynamic vs static extraction is
    // handled later: extr = spv_extractelt LoadedVector, IndexRegister
    MachineInstrBuilder ExtractInst = MIRBuilder.buildIntrinsic(
        Intrinsic::spv_extractelt, ArrayRef<Register>{Extracted}, true);
    ExtractInst.addUse(LoadedVector).addUse(IndexRegister);

    // If the index is dynamic, need check if it's < 3, and then use a select
    if (!IsConstantIndex) {
      insertAssignInstr(Extracted, nullptr, PointerSizeType, GR, MIRBuilder,
                        *MRI);

      auto IndexType = GR->getSPIRVTypeForVReg(IndexRegister);
      auto BoolType = GR->getOrCreateSPIRVBoolType(MIRBuilder);

      Register CompareRegister =
          MRI->createGenericVirtualRegister(LLT::scalar(1));
      GR->assignSPIRVTypeToVReg(BoolType, CompareRegister, MIRBuilder.getMF());

      // Use G_ICMP to check if idxVReg < 3
      MIRBuilder.buildICmp(CmpInst::ICMP_ULT, CompareRegister, IndexRegister,
                           GR->buildConstantInt(3, MIRBuilder, IndexType));

      // Get constant for the default value (0 or 1 depending on which function)
      Register DefaultRegister =
          GR->buildConstantInt(DefaultValue, MIRBuilder, PointerSizeType);

      // Get a register for the selection result (possibly a new temporary one)
      Register SelectionResult = Call->ReturnRegister;
      if (PointerSize != ResultWidth) {
        SelectionResult =
            MRI->createGenericVirtualRegister(LLT::scalar(PointerSize));
        GR->assignSPIRVTypeToVReg(PointerSizeType, SelectionResult,
                                  MIRBuilder.getMF());
      }

      // Create the final G_SELECT to return the extracted value or the default
      MIRBuilder.buildSelect(SelectionResult, CompareRegister, Extracted,
                             DefaultRegister);
      ToTruncate = SelectionResult;
    } else {
      ToTruncate = Extracted;
    }
  }

  // Alter the result's bitwidth if it does not match the SizeT value extracted
  if (PointerSize != ResultWidth) {
    MIRBuilder.buildZExtOrTrunc(Call->ReturnRegister, ToTruncate);
  }

  return true;
}

static bool generateBuiltinVar(const IncomingCall *Call,
                               MachineIRBuilder &MIRBuilder,
                               SPIRVGlobalRegistry *GR) {
  // Lookup the builtin variable record
  const DemangledBuiltin *Builtin = Call->Builtin;
  ::BuiltIn::BuiltIn Value =
      lookupGetBuiltin(Builtin->Name, Builtin->Set)->Value;

  if (Value == ::BuiltIn::GlobalInvocationId)
    return genWorkgroupQuery(Call, MIRBuilder, GR, Value, 0);

  // Build a load instruction for the builtin variable
  unsigned BitWidth = GR->getScalarOrVectorBitWidth(Call->ReturnType);
  LLT LLType;

  if (Call->ReturnType->getOpcode() == SPIRV::OpTypeVector)
    LLType =
        LLT::fixed_vector(Call->ReturnType->getOperand(2).getImm(), BitWidth);
  else
    LLType = LLT::scalar(BitWidth);

  return buildBuiltinVariableLoad(MIRBuilder, Call->ReturnType, GR, Value,
                                  LLType, Call->ReturnRegister);
}

static bool generateAtomicInst(const IncomingCall *Call,
                               MachineIRBuilder &MIRBuilder,
                               SPIRVGlobalRegistry *GR) {
  // Lookup the instruction opcode in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  unsigned Opcode = lookupNativeBuiltin(Builtin->Name, Builtin->Set)->Opcode;

  switch (Opcode) {
  case OpAtomicLoad:
    return buildAtomicLoadInst(Call, MIRBuilder, GR);
  case OpAtomicStore:
    return buildAtomicStoreInst(Call, MIRBuilder, GR);
  case OpAtomicCompareExchange:
  case OpAtomicCompareExchangeWeak:
    return buildAtomicCompareExchangeInst(Call, MIRBuilder, GR);
  case OpAtomicIAdd:
  case OpAtomicISub:
  case OpAtomicOr:
  case OpAtomicXor:
  case OpAtomicAnd:
    return buildAtomicRMWInst(Call, Opcode, MIRBuilder, GR);
  case OpMemoryBarrier:
    return buildBarrierInst(Call, OpMemoryBarrier, MIRBuilder, GR);
  default:
    return false;
  }
}

static bool generateBarrierInst(const IncomingCall *Call,
                                MachineIRBuilder &MIRBuilder,
                                SPIRVGlobalRegistry *GR) {
  // Lookup the instruction opcode in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  unsigned Opcode = lookupNativeBuiltin(Builtin->Name, Builtin->Set)->Opcode;

  return buildBarrierInst(Call, Opcode, MIRBuilder, GR);
}

static bool generateDotOrFMulInst(const IncomingCall *Call,
                                  MachineIRBuilder &MIRBuilder,
                                  SPIRVGlobalRegistry *GR) {
  MachineInstrBuilder MIB;

  if (GR->getSPIRVTypeForVReg(Call->Arguments[0])->getOpcode() ==
      SPIRV::OpTypeVector) {
    // Use OpDot only in case of vector args.
    MIB = MIRBuilder.buildInstr(OpDot);
  } else {
    // Use OpFMul in case of scalar args.
    MIB = MIRBuilder.buildInstr(OpFMulS);
  }

  MIB.addDef(Call->ReturnRegister)
      .addUse(GR->getSPIRVTypeID(Call->ReturnType))
      .addUse(Call->Arguments[0])
      .addUse(Call->Arguments[1]);
  return constrainRegOperands(MIB);
}

static bool generateGetQueryInst(const IncomingCall *Call,
                                 MachineIRBuilder &MIRBuilder,
                                 SPIRVGlobalRegistry *GR) {
  // Lookup the builtin record
  const DemangledBuiltin *Builtin = Call->Builtin;
  ::BuiltIn::BuiltIn Value =
      lookupGetBuiltin(Builtin->Name, Builtin->Set)->Value;

  uint64_t DefaultValue = (Value == GlobalSize || Value == WorkgroupSize ||
                           Value == EnqueuedWorkgroupSize)
                              ? 1
                              : 0;
  return genWorkgroupQuery(Call, MIRBuilder, GR, Value, DefaultValue);
}

static bool generateImageSizeQueryInst(const IncomingCall *Call,
                                       MachineIRBuilder &MIRBuilder,
                                       SPIRVGlobalRegistry *GR) {
  // Lookup the image size query component number in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  uint32_t Component =
      lookupImageQueryBuiltin(Builtin->Name, Builtin->Set)->Component;

  const auto MRI = MIRBuilder.getMRI();

  // Query result may either be a vector or a scalar. If return type is not a
  // vector, expect only a single size component. Otherwise get the number of
  // expected components.
  unsigned NumExpectedRetComponents =
      Call->ReturnType->getOpcode() == SPIRV::OpTypeVector
          ? Call->ReturnType->getOperand(2).getImm()
          : 1;

  // Get the actual number of query result/size components
  SPIRVType *ImgType = GR->getSPIRVTypeForVReg(Call->Arguments[0]);
  unsigned NumActualRetComponents = getNumSizeComponents(ImgType);

  Register QueryResult = Call->ReturnRegister;
  SPIRVType *QueryResultType = Call->ReturnType;
  if (NumExpectedRetComponents != NumActualRetComponents) {
    QueryResult = MRI->createGenericVirtualRegister(
        LLT::fixed_vector(NumActualRetComponents, 32));
    QueryResultType = GR->getOrCreateSPIRVVectorType(
        GR->getOrCreateSPIRVIntegerType(32, MIRBuilder), NumActualRetComponents,
        MIRBuilder);
    GR->assignSPIRVTypeToVReg(QueryResultType, QueryResult, MIRBuilder.getMF());
  }

  Dim::Dim Dimensionality =
      static_cast<Dim::Dim>(ImgType->getOperand(2).getImm());
  MachineInstrBuilder MIB;
  if (Dimensionality == Dim::DIM_Buffer) {
    MIB = MIRBuilder.buildInstr(SPIRV::OpImageQuerySize)
              .addDef(QueryResult)
              .addUse(GR->getSPIRVTypeID(QueryResultType))
              .addUse(Call->Arguments[0]);
  } else {
    auto LodRegister = GR->buildConstantInt(
        0, MIRBuilder, GR->getOrCreateSPIRVIntegerType(32, MIRBuilder));
    MIB = MIRBuilder.buildInstr(SPIRV::OpImageQuerySizeLod)
              .addDef(QueryResult)
              .addUse(GR->getSPIRVTypeID(QueryResultType))
              .addUse(Call->Arguments[0])
              .addUse(LodRegister);
  }

  if (!constrainRegOperands(MIB))
    return false;

  if (NumExpectedRetComponents == NumActualRetComponents)
    return true;

  if (NumExpectedRetComponents == 1) {
    // Only 1 component is expected, build OpCompositeExtract instruction.
    unsigned ExtractedComposite =
        Component == 3 ? NumActualRetComponents - 1 : Component;
    assert(ExtractedComposite < NumActualRetComponents &&
           "Invalid composite index!");
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpCompositeExtract)
                   .addDef(Call->ReturnRegister)
                   .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                   .addUse(QueryResult)
                   .addImm(ExtractedComposite);
    return constrainRegOperands(MIB);
  } else {
    // More than 1 component is expected, fill a new vector.
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpVectorShuffle)
                   .addDef(Call->ReturnRegister)
                   .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                   .addUse(QueryResult)
                   .addUse(QueryResult);
    for (unsigned i = 0; i < NumExpectedRetComponents; ++i) {
      uint32_t ComponentIndex = i < NumActualRetComponents ? i : 0xffffffff;
      MIB.addImm(ComponentIndex);
    }
    return constrainRegOperands(MIB);
  }
}

static bool generateImageMiscQueryInst(const IncomingCall *Call,
                                       MachineIRBuilder &MIRBuilder,
                                       SPIRVGlobalRegistry *GR) {
  // TODO: Add support for other image query builtins
  Register Image = Call->Arguments[0];

  assert(Call->ReturnType->getOpcode() == SPIRV::OpTypeInt &&
         "Image samples query result must be of int type!");
  assert(GR->getSPIRVTypeForVReg(Image)->getOperand(2).getImm() == 1 &&
         "Image must be of 2D dimensionality");

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageQuerySamples)
                 .addDef(Call->ReturnRegister)
                 .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                 .addUse(Image);
  return constrainRegOperands(MIB);
}

// TODO: Move to TableGen
static SamplerAddressingMode::SamplerAddressingMode
getSamplerAddressingModeFromBitmask(unsigned int bitmask) {
  switch (bitmask & CLK_ADDRESS_MODE_MASK) {
  case CLK_ADDRESS_CLAMP:
    return SamplerAddressingMode::Clamp;
  case CLK_ADDRESS_CLAMP_TO_EDGE:
    return SamplerAddressingMode::ClampToEdge;
  case CLK_ADDRESS_REPEAT:
    return SamplerAddressingMode::Repeat;
  case CLK_ADDRESS_MIRRORED_REPEAT:
    return SamplerAddressingMode::RepeatMirrored;
  case CLK_ADDRESS_NONE:
    return SamplerAddressingMode::None;
  default:
    llvm_unreachable("Unknown CL address mode");
  }
}

static unsigned int getSamplerParamFromBitmask(unsigned int bitmask) {
  return (bitmask & CLK_NORMALIZED_COORDS_TRUE) ? 1 : 0;
}

static SamplerFilterMode::SamplerFilterMode
getSamplerFilterModeFromBitmask(unsigned int bitmask) {
  if (bitmask & CLK_FILTER_LINEAR) {
    return SamplerFilterMode::Linear;
  } else if (bitmask & CLK_FILTER_NEAREST) {
    return SamplerFilterMode::Nearest;
  } else {
    return SamplerFilterMode::Nearest;
  }
}

static bool generateReadImageInst(const StringRef DemangledCall,
                                  const IncomingCall *Call,
                                  MachineIRBuilder &MIRBuilder,
                                  SPIRVGlobalRegistry *GR) {
  Register Image = Call->Arguments[0];

  const auto MRI = MIRBuilder.getMRI();

  if (DemangledCall.contains_insensitive("ocl_sampler")) {
    Register Sampler = Call->Arguments[1];

    if (!GR->isScalarOfType(Sampler, SPIRV::OpTypeSampler)) {
      if (getDefInstrMaybeConstant(Sampler, MRI)->getOperand(1).isCImm()) {
        uint64_t SamplerMask = getIConstVal(Sampler, MRI);
        Sampler = GR->buildConstantSampler(
            Register(), getSamplerAddressingModeFromBitmask(SamplerMask),
            getSamplerParamFromBitmask(SamplerMask),
            getSamplerFilterModeFromBitmask(SamplerMask), MIRBuilder,
            GR->getSPIRVTypeForVReg(Sampler));
      }
    }

    SPIRVType *ImageType = GR->getSPIRVTypeForVReg(Image);
    SPIRVType *SampledImageType =
        GR->getOrCreateOpTypeSampledImage(ImageType, MIRBuilder);
    Register SampledImage = MRI->createVirtualRegister(&SPIRV::IDRegClass);

    auto MIB = MIRBuilder.buildInstr(SPIRV::OpSampledImage)
                   .addDef(SampledImage)
                   .addUse(GR->getSPIRVTypeID(SampledImageType))
                   .addUse(Image)
                   .addUse(Sampler);

    if (!constrainRegOperands(MIB))
      return false;

    Register Coordinate = Call->Arguments[2];
    Register Lod = GR->buildConstantFP(APFloat::getZero(APFloat::IEEEsingle()),
                                       MIRBuilder);

    SPIRVType *TempType = Call->ReturnType;
    bool NeedsExtraction = false;
    if (TempType->getOpcode() != SPIRV::OpTypeVector) {
      TempType =
          GR->getOrCreateSPIRVVectorType(Call->ReturnType, 4, MIRBuilder);
      NeedsExtraction = true;
    }

    LLT LLType = LLT::scalar(GR->getScalarOrVectorBitWidth(TempType));
    Register TempRegister = MRI->createGenericVirtualRegister(LLType);
    GR->assignSPIRVTypeToVReg(TempType, TempRegister, MIRBuilder.getMF());

    MIB = MIRBuilder.buildInstr(SPIRV::OpImageSampleExplicitLod)
              .addDef(NeedsExtraction ? TempRegister : Call->ReturnRegister)
              .addUse(GR->getSPIRVTypeID(TempType))
              .addUse(SampledImage)
              .addUse(Coordinate)
              .addImm(ImageOperand::Lod)
              .addUse(Lod);

    if (!constrainRegOperands(MIB))
      return false;

    if (NeedsExtraction) {
      MIB = MIRBuilder.buildInstr(SPIRV::OpCompositeExtract)
                .addDef(Call->ReturnRegister)
                .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                .addUse(TempRegister)
                .addImm(0);

      if (!constrainRegOperands(MIB))
        return false;
    }

    return true;
  } else if (DemangledCall.contains_insensitive("msaa")) {
    Register Coordinate = Call->Arguments[1];
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageRead)
                   .addDef(Call->ReturnRegister)
                   .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                   .addUse(Image)
                   .addUse(Coordinate)
                   .addImm(ImageOperand::Sample)
                   .addUse(Call->Arguments[2]);
    return constrainRegOperands(MIB);
  } else {
    Register Coordinate = Call->Arguments[1];
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageRead)
                   .addDef(Call->ReturnRegister)
                   .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                   .addUse(Image)
                   .addUse(Coordinate);
    return constrainRegOperands(MIB);
  }
}

static bool generateWriteImageInst(const IncomingCall *Call,
                                   MachineIRBuilder &MIRBuilder,
                                   SPIRVGlobalRegistry *GR) {
  Register Image = Call->Arguments[0];
  Register Coordinate = Call->Arguments[1];
  Register Texel = Call->Arguments[2];

  auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageWrite)
                 .addUse(Image)
                 .addUse(Coordinate)
                 .addUse(Texel);
  return constrainRegOperands(MIB);
}

static bool generateSampleImageInst(const StringRef DemangledCall,
                                    const IncomingCall *Call,
                                    MachineIRBuilder &MIRBuilder,
                                    SPIRVGlobalRegistry *GR) {
  if (Call->Builtin->Name.contains_insensitive(
          "__translate_sampler_initializer")) {
    // Build sampler literal.
    uint64_t Bitmask = getIConstVal(Call->Arguments[0], MIRBuilder.getMRI());
    Register Sampler = GR->buildConstantSampler(
        Call->ReturnRegister, getSamplerAddressingModeFromBitmask(Bitmask),
        getSamplerParamFromBitmask(Bitmask),
        getSamplerFilterModeFromBitmask(Bitmask), MIRBuilder, Call->ReturnType);
    return Sampler.isValid();
  } else if (Call->Builtin->Name.contains_insensitive("__spirv_SampledImage")) {
    // Create OpSampledImage.
    Register Image = Call->Arguments[0];
    Register Sampler = Call->Arguments[1];

    const auto MRI = MIRBuilder.getMRI();
    SPIRVType *ImageType = GR->getSPIRVTypeForVReg(Image);
    SPIRVType *SampledImageType =
        GR->getOrCreateOpTypeSampledImage(ImageType, MIRBuilder);
    Register SampledImage =
        Call->ReturnRegister.isValid()
            ? Call->ReturnRegister
            : MRI->createVirtualRegister(&SPIRV::IDRegClass);

    auto MIB = MIRBuilder.buildInstr(SPIRV::OpSampledImage)
                   .addDef(SampledImage)
                   .addUse(GR->getSPIRVTypeID(SampledImageType))
                   .addUse(Image)
                   .addUse(Sampler);
    return constrainRegOperands(MIB);
  } else if (Call->Builtin->Name.contains_insensitive(
                 "__spirv_ImageSampleExplicitLod")) {
    Register Image = Call->Arguments[0];
    // Sample an image using an explicit level of detail.
    std::string ReturnType = DemangledCall.str();
    if (DemangledCall.contains("_R")) {
      ReturnType = ReturnType.substr(ReturnType.find("_R") + 2);
      ReturnType = ReturnType.substr(0, ReturnType.find('('));
    }

    SPIRVType *Type = GR->getOrCreateSPIRVTypeByName(ReturnType, MIRBuilder);
    Register Coordinate = Call->Arguments[1];
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpImageSampleExplicitLod)
                   .addDef(Call->ReturnRegister)
                   .addUse(GR->getSPIRVTypeID(Type))
                   .addUse(Image)
                   .addUse(Coordinate)
                   .addImm(ImageOperand::Lod)
                   .addUse(Call->Arguments[3]);
    return constrainRegOperands(MIB);
  }

  return false;
}

static bool generateSelectInst(const IncomingCall *Call,
                               MachineIRBuilder &MIRBuilder) {
  MIRBuilder.buildSelect(Call->ReturnRegister, Call->Arguments[0],
                         Call->Arguments[1], Call->Arguments[2]);
  return true;
}

static bool generateSpecConstantInst(const IncomingCall *Call,
                                     MachineIRBuilder &MIRBuilder,
                                     SPIRVGlobalRegistry *GR) {
  // Lookup the instruction opcode in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  unsigned Opcode = lookupNativeBuiltin(Builtin->Name, Builtin->Set)->Opcode;

  const auto MRI = MIRBuilder.getMRI();

  switch (Opcode) {
  case OpSpecConstant: {
    // Build the SpecID decoration.
    unsigned SpecId =
        static_cast<unsigned>(getIConstVal(Call->Arguments[0], MRI));
    buildOpDecorate(Call->ReturnRegister, MIRBuilder, Decoration::SpecId,
                    {SpecId});

    // Determine the constant MI.
    Register ConstRegister = Call->Arguments[1];
    const MachineInstr *Const = getDefInstrMaybeConstant(ConstRegister, MRI);
    assert(Const &&
           (Const->getOpcode() == TargetOpcode::G_CONSTANT ||
            Const->getOpcode() == TargetOpcode::G_FCONSTANT) &&
           "Argument should be either an int or floating-point constant");

    // Determine the opcode and built the OpSpec MI.
    const MachineOperand &ConstOperand = Const->getOperand(1);
    if (Call->ReturnType->getOpcode() == SPIRV::OpTypeBool) {
      assert(ConstOperand.isCImm() && "Int constant operand is expected");
      Opcode = ConstOperand.getCImm()->getValue().getZExtValue()
                   ? SPIRV::OpSpecConstantTrue
                   : SPIRV::OpSpecConstantFalse;
    }

    auto MIB = MIRBuilder.buildInstr(Opcode)
                   .addDef(Call->ReturnRegister)
                   .addUse(GR->getSPIRVTypeID(Call->ReturnType));

    if (Call->ReturnType->getOpcode() != SPIRV::OpTypeBool) {
      if (Const->getOpcode() == TargetOpcode::G_CONSTANT)
        addNumImm(ConstOperand.getCImm()->getValue(), MIB);
      else
        addNumImm(ConstOperand.getFPImm()->getValueAPF().bitcastToAPInt(), MIB);
    }

    return constrainRegOperands(MIB);
  }
  case OpSpecConstantComposite: {
    auto MIB = MIRBuilder.buildInstr(Opcode)
                   .addDef(Call->ReturnRegister)
                   .addUse(GR->getSPIRVTypeID(Call->ReturnType));

    for (unsigned i = 0; i < Call->Arguments.size(); i++)
      MIB.addUse(Call->Arguments[i]);
    return constrainRegOperands(MIB);
  }
  default:
    return false;
  }
}

static bool generateEnqueueInst(const IncomingCall *Call,
                                MachineIRBuilder &MIRBuilder,
                                SPIRVGlobalRegistry *GR) {
  // Lookup the instruction opcode in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  unsigned Opcode = lookupNativeBuiltin(Builtin->Name, Builtin->Set)->Opcode;

  switch (Opcode) {
  case OpRetainEvent:
  case OpReleaseEvent:
    return MIRBuilder.buildInstr(Opcode).addUse(Call->Arguments[0]);
  case OpCreateUserEvent:
    return MIRBuilder.buildInstr(Opcode)
        .addDef(Call->ReturnRegister)
        .addUse(GR->getSPIRVTypeID(Call->ReturnType));
  case OpIsValidEvent:
    return MIRBuilder.buildInstr(Opcode)
        .addDef(Call->ReturnRegister)
        .addUse(GR->getSPIRVTypeID(Call->ReturnType))
        .addUse(Call->Arguments[0]);
  case OpSetUserEventStatus:
    return MIRBuilder.buildInstr(Opcode)
        .addUse(Call->Arguments[0])
        .addUse(Call->Arguments[1]);
  case OpCaptureEventProfilingInfo:
    return MIRBuilder.buildInstr(Opcode)
        .addUse(Call->Arguments[0])
        .addUse(Call->Arguments[1])
        .addUse(Call->Arguments[2]);
  default:
    return false;
  }
}

static bool generateAsyncCopy(const IncomingCall *Call,
                              MachineIRBuilder &MIRBuilder,
                              SPIRVGlobalRegistry *GR) {
  // Lookup the instruction opcode in the TableGen records.
  const DemangledBuiltin *Builtin = Call->Builtin;
  unsigned Opcode = lookupNativeBuiltin(Builtin->Name, Builtin->Set)->Opcode;
  const auto I32Ty = GR->getOrCreateSPIRVIntegerType(32, MIRBuilder);
  auto Scope = GR->buildConstantInt(Scope::Workgroup, MIRBuilder, I32Ty);

  switch (Opcode) {
  case OpGroupAsyncCopy:
    return MIRBuilder.buildInstr(Opcode)
        .addDef(Call->ReturnRegister)
        .addUse(GR->getSPIRVTypeID(Call->ReturnType))
        .addUse(Scope)
        .addUse(Call->Arguments[0])
        .addUse(Call->Arguments[1])
        .addUse(Call->Arguments[2])
        .addUse(GR->buildConstantInt(1, MIRBuilder, I32Ty))
        .addUse(Call->Arguments[3]);
  case OpGroupWaitEvents:
    return MIRBuilder.buildInstr(Opcode)
        .addUse(Scope)
        .addUse(Call->Arguments[0])
        .addUse(Call->Arguments[1]);
  default:
    return false;
  }
}

static bool generateConvertInst(const StringRef DemangledCall,
                                const IncomingCall *Call,
                                MachineIRBuilder &MIRBuilder,
                                SPIRVGlobalRegistry *GR) {
  // Lookup the conversion builtin in the TableGen records.
  const ConvertBuiltin *Builtin =
      lookupConvertBuiltin(Call->Builtin->Name, Call->Builtin->Set);

  if (Builtin->IsSaturated)
    buildOpDecorate(Call->ReturnRegister, MIRBuilder,
                    Decoration::SaturatedConversion, {});

  if (Builtin->IsRounded)
    buildOpDecorate(Call->ReturnRegister, MIRBuilder,
                    Decoration::FPRoundingMode, {Builtin->RoundingMode});

  unsigned Opcode = OpNop;
  if (GR->isScalarOrVectorOfType(Call->Arguments[0], OpTypeInt)) {
    // Int -> ...
    if (GR->isScalarOrVectorOfType(Call->ReturnRegister, OpTypeInt)) {
      // Int -> Int
      if (Builtin->IsSaturated)
        Opcode =
            Builtin->IsDestinationSigned ? OpSatConvertUToS : OpSatConvertSToU;
      else
        Opcode = Builtin->IsDestinationSigned ? OpUConvert : OpSConvert;
    } else if (GR->isScalarOrVectorOfType(Call->ReturnRegister, OpTypeFloat)) {
      // Int -> Float
      bool IsSourceSigned =
          DemangledCall[DemangledCall.find_first_of('(') + 1] != 'u';
      Opcode = IsSourceSigned ? OpConvertSToF : OpConvertUToF;
    }
  } else if (GR->isScalarOrVectorOfType(Call->Arguments[0], OpTypeFloat)) {
    // Float -> ...
    if (GR->isScalarOrVectorOfType(Call->ReturnRegister, OpTypeInt))
      // Float -> Int
      Opcode = Builtin->IsDestinationSigned ? OpConvertFToS : OpConvertFToU;
    else if (GR->isScalarOrVectorOfType(Call->ReturnRegister, OpTypeFloat))
      // Float -> Float
      Opcode = OpFConvert;
  }

  assert(Opcode != OpNop && "Conversion between the types not implemented!");

  auto MIB = MIRBuilder.buildInstr(Opcode)
                 .addDef(Call->ReturnRegister)
                 .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                 .addUse(Call->Arguments[0]);

  return constrainRegOperands(MIB);
}

static bool generateVectorLoadStoreInst(const IncomingCall *Call,
                                        MachineIRBuilder &MIRBuilder,
                                        SPIRVGlobalRegistry *GR) {
  // Lookup the vector load/store builtin in the TableGen records.
  const VectorLoadStoreBuiltin *Builtin =
      lookupVectorLoadStoreBuiltin(Call->Builtin->Name, Call->Builtin->Set);

  // Build extended instruction.
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                 .addDef(Call->ReturnRegister)
                 .addUse(GR->getSPIRVTypeID(Call->ReturnType))
                 .addImm(static_cast<uint32_t>(::InstructionSet::OpenCL_std))
                 .addImm(Builtin->Number);

  for (auto Argument : Call->Arguments)
    MIB.addUse(Argument);

  // Rounding mode should be passed as a last argument in the MI for builtins
  // like "vstorea_halfn_r".
  if (Builtin->IsRounded)
    MIB.addImm(static_cast<uint32_t>(Builtin->RoundingMode));

  return constrainRegOperands(MIB);
}

/// Lowers a builtin funtion call using the provided \p DemangledCall skeleton
/// and external instruction \p Set.
bool llvm::lowerBuiltin(const StringRef DemangledCall,
                        ExternalInstructionSet::InstructionSet Set,
                        MachineIRBuilder &MIRBuilder, const Register OrigRet,
                        const Type *OrigRetTy,
                        const SmallVectorImpl<Register> &Args,
                        SPIRVGlobalRegistry *GR) {

  LLVM_DEBUG(dbgs() << "Lowering builtin call: " << DemangledCall << "\n");

  // SPIR-V type and return register
  Register ReturnRegister = OrigRet;
  SPIRVType *ReturnType = nullptr;
  if (OrigRetTy && !OrigRetTy->isVoidTy()) {
    ReturnType = GR->assignTypeToVReg(OrigRetTy, OrigRet, MIRBuilder);
  } else if (OrigRetTy && OrigRetTy->isVoidTy()) {
    ReturnRegister = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
    MIRBuilder.getMRI()->setType(ReturnRegister, LLT::scalar(32));
    ReturnType = GR->assignTypeToVReg(OrigRetTy, ReturnRegister, MIRBuilder);
  }

  // Lookup the builtin in the TableGen records
  std::unique_ptr<const IncomingCall> Call =
      lookupBuiltin(DemangledCall, Set, ReturnRegister, ReturnType, Args);

  if (!Call) {
    LLVM_DEBUG(dbgs() << "Builtin record was not found!");
    return false;
  }

  verifyBuiltinArgs(Call->Builtin, Args);

  // Match the builtin with implementation based on the grouping.
  switch (Call->Builtin->Group) {
  case Extended:
    return generateExtInst(Call.get(), MIRBuilder, GR);
  case Relational:
    return generateRelationalInst(Call.get(), MIRBuilder, GR);
  case Group:
    return generateGroupInst(Call.get(), MIRBuilder, GR);
  case Variable:
    return generateBuiltinVar(Call.get(), MIRBuilder, GR);
  case Atomic:
    return generateAtomicInst(Call.get(), MIRBuilder, GR);
  case Barrier:
    return generateBarrierInst(Call.get(), MIRBuilder, GR);
  case Dot:
    return generateDotOrFMulInst(Call.get(), MIRBuilder, GR);
  case GetQuery:
    return generateGetQueryInst(Call.get(), MIRBuilder, GR);
  case ImageSizeQuery:
    return generateImageSizeQueryInst(Call.get(), MIRBuilder, GR);
  case ImageMiscQuery:
    return generateImageMiscQueryInst(Call.get(), MIRBuilder, GR);
  case ReadImage:
    return generateReadImageInst(DemangledCall, Call.get(), MIRBuilder, GR);
  case WriteImage:
    return generateWriteImageInst(Call.get(), MIRBuilder, GR);
  case SampleImage:
    return generateSampleImageInst(DemangledCall, Call.get(), MIRBuilder, GR);
  case Select:
    return generateSelectInst(Call.get(), MIRBuilder);
  case SpecConstant:
    return generateSpecConstantInst(Call.get(), MIRBuilder, GR);
  case Enqueue:
    return generateEnqueueInst(Call.get(), MIRBuilder, GR);
  case AsyncCopy:
    return generateAsyncCopy(Call.get(), MIRBuilder, GR);
  case Convert:
    return generateConvertInst(DemangledCall, Call.get(), MIRBuilder, GR);
  case VectorLoadStore:
    return generateVectorLoadStoreInst(Call.get(), MIRBuilder, GR);
  }

  return false;
}
