//===- SPIRVModuleAnalysis.cpp - analysis of global instrs & regs - C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The analysis collects instructions that should be output at the module level
// and performs the global register numbering.
//
// The results of this analysis are used in AsmPrinter to rename registers
// globally and to output required instructions at the module level.
//
//===----------------------------------------------------------------------===//

#include "SPIRVModuleAnalysis.h"
#include "SPIRV.h"
#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVGlobalRegistry.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVUtils.h"
#include "TargetInfo/SPIRVTargetInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-module-analysis"

char llvm::SPIRVModuleAnalysis::ID = 0;

namespace llvm {
void initializeSPIRVModuleAnalysisPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS(SPIRVModuleAnalysis, DEBUG_TYPE, "SPIRV module analysis", true,
                true)

// Retrieve an unsigned int from an MDNode with a list of them as operands.
static unsigned int getMetadataUInt(MDNode *MdNode, unsigned int OpIndex,
                                    unsigned int DefaultVal = 0) {
  if (MdNode && OpIndex < MdNode->getNumOperands()) {
    const auto &Op = MdNode->getOperand(OpIndex);
    return mdconst::extract<ConstantInt>(Op)->getZExtValue();
  }
  return DefaultVal;
}

void SPIRVModuleAnalysis::setBaseInfo(const Module &M) {
  MAI.MaxID = 0;
  for (int i = 0; i < NUM_MODULE_SECTIONS; i++)
    MAI.MS[i].clear();
  MAI.RegisterAliasTable.clear();
  MAI.InstrsToDelete.clear();
  MAI.FuncNameMap.clear();
  MAI.GlobalVarList.clear();
  MAI.ExtInstSetMap.clear();
  MAI.Reqs.clear();

  // TODO: determine memory model and source language from the configuratoin.
  MAI.Mem = MemoryModel::OpenCL;
  MAI.SrcLang = SourceLanguage::OpenCL_C;
  unsigned PtrSize = ST->getPointerSize();
  MAI.Addr = PtrSize == 32 ? AddressingModel::Physical32
                           : PtrSize == 64 ? AddressingModel::Physical64
                                           : AddressingModel::Logical;
  // Get the OpenCL version number from metadata.
  // TODO: support other source languages.
  MAI.SrcLangVersion = 0;
  if (auto VerNode = M.getNamedMetadata("opencl.ocl.version")) {
    // Construct version literal according to OpenCL 2.2 environment spec.
    auto VersionMD = VerNode->getOperand(0);
    unsigned MajorNum = getMetadataUInt(VersionMD, 0, 2);
    unsigned MinorNum = getMetadataUInt(VersionMD, 1);
    unsigned RevNum = getMetadataUInt(VersionMD, 2);
    MAI.SrcLangVersion = 0 | (MajorNum << 16) | (MinorNum << 8) | RevNum;
  }
  // Update required capabilities for this memory model, addressing model and
  // source language.
  MAI.Reqs.addRequirements(getSymbolicOperandRequirements(
      OperandCategory::MemoryModelOperand, MAI.Mem, *ST));
  MAI.Reqs.addRequirements(getSymbolicOperandRequirements(
      OperandCategory::SourceLanguageOperand, MAI.SrcLang, *ST));
  MAI.Reqs.addRequirements(getSymbolicOperandRequirements(
      OperandCategory::AddressingModelOperand, MAI.Addr, *ST));

  // TODO: check if it's required by default.
  MAI.ExtInstSetMap[static_cast<unsigned>(ExtInstSet::OpenCL_std)] =
      Register::index2VirtReg(MAI.getNextID());
}

template <typename T>
static void makeRegisterAliases(SPIRVGlobalRegistry *GR,
                                ModuleAnalysisInfo &MAI) {
  // Make global registers for entries of DT.
  for (auto &CU : GR->getAllUses<T>()) {
    Register GlobalReg = Register::index2VirtReg(MAI.getNextID());
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      MAI.setRegisterAlias(MF, Reg, GlobalReg);
    }
  }
  // Make global registers for special types and constants collected in the map.
  if (std::is_same<T, Type>::value) {
    for (auto &CU : GR->getSpecialTypesAndConstsMap()) {
      for (auto &TypeGroup : CU.second) {
        Register GlobalReg = Register::index2VirtReg(MAI.getNextID());
        for (auto &t : TypeGroup) {
          MachineFunction *MF = t.first;
          assert(t.second->getOperand(0).isReg());
          Register Reg = t.second->getOperand(0).getReg();
          MAI.setRegisterAlias(MF, Reg, GlobalReg);
        }
      }
    }
  }
}

// The set stores IDs of types, consts, global vars and func decls
// already inserted into MS list.
static DenseSet<Register> InsertedTypeConstVarDefs;
// Insert instruction to a module section if it was not inserted before.
// Set skip emission on function output.
static void insertInstrToMS(Register GlobalReg, MachineInstr *MI,
                            ModuleAnalysisInfo *MAI, ModuleSectionType MSType) {
  assert(MI && "There should be an instruction that defines the register");
  MAI->setSkipEmission(MI);
  if (!InsertedTypeConstVarDefs.contains(GlobalReg)) {
    MAI->MS[MSType].push_back(MI);
    InsertedTypeConstVarDefs.insert(GlobalReg);
  }
}
// Collect MI which defines the register in the given machine function.
static void collectDefInstr(Register Reg, MachineFunction *MF,
                            ModuleAnalysisInfo *MAI, ModuleSectionType MSType) {
  assert(MAI->hasRegisterAlias(MF, Reg) && "Cannot find register alias");
  Register GlobalReg = MAI->getRegisterAlias(MF, Reg);
  MachineInstr *MI = MF->getRegInfo().getVRegDef(Reg);
  insertInstrToMS(GlobalReg, MI, MAI, MSType);
}

template <typename T> void SPIRVModuleAnalysis::collectTypesConstsVars() {
  // Collect instructions from DT.
  for (auto &CU : GR->getAllUses<T>()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      // Some instructions may be deleted during global isel, so collect only
      // those that exist in current IR.
      if (MF->getRegInfo().getVRegDef(Reg)) {
        collectDefInstr(Reg, MF, &MAI, MB_TypeConstVars);
        if (std::is_same<T, GlobalValue>::value)
          MAI.GlobalVarList.push_back(MF->getRegInfo().getVRegDef(Reg));
      }
    }
  }
  // Hoist special types and constants collected in the map.
  if (std::is_same<T, Type>::value) {
    for (auto &CU : GR->getSpecialTypesAndConstsMap())
      for (auto &TypeGroup : CU.second)
        for (auto &t : TypeGroup) {
          MachineFunction *MF = t.first;
          assert(t.second->getOperand(0).isReg());
          Register Reg = t.second->getOperand(0).getReg();
          collectDefInstr(Reg, MF, &MAI, MB_TypeConstVars);
        }
  }
}

// We need to process this separately as for function decls we want to not only
// collect OpFunctions but OpFunctionParameters too.
//
// TODO: maybe consider replacing this with explicit OpFunctionParameter
// generation here instead handling it in CallLowering.
static void collectFuncDecls(SPIRVGlobalRegistry *GR, ModuleAnalysisInfo *MAI) {
  for (auto &CU : GR->getAllUses<Function>()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      MachineInstr *MI = MF->getRegInfo().getVRegDef(Reg);
      while (MI && (MI->getOpcode() == SPIRV::OpFunction ||
                    MI->getOpcode() == SPIRV::OpFunctionParameter)) {
        Reg = MI->getOperand(0).getReg();
        if (MI->getOpcode() == SPIRV::OpFunctionParameter) {
          Register GlobalReg = Register::index2VirtReg(MAI->getNextID());
          ;
          MAI->setRegisterAlias(MF, Reg, GlobalReg);
          insertInstrToMS(GlobalReg, MI, MAI, MB_ExtFuncDecls);
        } else {
          collectDefInstr(Reg, MF, MAI, MB_ExtFuncDecls);
        }
        MI = MI->getNextNode();
        Reg = MI->getOperand(0).getReg();
      }
    }
  }
}

// The function initializes global register alias table for types, consts,
// global vars and func decls and collects these instruction for output
// at module level. Also it collects explicit OpExtension/OpCapability
// instructions.
void SPIRVModuleAnalysis::processDefInstrs(const Module &M) {
  // OpTypes and OpConstants can have cross references so at first we create
  // new global registers and map them to old regs, then walk the list of
  // instructions, and select ones that should be emitted at module level.
  // Also there are references to global values in constants.
  makeRegisterAliases<Type>(GR, MAI);
  makeRegisterAliases<Constant>(GR, MAI);
  makeRegisterAliases<GlobalValue>(GR, MAI);

  collectTypesConstsVars<Type>();
  collectTypesConstsVars<Constant>();

  for (auto F = M.begin(), E = M.end(); F != E; ++F) {
    MachineFunction *MF = MMI->getMachineFunction(*F);
    if (MF) {
      // Iterate through and hoist any instructions we can at this stage.
      // bool hasSeenFirstOpFunction = false;
      for (MachineBasicBlock &MBB : *MF) {
        for (MachineInstr &MI : MBB) {
          if (MI.getOpcode() == SPIRV::OpExtension) {
            // Here, OpExtension just has a single enum operand, not a string
            auto Ext = Extension::Extension(MI.getOperand(0).getImm());
            MAI.Reqs.addExtension(Ext);
            MAI.setSkipEmission(&MI);
          } else if (MI.getOpcode() == SPIRV::OpCapability) {
            auto Cap = Capability::Capability(MI.getOperand(0).getImm());
            MAI.Reqs.addCapability(Cap);
            MAI.setSkipEmission(&MI);
          }
        }
      }
    }
  }

  collectTypesConstsVars<GlobalValue>();
  makeRegisterAliases<Function>(GR, MAI);
  collectFuncDecls(GR, &MAI);
}

// True if there is an instruction in the MS list with all the same operands as
// the given instruction has (after the given starting index).
// TODO: maybe it needs to check Opcodes too.
static bool findSameInstrInMS(const MachineInstr &A, ModuleSectionType MSType,
                              ModuleAnalysisInfo &MAI,
                              unsigned int StartOpIndex = 0) {
  for (const auto *B : MAI.MS[MSType]) {
    const unsigned int NumAOps = A.getNumOperands();
    if (NumAOps == B->getNumOperands() && A.getNumDefs() == B->getNumDefs()) {
      bool AllOpsMatch = true;
      for (unsigned i = StartOpIndex; i < NumAOps && AllOpsMatch; ++i) {
        if (A.getOperand(i).isReg() && B->getOperand(i).isReg()) {
          Register RegA = A.getOperand(i).getReg();
          Register RegB = B->getOperand(i).getReg();
          AllOpsMatch = MAI.getRegisterAlias(A.getMF(), RegA) ==
                        MAI.getRegisterAlias(B->getMF(), RegB);
        } else {
          AllOpsMatch = A.getOperand(i).isIdenticalTo(B->getOperand(i));
        }
      }
      if (AllOpsMatch)
        return true;
    }
  }
  return false;
}

// Look for IDs declared with Import linkage, and map the imported name string
// to the register defining that variable (which will usually be the result of
// an OpFunction). This lets us call externally imported functions using
// the correct ID registers.
void SPIRVModuleAnalysis::collectFuncNames(MachineInstr &MI,
                                           const Function &F) {
  if (MI.getOpcode() == SPIRV::OpDecorate) {
    // If it's got Import linkage
    auto Dec = MI.getOperand(1).getImm();
    if (Dec == Decoration::LinkageAttributes) {
      auto Lnk = MI.getOperand(MI.getNumOperands() - 1).getImm();
      if (Lnk == LinkageType::Import) {
        // Map imported function name to function ID register.
        std::string Name = getStringImm(MI, 2);
        Register Target = MI.getOperand(0).getReg();
        // TODO: check defs from different MFs.
        MAI.FuncNameMap[Name] = MAI.getRegisterAlias(MI.getMF(), Target);
      }
    }
  } else if (MI.getOpcode() == SPIRV::OpFunction) {
    // Record all internal OpFunction declarations.
    Register Reg = MI.defs().begin()->getReg();
    Register GlobalReg = MAI.getRegisterAlias(MI.getMF(), Reg);
    assert(GlobalReg.isValid());
    // TODO: check that it does not conflict with existing entries.
    MAI.FuncNameMap[F.getGlobalIdentifier()] = GlobalReg;
  }
}

// Collect the given instruction in the specified MS. We assume global register
// numbering has already occurred by this point. We can directly compare reg
// arguments when detecting duplicates.
static void collectOtherInstr(MachineInstr &MI, ModuleAnalysisInfo &MAI,
                              ModuleSectionType MSType) {
  MAI.setSkipEmission(&MI);
  if (findSameInstrInMS(MI, MSType, MAI))
    return; // Found a duplicate, so don't add it
  // No duplicates, so add it.
  MAI.MS[MSType].push_back(&MI);
}

// Some global instructions make reference to function-local ID regs, so cannot
// be correctly collected until these registers are globally numbered.
void SPIRVModuleAnalysis::processOtherInstrs(const Module &M) {
  for (auto F = M.begin(), E = M.end(); F != E; ++F) {
    MachineFunction *MF = MMI->getMachineFunction(*F);
    if (MF) {
      for (MachineBasicBlock &MBB : *MF)
        for (MachineInstr &MI : MBB) {
          if (MAI.getSkipEmission(&MI))
            continue;
          const unsigned OpCode = MI.getOpcode();
          if (OpCode == SPIRV::OpName || OpCode == SPIRV::OpMemberName) {
            collectOtherInstr(MI, MAI, MB_DebugNames);
          } else if (OpCode == SPIRV::OpEntryPoint) {
            collectOtherInstr(MI, MAI, MB_EntryPoints);
          } else if (TII->isDecorationInstr(MI)) {
            collectOtherInstr(MI, MAI, MB_Annotations);
            collectFuncNames(MI, *F);
          } else if (TII->isConstantInstr(MI)) {
            // Now OpSpecConstant*s are not in DT,
            // but they need to be collected anyway.
            collectOtherInstr(MI, MAI, MB_TypeConstVars);
          } else if (OpCode == SPIRV::OpFunction) {
            collectFuncNames(MI, *F);
          }
        }
    }
  }
}

// Number registers in all functions globally from 0 onwards and store
// the result in global register alias table. Some registers are already
// numbered in makeRegisterAliases.
void SPIRVModuleAnalysis::numberRegistersGlobally(const Module &M) {
  for (auto F = M.begin(), E = M.end(); F != E; ++F) {
    MachineFunction *MF = MMI->getMachineFunction(*F);
    if (MF) {
      for (MachineBasicBlock &MBB : *MF) {
        for (MachineInstr &MI : MBB) {
          for (MachineOperand &Op : MI.operands()) {
            if (Op.isReg()) {
              Register Reg = Op.getReg();
              if (!MAI.hasRegisterAlias(MF, Reg)) {
                Register NewReg = Register::index2VirtReg(MAI.getNextID());
                MAI.setRegisterAlias(MF, Reg, NewReg);
              }
            }
          }
          if (MI.getOpcode() == SPIRV::OpExtInst) {
            auto Set = MI.getOperand(2).getImm();
            if (MAI.ExtInstSetMap.find(Set) == MAI.ExtInstSetMap.end())
              MAI.ExtInstSetMap[Set] = Register::index2VirtReg(MAI.getNextID());
          }
        }
      }
    }
  }
}

// Find OpIEqual and OpBranchConditional instructions originating from
// OpSwitches, mark them skipped for emission. Also mark MBB skipped if it
// contains only these instructions.
static void processSwitches(const Module &M, ModuleAnalysisInfo &MAI,
                            MachineModuleInfo *MMI) {
  DenseSet<Register> SwitchRegs;
  for (auto F = M.begin(), E = M.end(); F != E; ++F) {
    MachineFunction *MF = MMI->getMachineFunction(*F);
    if (MF) {
      for (MachineBasicBlock &MBB : *MF)
        for (MachineInstr &MI : MBB) {
          if (MAI.getSkipEmission(&MI))
            continue;
          if (MI.getOpcode() == SPIRV::OpSwitch) {
            assert(MI.getOperand(0).isReg());
            SwitchRegs.insert(MI.getOperand(0).getReg());
          }
          if (MI.getOpcode() == SPIRV::OpIEqual && MI.getOperand(2).isReg() &&
              SwitchRegs.contains(MI.getOperand(2).getReg())) {
            Register CmpReg = MI.getOperand(0).getReg();
            MachineInstr *CBr = MI.getNextNode();
            assert(CBr && CBr->getOpcode() == SPIRV::OpBranchConditional &&
                   CBr->getOperand(0).isReg() &&
                   CBr->getOperand(0).getReg() == CmpReg);
            MAI.setSkipEmission(&MI);
            MAI.setSkipEmission(CBr);
            if (&MBB.front() == &MI && &MBB.back() == CBr)
              MAI.MBBsToSkip.insert(&MBB);
          }
        }
    }
  }
}

// Add VariablePointers to the requirements if this instruction defines
// a pointer (Logical only).
static void addVariablePtrInstrReqs(const MachineInstr &MI,
                                    SPIRVRequirementHandler &Reqs,
                                    const SPIRVSubtarget &ST) {
  if (ST.isLogicalAddressing()) {
    const MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();
    Register Type = MI.getOperand(1).getReg();
    if (MRI.getVRegDef(Type)->getOpcode() == SPIRV::OpTypePointer)
      Reqs.addCapability(Capability::VariablePointers);
  }
}

// Add the required capabilities from a decoration instruction (including
// BuiltIns).
static void addOpDecorateReqs(const MachineInstr &MI, unsigned int DecIndex,
                              SPIRVRequirementHandler &Reqs,
                              const SPIRVSubtarget &ST) {
  int64_t DecOp = MI.getOperand(DecIndex).getImm();
  auto Dec = static_cast<Decoration::Decoration>(DecOp);
  Reqs.addRequirements(getSymbolicOperandRequirements(
      OperandCategory::DecorationOperand, Dec, ST));

  if (Dec == Decoration::BuiltIn) {
    int64_t BuiltInOp = MI.getOperand(DecIndex + 1).getImm();
    auto BuiltIn = static_cast<BuiltIn::BuiltIn>(BuiltInOp);
    Reqs.addRequirements(getSymbolicOperandRequirements(
        OperandCategory::BuiltInOperand, BuiltIn, ST));
  }
}

// Add requirements for image handling.
static void addOpTypeImageReqs(const MachineInstr &MI,
                               SPIRVRequirementHandler &Reqs,
                               const SPIRVSubtarget &ST) {
  assert(MI.getNumOperands() >= 8 && "Insufficient operands for OpTypeImage");
  // The operand indices used here are based on the OpTypeImage layout, which
  // the MachineInstr follows as well.
  int64_t ImgFormatOp = MI.getOperand(7).getImm();
  auto ImgFormat = static_cast<ImageFormat::ImageFormat>(ImgFormatOp);
  Reqs.addRequirements(getSymbolicOperandRequirements(
      OperandCategory::ImageFormatOperand, ImgFormat, ST));

  bool IsArrayed = MI.getOperand(4).getImm() == 1;
  bool IsMultisampled = MI.getOperand(5).getImm() == 1;
  bool NoSampler = MI.getOperand(6).getImm() == 2;
  // Add dimension requirements.
  assert(MI.getOperand(2).isImm());
  switch (MI.getOperand(2).getImm()) {
  case Dim::DIM_1D:
    Reqs.addRequirements(NoSampler ? Capability::Image1D
                                   : Capability::Sampled1D);
    break;
  case Dim::DIM_2D:
    if (IsMultisampled && NoSampler)
      Reqs.addRequirements(Capability::ImageMSArray);
    break;
  case Dim::DIM_Cube:
    Reqs.addRequirements(Capability::Shader);
    if (IsArrayed)
      Reqs.addRequirements(NoSampler ? Capability::ImageCubeArray
                                     : Capability::SampledCubeArray);
    break;
  case Dim::DIM_Rect:
    Reqs.addRequirements(NoSampler ? Capability::ImageRect
                                   : Capability::SampledRect);
    break;
  case Dim::DIM_Buffer:
    Reqs.addRequirements(NoSampler ? Capability::ImageBuffer
                                   : Capability::SampledBuffer);
    break;
  case Dim::DIM_SubpassData:
    Reqs.addRequirements(Capability::InputAttachment);
    break;
  }

  if (ST.isKernel()) {
    // Has optional access qualifier.
    if (MI.getNumOperands() > 8 && MI.getOperand(8).getImm() == AQ::ReadWrite)
      Reqs.addRequirements(Capability::ImageReadWrite);
    else
      Reqs.addRequirements(Capability::ImageBasic);
  }
}

void addInstrRequirements(const MachineInstr &MI, SPIRVRequirementHandler &Reqs,
                          const SPIRVSubtarget &ST) {
  switch (MI.getOpcode()) {
  case SPIRV::OpMemoryModel: {
    int64_t Addr = MI.getOperand(0).getImm();
    Reqs.addRequirements(getSymbolicOperandRequirements(
        OperandCategory::AddressingModelOperand, Addr, ST));
    int64_t Mem = MI.getOperand(1).getImm();
    Reqs.addRequirements(getSymbolicOperandRequirements(
        OperandCategory::MemoryModelOperand, Mem, ST));
    break;
  }
  case SPIRV::OpEntryPoint: {
    int64_t Exe = MI.getOperand(0).getImm();
    Reqs.addRequirements(getSymbolicOperandRequirements(
        OperandCategory::ExecutionModelOperand, Exe, ST));
    break;
  }
  case SPIRV::OpExecutionMode:
  case SPIRV::OpExecutionModeId: {
    int64_t Exe = MI.getOperand(1).getImm();
    Reqs.addRequirements(getSymbolicOperandRequirements(
        OperandCategory::ExecutionModeOperand, Exe, ST));
    break;
  }
  case SPIRV::OpTypeMatrix:
    Reqs.addCapability(Capability::Matrix);
    break;
  case SPIRV::OpTypeInt: {
    unsigned BitWidth = MI.getOperand(1).getImm();
    if (BitWidth == 64)
      Reqs.addCapability(Capability::Int64);
    else if (BitWidth == 16)
      Reqs.addCapability(Capability::Int16);
    else if (BitWidth == 8)
      Reqs.addCapability(Capability::Int8);
    break;
  }
  case SPIRV::OpTypeFloat: {
    unsigned BitWidth = MI.getOperand(1).getImm();
    if (BitWidth == 64)
      Reqs.addCapability(Capability::Float64);
    else if (BitWidth == 16)
      Reqs.addCapability(Capability::Float16);
    break;
  }
  case SPIRV::OpTypeVector: {
    unsigned NumComponents = MI.getOperand(2).getImm();
    if (NumComponents == 8 || NumComponents == 16)
      Reqs.addCapability(Capability::Vector16);
    break;
  }
  case SPIRV::OpTypePointer: {
    auto SC = MI.getOperand(1).getImm();
    Reqs.addRequirements(getSymbolicOperandRequirements(
        OperandCategory::StorageClassOperand, SC, ST));
    // If it's a type of pointer to float16, add Float16Buffer capability.
    assert(MI.getOperand(2).isReg());
    const MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();
    SPIRVType *TypeDef = MRI.getVRegDef(MI.getOperand(2).getReg());
    if (TypeDef->getOpcode() == SPIRV::OpTypeFloat &&
        TypeDef->getOperand(1).getImm() == 16)
      Reqs.addCapability(Capability::Float16Buffer);
    break;
  }
  case SPIRV::OpBitReverse:
  case SPIRV::OpTypeRuntimeArray:
    Reqs.addCapability(Capability::Shader);
    break;
  case SPIRV::OpTypeOpaque:
  case SPIRV::OpTypeEvent:
    Reqs.addCapability(Capability::Kernel);
    break;
  case SPIRV::OpTypePipe:
  case SPIRV::OpTypeReserveId:
    Reqs.addCapability(Capability::Pipes);
    break;
  case SPIRV::OpTypeDeviceEvent:
  case SPIRV::OpTypeQueue:
    Reqs.addCapability(Capability::DeviceEnqueue);
    break;
  case SPIRV::OpDecorate:
  case SPIRV::OpDecorateId:
  case SPIRV::OpDecorateString:
    addOpDecorateReqs(MI, 1, Reqs, ST);
    break;
  case SPIRV::OpMemberDecorate:
  case SPIRV::OpMemberDecorateString:
    addOpDecorateReqs(MI, 2, Reqs, ST);
    break;
  case SPIRV::OpInBoundsPtrAccessChain:
    Reqs.addCapability(Capability::Addresses);
    break;
  case SPIRV::OpConstantSampler:
    Reqs.addCapability(Capability::LiteralSampler);
    break;
  case SPIRV::OpTypeImage:
    addOpTypeImageReqs(MI, Reqs, ST);
    break;
  case SPIRV::OpTypeSampler:
    Reqs.addCapability(Capability::ImageBasic);
    break;
  case SPIRV::OpTypeForwardPointer:
    if (ST.isKernel())
      Reqs.addCapability(Capability::Addresses);
    else
      Reqs.addCapability(Capability::PhysicalStorageBufferAddressesEXT);
    break;
  // TODO: reconsider .td patterns to get rid of such a long switches maybe
  // just removing the pattern for OpSelect is enough, but binary ops may be
  // quite various too.
  case SPIRV::OpSelectSISCond:
  case SPIRV::OpSelectSFSCond:
  case SPIRV::OpSelectVISCond:
  case SPIRV::OpSelectVFSCond:
  case SPIRV::OpSelectSIVCond:
  case SPIRV::OpSelectSFVCond:
  case SPIRV::OpSelectVIVCond:
  case SPIRV::OpSelectVFVCond:
  case SPIRV::OpPhi:
  case SPIRV::OpFunctionCall:
  case SPIRV::OpPtrAccessChain:
  case SPIRV::OpLoad:
  case SPIRV::OpConstantNull:
    addVariablePtrInstrReqs(MI, Reqs, ST);
    break;
  case SPIRV::OpAtomicFlagTestAndSet:
  case SPIRV::OpAtomicLoad:
  case SPIRV::OpAtomicStore:
  case SPIRV::OpAtomicExchange:
  case SPIRV::OpAtomicCompareExchange:
  case SPIRV::OpAtomicIIncrement:
  case SPIRV::OpAtomicIDecrement:
  case SPIRV::OpAtomicIAdd:
  case SPIRV::OpAtomicISub:
  case SPIRV::OpAtomicUMin:
  case SPIRV::OpAtomicUMax:
  case SPIRV::OpAtomicSMin:
  case SPIRV::OpAtomicSMax:
  case SPIRV::OpAtomicAnd:
  case SPIRV::OpAtomicOr:
  case SPIRV::OpAtomicXor: {
    const MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();
    const MachineInstr *InstrPtr = &MI;
    if (MI.getOpcode() == SPIRV::OpAtomicStore) {
      assert(MI.getOperand(3).isReg());
      InstrPtr = MRI.getVRegDef(MI.getOperand(3).getReg());
      assert(InstrPtr && "Unexpected type instruction for OpAtomicStore");
    }
    assert(InstrPtr->getOperand(1).isReg() && "Unexpected operand in atomic");
    Register TypeReg = InstrPtr->getOperand(1).getReg();
    SPIRVType *TypeDef = MRI.getVRegDef(TypeReg);
    if (TypeDef->getOpcode() == SPIRV::OpTypeInt) {
      unsigned BitWidth = TypeDef->getOperand(1).getImm();
      if (BitWidth == 64)
        Reqs.addCapability(Capability::Int64Atomics);
    }
    break;
  }
  case SPIRV::OpGroupNonUniformIAdd:
  case SPIRV::OpGroupNonUniformFAdd:
  case SPIRV::OpGroupNonUniformIMul:
  case SPIRV::OpGroupNonUniformFMul:
  case SPIRV::OpGroupNonUniformSMin:
  case SPIRV::OpGroupNonUniformUMin:
  case SPIRV::OpGroupNonUniformFMin:
  case SPIRV::OpGroupNonUniformSMax:
  case SPIRV::OpGroupNonUniformUMax:
  case SPIRV::OpGroupNonUniformFMax:
  case SPIRV::OpGroupNonUniformBitwiseAnd:
  case SPIRV::OpGroupNonUniformBitwiseOr:
  case SPIRV::OpGroupNonUniformBitwiseXor:
  case SPIRV::OpGroupNonUniformLogicalAnd:
  case SPIRV::OpGroupNonUniformLogicalOr:
  case SPIRV::OpGroupNonUniformLogicalXor: {
    assert(MI.getOperand(3).isImm());
    int64_t GroupOp = MI.getOperand(3).getImm();
    switch (GroupOp) {
    case GroupOperation::Reduce:
    case GroupOperation::InclusiveScan:
    case GroupOperation::ExclusiveScan:
      Reqs.addCapability(Capability::Kernel);
      Reqs.addCapability(Capability::GroupNonUniformArithmetic);
      Reqs.addCapability(Capability::GroupNonUniformBallot);
      break;
    case GroupOperation::ClusteredReduce:
      Reqs.addCapability(Capability::GroupNonUniformClustered);
      break;
    case GroupOperation::PartitionedReduceNV:
    case GroupOperation::PartitionedInclusiveScanNV:
    case GroupOperation::PartitionedExclusiveScanNV:
      Reqs.addCapability(Capability::GroupNonUniformPartitionedNV);
      break;
    }
    break;
  }
  case SPIRV::OpGroupNonUniformShuffle:
  case SPIRV::OpGroupNonUniformShuffleXor:
    Reqs.addCapability(Capability::GroupNonUniformShuffle);
    break;
  case SPIRV::OpGroupNonUniformShuffleUp:
  case SPIRV::OpGroupNonUniformShuffleDown:
    Reqs.addCapability(Capability::GroupNonUniformShuffleRelative);
    break;
  case SPIRV::OpGroupAll:
  case SPIRV::OpGroupAny:
  case SPIRV::OpGroupBroadcast:
  case SPIRV::OpGroupIAdd:
  case SPIRV::OpGroupFAdd:
  case SPIRV::OpGroupFMin:
  case SPIRV::OpGroupUMin:
  case SPIRV::OpGroupSMin:
  case SPIRV::OpGroupFMax:
  case SPIRV::OpGroupUMax:
  case SPIRV::OpGroupSMax:
    Reqs.addCapability(Capability::Groups);
    break;
  case SPIRV::OpGroupNonUniformElect:
    Reqs.addCapability(Capability::GroupNonUniform);
    break;
  case SPIRV::OpGroupNonUniformAll:
  case SPIRV::OpGroupNonUniformAny:
  case SPIRV::OpGroupNonUniformAllEqual:
    Reqs.addCapability(Capability::GroupNonUniformVote);
    break;
  case SPIRV::OpGroupNonUniformBroadcast:
  case SPIRV::OpGroupNonUniformBroadcastFirst:
  case SPIRV::OpGroupNonUniformBallot:
  case SPIRV::OpGroupNonUniformInverseBallot:
  case SPIRV::OpGroupNonUniformBallotBitExtract:
  case SPIRV::OpGroupNonUniformBallotBitCount:
  case SPIRV::OpGroupNonUniformBallotFindLSB:
  case SPIRV::OpGroupNonUniformBallotFindMSB:
    Reqs.addCapability(Capability::GroupNonUniformBallot);
    break;
  default:
    break;
  }
}

static void collectInstrReqs(const Module &M, ModuleAnalysisInfo &MAI,
                             MachineModuleInfo *MMI, const SPIRVSubtarget &ST) {
  for (auto F = M.begin(), E = M.end(); F != E; ++F) {
    MachineFunction *MF = MMI->getMachineFunction(*F);
    if (MF) {
      for (const MachineBasicBlock &MBB : *MF)
        for (const MachineInstr &MI : MBB)
          addInstrRequirements(MI, MAI.Reqs, ST);
    }
  }
}

struct ModuleAnalysisInfo SPIRVModuleAnalysis::MAI;

void SPIRVModuleAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<MachineModuleInfoWrapperPass>();
}

bool SPIRVModuleAnalysis::runOnModule(Module &M) {
  SPIRVTargetMachine &TM =
      getAnalysis<TargetPassConfig>().getTM<SPIRVTargetMachine>();
  ST = TM.getSubtargetImpl();
  GR = ST->getSPIRVGlobalRegistry();
  TII = ST->getInstrInfo();

  MMI = &getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  InsertedTypeConstVarDefs.clear();

  setBaseInfo(M);

  collectInstrReqs(M, MAI, MMI, *ST);

  processSwitches(M, MAI, MMI);

  // Process type/const/global var/func decl instructions, number their
  // destination registers from 0 to N, collect Extensions and Capabilities.
  processDefInstrs(M);

  // Number rest of registers from N+1 onwards.
  numberRegistersGlobally(M);

  // Collect OpName, OpEntryPoint, OpDecorate etc, process other instructions.
  processOtherInstrs(M);

  // If there are no entry points, we need the Linkage capability.
  if (MAI.MS[MB_EntryPoints].empty())
    MAI.Reqs.addCapability(Capability::Linkage);

  return false;
}
