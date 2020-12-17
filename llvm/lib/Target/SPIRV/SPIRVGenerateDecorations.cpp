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

bool SPIRVGenerateDecorations::runOnMachineFunction(MachineFunction &MF) {
  MachineIRBuilder MIRBuilder(MF);
  for (auto &MBB : MF) {
    MIRBuilder.setMBB(MBB);
    for (auto &MI : MBB) {
      if (!canUseFastMathFlags(MI.getOpcode()))
        continue;
      MIRBuilder.setInstr(MI);
      auto fmFlags = getFastMathFlags(MI);
      if (fmFlags != FPFastMathMode::None) {
        // can be only a source of an ASSIGN_TYPE instr
        assert(MI.getMF()->getRegInfo().hasOneUse(MI.getOperand(0).getReg()));
        auto DstReg = MI.getMF()
                          ->getRegInfo()
                          .use_instr_begin(MI.getOperand(0).getReg())
                          ->getOperand(0)
                          .getReg();
        MIRBuilder.buildInstr(SPIRV::OpDecorate)
            .addUse(DstReg)
            .addImm(Decoration::FPFastMathMode)
            .addImm(fmFlags);
      }
    }
  }
  return true;
}

MachineFunctionPass *llvm::createSPIRVGenerateDecorationsPass() {
  return new SPIRVGenerateDecorations();
}
