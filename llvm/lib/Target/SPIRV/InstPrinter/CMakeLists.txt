add_llvm_component_library(LLVMSPIRVAsmPrinter
  SPIRVInstPrinter.cpp
  ../SPIRVEnums.cpp
  ../SPIRVExtInsts.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  SPIRV
  )