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
static unsigned int getMetadataUInt(MDNode *mdNode, unsigned int opIndex,
                                    unsigned int defaultVal = 0) {
  if (mdNode && opIndex < mdNode->getNumOperands()) {
    const auto &op = mdNode->getOperand(opIndex);
    return mdconst::extract<ConstantInt>(op)->getZExtValue();
  } else {
    return defaultVal;
  }
}

// Add some initial header instructions such as OpSource and OpMemoryModel
// TODO Maybe move the environment-specific logic used here elsewhere.
static void addHeaderOps(Module &M, MachineIRBuilder &MIRBuilder,
                         SPIRVRequirementHandler &reqs,
                         const SPIRVSubtarget &ST) {
  setMetaBlock(MIRBuilder, MB_MemoryModel);
  unsigned ptrSize = ST.getPointerSize();

  // Add OpMemoryModel
  using namespace AddressingModel;
  auto addr = ptrSize == 32 ? Physical32 : ptrSize == 64 ? Physical64 : Logical;
  auto mem = MemoryModel::OpenCL;
  MIRBuilder.buildInstr(SPIRV::OpMemoryModel).addImm(addr).addImm(mem);

  // Update required capabilities for this memory model
  reqs.addRequirements(getMemoryModelRequirements(mem, ST));
  reqs.addRequirements(getAddressingModelRequirements(addr, ST));

  // Get the OpenCL version number from metadata
  unsigned openCLVersion = 0;
  if (auto verNode = M.getNamedMetadata("opencl.ocl.version")) {
    // Construct version literal according to OpenCL 2.2 environment spec.
    auto versionMD = verNode->getOperand(0);
    unsigned majorNum = getMetadataUInt(versionMD, 0, 2);
    unsigned minorNum = getMetadataUInt(versionMD, 1);
    unsigned revNum = getMetadataUInt(versionMD, 2);
    openCLVersion = 0 | (majorNum << 16) | (minorNum << 8) | revNum;
    setMetaBlock(MIRBuilder, MB_DebugSourceAndStrings);
  }

  // Build the OpSource
  auto srcLang = SourceLanguage::OpenCL_C;
  MIRBuilder.buildInstr(SPIRV::OpSource).addImm(srcLang).addImm(openCLVersion);
  reqs.addRequirements(getSourceLanguageRequirements(srcLang, ST));
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
  Function *f = Function::Create(FType, Linkage, "GlobalStuff");

  // Add the function and a corresponding MachineFunction to the module
  f->getBasicBlockList().push_back(BasicBlock::Create(M.getContext()));
  M.getFunctionList().push_front(f);

  // Add the necessary basic blocks according to the MetaBlockTypes enum.
  MachineFunction &MetaMF = MMI.getOrCreateMachineFunction(*f);
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
                           MetaBlockType MBType,
                           LocalAliasTablesTy &LocalAliasTables) {
  // FIXME: setMetaBlock
  setMetaBlock(MetaBuilder, MBType);

  for (auto &CU : DT->getAllUses()) {
    auto MetaReg =
        MetaBuilder.getMRI()->createVirtualRegister(&SPIRV::IDRegClass);

    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      LocalAliasTables[MF][Reg] = MetaReg;
    }
  }
}

template <typename T>
static void hoistGlobalOps(MachineIRBuilder &MetaBuilder,
                           const SPIRVDuplicatesTracker<T> *DT,
                           MetaBlockType MBType,
                           LocalAliasTablesTy &LocalAliasTables) {
  setMetaBlock(MetaBuilder, MBType);
  SmallVector<MachineInstr *, 8> ToRemove;
  for (auto &CU : DT->getAllUses()) {
    for (auto &U : CU.second) {
      auto *MF = U.first;
      auto Reg = U.second;
      assert(LocalAliasTables[MF].find(Reg) != LocalAliasTables[MF].end() &&
             "Cannot find reg alias in LocalAliasTables");
      auto MetaReg = LocalAliasTables[MF][Reg];

      auto *ToHoist = MF->getRegInfo().getVRegDef(Reg);
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
            auto metaRegID = LocalAliasTables[MF].find(Op.getReg());
            if (metaRegID != LocalAliasTables[MF].end()) {
              Register metaReg = metaRegID->second;
              assert(metaReg.isValid() && "No reg alias found");
              MIB.addUse(metaReg);
            } else {
              llvm_unreachable("Generate WRONG type!\n");
            }
          } else {
            errs() << *ToHoist << "\n";
            llvm_unreachable(
                "Unexpected operand type when copying spirv meta instr");
          }
        }
      }
    }
  }

  for (auto *MI : ToRemove)
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
                           MetaBlockType MBType,
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
              Register metaReg = LocalAliasTables[MF].at(Op.getReg());
              assert(metaReg.isValid() && "No reg alias found");
              MIB.addUse(metaReg);
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
                                   SPIRVRequirementHandler &reqs) {
  const auto *ST = static_cast<const SPIRVSubtarget *>(&MIRBuilder.getMF().getSubtarget());
  auto *DT = ST->getSPIRVDuplicatesTracker();

  // OpTypes and OpConstants can have cross references so at first we create
  // new meta regs and map them to old regs, then walk the list of instructions,
  // and create new (hoisted) instructions with new meta regs instead of old ones.
  fillLocalAliasTables<Type>(MIRBuilder, DT->get<Type>(), MB_TypeConstVars, LocalAliasTables);
  fillLocalAliasTables<Constant>(MIRBuilder, DT->get<Constant>(), MB_TypeConstVars, LocalAliasTables);
  hoistGlobalOps<Type>(MIRBuilder, DT->get<Type>(), MB_TypeConstVars, LocalAliasTables);
  hoistGlobalOps<Constant>(MIRBuilder, DT->get<Constant>(), MB_TypeConstVars, LocalAliasTables);

  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)

  // Iterate through and hoist any instructions we can at this stage.
  // bool hasSeenFirstOpFunction = false;
  SmallVector<MachineInstr *, 16> toRemove;
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpExtension) {
        // Here, OpExtension just has a single enum operand, not a string
        auto ext = Extension::Extension(MI.getOperand(0).getImm());
        reqs.addExtension(ext);
        toRemove.push_back(&MI);
      } else if (MI.getOpcode() == SPIRV::OpCapability) {
        auto cap = Capability::Capability(MI.getOperand(0).getImm());
        reqs.addCapability(cap);
        toRemove.push_back(&MI);
      }
    }
  }
  for (MachineInstr *MI : toRemove) {
    MI->removeFromParent();
  }
  END_FOR_MF_IN_MODULE()
  fillLocalAliasTables<GlobalValue>(MIRBuilder, DT->get<GlobalValue>(), MB_TypeConstVars, LocalAliasTables);
  hoistGlobalOps<GlobalValue>(MIRBuilder, DT->get<GlobalValue>(), MB_TypeConstVars, LocalAliasTables);
  fillLocalAliasTables<Function>(MIRBuilder, DT->get<Function>(), MB_ExtFuncDecls, LocalAliasTables);
  hoistGlobalOps<Function>(MIRBuilder, DT->get<Function>(), MB_ExtFuncDecls, LocalAliasTables);
}

// True when all the operands of an instruction are an exact match (after the
// given starting index).
static bool allOpsMatch(const MachineInstr &A, const MachineInstr &B,
                        unsigned int startOpIndex = 0) {
  const unsigned int numAOps = A.getNumOperands();
  if (numAOps == B.getNumOperands() && A.getNumDefs() == B.getNumDefs()) {
    bool allOpsMatch = true;
    for (unsigned i = startOpIndex; i < numAOps && allOpsMatch; ++i) {
      allOpsMatch = A.getOperand(i).isIdenticalTo(B.getOperand(i));
    }
    if (allOpsMatch)
      return true;
  }
  return false;
}

static void addOpExtInstImports(Module &M, MachineModuleInfo &MMI,
                                MachineIRBuilder &MIRBuilder,
                                LocalAliasTablesTy &LocalAliasTables) {

  std::set<ExtInstSet> usedExtInstSets = {ExtInstSet::OpenCL_std};
  SmallVector<std::pair<MachineInstr *, LocalToGlobalRegTable *>, 8>
      extInstInstrs;

  // Record all OpExtInst instuctions and the instruction sets they use
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpExtInst) {
        auto set = static_cast<ExtInstSet>(MI.getOperand(2).getImm());
        usedExtInstSets.insert(set);
        extInstInstrs.push_back({&MI, &LocalAliasTables[MF]});
      }
    }
  }
  END_FOR_MF_IN_MODULE()

  std::map<ExtInstSet, Register> setEnumToGlobalIDReg;

  setMetaBlock(MIRBuilder, MB_ExtInstImports);
  auto MetaMRI = MIRBuilder.getMRI();
  for (const auto set : usedExtInstSets) {
    auto setReg = MetaMRI->createVirtualRegister(&SPIRV::IDRegClass);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInstImport).addDef(setReg);
    addStringImm(getExtInstSetName(set), MIB);
    setEnumToGlobalIDReg.insert({set, setReg});
  }

  // Replace all OpFunctionCalls with new ones referring to funcID vregs
  for (const auto &entry : extInstInstrs) {
    auto *MI = entry.first;
    auto *aliasTable = entry.second;

    auto extInstSet = static_cast<ExtInstSet>(MI->getOperand(2).getImm());

    MachineIRBuilder MIRBuilder;
    MIRBuilder.setMF(*MI->getMF());
    auto MRI = MIRBuilder.getMRI();

    // Ensure the mapping between local and global vregs is maintained for the
    // later reg numbering phase
    auto localReg = MRI->createVirtualRegister(&SPIRV::IDRegClass);
    auto globalReg = setEnumToGlobalIDReg[extInstSet];
    aliasTable->insert({localReg, globalReg});

    // Create a new copy of the OpExtInst but with the IDVReg for the imported
    // instruction set rather than an enum, then delete the old instruction.
    MIRBuilder.setMBB(*MI->getParent());
    MIRBuilder.setInstr(*MI);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtInst)
                   .addDef(MI->getOperand(0).getReg())
                   .addUse(MI->getOperand(1).getReg())
                   .addUse(localReg);
    const unsigned int numOps = MI->getNumOperands();
    for (unsigned int i = 3; i < numOps; ++i) {
      MIB.add(MI->getOperand(i));
    }

    MI->removeFromParent();
  }
}

// We need to add dummy registers whenever we need to reference a VReg that
// might be beyond the number of virtual registers declared in the function so
// far. This stops addUse and setReg from crashing when we use the new index.
static void addDummyVRegsUpToIndex(unsigned index, MachineRegisterInfo &MRI) {
  while (index >= MRI.getNumVirtRegs()) {
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
                                         MetaBlockType mbType) {

  setMetaBlock(MIRBuilder, mbType);
  auto &MBB = MIRBuilder.getMBB();
  for (const auto &I : MBB) {
    if (allOpsMatch(MI, I))
      return; // Found a duplicate, so don't add it
  }

  // No duplicates, so add it
  auto &MetaMRI = MIRBuilder.getMF().getRegInfo();
  const unsigned numOperands = MI.getNumOperands();
  auto MIB = MIRBuilder.buildInstr(MI.getOpcode());
  for (unsigned i = 0; i < numOperands; ++i) {
    MachineOperand op = MI.getOperand(i);
    if (op.isImm()) {
      MIB.addImm(op.getImm());
    } else if (op.isReg()) {
      // Add dummy regs to stop addUse crashing if Reg > max regs in func so far
      addDummyVRegsUpToIndex(op.getReg().virtRegIndex(), MetaMRI);
      MIB.addUse(op.getReg());
    } else {
      errs() << MI << "\n";
      llvm_unreachable("Unexpected operand type when copying spirv meta instr");
    }
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
  for (MachineBasicBlock &MBB : *MF) {
    SmallVector<MachineInstr *, 16> toRemove;
    for (MachineInstr &MI : MBB) {
      const unsigned OpCode = MI.getOpcode();
      if (OpCode == SPIRV::OpName || OpCode == SPIRV::OpMemberName) {
        hoistMetaInstrWithGlobalRegs(MI, MIRBuilder, MB_DebugNames);
        toRemove.push_back(&MI);
      } else if (OpCode == SPIRV::OpEntryPoint) {
        hoistMetaInstrWithGlobalRegs(MI, MIRBuilder, MB_EntryPoints);
        toRemove.push_back(&MI);
      } else if (TII->isDecorationInstr(MI)) {
        hoistMetaInstrWithGlobalRegs(MI, MIRBuilder, MB_Annotations);
        toRemove.push_back(&MI);
      }
    }
    for (MachineInstr *MI : toRemove) {
      MI->removeFromParent();
    }
  }
  END_FOR_MF_IN_MODULE()
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
  auto &decMBB = MIRBuilder.getMBB();
  SmallVector<Register, 4> inputLinkedIDs;
  for (MachineInstr &MI : decMBB) {
    const unsigned OpCode = MI.getOpcode();
    const unsigned numOps = MI.getNumOperands();
    if (OpCode == SPIRV::OpDecorate &&
        MI.getOperand(1).getImm() == Decoration::LinkageAttributes &&
        MI.getOperand(numOps - 1).getImm() == LinkageType::Import) {
      const Register target = MI.getOperand(0).getReg();
      inputLinkedIDs.push_back(target);
    }
  }

  // Add any Import linked IDs as interface args to all OpEntryPoints
  if (!inputLinkedIDs.empty()) {
    setMetaBlock(MIRBuilder, MB_EntryPoints);
    auto &entryMBB = MIRBuilder.getMBB();
    for (MachineInstr &MI : entryMBB) {
      for (unsigned id : inputLinkedIDs) {
        MI.addOperand(MachineOperand::CreateReg(id, false));
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
                                    LocalAliasTablesTy &regAliasTables) {

  // Use raw index 0 - inf, and convert with index2VirtReg later
  unsigned int RegBaseIndex = 0;
  BEGIN_FOR_MF_IN_MODULE(M, MMI)
  if (MFIndex == 0) {
    RegBaseIndex = MIRBuilder.getMF().getRegInfo().getNumVirtRegs();
  } else {
    auto &MRI = MF->getRegInfo();
    auto localToMetaVRegAliasMap = regAliasTables[MF];
    for (MachineBasicBlock &MBB : *MF) {
      for (MachineInstr &MI : MBB) {
        for (MachineOperand &op : MI.operands()) {
          if (op.isReg()) {
            Register newReg;
            auto VR = localToMetaVRegAliasMap.find(op.getReg());
            if (VR == localToMetaVRegAliasMap.end()) {
              // Stops setReg crashing if reg index > max regs in func
              addDummyVRegsUpToIndex(RegBaseIndex, MRI);

              newReg = Register::index2VirtReg(RegBaseIndex);
              ++RegBaseIndex;
              localToMetaVRegAliasMap.insert({op.getReg(), newReg});
            } else {
              newReg = VR->second;
            }
            op.setReg(newReg);
          }
        }
      }
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
                                           FuncNameToIDMap &funcNameToOpID) {

  // Only look for OpDecorates in the global meta function
  setMetaBlock(MetaBuilder, MB_Annotations);
  const auto &DecMBB = MetaBuilder.getMBB();
  for (const auto &MI : DecMBB) {
    if (MI.getOpcode() == SPIRV::OpDecorate) {
      // If it's got Import linkage
      auto dec = MI.getOperand(1).getImm();
      if (dec == Decoration::LinkageAttributes) {
        auto lnk = MI.getOperand(MI.getNumOperands() - 1).getImm();
        if (lnk == LinkageType::Import) {
          // Map imported function name to function ID VReg.
          std::string name = getStringImm(MI, 2);
          Register target = MI.getOperand(0).getReg();
          funcNameToOpID[name] = target;
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
  std::map<std::string, Register> funcNameToID;
  SmallVector<MachineInstr *, 8> funcCalls;

  addExternalDeclarationsToIDMap(MetaBuilder, funcNameToID);

  // Record all OpFunctionCalls, and all internal OpFunction declarations.
  BEGIN_FOR_MF_IN_MODULE_EXCEPT_FIRST(M, MMI)
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SPIRV::OpFunction) {
        funcNameToID[F->getGlobalIdentifier()] = MI.defs().begin()->getReg();
      } else if (MI.getOpcode() == SPIRV::OpFunctionCall) {
        funcCalls.push_back(&MI);
      }
    }
  }
  END_FOR_MF_IN_MODULE()

  // Replace all OpFunctionCalls with new ones referring to funcID vregs
  for (const auto funcCall : funcCalls) {
    auto funcName = funcCall->getOperand(2).getGlobal()->getGlobalIdentifier();

    auto funcID = funcNameToID.find(funcName);
    if (funcID == funcNameToID.end()) {
      errs() << "Unknown function: " << funcName << "\n";
      llvm_unreachable("Error: Could not find function id");
    }
    const auto MF = funcCall->getMF();

    // Stops addUse crashing if funcID > max regs in func so far
    addDummyVRegsUpToIndex(funcID->second.virtRegIndex(), MF->getRegInfo());

    // Create a new copy of the OpFunctionCall but with the IDVReg for the
    // callee rather than a GlobalValue, then delete the old instruction.
    MachineIRBuilder MIRBuilder;
    MIRBuilder.setMF(*MF);
    MIRBuilder.setMBB(*funcCall->getParent());
    MIRBuilder.setInstr(*funcCall);
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpFunctionCall)
                   .addDef(funcCall->getOperand(0).getReg())
                   .addUse(funcCall->getOperand(1).getReg())
                   .addUse(funcID->second);
    const unsigned int numOps = funcCall->getNumOperands();
    for (unsigned int i = 3; i < numOps; ++i) {
      MIB.addUse(funcCall->getOperand(i).getReg());
    }

    funcCall->removeFromParent();
  }
}

// Create global OpCapability instructions for the required capabilities
static void addGlobalRequirements(const SPIRVRequirementHandler &reqs,
                                  const SPIRVSubtarget &ST,
                                  MachineIRBuilder &MIRBuilder) {

  // Abort here if not all requirements can be satisfied
  reqs.checkSatisfiable(ST);
  setMetaBlock(MIRBuilder, MB_Capabilities);

  for (const auto &cap : *reqs.getMinimalCapabilities()) {
    MIRBuilder.buildInstr(SPIRV::OpCapability).addImm(cap);
  }

  // Generate the final OpExtensions with strings instead of enums
  for (const auto &ext : *reqs.getExtensions()) {
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpExtension);
    addStringImm(getExtensionName(ext), MIB);
  }

  // TODO add a pseudo instr for version number
}

// Add a meta function containing all OpType, OpConstant etc.
// Extract all OpType, OpConst etc. into this meta block
// Number registers globally, including references to global OpType etc.
bool SPIRVGlobalTypesAndRegNum::runOnModule(Module &M) {
  MachineModuleInfoWrapperPass &MMIWrapper = getAnalysis<MachineModuleInfoWrapperPass>();

  MachineIRBuilder MIRBuilder;
  initMetaBlockBuilder(M, MMIWrapper.getMMI(), MIRBuilder);

  const auto &metaMF = MIRBuilder.getMF();
  const auto &ST = static_cast<const SPIRVSubtarget &>(metaMF.getSubtarget());

  SPIRVRequirementHandler reqs;

  addHeaderOps(M, MIRBuilder, reqs, ST);

  LocalAliasTablesTy AliasMaps;

  addOpExtInstImports(M, MMIWrapper.getMMI(), MIRBuilder, AliasMaps);

  // Extract type instructions to the top MetaMBB and keep track of which local
  // VRegs the correspond to with functionLocalAliasTables
  hoistInstrsToMetablock(M, MMIWrapper.getMMI(), MIRBuilder, AliasMaps, reqs);

  // Number registers from 0 onwards, and fix references to global OpType etc
  numberRegistersGlobally(M, MMIWrapper.getMMI(), MIRBuilder, AliasMaps);

  // Extract instructions like OpName, OpEntryPoint, OpDecorate etc.
  // which all rely on globally numbered registers, which they forward-reference
  extractInstructionsWithGlobalRegsToMetablock(M, MMIWrapper.getMMI(), MIRBuilder);

  addEntryPointLinkageInterfaces(M, MMIWrapper.getMMI(), MIRBuilder);

  assignFunctionCallIDs(M, MMIWrapper.getMMI(), MIRBuilder);

  // If there are no entry points, we need the Linkage capability
  if (MIRBuilder.getMF().getBlockNumbered(MB_EntryPoints)->empty()) {
    reqs.addCapability(Capability::Linkage);
  }

  addGlobalRequirements(reqs, ST, MIRBuilder);

  return false;
}

INITIALIZE_PASS(SPIRVGlobalTypesAndRegNum, DEBUG_TYPE,
                "SPIRV hoist OpType etc. & number VRegs globally", false, false)

char SPIRVGlobalTypesAndRegNum::ID = 0;

ModulePass *llvm::createSPIRVGlobalTypesAndRegNumPass() {
  return new SPIRVGlobalTypesAndRegNum();
}
