//===-- SPIRVInstPrinter.cpp - Output SPIR-V MCInsts as ASM -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class prints a SPIR-V MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "SPIRVInstPrinter.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

#include "SPIRV.h"
#include "SPIRVEnums.h"
#include "SPIRVExtInsts.h"
#include "SPIRVStringReader.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// Include the auto-generated portion of the assembly writer.
#include "SPIRVGenAsmWriter.inc"

void SPIRVInstPrinter::printRemainingVariableOps(const MCInst *MI,
                                                 unsigned startIndex,
                                                 raw_ostream &O,
                                                 bool skipFirstSpace,
                                                 bool skipImmediates) {
  const unsigned int numOps = MI->getNumOperands();
  for (unsigned int i = startIndex; i < numOps; ++i) {
    if (!skipImmediates || !MI->getOperand(i).isImm()) {
      if (!skipFirstSpace || i != startIndex) {
        O << ' ';
      }
      printOperand(MI, i, O);
    }
  }
}

void SPIRVInstPrinter::printOpConstantVarOps(const MCInst *MI,
                                             unsigned startIndex,
                                             raw_ostream &O) {
  O << ' ';
  if (MI->getNumOperands() - startIndex == 2) { // Handle 64 bit literals
    uint64_t imm = MI->getOperand(startIndex).getImm();
    imm |= (MI->getOperand(startIndex + 1).getImm() << 32);
    O << imm;
  } else {
    printRemainingVariableOps(MI, startIndex, O, true, false);
  }
}

void SPIRVInstPrinter::recordOpExtInstImport(const MCInst *MI) {
  Register reg = MI->getOperand(0).getReg();
  auto name = getSPIRVStringOperand(*MI, 1);
  auto set = getExtInstSetFromString(name);
  extInstSetIDs.insert({reg, set});
}

void SPIRVInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                                 StringRef Annot, const MCSubtargetInfo &STI) {

  const unsigned int OpCode = MI->getOpcode();
  printInstruction(MI, O);

  if (OpCode == SPIRV::OpDecorate) {
    printOpDecorate(MI, O);
  } else if (OpCode == SPIRV::OpExtInstImport) {
    recordOpExtInstImport(MI);
  } else if (OpCode == SPIRV::OpExtInst) {
    printOpExtInst(MI, O);
  } else {

    // Print any extra operands for variadic instructions
    MCInstrDesc MCDesc = MII.get(OpCode);
    if (MCDesc.isVariadic()) {
      const unsigned numFixedOps = MCDesc.getNumOperands();
      const unsigned lastFixedIndex = numFixedOps - 1;
      const int firstVariableIndex = numFixedOps;
      if (numFixedOps > 0 &&
          MCDesc.OpInfo[lastFixedIndex].OperandType == MCOI::OPERAND_UNKNOWN) {
        // For instructions where a custom type (not reg or immediate) comes as
        // the last operand before the variable_ops. This is usually a StringImm
        // operand, but there are a few other cases
        switch (OpCode) {
        case SPIRV::OpTypeImage:
          O << ' ';
          printAccessQualifier(MI, firstVariableIndex, O);
          break;
        case SPIRV::OpVariable:
          O << ' ';
          printOperand(MI, firstVariableIndex, O);
          break;
        case SPIRV::OpEntryPoint: {
          // Print the interface ID operands, skipping the name's string literal
          printRemainingVariableOps(MI, numFixedOps, O, false, true);
          break;
        }
        case SPIRV::OpExecutionMode:
        case SPIRV::OpExecutionModeId:
        case SPIRV::OpLoopMerge: {
          // Print any literals after the OPERAND_UNKNOWN argument normally
          printRemainingVariableOps(MI, numFixedOps, O);
          break;
        }
        default:
          break; // printStringImm has already been handled
        }
      } else {
        // For instructions with no fixed ops or a reg/immediate as the final
        // fixed operand, we can usually print the rest with "printOperand", but
        // check for a few cases with custom types first
        switch (OpCode) {
        case SPIRV::OpLoad:
        case SPIRV::OpStore:
          O << ' ';
          printMemoryOperand(MI, firstVariableIndex, O);
          printRemainingVariableOps(MI, firstVariableIndex + 1, O);
          break;
        case SPIRV::OpImageSampleImplicitLod:
        case SPIRV::OpImageSampleDrefImplicitLod:
        case SPIRV::OpImageSampleProjImplicitLod:
        case SPIRV::OpImageSampleProjDrefImplicitLod:
        case SPIRV::OpImageFetch:
        case SPIRV::OpImageGather:
        case SPIRV::OpImageDrefGather:
        case SPIRV::OpImageRead:
        case SPIRV::OpImageWrite:
        case SPIRV::OpImageSparseSampleImplicitLod:
        case SPIRV::OpImageSparseSampleDrefImplicitLod:
        case SPIRV::OpImageSparseSampleProjImplicitLod:
        case SPIRV::OpImageSparseSampleProjDrefImplicitLod:
        case SPIRV::OpImageSparseFetch:
        case SPIRV::OpImageSparseGather:
        case SPIRV::OpImageSparseDrefGather:
        case SPIRV::OpImageSparseRead:
        case SPIRV::OpImageSampleFootprintNV:
          printImageOperand(MI, firstVariableIndex, O);
          printRemainingVariableOps(MI, numFixedOps + 1, O);
          break;
        case SPIRV::OpCopyMemory:
        case SPIRV::OpCopyMemorySized: {
          const unsigned int numOps = MI->getNumOperands();
          for (unsigned int i = numFixedOps; i < numOps; ++i) {
            O << ' ';
            printMemoryOperand(MI, i, O);
            if (MI->getOperand(i).getImm() & MemoryOperand::Aligned) {
              assert(i + 1 < numOps && "Missing alignment operand");
              O << ' ';
              printOperand(MI, i + 1, O);
              i += 1;
            }
          }
          break;
        }
        case SPIRV::OpConstant:
          printOpConstantVarOps(MI, numFixedOps, O);
          break;
        default:
          printRemainingVariableOps(MI, numFixedOps, O);
          break;
        }
      }
    }
  }

  printAnnotation(O, Annot);
}

void SPIRVInstPrinter::printOpExtInst(const MCInst *MI, raw_ostream &O) {
  // The fixed operands have already been printed, so just need to decide what
  // type of ExtInst operands to print based on the instruction set and number
  MCInstrDesc MCDesc = MII.get(MI->getOpcode());
  unsigned int numFixedOps = MCDesc.getNumOperands();
  const auto numOps = MI->getNumOperands();
  if (numOps == numFixedOps)
    return;

  O << ' ';

  auto setReg = MI->getOperand(2).getReg();
  auto set = extInstSetIDs[setReg];
  if (set == ExtInstSet::OpenCL_std) {
    auto inst = static_cast<OpenCL_std::OpenCL_std>(MI->getOperand(3).getImm());
    switch (inst) {
    case OpenCL_std::vstore_half_r:
    case OpenCL_std::vstore_halfn_r:
    case OpenCL_std::vstorea_halfn_r: {
      // These ops have a literal FPRoundingMode as the last arg
      for (unsigned int i = numFixedOps; i < numOps - 1; ++i) {
        printOperand(MI, i, O);
        O << ' ';
      }
      printFPRoundingMode(MI, numOps - 1, O);
      break;
    }
    default:
      printRemainingVariableOps(MI, numFixedOps, O, true);
    }
  } else {
    printRemainingVariableOps(MI, numFixedOps, O, true);
  }
}

void SPIRVInstPrinter::printOpDecorate(const MCInst *MI, raw_ostream &O) {
  // The fixed operands have already been printed, so just need to decide what
  // type of decoration operands to print based on the Decoration type
  MCInstrDesc MCDesc = MII.get(MI->getOpcode());
  unsigned int numFixedOps = MCDesc.getNumOperands();

  if (numFixedOps != MI->getNumOperands()) {
    auto decOp = MI->getOperand(numFixedOps - 1);
    auto dec = static_cast<Decoration::Decoration>(decOp.getImm());

    O << ' ';

    switch (dec) {
    case Decoration::BuiltIn:
      printBuiltIn(MI, numFixedOps, O);
      break;
    case Decoration::UniformId:
      printScope(MI, numFixedOps, O);
      break;
    case Decoration::FuncParamAttr:
      printFunctionParameterAttribute(MI, numFixedOps, O);
      break;
    case Decoration::FPRoundingMode:
      printFPRoundingMode(MI, numFixedOps, O);
      break;
    case Decoration::FPFastMathMode:
      printFPFastMathMode(MI, numFixedOps, O);
      break;
    case Decoration::LinkageAttributes:
      printStringImm(MI, numFixedOps, O);
      break;
    default:
      printRemainingVariableOps(MI, numFixedOps, O, true);
      break;
    }
  }
}

static void printExpr(const MCExpr *Expr, raw_ostream &O) {
#ifndef NDEBUG
  const MCSymbolRefExpr *SRE;

  if (const MCBinaryExpr *BE = dyn_cast<MCBinaryExpr>(Expr))
    SRE = dyn_cast<MCSymbolRefExpr>(BE->getLHS());
  else
    SRE = dyn_cast<MCSymbolRefExpr>(Expr);
  assert(SRE && "Unexpected MCExpr type.");

  MCSymbolRefExpr::VariantKind Kind = SRE->getKind();

  assert(Kind == MCSymbolRefExpr::VK_None);
#endif
  O << *Expr;
}

void SPIRVInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &O, const char *Modifier) {
  assert((Modifier == 0 || Modifier[0] == 0) && "No modifiers supported");
  if (OpNo < MI->getNumOperands()) {
    const MCOperand &Op = MI->getOperand(OpNo);
    if (Op.isReg()) {
      O << "%" << (Register::virtReg2Index(Op.getReg()) + 1);
    } else if (Op.isImm()) {
      O << formatImm((int32_t)Op.getImm());
    } else {
      assert(Op.isExpr() && "Expected an expression");
      printExpr(Op.getExpr(), O);
    }
  }
}

void SPIRVInstPrinter::printStringImm(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &O) {
  const unsigned numOps = MI->getNumOperands();
  unsigned strStartIndex = OpNo;
  while (strStartIndex < numOps) {
    if (MI->getOperand(strStartIndex).isReg())
      break;

    std::string str = getSPIRVStringOperand(*MI, OpNo);
    if (strStartIndex != OpNo)
      O << ' '; // Add a space if we're starting a new string/argument
    O << '"';
    for (char c : str) {
      if (c == '"')
        O.write('\\'); // Escape " characters (might break for complex UTF-8)
      O.write(c);
    }
    O << '"';

    unsigned numOpsInString = (str.size() / 4) + 1;
    strStartIndex += numOpsInString;

    // Check for final Op of "OpDecorate %x %stringImm %linkageAttribute"
    if (MI->getOpcode() == SPIRV::OpDecorate &&
        MI->getOperand(1).getImm() == Decoration::LinkageAttributes) {
      O << ' ';
      printLinkageType(MI, strStartIndex, O);
      break;
    }
  }
}

void SPIRVInstPrinter::printExtInst(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &O) {
  auto setReg = MI->getOperand(2).getReg();
  auto set = extInstSetIDs[setReg];
  auto op = MI->getOperand(OpNo).getImm();
  O << getExtInstName(set, op);
}

// Methods for printing textual names of SPIR-V enums (see SPIRVEnums.h)
GEN_INSTR_PRINTER_IMPL(Capability)
GEN_INSTR_PRINTER_IMPL(SourceLanguage)
GEN_INSTR_PRINTER_IMPL(ExecutionModel)
GEN_INSTR_PRINTER_IMPL(AddressingModel)
GEN_INSTR_PRINTER_IMPL(MemoryModel)
GEN_INSTR_PRINTER_IMPL(ExecutionMode)
GEN_INSTR_PRINTER_IMPL(StorageClass)
GEN_INSTR_PRINTER_IMPL(Dim)
GEN_INSTR_PRINTER_IMPL(SamplerAddressingMode)
GEN_INSTR_PRINTER_IMPL(SamplerFilterMode)
GEN_INSTR_PRINTER_IMPL(ImageFormat)
GEN_INSTR_PRINTER_IMPL(ImageChannelOrder)
GEN_INSTR_PRINTER_IMPL(ImageChannelDataType)
GEN_INSTR_PRINTER_IMPL(ImageOperand)
GEN_INSTR_PRINTER_IMPL(FPFastMathMode)
GEN_INSTR_PRINTER_IMPL(FPRoundingMode)
GEN_INSTR_PRINTER_IMPL(LinkageType)
GEN_INSTR_PRINTER_IMPL(AccessQualifier)
GEN_INSTR_PRINTER_IMPL(FunctionParameterAttribute)
GEN_INSTR_PRINTER_IMPL(Decoration)
GEN_INSTR_PRINTER_IMPL(BuiltIn)
GEN_INSTR_PRINTER_IMPL(SelectionControl)
GEN_INSTR_PRINTER_IMPL(LoopControl)
GEN_INSTR_PRINTER_IMPL(FunctionControl)
GEN_INSTR_PRINTER_IMPL(MemorySemantics)
GEN_INSTR_PRINTER_IMPL(MemoryOperand)
GEN_INSTR_PRINTER_IMPL(Scope)
GEN_INSTR_PRINTER_IMPL(GroupOperation)
GEN_INSTR_PRINTER_IMPL(KernelEnqueueFlags)
GEN_INSTR_PRINTER_IMPL(KernelProfilingInfo)

