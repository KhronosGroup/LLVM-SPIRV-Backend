//===-- SPIRV.td - Describe the SPIR-V Target Machine ------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

include "SPIRVRegisterInfo.td"
include "SPIRVRegisterBanks.td"
include "SPIRVInstrInfo.td"

def SPIRVInstrInfo : InstrInfo;

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic", []>;

def SPIRV10 : SubtargetFeature<"spirv1.0", "targetSPIRVVersion", "10",
                             "Use SPIR-V version 1.0">;
def SPIRV11 : SubtargetFeature<"spirv1.1", "targetSPIRVVersion", "11",
                             "Use SPIR-V version 1.1">;
def SPIRV12 : SubtargetFeature<"spirv1.2", "targetSPIRVVersion", "12",
                             "Use SPIR-V version 1.2">;
def SPIRV13 : SubtargetFeature<"spirv1.3", "targetSPIRVVersion", "13",
                             "Use SPIR-V version 1.3">;

def OCL12 : SubtargetFeature<"CL1.2", "targetOpenCLVersion", "12",
                             "Use OpenCL version 1.2">;
def OCL20 : SubtargetFeature<"CL2.0", "targetOpenCLVersion", "20",
                             "Use OpenCL version 2.0">;
def OCL22 : SubtargetFeature<"CL2.2", "targetOpenCLVersion", "22",
                             "Use OpenCL version 2.2">;

def SPIRVInstPrinter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def SPIRV : Target {
  let InstructionSet = SPIRVInstrInfo;
  let AssemblyWriters = [SPIRVInstPrinter];
}


