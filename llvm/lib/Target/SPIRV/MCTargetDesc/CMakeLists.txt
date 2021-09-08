add_llvm_component_library(LLVMSPIRVDesc
  SPIRVMCTargetDesc.cpp
  SPIRVAsmBackend.cpp
  SPIRVMCCodeEmitter.cpp
  SPIRVObjectTargetWriter.cpp

  LINK_COMPONENTS
  MC 
  SPIRVAsmPrinter 
  SPIRVInfo 
  Support

  ADD_TO_COMPONENT
  SPIRV
  )