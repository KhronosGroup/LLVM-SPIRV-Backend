//===-- SPIRVGlobalTypesAndRegNum.cpp - Hoist Globals & Number VRegs - C++ ===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass hoists all the required function-local instructions to global
// scope, deduplicating them where necessary.
//
// Before this pass, all SPIR-V instructions were local scope, and registers
// were numbered function-locally. However, SPIR-V requires Type instructions,
// global variables, capabilities, annotations etc. to be in global scope and
// occur at the start of the file. This pass copies these as necessary to dummy
// function which represents the global scope. The original local instructions
// are left to keep MIR consistent. They will not be emitted to asm/object file.
//
// This pass also re-numbers the registers globally, and saves global aliases of
// local registers to the global registry. AsmPrinter uses these aliases to
// output instructions that refers global registers.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVGlobalRegistry.h"
#include "SPIRVSubtarget.h"
#include "SPIRVUtils.h"
#include "TargetInfo/SPIRVTargetInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"

using namespace llvm;

#define DEBUG_TYPE "spirv-global-types-vreg"

namespace {
struct SPIRVGlobalTypesAndRegNum : public ModulePass {
  static char ID;

  SPIRVGlobalTypesAndRegNum() : ModulePass(ID) {
    initializeSPIRVGlobalTypesAndRegNumPass(*PassRegistry::getPassRegistry());
  }

public:
  // Perform passes such as hoisting global instructions and numbering vregs.
  // See end of file for details
  bool runOnModule(Module &M) override;

  // State that we need MachineModuleInfo to operate on MachineFunctions
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineModuleInfoWrapperPass>();
  }
};
} // namespace

// Basic block indices in the global meta-function representing sections of the
// SPIR-V module header that need filled in a specific order.
enum MetaBlockType {
  MB_Capabilities,   // All OpCapability instructions
  MB_Extensions,     // Optional OpExtension instructions
  MB_ExtInstImports, // Any language extension imports via OpExtInstImport
  MB_MemoryModel,    // A single required OpMemoryModel instruction
  MB_EntryPoints,    // All OpEntryPoint instructions (if any)
  MB_ExecutionModes, // All OpExecutionMode or OpExecutionModeId instrs
  MB_DebugSourceAndStrings, // OpString, OpSource, OpSourceExtension etc.
  MB_DebugNames,            // All OpName and OpMemberMemberName intrs.
  MB_DebugModuleProcessed,  // All OpModuleProcessed instructions.
  MB_Annotations,           // OpDecorate, OpMemberDecorate etc.
  MB_TypeConstVars,         // OpTypeXXX, OpConstantXXX, and global OpVariables
  MB_ExtFuncDecls,          // OpFunction etc. to declare for external funcs
  MB_TmpGlobalData,         // Tmp data in global vars processing
  NUM_META_BLOCKS           // Total number of sections requiring basic blocks
};

static MachineBasicBlock *CurrentMetaMBB = nullptr;
static MachineFunction *MetaMF = nullptr;

static MachineFunction *getMetaMF() { return MetaMF; }

static MachineBasicBlock *getMetaMBB(MetaBlockType block) {
  return MetaMF->getBlockNumbered(block);
}

// Set the builder's MBB to one of the sections from the MetaBlockType enum.
static void setMetaBlock(MetaBlockType block) {
  CurrentMetaMBB = getMetaMBB(block);
}

static MachineBasicBlock *getMetaBlock() { return CurrentMetaMBB; }

static MachineInstrBuilder buildInstrInCurrentMetaMBB(unsigned int Opcode) {
  const auto &ST = static_cast<const SPIRVSubtarget &>(MetaMF->getSubtarget());
  const SPIRVInstrInfo *TII = ST.getInstrInfo();
  return BuildMI(getMetaBlock(), DebugLoc(), TII->get(Opcode));
}

// Macros to make it simpler to iterate over MachineFunctions within a module
// Defines MachineFunction *MF and unsigned MFIndex between BEGIN and END lines.

#define BEGIN_FOR_MF_IN_MODULE(M, MMI)                                         \
  {                                                                            \
    unsigned MFIndex = 0;                                                      \
    for (Function & F : M) {                                                   \
      MachineFunction *MF = MMI.getMachineFunction(F);                         \
      if (MF) {

#define BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)                            \
  {                                                                            \
    unsigned MFIndex = 1;                                                      \
    for (auto F = (++M.begin()), E = M.end(); F != E; ++F) {                   \
      MachineFunction *MF = MMI.getMachineFunction(*F);                        \
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
  } else {
    return DefaultVal;
  }
}

// Add some initial header instructions such as OpSource and OpMemoryModel
// TODO Maybe move the environment-specific logic used here elsewhere.
static void addHeaderOps(Module &M, SPIRVRequirementHandler &Reqs,
                         const SPIRVSubtarget &ST) {
  setMetaBlock(MB_MemoryModel);
  unsigned PtrSize = ST.getPointerSize();

  // Add OpMemoryModel
  auto Addr = PtrSize == 32 ? AddressingModel::Physical32
                            : PtrSize == 64 ? AddressingModel::Physical64
                                            : AddressingModel::Logical;
  auto Mem = MemoryModel::OpenCL;
  buildInstrInCurrentMetaMBB(SPIRV::OpMemoryModel).addImm(Addr).addImm(Mem);

  // Update required capabilities for this memory model
  Reqs.addRequirements(getMemoryModelRequirements(Mem, ST));
  Reqs.addRequirements(getAddressingModelRequirements(Addr, ST));

  // Get the OpenCL version number from metadata
  unsigned OpenCLVersion = 0;
  if (auto VerNode = M.getNamedMetadata("opencl.ocl.version")) {
    // Construct version literal according to OpenCL 2.2 environment spec.
    auto VersionMD = VerNode->getOperand(0);
    unsigned MajorNum = getMetadataUInt(VersionMD, 0, 2);
    unsigned MinorNum = getMetadataUInt(VersionMD, 1);
    unsigned RevNum = getMetadataUInt(VersionMD, 2);
    OpenCLVersion = 0 | (MajorNum << 16) | (MinorNum << 8) | RevNum;
    setMetaBlock(MB_DebugSourceAndStrings);
  }

  // Build the OpSource
  auto SrcLang = SourceLanguage::OpenCL_C;
  buildInstrInCurrentMetaMBB(SPIRV::OpSource)
      .addImm(SrcLang)
      .addImm(OpenCLVersion);
  Reqs.addRequirements(getSourceLanguageRequirements(SrcLang, ST));
}

// Create a new Function and MachineFunction to add meta-instructions to.
// It should have a series of empty basic blocks, one for each required
// SPIR-V module section, so that subsequent users of the MachineFunction
// can hoist instructions into the right places easily.
static void initMetaBlockBuilder(Module &M, MachineModuleInfo &MMI) {
  // Add an empty placeholder function - MachineInstrs need to exist somewhere.
  auto VoidType = Type::getVoidTy(M.getContext());
  auto FType = FunctionType::get(VoidType, false);
  auto Linkage = Function::LinkageTypes::InternalLinkage;
  Function *F = Function::Create(FType, Linkage, "GlobalStuff");

  // Add the function and a corresponding MachineFunction to the module
  F->getBasicBlockList().push_back(BasicBlock::Create(M.getContext()));
  M.getFunctionList().push_front(F);

  // Add the necessary basic blocks according to the MetaBlockTypes enum.
  MetaMF = &MMI.getOrCreateMachineFunction(*F);
  for (unsigned int i = 0; i < NUM_META_BLOCKS; ++i) {
    MachineBasicBlock *MetaMBB = MetaMF->CreateMachineBasicBlock();
    MetaMF->push_back(MetaMBB);
  }

  // Tell the builder about the meta function and an initial basic block.
  setMetaBlock(MB_TypeConstVars);
}

template <typename T>
static void fillLocalAliasTables(SPIRVGlobalRegistry *GR,
                                 MetaBlockType MBType) {
  setMetaBlock(MBType);
  MachineRegisterInfo &MRI = getMetaMF()->getRegInfo();

  // Make meta registers for entries of DT.
  for (auto &CU : GR->getAllUses<T>()) {
    Register MetaReg = MRI.createVirtualRegister(&SPIRV::IDRegClass);
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
        Register MetaReg = MRI.createVirtualRegister(&SPIRV::IDRegClass);
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

static void makeGlobalOp(Register Reg, Register MetaReg, MachineInstr *ToHoist,
                         MachineFunction *MF, SPIRVGlobalRegistry *GR) {
  assert(ToHoist && "There should be an instruction that defines the register");
  GR->setSkipEmission(ToHoist);
  const MachineRegisterInfo &MRI = getMetaMF()->getRegInfo();

  if (!MRI.getVRegDef(MetaReg)) {
    auto MIB = buildInstrInCurrentMetaMBB(ToHoist->getOpcode());
    MIB.addDef(MetaReg);

    for (unsigned int i = ToHoist->getNumExplicitDefs();
         i < ToHoist->getNumOperands(); ++i) {
      MachineOperand Op = ToHoist->getOperand(i);
      if (Op.isImm()) {
        MIB.addImm(Op.getImm());
      } else if (Op.isFPImm()) {
        MIB.addFPImm(Op.getFPImm());
      } else if (Op.isReg()) {
        Register MetaReg2 = GR->getRegisterAlias(MF, Op.getReg());
        assert(MetaReg2.isValid() && "No reg alias found");
        MIB.addUse(MetaReg2);
      } else {
        errs() << *ToHoist;
        llvm_unreachable(
            "Unexpected operand type when copying spirv meta instr");
      }
    }
  }
}

static void hoistGlobalOp(Register Reg, MachineFunction *MF,
                          SPIRVGlobalRegistry *GR) {
  assert(GR->hasRegisterAlias(MF, Reg) && "Cannot find register alias");
  Register MetaReg = GR->getRegisterAlias(MF, Reg);
  MachineInstr *ToHoist = MF->getRegInfo().getVRegDef(Reg);
  makeGlobalOp(Reg, MetaReg, ToHoist, MF, GR);
}

template <typename T>
static void hoistGlobalOps(SPIRVGlobalRegistry *GR, MetaBlockType MBType) {
  setMetaBlock(MBType);
  // Hoist instructions from DT.
  for (auto &CU : GR->getAllUses<T>()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      // Some instructions may be deleted during global isel, so hoist only
      // those that exist in current IR.
      if (MF->getRegInfo().getVRegDef(Reg))
        hoistGlobalOp(Reg, MF, GR);
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
          hoistGlobalOp(Reg, MF, GR);
        }
  }
}

// We need this specialization as for function decls we want to not only hoist
// OpFunctions but OpFunctionParameters too.
//
// TODO: consider replacing this with explicit OpFunctionParameter generation
// here instead handling it in CallLowering.
static void hoistGlobalOpsFunction(SPIRVGlobalRegistry *GR) {
  setMetaBlock(MB_ExtFuncDecls);
  MachineRegisterInfo &MRI = getMetaMF()->getRegInfo();

  for (auto &CU : GR->getAllUses<Function>()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      auto *ToHoist = MF->getRegInfo().getVRegDef(Reg);
      while (ToHoist && (ToHoist->getOpcode() == SPIRV::OpFunction ||
                         ToHoist->getOpcode() == SPIRV::OpFunctionParameter)) {
        Reg = ToHoist->getOperand(0).getReg();
        if (ToHoist->getOpcode() == SPIRV::OpFunctionParameter) {
          Register MetaReg = MRI.createVirtualRegister(&SPIRV::IDRegClass);
          GR->setRegisterAlias(MF, Reg, MetaReg);
          makeGlobalOp(Reg, MetaReg, ToHoist, MF, GR);
        } else {
          hoistGlobalOp(Reg, MF, GR);
        }
        ToHoist = ToHoist->getNextNode();
        Reg = ToHoist->getOperand(0).getReg();
      }
    }
    buildInstrInCurrentMetaMBB(SPIRV::OpFunctionEnd);
  }
}

// Move all OpType, OpConstant etc. instructions into the meta block,
// avoiding creating duplicates, and mapping the global registers to the
// equivalent function-local ones via functionLocalAliasTables.
//
// This function runs early on in the pass to initialize the alias tables, and
// can only hoist instructions which never refer to function-local registers.
// OpTypeXXX, OpConstantXXX, and external OpFunction declarations only refer to
// other hoistable instructions vregs, so can be hoisted here.
//
// OpCapability instructions are removed and added to the minCaps and
// allCaps lists instead of hoisting right now. This allows capabilities added
// later, such as the Linkage capability for files with no OpEntryPoints, and
// still get deduplicated before the global OpCapability instructions are added.
static void hoistInstrsToMetablock(Module &M, MachineModuleInfo &MMI,
                                   SPIRVGlobalRegistry *GR,
                                   SPIRVRequirementHandler &Reqs) {
  // OpTypes and OpConstants can have cross references so at first we create
  // new meta regs and map them to old regs, then walk the list of instructions,
  // and create new (hoisted) instructions with new meta regs instead of old
  // ones. Also there are references to global values in constants.
  fillLocalAliasTables<Type>(GR, MB_TypeConstVars);
  fillLocalAliasTables<Constant>(GR, MB_TypeConstVars);
  fillLocalAliasTables<GlobalValue>(GR, MB_TypeConstVars);

  hoistGlobalOps<Type>(GR, MB_TypeConstVars);
  hoistGlobalOps<Constant>(GR, MB_TypeConstVars);

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

  hoistGlobalOps<GlobalValue>(GR, MB_TypeConstVars);
  fillLocalAliasTables<Function>(GR, MB_ExtFuncDecls);
  hoistGlobalOpsFunction(GR);
}

// True if there is an instruction in BB with all the same operands as
// the given instruction has (after the given starting index).
// TODO: maybe it needs to check Opcodes too.
static bool findSameInstrInMBB(const MachineInstr &A, MachineBasicBlock *MBB,
                               SPIRVGlobalRegistry *GR,
                               unsigned int StartOpIndex = 0) {
  for (const auto &B : *MBB) {
    const unsigned int NumAOps = A.getNumOperands();
    if (NumAOps == B.getNumOperands() && A.getNumDefs() == B.getNumDefs()) {
      bool AllOpsMatch = true;
      for (unsigned i = StartOpIndex; i < NumAOps && AllOpsMatch; ++i) {
        if (A.getOperand(i).isReg() && B.getOperand(i).isReg()) {
          Register RegA = A.getOperand(i).getReg();
          Register RegB = B.getOperand(i).getReg();
          AllOpsMatch = GR->getRegisterAlias(A.getMF(), RegA) == RegB;
        } else {
          AllOpsMatch = A.getOperand(i).isIdenticalTo(B.getOperand(i));
        }
      }
      if (AllOpsMatch)
        return true;
    }
  }
  return false;
}

static void addOpExtInstImports(Module &M, MachineModuleInfo &MMI,
                                const SPIRVInstrInfo *TII,
                                SPIRVGlobalRegistry *GR) {
  std::set<ExtInstSet> UsedExtInstSets = {ExtInstSet::OpenCL_std};
  SmallVector<MachineInstr *, 8> ExtInstInstrs;

  // Record all OpExtInst instuctions and the instruction sets they use
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpExtInst) {
        auto Set = static_cast<ExtInstSet>(MI.getOperand(2).getImm());
        UsedExtInstSets.insert(Set);
        ExtInstInstrs.push_back(&MI);
      }
    }
  }
  END_FOR_MF_IN_MODULE()

  std::map<ExtInstSet, Register> SetEnumToGlobalIDReg;

  setMetaBlock(MB_ExtInstImports);
  MachineRegisterInfo &MetaMRI = getMetaMF()->getRegInfo();
  for (const auto Set : UsedExtInstSets) {
    Register SetReg = MetaMRI.createVirtualRegister(&SPIRV::IDRegClass);
    auto MIB =
        buildInstrInCurrentMetaMBB(SPIRV::OpExtInstImport).addDef(SetReg);
    addStringImm(getExtInstSetName(Set), MIB);
    SetEnumToGlobalIDReg.insert({Set, SetReg});
  }

  // Replace all OpFunctionCalls with new ones referring to funcID vregs
  for (const auto MI : ExtInstInstrs) {
    auto ExtInstSetVar = static_cast<ExtInstSet>(MI->getOperand(2).getImm());
    MachineRegisterInfo &MRI = MI->getMF()->getRegInfo();

    // Ensure the mapping between local and global vregs is maintained for the
    // later reg numbering phase
    Register LocalReg = MRI.createVirtualRegister(&SPIRV::IDRegClass);
    Register GlobalReg = SetEnumToGlobalIDReg[ExtInstSetVar];
    GR->setRegisterAlias(MI->getMF(), LocalReg, GlobalReg);

    // Create a new copy of the OpExtInst but with the IDVReg for the imported
    // instruction set rather than an enum, then delete the old instruction.
    DebugLoc DL = MI->getDebugLoc();
    auto MIB = BuildMI(*MI->getParent(), MI, DL, TII->get(SPIRV::OpExtInst))
                   .addDef(MI->getOperand(0).getReg())
                   .addUse(MI->getOperand(1).getReg())
                   .addUse(LocalReg);
    const unsigned int NumOps = MI->getNumOperands();
    for (unsigned int i = 3; i < NumOps; ++i) {
      MIB.add(MI->getOperand(i));
    }
    GR->setSkipEmission(MI);
  }
}

// We need to add dummy registers whenever we need to reference a VReg that
// might be beyond the number of virtual registers declared in the function so
// far. This stops addUse and setReg from crashing when we use the new index.
static void addDummyVRegsUpToIndex(unsigned Index, MachineRegisterInfo &MRI) {
  while (Index >= MRI.getNumVirtRegs()) {
    MRI.createVirtualRegister(&SPIRV::IDRegClass);
  }
}

// Create a copy of the given instruction in the specified basic block of the
// global metadata function. We assume global register numbering has already
// occurred by this point, so we can directly copy VReg arguments (with padding
// to avoid crashed). We can also directly compare VReg arguments directly when
// detecting duplicates, rather than having to use local-to-global alias tables.
static void hoistMetaInstrWithGlobalRegs(MachineInstr &MI,
                                         SPIRVGlobalRegistry *GR,
                                         MetaBlockType MbType) {
  GR->setSkipEmission(&MI);
  if (findSameInstrInMBB(MI, getMetaMBB(MbType), GR))
    return; // Found a duplicate, so don't add it

  // TODO: unify with makeGlobalOp()
  // No duplicates, so add it
  setMetaBlock(MbType);
  MachineRegisterInfo &MetaMRI = getMetaMF()->getRegInfo();
  const unsigned NumOperands = MI.getNumOperands();
  auto MIB = buildInstrInCurrentMetaMBB(MI.getOpcode());
  for (unsigned i = 0; i < NumOperands; ++i) {
    MachineOperand Op = MI.getOperand(i);
    if (Op.isImm()) {
      MIB.addImm(Op.getImm());
    } else if (Op.isReg()) {
      // Add dummy regs to stop addUse crashing if Reg > max regs in func so far
      Register NewReg = GR->getRegisterAlias(MI.getMF(), Op.getReg());
      addDummyVRegsUpToIndex(NewReg.virtRegIndex(), MetaMRI);
      MIB.addUse(NewReg);
    } else {
      errs() << MI;
      llvm_unreachable("Unexpected operand type when copying spirv meta instr");
    }
  }
}

static void
extractInstructionsWithGlobalRegsToMetablockForMBB(MachineBasicBlock &MBB,
                                                   const SPIRVInstrInfo *TII,
                                                   SPIRVGlobalRegistry *GR) {
  for (MachineInstr &MI : MBB) {
    if (GR->getSkipEmission(&MI))
      continue;
    const unsigned OpCode = MI.getOpcode();
    if (OpCode == SPIRV::OpName || OpCode == SPIRV::OpMemberName)
      hoistMetaInstrWithGlobalRegs(MI, GR, MB_DebugNames);
    else if (OpCode == SPIRV::OpEntryPoint)
      hoistMetaInstrWithGlobalRegs(MI, GR, MB_EntryPoints);
    else if (TII->isDecorationInstr(MI))
      hoistMetaInstrWithGlobalRegs(MI, GR, MB_Annotations);
    else if (TII->isConstantInstr(MI))
      // Now OpSpecConstant*s are not in DT, but they need to be hoisted anyway.
      hoistMetaInstrWithGlobalRegs(MI, GR, MB_TypeConstVars);
  }
}

// Some global instructions make reference to function-local ID vregs, so cannot
// be hoisted until these registers are globally numbered and can be correctly
// referenced from the instructions' new global context.
//
// This pass extracts and deduplicates these instructions assuming global VReg
// numbers rather than using function-local alias tables like before.
static void
extractInstructionsWithGlobalRegsToMetablock(Module &M, MachineModuleInfo &MMI,
                                             const SPIRVInstrInfo *TII,
                                             SPIRVGlobalRegistry *GR) {
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF)
    extractInstructionsWithGlobalRegsToMetablockForMBB(MBB, TII, GR);
  END_FOR_MF_IN_MODULE()

  MachineBasicBlock *MBB = getMetaMBB(MB_TmpGlobalData);
  extractInstructionsWithGlobalRegsToMetablockForMBB(*MBB, TII, GR);
}

// After all OpEntryPoint and OpDecorate instructions have been globally
// extracted, we need to add all IDs with Import linkage as interface arguments
// to all OpEntryPoints.
//
// Currently, this adds all Import linked IDs to all EntryPoints, rather than
// walking the function CFG to see exactly which ones are used, so it may
// declare more interface variables than strictly necessary.
static void addEntryPointLinkageInterfaces(Module &M, MachineModuleInfo &MMI) {
  // Find all IDs with Import linkage by examining OpDecorates.
  MachineBasicBlock *DecMBB = getMetaMBB(MB_Annotations);
  SmallVector<Register, 4> InputLinkedIDs;
  for (MachineInstr &MI : *DecMBB) {
    const unsigned OpCode = MI.getOpcode();
    const unsigned NumOps = MI.getNumOperands();
    if (OpCode == SPIRV::OpDecorate &&
        MI.getOperand(1).getImm() == Decoration::LinkageAttributes &&
        MI.getOperand(NumOps - 1).getImm() == LinkageType::Import) {
      const Register Target = MI.getOperand(0).getReg();
      InputLinkedIDs.push_back(Target);
    }
  }

  // Add any Import linked IDs as interface args to all OpEntryPoints.
  if (!InputLinkedIDs.empty()) {
    MachineBasicBlock *EntryMBB = getMetaMBB(MB_EntryPoints);
    for (MachineInstr &MI : *EntryMBB)
      for (unsigned Id : InputLinkedIDs)
        MI.addOperand(MachineOperand::CreateReg(Id, false));
  }
}

static void numberRegistersInMBB(MachineBasicBlock &MBB,
                                 unsigned int &RegBaseIndex,
                                 MachineRegisterInfo &MRI,
                                 SPIRVGlobalRegistry *GR) {
  auto *MF = MBB.getParent();
  for (MachineInstr &MI : MBB) {
    for (MachineOperand &Op : MI.operands()) {
      if (Op.isReg()) {
        Register Reg = Op.getReg();
        addDummyVRegsUpToIndex(RegBaseIndex, MRI);
        if (!GR->hasRegisterAlias(MF, Reg)) {
          Register NewReg = Register::index2VirtReg(RegBaseIndex);
          ++RegBaseIndex;
          GR->setRegisterAlias(MF, Reg, NewReg);
        }
      }
    }
  }
}

// Starting with the metablock, number all registers in all functions
// globally from 0 onwards. Local registers aliasing results of OpType,
// OpConstant etc. that were extracted to the metablock are now assigned
// the correct global registers instead of the function-local ones.
static void numberRegistersGlobally(Module &M, MachineModuleInfo &MMI,
                                    SPIRVGlobalRegistry *GR) {
  // Use raw index 0 - inf, and convert with index2VirtReg later
  unsigned int RegBaseIndex = 0;
  BEGIN_FOR_MF_IN_MODULE(M, MMI)
  auto &MRI = MF->getRegInfo();
  if (MFIndex == 0) {
    RegBaseIndex = GR->getMetaMF()->getRegInfo().getNumVirtRegs();
    MachineBasicBlock *MBB = getMetaMBB(MB_TmpGlobalData);
    numberRegistersInMBB(*MBB, RegBaseIndex, MRI, GR);
  } else {
    for (MachineBasicBlock &MBB : *MF)
      numberRegistersInMBB(MBB, RegBaseIndex, MRI, GR);
  }
  END_FOR_MF_IN_MODULE()
}

using FuncNameToIDMap = std::map<std::string, Register>;

// Iterate over all extracted OpDecorate instructions to look for IDs declared
// with Import linkage, and map the imported name string to the VReg defining
// that variable (which will usually be the result of an OpFunction).
// This lets us call externally imported functions using the correct VReg ID.
static void addExternalDeclarationsToIDMap(FuncNameToIDMap &FuncNameToOpID) {
  // Only look for OpDecorates in the global meta function
  const MachineBasicBlock *DecMBB = getMetaMBB(MB_Annotations);
  for (const auto &MI : *DecMBB) {
    if (MI.getOpcode() == SPIRV::OpDecorate) {
      // If it's got Import linkage
      auto Dec = MI.getOperand(1).getImm();
      if (Dec == Decoration::LinkageAttributes) {
        auto Lnk = MI.getOperand(MI.getNumOperands() - 1).getImm();
        if (Lnk == LinkageType::Import) {
          // Map imported function name to function ID VReg.
          std::string Name = getStringImm(MI, 2);
          Register Target = MI.getOperand(0).getReg();
          FuncNameToOpID[Name] = Target;
        }
      }
    }
  }
}

// Replace global value for the  Callee argument of OpFunctionCall with a
// register number for the function ID now that all results of OpFunction are
// globally numbered registers
static void assignFunctionCallIDs(Module &M, MachineModuleInfo &MMI,
                                  const SPIRVInstrInfo *TII,
                                  SPIRVGlobalRegistry *GR) {
  std::map<std::string, Register> FuncNameToID;
  SmallVector<MachineInstr *, 8> FuncCalls;

  addExternalDeclarationsToIDMap(FuncNameToID);

  // Record all OpFunctionCalls, and all internal OpFunction declarations.
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpFunction && !GR->getSkipEmission(&MI)) {
        Register Reg = GR->getRegisterAlias(MF, MI.defs().begin()->getReg());
        assert(Reg.isValid());
        FuncNameToID[F->getGlobalIdentifier()] = Reg;
      } else if (MI.getOpcode() == SPIRV::OpFunctionCall) {
        FuncCalls.push_back(&MI);
      }
    }
  }
  END_FOR_MF_IN_MODULE()

  // Replace all OpFunctionCalls with new ones referring to FuncID vregs
  for (const auto FuncCall : FuncCalls) {
    auto FuncName = FuncCall->getOperand(2).getGlobal()->getGlobalIdentifier();
    auto FuncID = FuncNameToID.find(FuncName);
    if (FuncID == FuncNameToID.end()) {
      errs() << "Unknown function: " << FuncName << "\n";
      llvm_unreachable("Error: Could not find function id");
    }
    const auto MF = FuncCall->getMF();

    // Stops addUse crashing if FuncID > max regs in func so far
    addDummyVRegsUpToIndex(FuncID->second.virtRegIndex(), MF->getRegInfo());

    // Create a new copy of the OpFunctionCall but with the IDVReg for the
    // callee rather than a GlobalValue, then delete the old instruction.
    // Also insert dummy OpFunction which generates local register.
    // TODO: maybe move OpFunction creation to SPIRVCallLowering.
    Register GlobaFuncReg = FuncID->second;
    Register FuncTypeReg = FuncCall->getOperand(1).getReg();
    Register LocalFuncReg =
        MF->getRegInfo().createVirtualRegister(&SPIRV::IDRegClass);
    DebugLoc DL = FuncCall->getDebugLoc();
    auto MIB = BuildMI(*FuncCall->getParent(), FuncCall, DL,
                       TII->get(SPIRV::OpFunction))
                   .addDef(LocalFuncReg)
                   .addUse(FuncCall->getOperand(1).getReg())
                   .addImm(FunctionControl::None)
                   .addUse(FuncTypeReg);
    GR->setSkipEmission(MIB);

    auto MIB2 = BuildMI(*FuncCall->getParent(), FuncCall, DL,
                        TII->get(SPIRV::OpFunctionCall))
                    .addDef(FuncCall->getOperand(0).getReg())
                    .addUse(FuncCall->getOperand(1).getReg())
                    .addUse(LocalFuncReg);
    const unsigned int NumOps = FuncCall->getNumOperands();
    for (unsigned int i = 3; i < NumOps; ++i) {
      MIB2.addUse(FuncCall->getOperand(i).getReg());
    }
    GR->setRegisterAlias(MF, LocalFuncReg, GlobaFuncReg);
    FuncCall->removeFromParent();
  }
}

// Create global OpCapability instructions for the required capabilities
static void addGlobalRequirements(const SPIRVRequirementHandler &Reqs,
                                  const SPIRVSubtarget &ST) {
  // Abort here if not all requirements can be satisfied
  Reqs.checkSatisfiable(ST);
  setMetaBlock(MB_Capabilities);

  for (const auto &Cap : Reqs.getMinimalCapabilities())
    buildInstrInCurrentMetaMBB(SPIRV::OpCapability).addImm(Cap);

  // Generate the final OpExtensions with strings instead of enums
  for (const auto &Ext : Reqs.getExtensions()) {
    auto MIB = buildInstrInCurrentMetaMBB(SPIRV::OpExtension);
    addStringImm(getExtensionName(Ext), MIB);
  }

  // TODO add a pseudo instr for version number
}

// Add all requirements needed for the given instruction.
// Defined in SPIRVAddRequirementsPass.cpp
extern void addInstrRequirements(const MachineInstr &MI,
                                 SPIRVRequirementHandler &Reqs,
                                 const SPIRVSubtarget &ST);

// Process each unreferenced global variable so its type and initializer
// will be emitted to MB_TmpGlobalData. Apply addInstrRequirements to it.
// Substitute each created G_CONSTANT to OpConstantI.
static void processGlobalUnrefVars(Module &M, MachineModuleInfo &MMI,
                                   SPIRVRequirementHandler &Reqs,
                                   SPIRVGlobalRegistry *GR,
                                   const SPIRVSubtarget &ST) {
  // Walk over each MF's DT and select all unreferenced global variables.
  SmallVector<GlobalVariable *, 8> GlobalVarList;
  auto Map = GR->getAllUses<GlobalValue>();
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E;) {
    GlobalVariable *GV = &*I++;
    auto AddrSpace = GV->getAddressSpace();
    auto Storage = addressSpaceToStorageClass(AddrSpace);
    if (Storage != StorageClass::Function) {
      auto t = Map.find(GV);
      if (t == Map.end())
        // GV is not found in maps of all MFs so it's unreferenced.
        GlobalVarList.push_back(GV);
    }
  }

  // Walk over all created instructions and add requirement. Also convert all
  // G_CONSTANTs (GR creates them) to OpConstantI.
  // TODO: clean up this.
  GR->setCurrentFunc(*getMetaMF());
  setMetaBlock(MB_TmpGlobalData);
  MachineBasicBlock *MBB = getMetaBlock();
  for (MachineInstr &MI : *MBB) {
    addInstrRequirements(MI, Reqs, ST);
    auto Opcode = MI.getOpcode();
    if (Opcode == TargetOpcode::G_CONSTANT ||
        Opcode == TargetOpcode::G_FCONSTANT) {
      Register Res = MI.getOperand(0).getReg();
      APInt Imm;
      if (Opcode == TargetOpcode::G_FCONSTANT) {
        const auto FPImm = MI.getOperand(1).getFPImm();
        Imm = FPImm->getValueAPF().bitcastToAPInt();
      } else {
        Imm = MI.getOperand(1).getCImm()->getValue();
      }
      SPIRVType *ResType = GR->getSPIRVTypeForVReg(Res);
      auto MIB = buildInstrInCurrentMetaMBB(SPIRV::OpConstantI)
                     .addDef(Res)
                     .addUse(GR->getSPIRVTypeID(ResType))
                     .addImm(Imm.getZExtValue());
      // TODO: remove this call
      constrainRegOperands(MIB);
      GR->setSkipEmission(&MI);
    }
  }
}

// Add a meta function containing all OpType, OpConstant etc.
// Extract all OpType, OpConst etc. into this meta block
// Number registers globally, including references to global OpType etc.
bool SPIRVGlobalTypesAndRegNum::runOnModule(Module &M) {
  MachineModuleInfoWrapperPass &MMIWrapper =
      getAnalysis<MachineModuleInfoWrapperPass>();

  initMetaBlockBuilder(M, MMIWrapper.getMMI());
  const auto &ST = static_cast<const SPIRVSubtarget &>(MetaMF->getSubtarget());
  const SPIRVInstrInfo *TII = ST.getInstrInfo();
  SPIRVGlobalRegistry *GR = ST.getSPIRVGlobalRegistry();
  GR->setMetaMF(getMetaMF());

  SPIRVRequirementHandler Reqs;

  addHeaderOps(M, Reqs, ST);

  // Process global variables which are not referenced in all functions.
  processGlobalUnrefVars(M, MMIWrapper.getMMI(), Reqs, GR, ST);

  addOpExtInstImports(M, MMIWrapper.getMMI(), TII, GR);

  // Extract type instructions to the top MetaMBB and keep track of which local
  // VRegs the correspond to with functionLocalAliasTables
  hoistInstrsToMetablock(M, MMIWrapper.getMMI(), GR, Reqs);

  // Number registers from 0 onwards, and fix references to global OpType etc
  numberRegistersGlobally(M, MMIWrapper.getMMI(), GR);

  // Extract instructions like OpName, OpEntryPoint, OpDecorate etc.
  // which all rely on globally numbered registers, which they forward-reference
  extractInstructionsWithGlobalRegsToMetablock(M, MMIWrapper.getMMI(), TII, GR);

  addEntryPointLinkageInterfaces(M, MMIWrapper.getMMI());

  assignFunctionCallIDs(M, MMIWrapper.getMMI(), TII, GR);

  // If there are no entry points, we need the Linkage capability
  if (getMetaMBB(MB_EntryPoints)->empty())
    Reqs.addCapability(Capability::Linkage);

  addGlobalRequirements(Reqs, ST);

  // Check that all instructions have been moved from the TmpGlobalData block.
  assert(getMetaMBB(MB_TmpGlobalData)->empty() &&
         "TmpGlobalData block is expected to be empty");

  return false;
}

INITIALIZE_PASS(SPIRVGlobalTypesAndRegNum, DEBUG_TYPE,
                "SPIRV hoist OpType etc. & number VRegs globally", false, false)

char SPIRVGlobalTypesAndRegNum::ID = 0;

ModulePass *llvm::createSPIRVGlobalTypesAndRegNumPass() {
  return new SPIRVGlobalTypesAndRegNum();
}
