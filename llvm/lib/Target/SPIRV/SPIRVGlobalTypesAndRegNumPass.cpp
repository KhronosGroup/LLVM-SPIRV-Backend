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

#include "SPIRV.h"
#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVGlobalRegistry.h"
#include "SPIRVModuleAnalysis.h"
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
  MAI.Reqs.addRequirements(getMemoryModelRequirements(MAI.Mem, *ST));
  MAI.Reqs.addRequirements(getSourceLanguageRequirements(MAI.SrcLang, *ST));
  MAI.Reqs.addRequirements(getAddressingModelRequirements(MAI.Addr, *ST));

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
