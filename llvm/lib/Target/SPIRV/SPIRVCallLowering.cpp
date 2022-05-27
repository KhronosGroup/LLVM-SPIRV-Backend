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
#include "SPIRVGlobalRegistry.h"
#include "SPIRVISelLowering.h"
#include "SPIRVOpenCLBIFs.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVSubtarget.h"
#include "SPIRVUtils.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/Demangle/Demangle.h"

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
  uint32_t FuncControl = FunctionControl::None;
  if (F.hasFnAttribute(Attribute::AttrKind::AlwaysInline)) {
    FuncControl |= FunctionControl::Inline;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::ReadNone)) {
    FuncControl |= FunctionControl::Pure;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::ReadOnly)) {
    FuncControl |= FunctionControl::Const;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::NoInline)) {
    FuncControl |= FunctionControl::DontInline;
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

bool SPIRVCallLowering::lowerFormalArguments(MachineIRBuilder &MIRBuilder,
                                             const Function &F,
                                             ArrayRef<ArrayRef<Register>> VRegs,
                                             FunctionLoweringInfo &FLI) const {
  assert(GR && "Must initialize the SPIRV type registry before lowering args.");
  GR->setCurrentFunc(MIRBuilder.getMF());

  // Assign types and names to all args, and store their types for later.
  FunctionType *FTy = getOriginalFunctionType(F);
  SmallVector<Register, 4> ArgTypeVRegs;
  if (VRegs.size() > 0) {
    unsigned i = 0;
    for (const auto &Arg : F.args()) {
      assert(VRegs[i].size() == 1 && "Formal arg has multiple vregs");
      // auto *SpirvTy = GR->getOrCreateSPIRVType(Arg.getType(), MIRBuilder);
      // SPIRVType *SpirvTy = GR->getSPIRVTypeForVReg(VRegs[i][0]);
      // if (!SpirvTy)
      Type *ArgTy = FTy->getParamType(i);
      auto *SpirvTy = GR->assignTypeToVReg(ArgTy, VRegs[i][0], MIRBuilder);
      ArgTypeVRegs.push_back(GR->getSPIRVTypeID(SpirvTy));

      if (Arg.hasName())
        buildOpName(VRegs[i][0], Arg.getName(), MIRBuilder);
      if (Arg.getType()->isPointerTy()) {
        auto DerefBytes = static_cast<unsigned>(Arg.getDereferenceableBytes());
        if (DerefBytes != 0)
          buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration::MaxByteOffset,
                          {DerefBytes});
      }
      if (Arg.hasAttribute(Attribute::Alignment)) {
        auto Alignment = static_cast<unsigned>(
            Arg.getAttribute(Attribute::Alignment).getValueAsInt());
        buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration::Alignment,
                        {Alignment});
      }
      if (Arg.hasAttribute(Attribute::ReadOnly)) {
        buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration::FuncParamAttr,
                        {FunctionParameterAttribute::NoWrite});
      }
      if (Arg.hasAttribute(Attribute::ZExt)) {
        buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration::FuncParamAttr,
                        {FunctionParameterAttribute::Zext});
      }
      if (Arg.hasAttribute(Attribute::NoAlias)) {
        buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration::FuncParamAttr,
                        {FunctionParameterAttribute::NoAlias});
      }

      auto Node = F.getMetadata("kernel_arg_type_qual");
      if (Node && i < Node->getNumOperands()) {
        StringRef TypeQual = cast<MDString>(Node->getOperand(i))->getString();
        if (TypeQual.compare("volatile") == 0)
          buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration::Volatile, {});
      }
      Node = F.getMetadata("spirv.ParameterDecorations");
      if (Node && i < Node->getNumOperands() &&
          isa<MDNode>(Node->getOperand(i))) {
        MDNode *MD = cast<MDNode>(Node->getOperand(i));
        for (const MDOperand &MDOp : MD->operands()) {
          MDNode *MD2 = dyn_cast<MDNode>(MDOp);
          assert(MD2 && "Metadata operand is expected");
          ConstantInt *Const = getConstInt(MD2, 0);
          assert(Const && "MDOperand should be ConstantInt");
          auto Dec = static_cast<Decoration::Decoration>(Const->getZExtValue());
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

  auto FuncTy = GR->assignTypeToVReg(FTy, FuncVReg, MIRBuilder);

  // Build the OpTypeFunction declaring it.
  Register ReturnTypeID = FuncTy->getOperand(1).getReg();
  uint32_t FuncControl = getFunctionControl(F);

  MIRBuilder.buildInstr(SPIRV::OpFunction)
      .addDef(FuncVReg)
      .addUse(ReturnTypeID)
      .addImm(FuncControl)
      .addUse(GR->getSPIRVTypeID(FuncTy));

  // Add OpFunctionParameters.
  int i = 0;
  for (const auto &Arg : F.args()) {
    assert(VRegs[i].size() == 1 && "Formal arg has multiple vregs");
    MRI->setRegClass(VRegs[i][0], &SPIRV::IDRegClass);
    MIRBuilder.buildInstr(SPIRV::OpFunctionParameter)
        .addDef(VRegs[i][0])
        .addUse(ArgTypeVRegs[i]);
    if (F.isDeclaration())
      GR->add(&Arg, &MIRBuilder.getMF(), VRegs[i][0]);
    i++;
  }

  // Name the function.
  if (F.hasName())
    buildOpName(FuncVReg, F.getName(), MIRBuilder);

  // Handle entry points and function linkage.
  if (F.getCallingConv() == CallingConv::SPIR_KERNEL) {
    auto ExecModel = ExecutionModel::Kernel;
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpEntryPoint)
                   .addImm(ExecModel)
                   .addUse(FuncVReg);
    addStringImm(F.getName(), MIB);
  } else if (F.getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage ||
             F.getLinkage() == GlobalValue::LinkOnceODRLinkage) {
    auto LnkTy = F.isDeclaration() ? LinkageType::Import : LinkageType::Export;
    buildOpDecorate(FuncVReg, MIRBuilder, Decoration::LinkageAttributes,
                    {static_cast<uint32_t>(LnkTy)}, F.getGlobalIdentifier());
  }

  return true;
}

bool SPIRVCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                  CallLoweringInfo &Info) const {
  auto FuncName = Info.Callee.getGlobal()->getGlobalIdentifier();
  auto &MF = MIRBuilder.getMF();
  GR->setCurrentFunc(MF);
  FunctionType *FTy = nullptr;

  size_t n;
  int Status;
  char *DemangledName = itaniumDemangle(FuncName.c_str(), nullptr, &n, &Status);

  assert(Info.OrigRet.Regs.size() < 2 && "Call returns multiple vregs");

  Register ResVReg =
      Info.OrigRet.Regs.empty() ? Register(0) : Info.OrigRet.Regs[0];
  bool SingleUnderscore = FuncName.size() >= 1 && FuncName[0] == '_';
  bool DoubleUnderscore =
      SingleUnderscore && FuncName.size() >= 2 && FuncName[1] == '_';
  // FIXME: OCL builtin checks should be the same as in clang
  //        or SPIRV-LLVM translator.
  if ((Status == demangle_success && SingleUnderscore) || DoubleUnderscore) {
    const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());
    if (ST->canUseExtInstSet(ExtInstSet::OpenCL_std)) {
      // Mangled names are for OpenCL builtins, so pass off to OpenCLBIFs.cpp.
      SmallVector<Register, 8> ArgVRegs;
      for (auto Arg : Info.OrigArgs) {
        assert(Arg.Regs.size() == 1 && "Call arg has multiple VRegs");
        ArgVRegs.push_back(Arg.Regs[0]);
        auto SPIRVTy = GR->getOrCreateSPIRVType(Arg.Ty, MIRBuilder);
        GR->assignSPIRVTypeToVReg(SPIRVTy, Arg.Regs[0], MIRBuilder.getMF());
      }
      bool Result = generateOpenCLBuiltinCall(
          DoubleUnderscore ? FuncName : DemangledName, MIRBuilder, ResVReg,
          Info.OrigRet.Ty, ArgVRegs, GR);
      free(DemangledName);
      return Result;
    }
    llvm_unreachable("Unable to handle this environment's built-in funcs.");
  } else {
    free(DemangledName);
    // Emit a regular OpFunctionCall. If it's an externally declared function,
    // be sure to emit its type and function declaration here. It will be
    // hoisted globally later.
    if (Info.Callee.isGlobal()) {
      const Function *CF = cast<const Function>(Info.Callee.getGlobal());
      Register FuncVReg;
      if (CF->isDeclaration() &&
          (GR->find(CF, &MIRBuilder.getMF(), FuncVReg) == false)) {
        // Emit the type info and forward function declaration to the first MBB
        // to ensure VReg definition dependencies are valid across all MBBs.
        MachineIRBuilder FirstBlockBuilder;
        FirstBlockBuilder.setMF(MF);
        FirstBlockBuilder.setMBB(*MF.getBlockNumbered(0));

        SmallVector<ArrayRef<Register>, 8> VRegArgs;
        SmallVector<SmallVector<Register, 1>, 8> ToInsert;
        for (const Argument &Arg : CF->args()) {
          if (MIRBuilder.getDataLayout()
                  .getTypeStoreSize(Arg.getType())
                  .isZero())
            continue; // Don't handle zero sized types.
          ToInsert.push_back({MIRBuilder.getMRI()->createGenericVirtualRegister(
              LLT::scalar(32))});
          VRegArgs.push_back(ToInsert.back());
        }
        // TODO: Reuse FunctionLoweringInfo
        FunctionLoweringInfo FuncInfo;
        lowerFormalArguments(FirstBlockBuilder, *CF, VRegArgs, FuncInfo);
      }
      FTy = getOriginalFunctionType(*CF);
    }

    // Make sure there's a valid return reg, even for functions returning void.
    if (!ResVReg.isValid()) {
      ResVReg = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
    }
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
}
