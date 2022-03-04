//===- SPIRVModuleAnalysis.h - analysis of global instrs & regs -*- C++ -*-===//
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
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVMODULEANALYSIS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVMODULEANALYSIS_H

#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

namespace llvm {

// The enum contains logical module sections for instruction callection.
enum ModuleSectionType {
  //  MB_Capabilities,   // All OpCapability instructions
  //  MB_Extensions,     // Optional OpExtension instructions
  //  MB_ExtInstImports, // Any language extension imports via OpExtInstImport
  //  MB_MemoryModel,    // A single required OpMemoryModel instruction
  MB_EntryPoints, // All OpEntryPoint instructions (if any)
  //  MB_ExecutionModes, // All OpExecutionMode or OpExecutionModeId instrs
  //  MB_DebugSourceAndStrings, // OpString, OpSource, OpSourceExtension etc.
  MB_DebugNames,           // All OpName and OpMemberName intrs.
  MB_DebugModuleProcessed, // All OpModuleProcessed instructions.
  MB_Annotations,          // OpDecorate, OpMemberDecorate etc.
  MB_TypeConstVars,        // OpTypeXXX, OpConstantXXX, and global OpVariables
  MB_ExtFuncDecls,         // OpFunction etc. to declare for external funcs
  NUM_MODULE_SECTIONS      // Total number of sections requiring basic blocks
};

class MachineFunction;
class MachineModuleInfo;

using InstrList = SmallVector<MachineInstr *>;
// Maps a local register to the corresponding global alias.
using LocalToGlobalRegTable = std::map<Register, Register>;
using RegisterAliasMapTy =
    std::map<const MachineFunction *, LocalToGlobalRegTable>;

// The struct contains results of the module analysis and methods
// to access them.
struct ModuleAnalysisInfo {
  SPIRVRequirementHandler Reqs;
  MemoryModel::MemoryModel Mem;
  AddressingModel::AddressingModel Addr;
  SourceLanguage::SourceLanguage SrcLang;
  unsigned int SrcLangVersion;
  // Maps ExtInstSet to corresponding ID register.
  DenseMap<unsigned, Register> ExtInstSetMap;
  // Contains the list of all global OpVariables in the module.
  SmallVector<MachineInstr *, 4> GlobalVarList;
  // Maps function names to coresponding function ID registers.
  StringMap<Register> FuncNameMap;
  // The set contains machine instructions which are necessary
  // for correct MIR but will not be emitted in function bodies.
  DenseSet<MachineInstr *> InstrsToDelete;
  // The table contains global aliases of local registers for each machine
  // function. The aliases are used to substitute local registers during
  // code emission.
  RegisterAliasMapTy RegisterAliasTable;
  // The counter holds the maximum ID we have in the module.
  unsigned MaxID;
  // The array contains lists of MIs for each module section.
  InstrList MS[NUM_MODULE_SECTIONS];

  Register getFuncReg(std::string FuncName) {
    auto FuncReg = FuncNameMap.find(FuncName);
    assert(FuncReg != FuncNameMap.end() && "Cannot find function Id");
    return FuncReg->second;
  }
  Register getExtInstSetReg(unsigned SetNum) { return ExtInstSetMap[SetNum]; }
  InstrList &getMSInstrs(unsigned MSType) { return MS[MSType]; }
  void setSkipEmission(MachineInstr *MI) { InstrsToDelete.insert(MI); }
  bool getSkipEmission(const MachineInstr *MI) {
    return InstrsToDelete.contains(MI);
  }
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
    return RegisterAliasTable.find(MF) != RegisterAliasTable.end() &&
           RegisterAliasTable[MF].find(Reg) != RegisterAliasTable[MF].end();
  }
  unsigned getNextID() { return MaxID++; }
};

struct SPIRVModuleAnalysis : public ModulePass {
  static char ID;

public:
  SPIRVModuleAnalysis() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  static struct ModuleAnalysisInfo MAI;

private:
  void setBaseInfo(const Module &M);
  template <typename T> void collectTypesConstsVars();
  void processDefInstrs(const Module &M);
  void collectFuncNames(MachineInstr &MI, const Function &F);
  void processOtherInstrs(const Module &M);
  void numberRegistersGlobally(const Module &M);

  const SPIRVSubtarget *ST;
  SPIRVGlobalRegistry *GR;
  const SPIRVInstrInfo *TII;
  MachineModuleInfo *MMI;
};
} // namespace llvm
#endif // LLVM_LIB_TARGET_SPIRV_SPIRVMODULEANALYSIS_H
