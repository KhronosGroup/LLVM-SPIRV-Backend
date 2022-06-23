add_llvm_component_group(SPIRV)

set(LLVM_TARGET_DEFINITIONS SPIRV.td)

tablegen(LLVM SPIRVGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM SPIRVGenGlobalISel.inc -gen-global-isel)
tablegen(LLVM SPIRVGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SPIRVGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM SPIRVGenRegisterBank.inc -gen-register-bank)
tablegen(LLVM SPIRVGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SPIRVGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM SPIRVGenTables.inc -gen-searchable-tables)

add_public_tablegen_target(SPIRVCommonTableGen)

add_llvm_target(SPIRVCodeGen
  SPIRVAsmPrinter.cpp
  SPIRVBasicBlockDominance.cpp
  SPIRVCallLowering.cpp
  SPIRVCapabilityUtils.cpp
  SPIRVDuplicatesTracker.cpp
  SPIRVEmitIntrinsics.cpp
  SPIRVEnumRequirements.cpp
  SPIRVExtInsts.cpp
  SPIRVGenerateDecorations.cpp
  SPIRVGlobalRegistry.cpp
  SPIRVInstrInfo.cpp
  SPIRVInstructionSelector.cpp
  SPIRVISelLowering.cpp
  SPIRVLegalizerInfo.cpp
  SPIRVLowerConstExpr.cpp
  SPIRVMCInstLower.cpp
  SPIRVModuleAnalysis.cpp
  SPIRVOCLRegularizer.cpp
  SPIRVBuiltins.cpp
  SPIRVOpenCLBIFs.cpp
  SPIRVPreLegalizer.cpp
  SPIRVRegisterBankInfo.cpp
  SPIRVRegisterInfo.cpp
  SPIRVSubtarget.cpp
  SPIRVTargetMachine.cpp
  SPIRVPreTranslationLegalizer.cpp
  SPIRVUtils.cpp

  LINK_COMPONENTS
  AsmPrinter
  Analysis
  CodeGen
  Core
  MC
  SPIRVDesc
  SPIRVInfo
  SelectionDAG
  Support
  Target
  TransformUtils
  GlobalISel
  Demangle

  ADD_TO_COMPONENT
  SPIRV
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
