#include "SPIRV.h"
#include "SPIRVSubtarget.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

using namespace llvm;

namespace llvm {
void initializeSPIRVGenerateDecorationsPass(PassRegistry &);
}

namespace {

struct SPIRVGenerateDecorations : public MachineFunctionPass {
public:
  static char ID;
  SPIRVGenerateDecorations() : MachineFunctionPass(ID) {
    initializeSPIRVGenerateDecorationsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return "SPIRV Decorations Generation";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
  }

private:
  void replaceMachineInstructionUsage(MachineFunction &MF, MachineInstr &MI);

  void replaceRegisterUsage(MachineInstr &Instr, MachineOperand &From,
                            MachineOperand &To);
};

} // namespace

char SPIRVGenerateDecorations::ID = 0;

INITIALIZE_PASS(SPIRVGenerateDecorations, "spirv-decor-generation",
                "SPIRV Decorations Generation", false, false)

static bool canUseFastMathFlags(unsigned opCode) {
  switch (opCode) {
  // TODO: this should be a property of SPIRV opcode but not a gMIR one
  case TargetOpcode::G_FADD:
  case TargetOpcode::G_FSUB:
  case TargetOpcode::G_FMUL:
  case TargetOpcode::G_FDIV:
  case TargetOpcode::G_FREM:
    return true;
  default:
    return false;
  }
}

static uint32_t getFastMathFlags(const MachineInstr &I) {
  uint32_t flags = FPFastMathMode::None;
  if (I.getFlag(MachineInstr::MIFlag::FmNoNans))
    flags |= FPFastMathMode::NotNaN;
  if (I.getFlag(MachineInstr::MIFlag::FmNoInfs))
    flags |= FPFastMathMode::NotInf;
  if (I.getFlag(MachineInstr::MIFlag::FmNsz))
    flags |= FPFastMathMode::NSZ;
  if (I.getFlag(MachineInstr::MIFlag::FmArcp))
    flags |= FPFastMathMode::AllowRecip;
  if (I.getFlag(MachineInstr::MIFlag::FmReassoc))
    flags |= FPFastMathMode::Fast;
  return flags;
}

// Defined in SPIRVLegalizerInfo.cpp
extern bool isTypeFoldingSupported(unsigned Opcode);

bool SPIRVGenerateDecorations::runOnMachineFunction(MachineFunction &MF) {
  MachineIRBuilder MIRBuilder(MF);
  auto &MRI = MF.getRegInfo();
  for (auto &MBB : MF) {
    MIRBuilder.setMBB(MBB);
    for (auto &MI : MBB) {
      if (!canUseFastMathFlags(MI.getOpcode()))
        continue;
      MIRBuilder.setInstr(MI);
      auto FMFlags = getFastMathFlags(MI);
      if (FMFlags != FPFastMathMode::None) {
        Register DstReg = MI.getOperand(0).getReg();
        // can be only a source of an ASSIGN_TYPE instr
        assert(MRI.hasOneUse(DstReg) && "One use is expected");
        Register NewReg = MRI.use_instr_begin(DstReg)->getOperand(0).getReg();
        MIRBuilder.buildInstr(SPIRV::OpDecorate)
            .addUse(NewReg)
            .addImm(Decoration::FPFastMathMode)
            .addImm(FMFlags);
      }
    }
  }

  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      // We need to rewrite dst types for ASSIGN_TYPE instrs to be able
      // to perform tblgen'erated selection and we can't do that on Legalizer
      // as it operates on gMIR only.
      if (MI.getOpcode() == SPIRV::ASSIGN_TYPE) {
        Register SrcReg = MI.getOperand(1).getReg();
        if (isTypeFoldingSupported(MRI.getVRegDef(SrcReg)->getOpcode())) {
          Register DstReg = MI.getOperand(0).getReg();
          if (MRI.getType(DstReg).isVector())
            MRI.setRegClass(DstReg, &SPIRV::IDRegClass);
          MRI.setType(DstReg, LLT::scalar(32));
        }
      }
    }
  }
  return true;
}

MachineFunctionPass *llvm::createSPIRVGenerateDecorationsPass() {
  return new SPIRVGenerateDecorations();
}
