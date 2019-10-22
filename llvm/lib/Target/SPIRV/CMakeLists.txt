set(LLVM_TARGET_DEFINITIONS SPIRV.td)

tablegen(LLVM SPIRVGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM SPIRVGenGlobalISel.inc -gen-global-isel)
tablegen(LLVM SPIRVGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SPIRVGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM SPIRVGenRegisterBank.inc -gen-register-bank)
tablegen(LLVM SPIRVGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SPIRVGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(SPIRVCommonTableGen)

add_llvm_target(SPIRVCodeGen
  SPIRVAddRequirementsPass.cpp
  SPIRVAsmPrinter.cpp
  SPIRVBasicBlockDominance.cpp
  SPIRVBlockLabeler.cpp
  SPIRVCallLowering.cpp
  SPIRVCapabilityUtils.cpp
  SPIRVEnums.cpp
  SPIRVEnumRequirements.cpp
  SPIRVExtInsts.cpp
  SPIRVGlobalTypesAndRegNumPass.cpp
  SPIRVInstrInfo.cpp
  SPIRVInstructionSelector.cpp
  SPIRVIRTranslator.cpp
  SPIRVLegalizerInfo.cpp
  SPIRVMCInstLower.cpp
  SPIRVOpenCLBIFs.cpp
  SPIRVRegisterBankInfo.cpp
  SPIRVRegisterInfo.cpp
  SPIRVStrings.cpp
  SPIRVSubtarget.cpp
  SPIRVTargetMachine.cpp
  SPIRVTypeRegistry.cpp
  )

add_subdirectory(InstPrinter)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
