//===--- SPIRVCallLowering.cpp - Call lowering ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the lowering of LLVM calls to machine code calls for
// GlobalISel.
//
//===----------------------------------------------------------------------===//

#include "SPIRVCallLowering.h"
#include "MCTargetDesc/SPIRVBaseInfo.h"
#include "SPIRV.h"
#include "SPIRVBuiltins.h"
#include "SPIRVGlobalRegistry.h"
#include "SPIRVISelLowering.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVSubtarget.h"
#include "SPIRVUtils.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"

using namespace llvm;

SPIRVCallLowering::SPIRVCallLowering(const SPIRVTargetLowering &TLI,
                                     SPIRVGlobalRegistry *GR)
    : CallLowering(&TLI), GR(GR) {}

bool SPIRVCallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                    const Value *Val, ArrayRef<Register> VRegs,
                                    FunctionLoweringInfo &FLI,
                                    Register SwiftErrorVReg) const {
  assert(VRegs.size() < 2 && "All return types should use a single register");
  if (Val) {
    const auto &STI = MIRBuilder.getMF().getSubtarget();
    return MIRBuilder.buildInstr(SPIRV::OpReturnValue)
        .addUse(VRegs[0])
        .constrainAllUses(MIRBuilder.getTII(), *STI.getRegisterInfo(),
                          *STI.getRegBankInfo());
  }
  MIRBuilder.buildInstr(SPIRV::OpReturn);
  return true;
}

// Based on the LLVM function attributes, get a SPIR-V FunctionControl.
static uint32_t getFunctionControl(const Function &F) {
  uint32_t FuncControl = SPIRV::FunctionControl::None;
  if (F.hasFnAttribute(Attribute::AttrKind::AlwaysInline)) {
    FuncControl |= SPIRV::FunctionControl::Inline;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::ReadNone)) {
    FuncControl |= SPIRV::FunctionControl::Pure;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::ReadOnly)) {
    FuncControl |= SPIRV::FunctionControl::Const;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::NoInline)) {
    FuncControl |= SPIRV::FunctionControl::DontInline;
  }
  return FuncControl;
}

static ConstantInt *getConstInt(MDNode *MD, unsigned NumOp) {
  if (MD->getNumOperands() > NumOp) {
    auto *CMeta = dyn_cast<ConstantAsMetadata>(MD->getOperand(NumOp));
    if (CMeta)
      return dyn_cast<ConstantInt>(CMeta->getValue());
  }
  return nullptr;
}

// This code restores function args/retvalue types for composite cases
// because the final types should still be aggregate whereas they're i32
// during the translation to cope with aggregate flattening etc.
static FunctionType *getOriginalFunctionType(const Function &F) {
  auto *NamedMD = F.getParent()->getNamedMetadata("spv.cloned_funcs");
  if (NamedMD == nullptr)
    return F.getFunctionType();

  Type *RetTy = F.getFunctionType()->getReturnType();
  SmallVector<Type *, 4> ArgTypes;
  for (auto &Arg : F.args())
    ArgTypes.push_back(Arg.getType());

  auto ThisFuncMDIt =
      std::find_if(NamedMD->op_begin(), NamedMD->op_end(), [&F](MDNode *N) {
        return isa<MDString>(N->getOperand(0)) &&
               cast<MDString>(N->getOperand(0))->getString() == F.getName();
      });
  // TODO: probably one function can have numerous type mutations,
  // so we should support this.
  if (ThisFuncMDIt != NamedMD->op_end()) {
    auto *ThisFuncMD = *ThisFuncMDIt;
    MDNode *MD = dyn_cast<MDNode>(ThisFuncMD->getOperand(1));
    assert(MD && "MDNode operand is expected");
    ConstantInt *Const = getConstInt(MD, 0);
    if (Const) {
      auto *CMeta = dyn_cast<ConstantAsMetadata>(MD->getOperand(1));
      assert(CMeta && "ConstantAsMetadata operand is expected");
      assert(Const->getSExtValue() >= -1);
      // Currently -1 indicates return value, greater values mean
      // argument numbers.
      if (Const->getSExtValue() == -1)
        RetTy = CMeta->getType();
      else
        ArgTypes[Const->getSExtValue()] = CMeta->getType();
    }
  }

  return FunctionType::get(RetTy, ArgTypes, F.isVarArg());
}

static MDNode *getKernelMetadata(const Function &F, const StringRef Kind) {
  assert(F.getCallingConv() == CallingConv::SPIR_KERNEL &&
         "Kernel attributes are only attached/belong to kernel functions.");

  // Lookup the attribute in metadata attached to the kernel function.
  MDNode *Node = F.getMetadata(Kind);
  if (Node)
    return Node;

  // Sometimes metadata containing kernel attributes is not attached to the
  // function, but can be found in the named module-level metadata instead. For
  // example:
  // !opencl.kernels = !{!0}
  // !0 = !{void ()* @someKernelFunction, !1, ...}
  // !1 = !{!"kernel_arg_addr_space", ...}
  NamedMDNode *OpenCLKernelsMD =
      F.getParent()->getNamedMetadata("opencl.kernels");
  if (!OpenCLKernelsMD || OpenCLKernelsMD->getNumOperands() == 0)
    return nullptr;

  MDNode *KernelToMDNodeList = OpenCLKernelsMD->getOperand(0);
  // KernelToMDNodeList contains kernel function declarations followed by
  // corresponding MDNodes for each attribute. Search only MDNodes "belonging"
  // to the currently lowered kernel function.
  bool FoundLoweredKernelFunction = false;
  for (const MDOperand &Operand : KernelToMDNodeList->operands()) {
    ValueAsMetadata *MaybeValue = dyn_cast<ValueAsMetadata>(Operand);
    if (MaybeValue &&
        dyn_cast_or_null<Function>(MaybeValue->getValue())->getName() ==
            F.getName()) {
      FoundLoweredKernelFunction = true;
      continue;
    } else if (MaybeValue && FoundLoweredKernelFunction) {
      return nullptr;
    }

    MDNode *MaybeNode = dyn_cast<MDNode>(Operand);
    if (FoundLoweredKernelFunction && MaybeNode &&
        cast<MDString>(MaybeNode->getOperand(0))->getString() == Kind)
      return MaybeNode;
  }

  return nullptr;
}

static SPIRV::AccessQualifier::AccessQualifier
getArgAccessQual(const Function &F, unsigned ArgIdx) {
  SPIRV::AccessQualifier::AccessQualifier AQ =
      SPIRV::AccessQualifier::ReadWrite;

  if (F.getCallingConv() != CallingConv::SPIR_KERNEL)
    return AQ;

  MDNode *Node = getKernelMetadata(F, "kernel_arg_access_qual");
  if (Node && ArgIdx < Node->getNumOperands()) {
    StringRef AQString = cast<MDString>(Node->getOperand(ArgIdx))->getString();
    if (AQString.compare("read_only") == 0)
      AQ = SPIRV::AccessQualifier::ReadOnly;
    else if (AQString.compare("write_only") == 0)
      AQ = SPIRV::AccessQualifier::WriteOnly;
  }

  return AQ;
}

static std::vector<SPIRV::Decoration::Decoration>
getKernelArgTypeQual(const Function &F, unsigned ArgIdx) {
  MDNode *Node = getKernelMetadata(F, "kernel_arg_type_qual");
  if (Node && ArgIdx < Node->getNumOperands()) {
    StringRef TypeQual = cast<MDString>(Node->getOperand(ArgIdx))->getString();
    if (TypeQual.compare("volatile") == 0)
      return {SPIRV::Decoration::Volatile};
  }

  return {};
}

static Type *getArgType(const Function &F, unsigned ArgIdx) {
  if (F.getCallingConv() == CallingConv::SPIR_KERNEL) {
    MDNode *KernelArgTypes = getKernelMetadata(F, "kernel_arg_type");
    std::string a = cast<MDString>(KernelArgTypes->getOperand(ArgIdx))->getString().str();
    if (KernelArgTypes && ArgIdx < KernelArgTypes->getNumOperands() &&
        cast<MDString>(KernelArgTypes->getOperand(ArgIdx))
            ->getString()
            .endswith("_t")) {
      std::string KernelArgTypeStr =
          "opencl." +
          cast<MDString>(KernelArgTypes->getOperand(ArgIdx))->getString().str();

      Type *ExistingOpaqueType =
          StructType::getTypeByName(F.getContext(), KernelArgTypeStr);
      return ExistingOpaqueType
                 ? ExistingOpaqueType
                 : StructType::create(F.getContext(), KernelArgTypeStr);
    }
  }

  return getOriginalFunctionType(F)->getParamType(ArgIdx);
}

bool SPIRVCallLowering::lowerFormalArguments(MachineIRBuilder &MIRBuilder,
                                             const Function &F,
                                             ArrayRef<ArrayRef<Register>> VRegs,
                                             FunctionLoweringInfo &FLI) const {
  assert(GR && "Must initialize the SPIRV type registry before lowering args.");
  GR->setCurrentFunc(MIRBuilder.getMF());

  // Assign types and names to all args, and store their types for later.
  FunctionType *FTy = getOriginalFunctionType(F);
  SmallVector<SPIRVType *, 4> ArgTypeVRegs;
  if (VRegs.size() > 0) {
    unsigned i = 0;
    for (const auto &Arg : F.args()) {
      assert(VRegs[i].size() == 1 && "Formal arg has multiple vregs");

      SPIRV::AccessQualifier::AccessQualifier ArgAccessQual = getArgAccessQual(F, i);
      auto *SpirvTy = GR->assignTypeToVReg(getArgType(F, i), VRegs[i][0],
                                           MIRBuilder, ArgAccessQual);
      ArgTypeVRegs.push_back(SpirvTy);

      if (Arg.hasName())
        buildOpName(VRegs[i][0], Arg.getName(), MIRBuilder);
      if (Arg.getType()->isPointerTy()) {
        auto DerefBytes = static_cast<unsigned>(Arg.getDereferenceableBytes());
        if (DerefBytes != 0)
          buildOpDecorate(VRegs[i][0], MIRBuilder,
                          SPIRV::Decoration::MaxByteOffset, {DerefBytes});
      }
      if (Arg.hasAttribute(Attribute::Alignment)) {
        auto Alignment = static_cast<unsigned>(
            Arg.getAttribute(Attribute::Alignment).getValueAsInt());
        buildOpDecorate(VRegs[i][0], MIRBuilder, SPIRV::Decoration::Alignment,
                        {Alignment});
      }
      if (Arg.hasAttribute(Attribute::ReadOnly)) {
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr,
                        {SPIRV::FunctionParameterAttribute::NoWrite});
      }
      if (Arg.hasAttribute(Attribute::ZExt)) {
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr,
                        {SPIRV::FunctionParameterAttribute::Zext});
      }
      if (Arg.hasAttribute(Attribute::NoAlias)) {
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr,
                        {SPIRV::FunctionParameterAttribute::NoAlias});
      }

      if (F.getCallingConv() == CallingConv::SPIR_KERNEL) {
        std::vector<SPIRV::Decoration::Decoration> ArgTypeQualDecs =
            getKernelArgTypeQual(F, i);
        for (SPIRV::Decoration::Decoration Decoration : ArgTypeQualDecs)
          buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration, {});
      }

      MDNode *Node = F.getMetadata("spirv.ParameterDecorations");
      if (Node && i < Node->getNumOperands() &&
          isa<MDNode>(Node->getOperand(i))) {
        MDNode *MD = cast<MDNode>(Node->getOperand(i));
        for (const MDOperand &MDOp : MD->operands()) {
          MDNode *MD2 = dyn_cast<MDNode>(MDOp);
          assert(MD2 && "Metadata operand is expected");
          ConstantInt *Const = getConstInt(MD2, 0);
          assert(Const && "MDOperand should be ConstantInt");
          auto Dec =
              static_cast<SPIRV::Decoration::Decoration>(Const->getZExtValue());
          std::vector<uint32_t> DecVec;
          for (unsigned j = 1; j < MD2->getNumOperands(); j++) {
            ConstantInt *Const = getConstInt(MD2, j);
            assert(Const && "MDOperand should be ConstantInt");
            DecVec.push_back(static_cast<uint32_t>(Const->getZExtValue()));
          }
          buildOpDecorate(VRegs[i][0], MIRBuilder, Dec, DecVec);
        }
      }
      ++i;
    }
  }

  // Generate a SPIR-V type for the function.
  auto MRI = MIRBuilder.getMRI();
  Register FuncVReg = MRI->createGenericVirtualRegister(LLT::scalar(32));
  if (F.isDeclaration())
    GR->add(&F, &MIRBuilder.getMF(), FuncVReg);
  MRI->setRegClass(FuncVReg, &SPIRV::IDRegClass);

  SPIRVType *RetTy = GR->getOrCreateSPIRVType(FTy->getReturnType(), MIRBuilder);
  SPIRVType *FuncTy = GR->getOrCreateOpTypeFunctionWithArgs(
      FTy, RetTy, ArgTypeVRegs, MIRBuilder);

  // Build the OpTypeFunction declaring it.
  uint32_t FuncControl = getFunctionControl(F);

  MIRBuilder.buildInstr(SPIRV::OpFunction)
      .addDef(FuncVReg)
      .addUse(GR->getSPIRVTypeID(RetTy))
      .addImm(FuncControl)
      .addUse(GR->getSPIRVTypeID(FuncTy));

  // Add OpFunctionParameters.
  int i = 0;
  for (const auto &Arg : F.args()) {
    assert(VRegs[i].size() == 1 && "Formal arg has multiple vregs");
    MRI->setRegClass(VRegs[i][0], &SPIRV::IDRegClass);
    MIRBuilder.buildInstr(SPIRV::OpFunctionParameter)
        .addDef(VRegs[i][0])
        .addUse(GR->getSPIRVTypeID(ArgTypeVRegs[i]));
    if (F.isDeclaration())
      GR->add(&Arg, &MIRBuilder.getMF(), VRegs[i][0]);
    i++;
  }

  // Name the function.
  if (F.hasName())
    buildOpName(FuncVReg, F.getName(), MIRBuilder);

  // Handle entry points and function linkage.
  if (F.getCallingConv() == CallingConv::SPIR_KERNEL) {
    auto ExecModel = SPIRV::ExecutionModel::Kernel;
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpEntryPoint)
                   .addImm(ExecModel)
                   .addUse(FuncVReg);
    addStringImm(F.getName(), MIB);
  } else if (F.getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage ||
             F.getLinkage() == GlobalValue::LinkOnceODRLinkage) {
    auto LnkTy = F.isDeclaration() ? SPIRV::LinkageType::Import
                                   : SPIRV::LinkageType::Export;
    buildOpDecorate(FuncVReg, MIRBuilder, SPIRV::Decoration::LinkageAttributes,
                    {static_cast<uint32_t>(LnkTy)}, F.getGlobalIdentifier());
  }

  return true;
}

bool SPIRVCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                  CallLoweringInfo &Info) const {
  MachineFunction &MF = MIRBuilder.getMF();
  GR->setCurrentFunc(MF);
  FunctionType *FTy = nullptr;
  const Function *CF = nullptr;

  // Emit a regular OpFunctionCall. If it's an externally declared function,
  // be sure to emit its type and function declaration here. It will be hoisted
  // globally later.
  if (Info.Callee.isGlobal()) {
    CF = cast<const Function>(Info.Callee.getGlobal());
    FTy = getOriginalFunctionType(*CF);
  }

  assert(Info.OrigRet.Regs.size() < 2 && "Call returns multiple vregs");

  Register ResVReg =
      Info.OrigRet.Regs.empty() ? Register(0) : Info.OrigRet.Regs[0];
  std::string FuncName = Info.Callee.getGlobal()->getGlobalIdentifier();
  std::string DemangledName = isOclOrSpirvBuiltin(FuncName);
  if (!DemangledName.empty() && CF && CF->isDeclaration()) {
    // TODO: check that it's OCL builtin, then apply OpenCL_std.
    const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());
    if (ST->canUseExtInstSet(SPIRV::InstructionSet::OpenCL_std)) {
      // Mangled names are for OpenCL builtins, so pass off to OpenCLBIFs.cpp.
      const Type *OrigRetTy = Info.OrigRet.Ty;
      if (FTy)
        OrigRetTy = FTy->getReturnType();
      SmallVector<Register, 8> ArgVRegs;
      for (auto Arg : Info.OrigArgs) {
        assert(Arg.Regs.size() == 1 && "Call arg has multiple VRegs");
        ArgVRegs.push_back(Arg.Regs[0]);
        auto SPIRVTy = GR->getOrCreateSPIRVType(Arg.Ty, MIRBuilder);
        GR->assignSPIRVTypeToVReg(SPIRVTy, Arg.Regs[0], MIRBuilder.getMF());
      }
      return lowerBuiltin(DemangledName, SPIRV::InstructionSet::OpenCL_std,
                          MIRBuilder, ResVReg, OrigRetTy, ArgVRegs, GR);
    }
    llvm_unreachable("Unable to handle this environment's built-in funcs.");
  }

  if (CF && CF->isDeclaration() &&
      !GR->find(CF, &MIRBuilder.getMF()).isValid()) {
    // Emit the type info and forward function declaration to the first MBB
    // to ensure VReg definition dependencies are valid across all MBBs.
    MachineIRBuilder FirstBlockBuilder;
    FirstBlockBuilder.setMF(MF);
    FirstBlockBuilder.setMBB(*MF.getBlockNumbered(0));

    SmallVector<ArrayRef<Register>, 8> VRegArgs;
    SmallVector<SmallVector<Register, 1>, 8> ToInsert;
    for (const Argument &Arg : CF->args()) {
      if (MIRBuilder.getDataLayout().getTypeStoreSize(Arg.getType()).isZero())
        continue; // Don't handle zero sized types.
      ToInsert.push_back(
          {MIRBuilder.getMRI()->createGenericVirtualRegister(LLT::scalar(32))});
      VRegArgs.push_back(ToInsert.back());
    }
    // TODO: Reuse FunctionLoweringInfo
    FunctionLoweringInfo FuncInfo;
    lowerFormalArguments(FirstBlockBuilder, *CF, VRegArgs, FuncInfo);
  }

  // Make sure there's a valid return reg, even for functions returning void.
  if (!ResVReg.isValid())
    ResVReg = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
  SPIRVType *RetType =
      GR->assignTypeToVReg(FTy->getReturnType(), ResVReg, MIRBuilder);

  // Emit the OpFunctionCall and its args.
  auto MIB = MIRBuilder.buildInstr(SPIRV::OpFunctionCall)
                 .addDef(ResVReg)
                 .addUse(GR->getSPIRVTypeID(RetType))
                 .add(Info.Callee);

  for (const auto &arg : Info.OrigArgs) {
    assert(arg.Regs.size() == 1 && "Call arg has multiple VRegs");
    MIB.addUse(arg.Regs[0]);
  }
  const auto &STI = MF.getSubtarget();
  return MIB.constrainAllUses(MIRBuilder.getTII(), *STI.getRegisterInfo(),
                              *STI.getRegBankInfo());
}
