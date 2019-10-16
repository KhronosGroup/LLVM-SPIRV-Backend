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
#include "SPIRV.h"
#include "SPIRVEnums.h"
#include "SPIRVISelLowering.h"
#include "SPIRVOpenCLBIFs.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVStrings.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTypeRegistry.h"

#include "llvm/Demangle/Demangle.h"

using namespace llvm;

SPIRVCallLowering::SPIRVCallLowering(const SPIRVTargetLowering &TLI,
                                     SPIRVTypeRegistry *TR)
    : CallLowering(&TLI), TR(TR) {}

bool SPIRVCallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                    const Value *Val,
                                    ArrayRef<Register> VRegs) const {
  assert(VRegs.size() < 2 && "All return types should use a single register");
  if (Val) {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpReturnValue).addUse(VRegs[0]);
    return TR->constrainRegOperands(MIB);
  } else {
    MIRBuilder.buildInstr(SPIRV::OpReturn);
    return true;
  }
}

// Based on the LLVM function attributes, get a SPIR-V FunctionControl
static uint32_t getFunctionControl(const Function &F) {
  uint32_t funcControl = FunctionControl::None;
  if (F.hasFnAttribute(Attribute::AttrKind::AlwaysInline)) {
    funcControl |= FunctionControl::Inline;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::ReadNone)) {
    funcControl |= FunctionControl::Const;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::ReadOnly)) {
    funcControl |= FunctionControl::Pure;
  }
  if (F.hasFnAttribute(Attribute::AttrKind::NoInline)) {
    funcControl |= FunctionControl::DontInline;
  }
  return funcControl;
}

bool SPIRVCallLowering::lowerFormalArguments(
    MachineIRBuilder &MIRBuilder, const Function &F,
    ArrayRef<ArrayRef<Register>> VRegs) const {

  assert(TR && "Must initialize the SPIRV type registry before lowering args.");

  // Assign types and names to all args, and store their types for later
  SmallVector<Register, 4> argTypeVRegs;
  if (VRegs.size() > 0) {
    unsigned int i = 0;
    for (const auto &Arg : F.args()) {
      assert(VRegs[i].size() == 1 && "Formal arg has multiple vregs");
      SPIRVType *spirvTy = TR->getSPIRVTypeForVReg(VRegs[i][0]);
      if (!spirvTy) {
        spirvTy = TR->assignTypeToVReg(Arg.getType(), VRegs[i][0], MIRBuilder);
      }
      argTypeVRegs.push_back(TR->getSPIRVTypeID(spirvTy));

      if (Arg.hasName()) {
        buildOpName(VRegs[i][0], Arg.getName(), MIRBuilder);
      }
      ++i;
    }
  }

  // Generate a SPIR-V type for the function
  auto MRI = MIRBuilder.getMRI();
  Register funcVReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
  auto funcTy = TR->assignTypeToVReg(F.getFunctionType(), funcVReg, MIRBuilder);

  // Build the OpTypeFunction declaring it
  Register returnTypeID = funcTy->getOperand(1).getReg();
  uint32_t funcControl = getFunctionControl(F);
  MIRBuilder.buildInstr(SPIRV::OpFunction)
      .addDef(funcVReg)
      .addUse(returnTypeID)
      .addImm(funcControl)
      .addUse(TR->getSPIRVTypeID(funcTy));

  // Add OpFunctionParameters
  const unsigned int numArgs = argTypeVRegs.size();
  for (unsigned int i = 0; i < numArgs; ++i) {
    assert(VRegs[i].size() == 1 && "Formal arg has multiple vregs");
    MIRBuilder.buildInstr(SPIRV::OpFunctionParameter)
        .addDef(VRegs[i][0])
        .addUse(argTypeVRegs[i]);
  }

  // Name the function
  if (F.hasName()) {
    buildOpName(funcVReg, F.getName(), MIRBuilder);
  }

  // Handle entry points and function linkage
  if (F.getCallingConv() == CallingConv::SPIR_KERNEL) {
    auto execModel = ExecutionModel::Kernel;
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpEntryPoint)
                   .addImm(execModel)
                   .addUse(funcVReg);
    addStringImm(F.getName(), MIB);
  } else if (F.getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage) {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpDecorate)
                   .addUse(funcVReg)
                   .addImm(Decoration::LinkageAttributes);
    addStringImm(F.getGlobalIdentifier(), MIB);
    auto lnk = F.isDeclaration() ? LinkageType::Import : LinkageType::Export;
    MIB.addImm(lnk);
  }

  return true;
}

bool SPIRVCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                  CallLoweringInfo &Info) const {
  // TODO It's not clear if Info needs to be updated in any way.
  //      Why isn't it a const&?
  //      See https://reviews.llvm.org/D65850#inline-620779
  // TODO assert Info.CallConv has an appropriate value.
  // TODO Handle IsMustTailCall, IsTailCall, LoweredTailCall and IsVarArg.

  auto funcName = Info.Callee.getGlobal()->getGlobalIdentifier();

  size_t n;
  int status;
  char *demangledName = itaniumDemangle(funcName.c_str(), nullptr, &n, &status);

  assert(Info.OrigRet.Regs.size() < 2 && "Call returns multiple vregs");

  Register resVReg =
      Info.OrigRet.Regs.empty() ? Register(0) : Info.OrigRet.Regs[0];
  bool doubleUnderscore =
      funcName.size() >= 2 && funcName[0] == '_' && funcName[1] == '_';
  if (status == demangle_success || doubleUnderscore) {
    const auto &MF = MIRBuilder.getMF();
    const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());
    if (ST->canUseExtInstSet(ExtInstSet::OpenCL_std)) {
      // Mangled names are for OpenCL builtins, so pass off to OpenCLBIFs.cpp
      SmallVector<Register, 8> argVRegs;
      for (auto Arg : Info.OrigArgs) {
        assert(Arg.Regs.size() == 1 && "Call arg has multiple VRegs");
        argVRegs.push_back(Arg.Regs[0]);
      }
      return generateOpenCLBuiltinCall(
          doubleUnderscore ? funcName : demangledName, MIRBuilder, resVReg,
          Info.OrigRet.Ty, argVRegs, TR);
    }
    report_fatal_error("Unable to handle this environment's built-in funcs.");
  } else {
    // Emit a regular OpFunctionCall

    // If it's an externally declared function, be sure to emit its type and
    // function declaration here. It will be hoisted globally later
    auto M = MIRBuilder.getMF().getFunction().getParent();
    Function *calledFunc = M->getFunction(funcName);
    if (calledFunc && calledFunc->isDeclaration()) {
      // Emit the type info and forward function declaration to the first MBB
      // to ensure VReg definition dependencies are valid across all MBBs
      MachineIRBuilder firstBlockBuilder;
      auto &MF = MIRBuilder.getMF();
      firstBlockBuilder.setMF(MF);
      firstBlockBuilder.setMBB(*MF.getBlockNumbered(0));
      lowerFormalArguments(firstBlockBuilder, *calledFunc, {});
    }

    // Make sure there's a valid return reg, even for functions returning void
    if (!resVReg.isValid()) {
      resVReg = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
    }
    SPIRVType *retType =
        TR->assignTypeToVReg(Info.OrigRet.Ty, resVReg, MIRBuilder);

    // Emit the OpFunctionCall and its args
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpFunctionCall)
                   .addDef(resVReg)
                   .addUse(TR->getSPIRVTypeID(retType))
                   .add(Info.Callee);

    for (const auto &arg : Info.OrigArgs) {
      assert(arg.Regs.size() == 1 && "Call arg has multiple VRegs");
      MIB.addUse(arg.Regs[0]);
    }
    return TR->constrainRegOperands(MIB);
  }
}
