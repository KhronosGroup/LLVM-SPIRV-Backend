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
// occur at the start of the file. This pass hoists these as necessary.
//
// This pass also re-numbers the registers globally, and patches up any
// references to previously local registers that were hoisted, or function IDs
// which require globally scoped registers.
//
// This pass breaks all notion of register def/use, and generated MachineInstrs
// that are technically invalid as a result. As such, it must be the last pass,
// and requires instruction verification to be disabled afterwards.
//
//===----------------------------------------------------------------------===//

#include "SPIRV.h"
#include "SPIRVCapabilityUtils.h"
#include "SPIRVEnumRequirements.h"
#include "SPIRVStrings.h"
#include "SPIRVSubtarget.h"
#include "SPIRVTypeRegistry.h"
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

// Set the builder's MBB to one of the sections from the MetaBlockType enum.
static void setMetaBlock(MachineIRBuilder &MetaBuilder, MetaBlockType block) {
  const auto &MF = MetaBuilder.getMF();
  MetaBuilder.setMBB(*MF.getBlockNumbered(block));
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

// Maps a local VReg id to the corresponding VReg id in the global meta-function
using LocalToGlobalRegTable = std::map<Register, Register>;

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
static void addHeaderOps(Module &M, MachineIRBuilder &MIRBuilder,
                         SPIRVRequirementHandler &Reqs,
                         const SPIRVSubtarget &ST) {
  setMetaBlock(MIRBuilder, MB_MemoryModel);
  unsigned PtrSize = ST.getPointerSize();

  // Add OpMemoryModel
  using namespace AddressingModel;
  auto Addr = PtrSize == 32 ? Physical32 : PtrSize == 64 ? Physical64 : Logical;
  auto Mem = MemoryModel::OpenCL;
  MIRBuilder.buildInstr(SPIRV::OpMemoryModel).addImm(Addr).addImm(Mem);

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
    setMetaBlock(MIRBuilder, MB_DebugSourceAndStrings);
  }

  // Build the OpSource
  auto SrcLang = SourceLanguage::OpenCL_C;
  MIRBuilder.buildInstr(SPIRV::OpSource).addImm(SrcLang).addImm(OpenCLVersion);
  Reqs.addRequirements(getSourceLanguageRequirements(SrcLang, ST));
}

// Create a new Function and MachineFunction for the given MachineIRBuilder to
// add meta-instructions to. It should have a series of empty basic blocks, one
// for each required SPIR-V module section, so that subsequent users of the
// MachineIRBuilder can hoist instructions into the right places easily.
static void initMetaBlockBuilder(Module &M, MachineModuleInfo &MMI,
                                 MachineIRBuilder &MetaBuilder) {

  // Add an empty placeholder function - MachineInstrs need to exist somewhere.
  auto VoidType = Type::getVoidTy(M.getContext());
  auto FType = FunctionType::get(VoidType, false);
  auto Linkage = Function::LinkageTypes::InternalLinkage;
  Function *F = Function::Create(FType, Linkage, "GlobalStuff");

  // Add the function and a corresponding MachineFunction to the module
  F->getBasicBlockList().push_back(BasicBlock::Create(M.getContext()));
  M.getFunctionList().push_front(F);

  // Add the necessary basic blocks according to the MetaBlockTypes enum.
  MachineFunction &MetaMF = MMI.getOrCreateMachineFunction(*F);
  for (unsigned int i = 0; i < NUM_META_BLOCKS; ++i) {
    MachineBasicBlock *MetaMBB = MetaMF.CreateMachineBasicBlock();
    MetaMF.push_back(MetaMBB);
  }

  // Tell the builder about the meta function and an initial basic block.
  MetaBuilder.setMF(MetaMF);
  setMetaBlock(MetaBuilder, MB_TypeConstVars);
}

using LocalAliasTablesTy = std::map<MachineFunction *, LocalToGlobalRegTable>;

template <typename T>
static void fillLocalAliasTables(MachineIRBuilder &MetaBuilder,
                                 const SPIRVDuplicatesTracker<T> *DT,
                                 SPIRVTypeRegistry *TR, MetaBlockType MBType,
                                 LocalAliasTablesTy &LocalAliasTables) {
  // FIXME: setMetaBlock
  setMetaBlock(MetaBuilder, MBType);
  // Make meta registers for entries of DT.
  for (auto &CU : DT->getAllUses()) {
    auto MetaReg =
        MetaBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);

    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      LocalAliasTables[MF][Reg] = MetaReg;
    }
  }
  // Make meta registers for special types and constants collected in the map.
  if (std::is_same<T, Type>::value) {
    for (auto &CU : TR->getSpecialTypesAndConstsMap()) {
      for (auto &typeGroup : CU.second) {
        auto MetaReg =
            MetaBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);
        for (auto &t : typeGroup) {
          MachineFunction *MF = t.first;
          assert(t.second->getOperand(0).isReg());
          Register reg = t.second->getOperand(0).getReg();
          LocalAliasTables[MF][reg] = MetaReg;
        }
      }
    }
  }
}

template <typename T>
static void hoistGlobalOp(MachineIRBuilder &MetaBuilder, Register Reg,
                          MachineFunction *MF,
                          SmallVector<MachineInstr *, 8> &ToRemove,
                          LocalAliasTablesTy &LocalAliasTables) {
  assert(LocalAliasTables[MF].find(Reg) != LocalAliasTables[MF].end() &&
         "Cannot find reg alias in LocalAliasTables");
  auto MetaReg = LocalAliasTables[MF][Reg];
  auto *ToHoist = MF->getRegInfo().getVRegDef(Reg);
  assert(ToHoist && "There should be an instruction that defines the register");
  ToRemove.push_back(ToHoist);

  if (!MetaBuilder.getMRI()->getVRegDef(MetaReg)) {
    auto MIB = MetaBuilder.buildInstr(ToHoist->getOpcode());
    MIB.addDef(MetaReg);

    for (unsigned int i = ToHoist->getNumExplicitDefs();
         i < ToHoist->getNumOperands(); ++i) {
      MachineOperand Op = ToHoist->getOperand(i);
      if (Op.isImm()) {
        MIB.addImm(Op.getImm());
      } else if (Op.isFPImm()) {
        MIB.addFPImm(Op.getFPImm());
      } else if (Op.isReg()) {
        auto MetaRegID = LocalAliasTables[MF].find(Op.getReg());
        if (MetaRegID != LocalAliasTables[MF].end()) {
          Register MetaReg2 = MetaRegID->second;
          assert(MetaReg2.isValid() && "No reg alias found");
          MIB.addUse(MetaReg2);
        } else {
          llvm_unreachable("Generate WRONG type!\n");
        }
      } else {
        llvm_unreachable(
            "Unexpected operand type when copying spirv meta instr");
      }
    }
  }
}

template <typename T>
static void hoistGlobalOps(MachineIRBuilder &MetaBuilder,
                           const SPIRVDuplicatesTracker<T> *DT,
                           SPIRVTypeRegistry *TR, MetaBlockType MBType,
                           LocalAliasTablesTy &LocalAliasTables) {
  setMetaBlock(MetaBuilder, MBType);
  SmallVector<MachineInstr *, 8> ToRemove;
  // Hoist instructions from DT.
  for (auto &CU : DT->getAllUses()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      // Some instructions may be deleted during global isel, so hoist only
      // those that exist in current IR.
      if (MF->getRegInfo().getVRegDef(Reg))
        hoistGlobalOp<T>(MetaBuilder, Reg, MF, ToRemove, LocalAliasTables);
    }
  }
  // Hoist special types and constants collected in the map.
  if (std::is_same<T, Type>::value) {
    for (auto &CU : TR->getSpecialTypesAndConstsMap())
      for (auto &TypeGroup : CU.second)
        for (auto &t : TypeGroup) {
          MachineFunction *MF = t.first;
          assert(t.second->getOperand(0).isReg());
          Register Reg = t.second->getOperand(0).getReg();
          hoistGlobalOp<T>(MetaBuilder, Reg, MF, ToRemove, LocalAliasTables);
        }
  }

  for (auto *MI : ToRemove)
    // If we have two builtin IR types that correspond to the same SPIR-V type,
    // current MI may be already erased, so check it before the erase.
    if (MI && MI->getParent())
      MI->eraseFromParent();
}

// we need this specialization as
// for function decls we want to not only hoist OpFunctions
// but OpFunctionParameters too
//
// TODO: consider replacing this with explicit
// OpFunctionParameter generation here instead handling it
// in CallLowering
template <>
void hoistGlobalOps(MachineIRBuilder &MetaBuilder,
                    const SPIRVDuplicatesTracker<Function> *DT,
                    SPIRVTypeRegistry *TR, MetaBlockType MBType,
                    LocalAliasTablesTy &LocalAliasTables) {
  setMetaBlock(MetaBuilder, MBType);
  SmallVector<MachineInstr *, 8> ToRemove;

  for (auto &CU : DT->getAllUses()) {
    SmallVector<Register, 4> MetaRegs;
    MetaRegs.push_back(
        MetaBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass));
    for (auto &Arg : CU.first->args())
      MetaRegs.push_back(
          MetaBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass));

    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      auto *ToHoist = MF->getRegInfo().getVRegDef(Reg);
      auto CurMetaReg = MetaRegs.begin();
      while (ToHoist && (ToHoist->getOpcode() == SPIRV::OpFunction ||
                         ToHoist->getOpcode() == SPIRV::OpFunctionParameter)) {
        ToRemove.push_back(ToHoist);
        Reg = ToHoist->getOperand(0).getReg();
        assert(CurMetaReg != MetaRegs.end());
        auto MetaReg = *CurMetaReg;
        CurMetaReg++;

        if (!MetaBuilder.getMRI()->getVRegDef(MetaReg)) {
          auto MIB = MetaBuilder.buildInstr(ToHoist->getOpcode());
          MIB.addDef(MetaReg);

          for (unsigned int i = ToHoist->getNumExplicitDefs();
               i < ToHoist->getNumOperands(); ++i) {
            MachineOperand Op = ToHoist->getOperand(i);
            if (Op.isImm()) {
              MIB.addImm(Op.getImm());
            } else if (Op.isFPImm()) {
              MIB.addFPImm(Op.getFPImm());
            } else if (Op.isReg()) {
              Register MetaReg2 = LocalAliasTables[MF].at(Op.getReg());
              assert(MetaReg2.isValid() && "No reg alias found");
              MIB.addUse(MetaReg2);
            } else {
              errs() << *ToHoist << "\n";
              llvm_unreachable(
                  "Unexpected operand type when copying spirv meta instr");
            }
          }
        }
        LocalAliasTables[MF][Reg] = MetaReg;
        ToHoist = ToHoist->getNextNode();
        Reg = ToHoist->getOperand(0).getReg();
      }
    }
    MetaBuilder.buildInstr(SPIRV::OpFunctionEnd);
  }

  for (auto *MI : ToRemove)
    MI->eraseFromParent();
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
                                   MachineIRBuilder &MIRBuilder,
                                   LocalAliasTablesTy &LocalAliasTables,
                                   SPIRVRequirementHandler &Reqs) {
  const auto *ST =
      static_cast<const SPIRVSubtarget *>(&MIRBuilder.getMF().getSubtarget());
  auto *DT = ST->getSPIRVDuplicatesTracker();
  auto TR = ST->getSPIRVTypeRegistry();

  // OpTypes and OpConstants can have cross references so at first we create
  // new meta regs and map them to old regs, then walk the list of instructions,
  // and create new (hoisted) instructions with new meta regs instead of old
  // ones. Also there are references to global values in constants.
  fillLocalAliasTables<Type>(MIRBuilder, DT->get<Type>(), TR, MB_TypeConstVars,
                             LocalAliasTables);
  fillLocalAliasTables<Constant>(MIRBuilder, DT->get<Constant>(), TR,
                                 MB_TypeConstVars, LocalAliasTables);
  fillLocalAliasTables<GlobalValue>(MIRBuilder, DT->get<GlobalValue>(), TR,
                                    MB_TypeConstVars, LocalAliasTables);

  hoistGlobalOps<Type>(MIRBuilder, DT->get<Type>(), TR, MB_TypeConstVars,
                       LocalAliasTables);
  hoistGlobalOps<Constant>(MIRBuilder, DT->get<Constant>(), TR,
                           MB_TypeConstVars, LocalAliasTables);

  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)

  // Iterate through and hoist any instructions we can at this stage.
  // bool hasSeenFirstOpFunction = false;
  SmallVector<MachineInstr *, 16> ToRemove;
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpExtension) {
        // Here, OpExtension just has a single enum operand, not a string
        auto Ext = Extension::Extension(MI.getOperand(0).getImm());
        Reqs.addExtension(Ext);
        ToRemove.push_back(&MI);
      } else if (MI.getOpcode() == SPIRV::OpCapability) {
        auto Cap = Capability::Capability(MI.getOperand(0).getImm());
        Reqs.addCapability(Cap);
        ToRemove.push_back(&MI);
      }
    }
  }
  for (MachineInstr *MI : ToRemove) {
    MI->removeFromParent();
  }
  END_FOR_MF_IN_MODULE()
  hoistGlobalOps<GlobalValue>(MIRBuilder, DT->get<GlobalValue>(), TR,
                              MB_TypeConstVars, LocalAliasTables);
  fillLocalAliasTables<Function>(MIRBuilder, DT->get<Function>(), TR,
                                 MB_ExtFuncDecls, LocalAliasTables);
  hoistGlobalOps<Function>(MIRBuilder, DT->get<Function>(), TR, MB_ExtFuncDecls,
                           LocalAliasTables);
}

// True when all the operands of an instruction are an exact match (after the
// given starting index).
static bool allOpsMatch(const MachineInstr &A, const MachineInstr &B,
                        unsigned int StartOpIndex = 0) {
  const unsigned int NumAOps = A.getNumOperands();
  if (NumAOps == B.getNumOperands() && A.getNumDefs() == B.getNumDefs()) {
    bool AllOpsMatch = true;
    for (unsigned i = StartOpIndex; i < NumAOps && AllOpsMatch; ++i) {
      AllOpsMatch = A.getOperand(i).isIdenticalTo(B.getOperand(i));
    }
    if (AllOpsMatch)
      return true;
  }
  return false;
}

static void addOpExtInstImports(Module &M, MachineModuleInfo &MMI,
                                MachineIRBuilder &MIRBuilder,
                                LocalAliasTablesTy &LocalAliasTables) {

  std::set<ExtInstSet> UsedExtInstSets = {ExtInstSet::OpenCL_std};
  SmallVector<std::pair<MachineInstr *, LocalToGlobalRegTable *>, 8>
      ExtInstInstrs;

  // Record all OpExtInst instuctions and the instruction sets they use
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpExtInst) {
        auto Set = static_cast<ExtInstSet>(MI.getOperand(2).getImm());
        UsedExtInstSets.insert(Set);
        ExtInstInstrs.push_back({&MI, &LocalAliasTables[MF]});
      }
    }
  }
  END_FOR_MF_IN_MODULE()

  std::map<ExtInstSet, Register> SetEnumToGlobalIDReg;

  setMetaBlock(MIRBuilder, MB_ExtInstImports);
  auto MetaMRI = MIRBuilder.getMRI();
  for (const auto Set : UsedExtInstSets) {
    auto SetReg = MetaMRI->createVirtualRegister(&SPIRV::IDRegClass);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInstImport).addDef(SetReg);
    addStringImm(getExtInstSetName(Set), MIB);
    SetEnumToGlobalIDReg.insert({Set, SetReg});
  }

  // Replace all OpFunctionCalls with new ones referring to funcID vregs
  for (const auto &Entry : ExtInstInstrs) {
    auto *MI = Entry.first;
    auto *AliasTable = Entry.second;

    auto ExtInstSetVar = static_cast<ExtInstSet>(MI->getOperand(2).getImm());

    MachineIRBuilder MIRBuilder;
    MIRBuilder.setMF(*MI->getMF());
    auto MRI = MIRBuilder.getMRI();

    // Ensure the mapping between local and global vregs is maintained for the
    // later reg numbering phase
    auto LocalReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
    auto GlobalReg = SetEnumToGlobalIDReg[ExtInstSetVar];
    AliasTable->insert({LocalReg, GlobalReg});

    // Create a new copy of the OpExtInst but with the IDVReg for the imported
    // instruction set rather than an enum, then delete the old instruction.
    MIRBuilder.setMBB(*MI->getParent());
    MIRBuilder.setInstr(*MI);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                   .addDef(MI->getOperand(0).getReg())
                   .addUse(MI->getOperand(1).getReg())
                   .addUse(LocalReg);
    const unsigned int NumOps = MI->getNumOperands();
    for (unsigned int i = 3; i < NumOps; ++i) {
      MIB.add(MI->getOperand(i));
    }

    MI->removeFromParent();
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
                                         MachineIRBuilder &MIRBuilder,
                                         MetaBlockType MbType) {

  setMetaBlock(MIRBuilder, MbType);
  auto &MBB = MIRBuilder.getMBB();
  for (const auto &I : MBB) {
    if (allOpsMatch(MI, I))
      return; // Found a duplicate, so don't add it
  }

  // No duplicates, so add it
  auto &MetaMRI = MIRBuilder.getMF().getRegInfo();
  const unsigned NumOperands = MI.getNumOperands();
  auto MIB = MIRBuilder.buildInstr(MI.getOpcode());
  for (unsigned i = 0; i < NumOperands; ++i) {
    MachineOperand Op = MI.getOperand(i);
    if (Op.isImm()) {
      MIB.addImm(Op.getImm());
    } else if (Op.isReg()) {
      // Add dummy regs to stop addUse crashing if Reg > max regs in func so far
      addDummyVRegsUpToIndex(Op.getReg().virtRegIndex(), MetaMRI);
      MIB.addUse(Op.getReg());
    } else {
      errs() << MI << "\n";
      llvm_unreachable("Unexpected operand type when copying spirv meta instr");
    }
  }
}

static void extractInstructionsWithGlobalRegsToMetablockForMBB(
    MachineBasicBlock &MBB, const SPIRVInstrInfo *TII,
    MachineIRBuilder &MIRBuilder) {
  SmallVector<MachineInstr *, 16> ToRemove;
  for (MachineInstr &MI : MBB) {
    const unsigned OpCode = MI.getOpcode();
    if (OpCode == SPIRV::OpName || OpCode == SPIRV::OpMemberName) {
      hoistMetaInstrWithGlobalRegs(MI, MIRBuilder, MB_DebugNames);
      ToRemove.push_back(&MI);
    } else if (OpCode == SPIRV::OpEntryPoint) {
      hoistMetaInstrWithGlobalRegs(MI, MIRBuilder, MB_EntryPoints);
      ToRemove.push_back(&MI);
    } else if (TII->isDecorationInstr(MI)) {
      hoistMetaInstrWithGlobalRegs(MI, MIRBuilder, MB_Annotations);
      ToRemove.push_back(&MI);
    } else if (TII->isConstantInstr(MI)) {
      // Now OpSpecConstant*s are not in DT, but they need to be hoisted anyway.
      hoistMetaInstrWithGlobalRegs(MI, MIRBuilder, MB_TypeConstVars);
      ToRemove.push_back(&MI);
    }
  }
  for (MachineInstr *MI : ToRemove) {
    MI->removeFromParent();
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
                                             MachineIRBuilder &MIRBuilder) {
  const auto TII = static_cast<const SPIRVInstrInfo *>(&MIRBuilder.getTII());
  setMetaBlock(MIRBuilder, MB_DebugNames);
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF)
    extractInstructionsWithGlobalRegsToMetablockForMBB(MBB, TII, MIRBuilder);
  END_FOR_MF_IN_MODULE()
  setMetaBlock(MIRBuilder, MB_TmpGlobalData);
  auto &MBB = MIRBuilder.getMBB();
  extractInstructionsWithGlobalRegsToMetablockForMBB(MBB, TII, MIRBuilder);
}

// After all OpEntryPoint and OpDecorate instructions have been globally
// extracted, we need to add all IDs with Import linkage as interface arguments
// to all OpEntryPoints.
//
// Currently, this adds all Import linked IDs to all EntryPoints, rather than
// walking the function CFG to see exactly which ones are used, so it may
// declare more interface variables than strictly necessary.
static void addEntryPointLinkageInterfaces(Module &M, MachineModuleInfo &MMI,
                                           MachineIRBuilder &MIRBuilder) {
  // Find all IDs with Import linkage by examining OpDecorates
  setMetaBlock(MIRBuilder, MB_Annotations);
  auto &DecMBB = MIRBuilder.getMBB();
  SmallVector<Register, 4> InputLinkedIDs;
  for (MachineInstr &MI : DecMBB) {
    const unsigned OpCode = MI.getOpcode();
    const unsigned NumOps = MI.getNumOperands();
    if (OpCode == SPIRV::OpDecorate &&
        MI.getOperand(1).getImm() == Decoration::LinkageAttributes &&
        MI.getOperand(NumOps - 1).getImm() == LinkageType::Import) {
      const Register Target = MI.getOperand(0).getReg();
      InputLinkedIDs.push_back(Target);
    }
  }

  // Add any Import linked IDs as interface args to all OpEntryPoints
  if (!InputLinkedIDs.empty()) {
    setMetaBlock(MIRBuilder, MB_EntryPoints);
    auto &EntryMBB = MIRBuilder.getMBB();
    for (MachineInstr &MI : EntryMBB) {
      for (unsigned Id : InputLinkedIDs) {
        MI.addOperand(MachineOperand::CreateReg(Id, false));
      }
    }
  }
}

static void numberRegistersInMBB(MachineBasicBlock &MBB,
                                 unsigned int &RegBaseIndex,
                                 LocalToGlobalRegTable &LocalToMetaVRegAliasMap,
                                 MachineRegisterInfo &MRI) {
  for (MachineInstr &MI : MBB) {
    for (MachineOperand &Op : MI.operands()) {
      if (Op.isReg()) {
        Register NewReg;
        auto VR = LocalToMetaVRegAliasMap.find(Op.getReg());
        // Stops setReg crashing if reg index > max regs in func
        addDummyVRegsUpToIndex(RegBaseIndex, MRI);
        if (VR == LocalToMetaVRegAliasMap.end()) {
          NewReg = Register::index2VirtReg(RegBaseIndex);
          ++RegBaseIndex;
          LocalToMetaVRegAliasMap.insert({Op.getReg(), NewReg});
        } else {
          NewReg = VR->second;
        }
        Op.setReg(NewReg);
      }
    }
  }
}

// Starting with the metablock, number all registers in all functions
// globally from 0 onwards. Local registers aliasing results of OpType,
// OpConstant etc. that were extracted to the metablock are now assigned
// the correct global registers instead of the function-local ones.
static void numberRegistersGlobally(Module &M, MachineModuleInfo &MMI,
                                    MachineIRBuilder &MIRBuilder,
                                    LocalAliasTablesTy &RegAliasTables) {

  // Use raw index 0 - inf, and convert with index2VirtReg later
  unsigned int RegBaseIndex = 0;
  BEGIN_FOR_MF_IN_MODULE(M, MMI)
  auto &MRI = MF->getRegInfo();
  auto LocalToMetaVRegAliasMap = RegAliasTables[MF];
  if (MFIndex == 0) {
    RegBaseIndex = MIRBuilder.getMF().getRegInfo().getNumVirtRegs();
    setMetaBlock(MIRBuilder, MB_TmpGlobalData);
    auto &MBB = MIRBuilder.getMBB();
    numberRegistersInMBB(MBB, RegBaseIndex, LocalToMetaVRegAliasMap, MRI);
  } else {
    for (MachineBasicBlock &MBB : *MF) {
      numberRegistersInMBB(MBB, RegBaseIndex, LocalToMetaVRegAliasMap, MRI);
    }
  }
  END_FOR_MF_IN_MODULE()
}

using FuncNameToIDMap = std::map<std::string, Register>;

// Iterate over all extracted OpDecorate instructions to look for IDs declared
// with Import linkage, and map the imported name string to the VReg defining
// that variable (which will usually be the result of an OpFunction).
// This lets us call externally imported functions using the correct VReg ID.
static void addExternalDeclarationsToIDMap(MachineIRBuilder &MetaBuilder,
                                           FuncNameToIDMap &FuncNameToOpID) {

  // Only look for OpDecorates in the global meta function
  setMetaBlock(MetaBuilder, MB_Annotations);
  const auto &DecMBB = MetaBuilder.getMBB();
  for (const auto &MI : DecMBB) {
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
                                  MachineIRBuilder &MetaBuilder) {
  std::map<std::string, Register> FuncNameToID;
  SmallVector<MachineInstr *, 8> FuncCalls;

  addExternalDeclarationsToIDMap(MetaBuilder, FuncNameToID);

  // Record all OpFunctionCalls, and all internal OpFunction declarations.
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpFunction) {
        FuncNameToID[F->getGlobalIdentifier()] = MI.defs().begin()->getReg();
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
    MachineIRBuilder MIRBuilder;
    MIRBuilder.setMF(*MF);
    MIRBuilder.setMBB(*FuncCall->getParent());
    MIRBuilder.setInstr(*FuncCall);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpFunctionCall)
                   .addDef(FuncCall->getOperand(0).getReg())
                   .addUse(FuncCall->getOperand(1).getReg())
                   .addUse(FuncID->second);
    const unsigned int NumOps = FuncCall->getNumOperands();
    for (unsigned int i = 3; i < NumOps; ++i) {
      MIB.addUse(FuncCall->getOperand(i).getReg());
    }

    FuncCall->removeFromParent();
  }
}

// Create global OpCapability instructions for the required capabilities
static void addGlobalRequirements(const SPIRVRequirementHandler &Reqs,
                                  const SPIRVSubtarget &ST,
                                  MachineIRBuilder &MIRBuilder) {

  // Abort here if not all requirements can be satisfied
  Reqs.checkSatisfiable(ST);
  setMetaBlock(MIRBuilder, MB_Capabilities);

  for (const auto &Cap : Reqs.getMinimalCapabilities()) {
    MIRBuilder.buildInstr(SPIRV::OpCapability).addImm(Cap);
  }

  // Generate the final OpExtensions with strings instead of enums
  for (const auto &Ext : Reqs.getExtensions()) {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtension);
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
                                   MachineIRBuilder &MIRBuilder,
                                   SPIRVRequirementHandler &Reqs,
                                   const SPIRVSubtarget &ST) {
  auto *DT = ST.getSPIRVDuplicatesTracker();
  auto *TR = ST.getSPIRVTypeRegistry();
  auto &MF = MIRBuilder.getMF();

  // Walk over each MF's DT and select all unreferenced global variables.
  SmallVector<GlobalVariable *, 8> GlobalVarList;
  const SPIRVDuplicatesTracker<GlobalValue> *GVDT = DT->get<GlobalValue>();
  auto Map = GVDT->getAllUses();
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E;) {
    GlobalVariable *GV = &*I++;
    auto AddrSpace = GV->getAddressSpace();
    auto Storage = TR->addressSpaceToStorageClass(AddrSpace);
    if (Storage != StorageClass::Function) {
      auto t = Map.find(GV);
      if (t == Map.end())
        // GV is not found in maps of all MFs so it's unreferenced.
        GlobalVarList.push_back(GV);
    }
  }

  // Walk over all unreferenced global variables and create required types,
  // constants, OpNames, OpVariables and OpDecorates.
  TR->setCurrentFunc(MF);
  setMetaBlock(MIRBuilder, MB_TmpGlobalData);

  // Walk over all created instructions and add requirement. Also convert all
  // G_CONSTANTs (TR creates them) to OpConstantI.
  setMetaBlock(MIRBuilder, MB_TmpGlobalData);
  auto &MBB = MIRBuilder.getMBB();
  SmallVector<MachineInstr *, 8> ToRemove;
  for (MachineInstr &MI : MBB) {
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
      SPIRVType *ResType = TR->getSPIRVTypeForVReg(Res);
      auto MIB = MIRBuilder.buildInstr(SPIRV::OpConstantI)
                     .addDef(Res)
                     .addUse(TR->getSPIRVTypeID(ResType))
                     .addImm(Imm.getZExtValue());
      TR->constrainRegOperands(MIB);
      ToRemove.push_back(&MI);
    }
  }

  for (auto *MI : ToRemove)
    MI->eraseFromParent();
}

// Add a meta function containing all OpType, OpConstant etc.
// Extract all OpType, OpConst etc. into this meta block
// Number registers globally, including references to global OpType etc.
bool SPIRVGlobalTypesAndRegNum::runOnModule(Module &M) {
  MachineModuleInfoWrapperPass &MMIWrapper =
      getAnalysis<MachineModuleInfoWrapperPass>();

  MachineIRBuilder MIRBuilder;
  initMetaBlockBuilder(M, MMIWrapper.getMMI(), MIRBuilder);

  const auto &MetaMF = MIRBuilder.getMF();
  const auto &ST = static_cast<const SPIRVSubtarget &>(MetaMF.getSubtarget());

  SPIRVRequirementHandler Reqs;

  addHeaderOps(M, MIRBuilder, Reqs, ST);

  // Process global variables which are not referenced in all functions.
  processGlobalUnrefVars(M, MMIWrapper.getMMI(), MIRBuilder, Reqs, ST);

  LocalAliasTablesTy AliasMaps;

  addOpExtInstImports(M, MMIWrapper.getMMI(), MIRBuilder, AliasMaps);

  // Extract type instructions to the top MetaMBB and keep track of which local
  // VRegs the correspond to with functionLocalAliasTables
  hoistInstrsToMetablock(M, MMIWrapper.getMMI(), MIRBuilder, AliasMaps, Reqs);

  // Number registers from 0 onwards, and fix references to global OpType etc
  numberRegistersGlobally(M, MMIWrapper.getMMI(), MIRBuilder, AliasMaps);

  // Extract instructions like OpName, OpEntryPoint, OpDecorate etc.
  // which all rely on globally numbered registers, which they forward-reference
  extractInstructionsWithGlobalRegsToMetablock(M, MMIWrapper.getMMI(),
                                               MIRBuilder);

  addEntryPointLinkageInterfaces(M, MMIWrapper.getMMI(), MIRBuilder);

  assignFunctionCallIDs(M, MMIWrapper.getMMI(), MIRBuilder);

  // If there are no entry points, we need the Linkage capability
  if (MIRBuilder.getMF().getBlockNumbered(MB_EntryPoints)->empty()) {
    Reqs.addCapability(Capability::Linkage);
  }

  addGlobalRequirements(Reqs, ST, MIRBuilder);

  // Check that all instructions have been moved from the TmpGlobalData block.
  setMetaBlock(MIRBuilder, MB_TmpGlobalData);
  auto &MBB = MIRBuilder.getMBB();
  assert(MBB.empty() && "TmpGlobalData block is expected to be empty");

  return false;
}

INITIALIZE_PASS(SPIRVGlobalTypesAndRegNum, DEBUG_TYPE,
                "SPIRV hoist OpType etc. & number VRegs globally", false, false)

char SPIRVGlobalTypesAndRegNum::ID = 0;

ModulePass *llvm::createSPIRVGlobalTypesAndRegNumPass() {
  return new SPIRVGlobalTypesAndRegNum();
}
