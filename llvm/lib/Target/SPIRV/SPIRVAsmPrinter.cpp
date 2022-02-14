//===-- SPIRVAsmPrinter.cpp - SPIR-V LLVM assembly writer ------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the SPIR-V assembly language.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SPIRVInstPrinter.h"
#include "SPIRV.h"
#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVGlobalRegistry.h"
#include "SPIRVInstrInfo.h"
#include "SPIRVMCInstLower.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTargetMachine.h"
#include "SPIRVUtils.h"
#include "TargetInfo/SPIRVTargetInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// TODO:
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

using InstrList = SmallVector<MachineInstr *>;
// static SmallVector<InstrList, NUM_MODULE_SECTIONS> MS(NUM_MODULE_SECTIONS);
static InstrList MS[NUM_MODULE_SECTIONS];

namespace {
class SPIRVAsmPrinter : public AsmPrinter {
  const MCSubtargetInfo *STI;

public:
  explicit SPIRVAsmPrinter(TargetMachine &TM,
                           std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)), STI(TM.getMCSubtargetInfo()) {
    ST = static_cast<const SPIRVTargetMachine &>(TM).getSubtargetImpl();
    GR = ST->getSPIRVGlobalRegistry();
    TII = ST->getInstrInfo();
  }
  bool ModuleSectionsEmitted;
  const SPIRVSubtarget *ST;
  SPIRVGlobalRegistry *GR;
  const SPIRVInstrInfo *TII;
  SmallVector<MachineInstr *, 4> GlobalOpVariableList;
  StringMap<Register> FuncNameToID;
  DenseMap<unsigned, Register> ExtInstSetToRegMap;

  StringRef getPassName() const override { return "SPIRV Assembly Printer"; }
  void printOperand(const MachineInstr *MI, int OpNum, raw_ostream &O);

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &O) override;
  template <typename T> void collectTypesConstsVars();
  void collectInstrsForMS(const Module &M, SPIRVRequirementHandler &Reqs);
  void addExtDeclToIDMap(MachineInstr &MI);
  void collectOtherInstrsForMS(const Module &M);
  void processFunctionCallIDs(const Module &M);
  void numberRegistersGlobally(const Module &M);

  void outputMCInst(MCInst &Inst);
  void outputInstruction(const MachineInstr *MI);
  void outputModuleSection(ModuleSectionType MSType);
  void outputGlobalRequirements(const SPIRVRequirementHandler &Reqs);
  void outputEntryPoints();
  void outputDebugSourceAndStrings(const Module &M);
  void outputOpExtInstImports(const Module &M);
  void outputOpMemoryModel(AddressingModel::AddressingModel Addr,
                           MemoryModel::MemoryModel Mem);
  void outputOpFunctionEnd();
  void outputExtFuncDecls();
  void outputModuleSections();

  void emitInstruction(const MachineInstr *MI) override;
  bool doInitialization(Module &M) override;

  //  void emitFunctionEntryLabel() override {};
  void emitFunctionHeader() override;
  void emitFunctionBodyStart() override {}
  void emitFunctionBodyEnd() override;
  void emitBasicBlockStart(const MachineBasicBlock &MBB) override;
  void emitBasicBlockEnd(const MachineBasicBlock &MBB) override {}
  void emitGlobalVariable(const GlobalVariable *GV) override {}
  void emitOpLabel(const MachineBasicBlock &MBB);
  void emitEndOfAsmFile(Module &M) override;
};
} // namespace

// If the module has no functions, we need output global info anyway.
void SPIRVAsmPrinter::emitEndOfAsmFile(Module &M) {
  if (ModuleSectionsEmitted == false) {
    outputModuleSections();
    ModuleSectionsEmitted = true;
  }
}

void SPIRVAsmPrinter::emitFunctionHeader() {
  if (ModuleSectionsEmitted == false) {
    outputModuleSections();
    ModuleSectionsEmitted = true;
  }
  const Function &F = MF->getFunction();

  if (isVerbose())
    OutStreamer->GetCommentOS()
        << "-- Begin function "
        << GlobalValue::dropLLVMManglingEscape(F.getName()) << '\n';

  auto Section = getObjFileLowering().SectionForGlobal(&F, TM);
  MF->setSection(Section);
}

// The table maps MMB number to SPIR-V unique ID register.
static DenseMap<int, Register> BBNumToRegMap;

void SPIRVAsmPrinter::outputOpFunctionEnd() {
  MCInst FunctionEndInst;
  FunctionEndInst.setOpcode(SPIRV::OpFunctionEnd);
  outputMCInst(FunctionEndInst);
}

// Emit OpFunctionEnd at the end of MF and clear BBNumToRegMap.
void SPIRVAsmPrinter::emitFunctionBodyEnd() {
  outputOpFunctionEnd();
  BBNumToRegMap.clear();
}

static bool hasMBBRegister(const MachineBasicBlock &MBB) {
  return BBNumToRegMap.find(MBB.getNumber()) != BBNumToRegMap.end();
}

// Convert MBB's number to correspnding ID register.
Register getOrCreateMBBRegister(const MachineBasicBlock &MBB,
                                SPIRVGlobalRegistry *GR) {
  auto f = BBNumToRegMap.find(MBB.getNumber());
  if (f != BBNumToRegMap.end())
    return f->second;
  Register NewReg = Register::index2VirtReg(GR->getNextID());
  BBNumToRegMap[MBB.getNumber()] = NewReg;
  return NewReg;
}

void SPIRVAsmPrinter::emitOpLabel(const MachineBasicBlock &MBB) {
  MCInst LabelInst;
  LabelInst.setOpcode(SPIRV::OpLabel);
  LabelInst.addOperand(MCOperand::createReg(getOrCreateMBBRegister(MBB, GR)));
  //  EmitToStreamer(*OutStreamer, LabelInst);
  outputMCInst(LabelInst);
}

void SPIRVAsmPrinter::emitBasicBlockStart(const MachineBasicBlock &MBB) {
  // If it's the first MBB in MF, it has OpFunction and OpFunctionParameter, so
  // OpLabel should be output after them.
  if (MBB.getNumber() == MF->front().getNumber()) {
    for (const MachineInstr &MI : MBB)
      if (MI.getOpcode() == SPIRV::OpFunction)
        return;
    llvm_unreachable("OpFunction is expected in the front MBB of MF");
  }
  emitOpLabel(MBB);
}

void SPIRVAsmPrinter::printOperand(const MachineInstr *MI, int OpNum,
                                   raw_ostream &O) {
  const MachineOperand &MO = MI->getOperand(OpNum);

  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    O << SPIRVInstPrinter::getRegisterName(MO.getReg());
    break;

  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    break;

  case MachineOperand::MO_FPImmediate:
    O << MO.getFPImm();
    break;

  case MachineOperand::MO_MachineBasicBlock:
    O << *MO.getMBB()->getSymbol();
    break;

  case MachineOperand::MO_GlobalAddress:
    O << *getSymbol(MO.getGlobal());
    break;

  case MachineOperand::MO_BlockAddress: {
    MCSymbol *BA = GetBlockAddressSymbol(MO.getBlockAddress());
    O << BA->getName();
    break;
  }

  case MachineOperand::MO_ExternalSymbol:
    O << *GetExternalSymbolSymbol(MO.getSymbolName());
    break;

  case MachineOperand::MO_JumpTableIndex:
  case MachineOperand::MO_ConstantPoolIndex:
  default:
    llvm_unreachable("<unknown operand type>");
  }
}

bool SPIRVAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                      const char *ExtraCode, raw_ostream &O) {
  if (ExtraCode && ExtraCode[0])
    return true; // Invalid instruction - SPIR-V does not have special modifiers

  printOperand(MI, OpNo, O);
  return false;
}

static bool isFuncOrHeaderInstr(const MachineInstr *MI,
                                const SPIRVInstrInfo *TII) {
  return TII->isHeaderInstr(*MI) || MI->getOpcode() == SPIRV::OpFunction ||
         MI->getOpcode() == SPIRV::OpFunctionParameter;
}

void SPIRVAsmPrinter::outputMCInst(MCInst &Inst) {
  (*OutStreamer).emitInstruction(Inst, *STI);
}

void SPIRVAsmPrinter::outputInstruction(const MachineInstr *MI) {
  SPIRVMCInstLower MCInstLowering;
  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst, GR, MF, FuncNameToID, ExtInstSetToRegMap);
  //  EmitToStreamer(*OutStreamer, TmpInst);
  outputMCInst(TmpInst);
}

void SPIRVAsmPrinter::emitInstruction(const MachineInstr *MI) {
  if (!GR->getSkipEmission(MI))
    outputInstruction(MI);

  // Output OpLabel after OpFunction and OpFunctionParameter in the first MMB.
  const MachineInstr *NextMI = MI->getNextNode();
  if (!hasMBBRegister(*MI->getParent()) && isFuncOrHeaderInstr(MI, TII) &&
      (!NextMI || !isFuncOrHeaderInstr(NextMI, TII))) {
    assert(MI->getParent()->getNumber() == MF->front().getNumber() &&
           "OpFunction is not in the front MBB of MF");
    emitOpLabel(*MI->getParent());
  }
}

static void insertInstrToMS(MachineInstr *MI, ModuleSectionType Type) {
  MS[Type].push_back(MI);
}

void SPIRVAsmPrinter::outputModuleSection(ModuleSectionType MSType) {
  for (MachineInstr *MI : MS[MSType])
    outputInstruction(MI);
}

// Macros to make it simpler to iterate over MachineFunctions within a module
// Defines MachineFunction *MF and unsigned MFIndex between BEGIN and END lines.
#define BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)                            \
  {                                                                            \
    unsigned MFIndex = 1;                                                      \
    for (auto F = M.begin(), E = M.end(); F != E; ++F) {                       \
      MachineFunction *MF = MMI->getMachineFunction(*F);                       \
      if (MF) {

#define END_FOR_MF_IN_MODULE()                                                 \
  ++MFIndex;                                                                   \
  } /* close if statement */                                                   \
  } /* close for loop */                                                       \
  } /* close outer block */

// Retrieve an unsigned int from an MDNode with a list of them as operands
static unsigned int getMetadataUInt(MDNode *MdNode, unsigned int OpIndex,
                                    unsigned int DefaultVal = 0) {
  if (MdNode && OpIndex < MdNode->getNumOperands()) {
    const auto &Op = MdNode->getOperand(OpIndex);
    return mdconst::extract<ConstantInt>(Op)->getZExtValue();
  }
  return DefaultVal;
}

void SPIRVAsmPrinter::outputDebugSourceAndStrings(const Module &M) {
  // Get the OpenCL version number from metadata.
  unsigned OpenCLVersion = 0;
  if (auto VerNode = M.getNamedMetadata("opencl.ocl.version")) {
    // Construct version literal according to OpenCL 2.2 environment spec.
    auto VersionMD = VerNode->getOperand(0);
    unsigned MajorNum = getMetadataUInt(VersionMD, 0, 2);
    unsigned MinorNum = getMetadataUInt(VersionMD, 1);
    unsigned RevNum = getMetadataUInt(VersionMD, 2);
    OpenCLVersion = 0 | (MajorNum << 16) | (MinorNum << 8) | RevNum;
  }

  // Output the OpSource.
  MCInst Inst;
  Inst.setOpcode(SPIRV::OpSource);
  Inst.addOperand(MCOperand::createImm(SourceLanguage::OpenCL_C));
  Inst.addOperand(MCOperand::createImm(OpenCLVersion));
  outputMCInst(Inst);
}

template <typename T>
static void fillLocalAliasTables(SPIRVGlobalRegistry *GR) {
  // Make meta registers for entries of DT.
  for (auto &CU : GR->getAllUses<T>()) {
    Register MetaReg = Register::index2VirtReg(GR->getNextID());
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      GR->setRegisterAlias(MF, Reg, MetaReg);
    }
  }
  // Make meta registers for special types and constants collected in the map.
  if (std::is_same<T, Type>::value) {
    for (auto &CU : GR->getSpecialTypesAndConstsMap()) {
      for (auto &typeGroup : CU.second) {
        Register MetaReg = Register::index2VirtReg(GR->getNextID());
        for (auto &t : typeGroup) {
          MachineFunction *MF = t.first;
          assert(t.second->getOperand(0).isReg());
          Register Reg = t.second->getOperand(0).getReg();
          GR->setRegisterAlias(MF, Reg, MetaReg);
        }
      }
    }
  }
}

static DenseSet<Register> InsertedTypeConstVarDefs;

static void makeGlobalOp(Register Reg, Register MetaReg, MachineInstr *ToHoist,
                         MachineFunction *MF, SPIRVGlobalRegistry *GR,
                         ModuleSectionType MSType) {
  assert(ToHoist && "There should be an instruction that defines the register");
  GR->setSkipEmission(ToHoist);
  if (!InsertedTypeConstVarDefs.contains(MetaReg)) {
    insertInstrToMS(ToHoist, MSType);
    InsertedTypeConstVarDefs.insert(MetaReg);
  }
}

static void hoistGlobalOp(Register Reg, MachineFunction *MF,
                          SPIRVGlobalRegistry *GR, ModuleSectionType MSType) {
  assert(GR->hasRegisterAlias(MF, Reg) && "Cannot find register alias");
  Register MetaReg = GR->getRegisterAlias(MF, Reg);
  MachineInstr *ToHoist = MF->getRegInfo().getVRegDef(Reg);
  makeGlobalOp(Reg, MetaReg, ToHoist, MF, GR, MSType);
}

template <typename T> void SPIRVAsmPrinter::collectTypesConstsVars() {
  // Hoist instructions from DT.
  for (auto &CU : GR->getAllUses<T>()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      // Some instructions may be deleted during global isel, so collect only
      // those that exist in current IR.
      if (MF->getRegInfo().getVRegDef(Reg)) {
        hoistGlobalOp(Reg, MF, GR, MB_TypeConstVars);
        if (std::is_same<T, GlobalValue>::value)
          GlobalOpVariableList.push_back(MF->getRegInfo().getVRegDef(Reg));
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
          hoistGlobalOp(Reg, MF, GR, MB_TypeConstVars);
        }
  }
}

// We need this specialization as for function decls we want to not only hoist
// OpFunctions but OpFunctionParameters too.
//
// TODO: consider replacing this with explicit OpFunctionParameter generation
// here instead handling it in CallLowering.
static void hoistGlobalOpsFunction(SPIRVGlobalRegistry *GR) {
  for (auto &CU : GR->getAllUses<Function>()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      auto *ToHoist = MF->getRegInfo().getVRegDef(Reg);
      while (ToHoist && (ToHoist->getOpcode() == SPIRV::OpFunction ||
                         ToHoist->getOpcode() == SPIRV::OpFunctionParameter)) {
        Reg = ToHoist->getOperand(0).getReg();
        if (ToHoist->getOpcode() == SPIRV::OpFunctionParameter) {
          Register MetaReg = Register::index2VirtReg(GR->getNextID());
          ;
          GR->setRegisterAlias(MF, Reg, MetaReg);
          makeGlobalOp(Reg, MetaReg, ToHoist, MF, GR, MB_ExtFuncDecls);
        } else {
          hoistGlobalOp(Reg, MF, GR, MB_ExtFuncDecls);
        }
        ToHoist = ToHoist->getNextNode();
        Reg = ToHoist->getOperand(0).getReg();
      }
    }
  }
}

// This function runs early on in the pass to initialize the alias tables, and
// can only hoist instructions which never refer to function-local registers.
// OpTypeXXX, OpConstantXXX, and external OpFunction declarations only refer to
// other hoistable instructions vregs, so can be hoisted here.
//
// OpCapability instructions are removed and added to the minCaps and
// allCaps lists instead of hoisting right now. This allows capabilities added
// later, such as the Linkage capability for files with no OpEntryPoints, and
// still get deduplicated before the global OpCapability instructions are added.
void SPIRVAsmPrinter::collectInstrsForMS(const Module &M,
                                         SPIRVRequirementHandler &Reqs) {
  // OpTypes and OpConstants can have cross references so at first we create
  // new meta regs and map them to old regs, then walk the list of instructions,
  // and create new (hoisted) instructions with new meta regs instead of old
  // ones. Also there are references to global values in constants.
  fillLocalAliasTables<Type>(GR);
  fillLocalAliasTables<Constant>(GR);
  fillLocalAliasTables<GlobalValue>(GR);

  collectTypesConstsVars<Type>();
  collectTypesConstsVars<Constant>();

  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  // Iterate through and hoist any instructions we can at this stage.
  // bool hasSeenFirstOpFunction = false;
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpExtension) {
        // Here, OpExtension just has a single enum operand, not a string
        auto Ext = Extension::Extension(MI.getOperand(0).getImm());
        Reqs.addExtension(Ext);
        GR->setSkipEmission(&MI);
      } else if (MI.getOpcode() == SPIRV::OpCapability) {
        auto Cap = Capability::Capability(MI.getOperand(0).getImm());
        Reqs.addCapability(Cap);
        GR->setSkipEmission(&MI);
      }
    }
  }
  END_FOR_MF_IN_MODULE()

  collectTypesConstsVars<GlobalValue>();
  fillLocalAliasTables<Function>(GR);
  hoistGlobalOpsFunction(GR);
}

// True if there is an instruction in MS with all the same operands as
// the given instruction has (after the given starting index).
// TODO: maybe it needs to check Opcodes too.
static bool findSameInstrInMS(const MachineInstr &A, ModuleSectionType MSType,
                              SPIRVGlobalRegistry *GR,
                              unsigned int StartOpIndex = 0) {
  for (const auto *B : MS[MSType]) {
    const unsigned int NumAOps = A.getNumOperands();
    if (NumAOps == B->getNumOperands() && A.getNumDefs() == B->getNumDefs()) {
      bool AllOpsMatch = true;
      for (unsigned i = StartOpIndex; i < NumAOps && AllOpsMatch; ++i) {
        if (A.getOperand(i).isReg() && B->getOperand(i).isReg()) {
          Register RegA = A.getOperand(i).getReg();
          Register RegB = B->getOperand(i).getReg();
          AllOpsMatch = GR->getRegisterAlias(A.getMF(), RegA) ==
                        GR->getRegisterAlias(B->getMF(), RegB);
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

void SPIRVAsmPrinter::outputOpExtInstImports(const Module &M) {
  for (auto &CU : ExtInstSetToRegMap) {
    unsigned Set = CU.first;
    Register Reg = CU.second;
    MCInst Inst;
    Inst.setOpcode(SPIRV::OpExtInstImport);
    Inst.addOperand(MCOperand::createReg(Reg));
    addStringImm(getExtInstSetName(static_cast<ExtInstSet>(Set)), Inst);
    outputMCInst(Inst);
  }
}

void SPIRVAsmPrinter::outputOpMemoryModel(AddressingModel::AddressingModel Addr,
                                          MemoryModel::MemoryModel Mem) {
  MCInst Inst;
  Inst.setOpcode(SPIRV::OpMemoryModel);
  Inst.addOperand(MCOperand::createImm(Addr));
  Inst.addOperand(MCOperand::createImm(Mem));
  outputMCInst(Inst);
}

// Look for IDs declared with Import linkage, and map the imported name string
// to the VReg defining that variable (which will usually be the result of
// an OpFunction). This lets us call externally imported functions using
// the correct VReg ID.
void SPIRVAsmPrinter::addExtDeclToIDMap(MachineInstr &MI) {
  // Only look for OpDecorates in the global meta function
  if (MI.getOpcode() == SPIRV::OpDecorate) {
    // If it's got Import linkage
    auto Dec = MI.getOperand(1).getImm();
    if (Dec == Decoration::LinkageAttributes) {
      auto Lnk = MI.getOperand(MI.getNumOperands() - 1).getImm();
      if (Lnk == LinkageType::Import) {
        // Map imported function name to function ID VReg.
        std::string Name = getStringImm(MI, 2);
        Register Target = MI.getOperand(0).getReg();
        // TODO: check defs from different MFs.
        FuncNameToID[Name] = GR->getRegisterAlias(MI.getMF(), Target);
      }
    }
  }
}

// Collect the given instruction in the specified MS. We assume global register
// numbering has already occurred by this point. We can directly compare VReg
// arguments when detecting duplicates.
static void collectOtherInstr(MachineInstr &MI, SPIRVGlobalRegistry *GR,
                              ModuleSectionType MSType) {
  GR->setSkipEmission(&MI);
  if (findSameInstrInMS(MI, MSType, GR))
    return; // Found a duplicate, so don't add it
  // No duplicates, so add it.
  insertInstrToMS(&MI, MSType);
}

// Some global instructions make reference to function-local ID VRegs, so cannot
// be correctly collected until these registers are globally numbered.
void SPIRVAsmPrinter::collectOtherInstrsForMS(const Module &M) {
  for (auto F = M.begin(), E = M.end(); F != E; ++F) {
    MachineFunction *MF = MMI->getMachineFunction(*F);
    if (MF) {

      //  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
      for (MachineBasicBlock &MBB : *MF)
        for (MachineInstr &MI : MBB) {
          if (GR->getSkipEmission(&MI))
            continue;
          const unsigned OpCode = MI.getOpcode();
          if (OpCode == SPIRV::OpName || OpCode == SPIRV::OpMemberName) {
            collectOtherInstr(MI, GR, MB_DebugNames);
          } else if (OpCode == SPIRV::OpEntryPoint) {
            collectOtherInstr(MI, GR, MB_EntryPoints);
          } else if (TII->isDecorationInstr(MI)) {
            collectOtherInstr(MI, GR, MB_Annotations);
            addExtDeclToIDMap(MI);
          } else if (TII->isConstantInstr(MI)) {
            // Now OpSpecConstant*s are not in DT,
            // but they need to be collected anyway.
            collectOtherInstr(MI, GR, MB_TypeConstVars);
          }
        }
    }
  }
  //  END_FOR_MF_IN_MODULE()
}

// Before the OpEntryPoints' output, we need to add the entry point's
// interfaces. The interface is a list of IDs of global OpVariable instructions.
// These declare the set of global variables from a module that form
// the interface of this entry point.
void SPIRVAsmPrinter::outputEntryPoints() {
  // Find all OpVariable IDs with required StorageClass.
  DenseSet<Register> InterfaceIDs;
  for (MachineInstr *MI : GlobalOpVariableList) {
    assert(MI->getOpcode() == SPIRV::OpVariable);
    auto SC =
        static_cast<StorageClass::StorageClass>(MI->getOperand(2).getImm());
    // Before version 1.4, the interface's storage classes are limited to
    // the Input and Output storage classes. Starting with version 1.4,
    // the interface's storage classes are all storage classes used in
    // declaring all global variables referenced by the entry point call tree.
    if (ST->getTargetSPIRVVersion() >= 14 || SC == StorageClass::Input ||
        SC == StorageClass::Output) {
      MachineFunction *MF = MI->getMF();
      Register Reg = GR->getRegisterAlias(MF, MI->getOperand(0).getReg());
      InterfaceIDs.insert(Reg);
    }
  }

  // Output OpEntryPoints adding interface args to all of them.
  for (MachineInstr *MI : MS[MB_EntryPoints]) {
    SPIRVMCInstLower MCInstLowering;
    MCInst TmpInst;
    MCInstLowering.Lower(MI, TmpInst, GR, MF, FuncNameToID, ExtInstSetToRegMap);
    for (Register Reg : InterfaceIDs) {
      assert(Reg.isValid());
      TmpInst.addOperand(MCOperand::createReg(Reg));
    }
    outputMCInst(TmpInst);
  }
}

// Number all registers in all functions globally from 0 onwards and store
// the result in GR's register alias table.
void SPIRVAsmPrinter::numberRegistersGlobally(const Module &M) {
  // TODO: check if it's required by default.
  ExtInstSetToRegMap[static_cast<unsigned>(ExtInstSet::OpenCL_std)] =
      Register::index2VirtReg(GR->getNextID());
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      for (MachineOperand &Op : MI.operands()) {
        if (Op.isReg()) {
          Register Reg = Op.getReg();
          if (!GR->hasRegisterAlias(MF, Reg)) {
            Register NewReg = Register::index2VirtReg(GR->getNextID());
            GR->setRegisterAlias(MF, Reg, NewReg);
          }
        }
      }
      if (MI.getOpcode() == SPIRV::OpExtInst) {
        auto Set = MI.getOperand(2).getImm();
        if (ExtInstSetToRegMap.find(Set) == ExtInstSetToRegMap.end())
          ExtInstSetToRegMap[Set] = Register::index2VirtReg(GR->getNextID());
      }
    }
  }
  END_FOR_MF_IN_MODULE()
}

void SPIRVAsmPrinter::processFunctionCallIDs(const Module &M) {
  // Record all OpFunctionCalls, and all internal OpFunction declarations.
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpFunction && !GR->getSkipEmission(&MI)) {
        Register Reg = GR->getRegisterAlias(MF, MI.defs().begin()->getReg());
        assert(Reg.isValid());
        // TODO: check that it does not conflict with existing entries.
        FuncNameToID[F->getGlobalIdentifier()] = Reg;
      }
    }
  }
  END_FOR_MF_IN_MODULE()
}

// Create global OpCapability instructions for the required capabilities
void SPIRVAsmPrinter::outputGlobalRequirements(
    const SPIRVRequirementHandler &Reqs) {
  // Abort here if not all requirements can be satisfied
  Reqs.checkSatisfiable(*ST);

  for (const auto &Cap : Reqs.getMinimalCapabilities()) {
    MCInst Inst;
    Inst.setOpcode(SPIRV::OpCapability);
    Inst.addOperand(MCOperand::createImm(Cap));
    outputMCInst(Inst);
  }

  // Generate the final OpExtensions with strings instead of enums
  for (const auto &Ext : Reqs.getExtensions()) {
    MCInst Inst;
    Inst.setOpcode(SPIRV::OpExtension);
    addStringImm(getExtensionName(Ext), Inst);
    outputMCInst(Inst);
  }
  // TODO add a pseudo instr for version number
}

void SPIRVAsmPrinter::outputExtFuncDecls() {
  // Insert OpFunctionEnd after each declaration.
  SmallVectorImpl<MachineInstr *>::iterator I = MS[MB_ExtFuncDecls].begin(),
                                            E = MS[MB_ExtFuncDecls].end();
  for (; I != E; ++I) {
    outputInstruction(*I);
    if ((I + 1) == E || (*(I + 1))->getOpcode() == SPIRV::OpFunction)
      outputOpFunctionEnd();
  }
}

void SPIRVAsmPrinter::outputModuleSections() {
  const Module *M = MMI->getModule();
  InsertedTypeConstVarDefs.clear();
  //  MS.clear();
  SPIRVRequirementHandler Reqs;
  // TODO: determine memory model and source language from the configuratoin.
  MemoryModel::MemoryModel Mem = MemoryModel::OpenCL;
  SourceLanguage::SourceLanguage SrcLang = SourceLanguage::OpenCL_C;
  unsigned PtrSize = ST->getPointerSize();
  AddressingModel::AddressingModel Addr =
      PtrSize == 32 ? AddressingModel::Physical32
                    : PtrSize == 64 ? AddressingModel::Physical64
                                    : AddressingModel::Logical;
  // Update required capabilities for this memory model, addressing model and
  // source language.
  Reqs.addRequirements(getMemoryModelRequirements(Mem, *ST));
  Reqs.addRequirements(getSourceLanguageRequirements(SrcLang, *ST));
  Reqs.addRequirements(getAddressingModelRequirements(Addr, *ST));

  // Prepare the module for output.
  // Collect type/const/global var/func decl instructions, number their
  // destination registers from 0 to N, collect Extensions and Capabilities.
  collectInstrsForMS(*M, Reqs);

  // Number rest of registers from N+1 onwards.
  numberRegistersGlobally(*M);

  // Collect OpName, OpEntryPoint, OpDecorate etc instructions.
  collectOtherInstrsForMS(*M);

  processFunctionCallIDs(*M);

  // If there are no entry points, we need the Linkage capability.
  if (MS[MB_EntryPoints].empty())
    Reqs.addCapability(Capability::Linkage);

  // Output instructions according to the Logical Layout of a Module:
  // 1,2. All OpCapability instructions, then optional OpExtension instructions.
  outputGlobalRequirements(Reqs);
  // 3. Optional OpExtInstImport instructions.
  outputOpExtInstImports(*M);
  // 4. The single required OpMemoryModel instruction.
  outputOpMemoryModel(Addr, Mem);
  // 5. All entry point declarations, using OpEntryPoint.
  outputEntryPoints();
  // 6. Execution-mode declarations, using OpExecutionMode or OpExecutionModeId.
  // TODO:
  // 7a. Debug: all OpString, OpSourceExtension, OpSource, and
  // OpSourceContinued, without forward references.
  outputDebugSourceAndStrings(*M);
  // 7b. Debug: all OpName and all OpMemberName.
  outputModuleSection(MB_DebugNames);
  // 7c. Debug: all OpModuleProcessed instructions.
  outputModuleSection(MB_DebugModuleProcessed);
  // 8. All annotation instructions (all decorations).
  outputModuleSection(MB_Annotations);
  // 9. All type declarations (OpTypeXXX instructions), all constant
  // instructions, and all global variable declarations. This section is
  // the first section to allow use of: OpLine and OpNoLine debug information;
  // non-semantic instructions with OpExtInst.
  outputModuleSection(MB_TypeConstVars);
  // 10. All function declarations (functions without a body).
  outputExtFuncDecls();
  // 11. All function definitions (functions with a body).
  // This is done in regular function output.
}

bool SPIRVAsmPrinter::doInitialization(Module &M) {
  // We need to call the parent's one explicitly.
  bool Result = AsmPrinter::doInitialization(M);

  ModuleSectionsEmitted = false;

  return Result;
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSPIRVAsmPrinter() {
  RegisterAsmPrinter<SPIRVAsmPrinter> X(getTheSPIRV32Target());
  RegisterAsmPrinter<SPIRVAsmPrinter> Y(getTheSPIRV64Target());
  RegisterAsmPrinter<SPIRVAsmPrinter> Z(getTheSPIRVLogicalTarget());
}
