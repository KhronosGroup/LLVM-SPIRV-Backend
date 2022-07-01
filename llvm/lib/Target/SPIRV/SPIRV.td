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
include "SPIRVBuiltins.td"

def SPIRVInstrInfo : InstrInfo;

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic", []>;

def SPIRV10 : SubtargetFeature<"spirv1.0", "TargetSPIRVVersion", "10",
                             "Use SPIR-V version 1.0">;
def SPIRV11 : SubtargetFeature<"spirv1.1", "TargetSPIRVVersion", "11",
                             "Use SPIR-V version 1.1">;
def SPIRV12 : SubtargetFeature<"spirv1.2", "TargetSPIRVVersion", "12",
                             "Use SPIR-V version 1.2">;
def SPIRV13 : SubtargetFeature<"spirv1.3", "TargetSPIRVVersion", "13",
                             "Use SPIR-V version 1.3">;
def SPIRV14 : SubtargetFeature<"spirv1.4", "TargetSPIRVVersion", "14",
                             "Use SPIR-V version 1.4">;
def SPIRV15 : SubtargetFeature<"spirv1.5", "TargetSPIRVVersion", "15",
                             "Use SPIR-V version 1.5">;

def OCL12 : SubtargetFeature<"CL1.2", "TargetOpenCLVersion", "12",
                             "Use OpenCL version 1.2">;
def OCL20 : SubtargetFeature<"CL2.0", "TargetOpenCLVersion", "20",
                             "Use OpenCL version 2.0">;
def OCL22 : SubtargetFeature<"CL2.2", "TargetOpenCLVersion", "22",
                             "Use OpenCL version 2.2">;

def SPIRVInstPrinter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def SPIRV : Target {
  let InstructionSet = SPIRVInstrInfo;
  let AssemblyWriters = [SPIRVInstPrinter];
}
