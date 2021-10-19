//===- SkeletonEmitter.cpp - Skeleton TableGen backend          -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This Tablegen backend emits ...
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/StringMatcher.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"
#include <algorithm>
#include <set>
#include <string>
#include <vector>

#define DEBUG_TYPE "openclbuiltins-emitter"
using namespace llvm;
using namespace std;

namespace {

class TypeCount {
public:

  static TypeCount* getInstance(){
    if (inst_ == NULL) {
       inst_ = new TypeCount();
    }
    return inst_;

  }
  int getValue() {
    int val = value;
    value ++;
    return val;
  };
  int getValNoIncrease(){
    return value;
  }
protected:
  int value;
private:
  static TypeCount* inst_;
  TypeCount() : value(0) {}
  TypeCount(const TypeCount&);
  TypeCount& operator=(const TypeCount&);

};

TypeCount* TypeCount::inst_ = NULL;


// Any helper data structures can be defined here. Some backends use
// structs to collect information from the records.
typedef struct {
  StringRef Opcode;
  vector<Record *> operands;
} SPIRVInstruction;

typedef struct {
  StringRef functionName;
  MapVector<StringRef, SPIRVInstruction> instrList;
} OCLFunction;


class SPIRVOpenCLBuiltinsEmitter {
private:
  RecordKeeper &Records;
  TypeCount* typeCount = TypeCount::getInstance();
  vector<OCLFunction> mappingsList;
  string createBaseType(Record* baseType);
  string createVectorReturnType(Record* vectorType);
  string createInstrReturnType(Record* returnTypeOperand, unsigned count);
  vector<pair<StringRef, StringRef>> functionMappingList;
  vector<pair<StringRef, StringRef>> matchingMappingList;
  void getOCLFunctions();
  void emitStringMatcher(raw_ostream &OS);
  string emitSelectRecord(OCLFunction function);
  void emitImports(raw_ostream &OS);
public:
  SPIRVOpenCLBuiltinsEmitter(RecordKeeper &RK) :
      Records(RK) {
  }
  void run(raw_ostream &OS);
};
// emitter class

}// anonymous namespace

void SPIRVOpenCLBuiltinsEmitter::getOCLFunctions() {
  auto OCLRecords = Records.getAllDerivedDefinitions("OCLBuiltinMapping");
  for (auto const &record : OCLRecords) {
    StringRef oclFunctionName = record->getValueAsString("opencl_name");
    auto instrList = record->getValueAsListOfDefs("InstrList");
    auto operandsMap = MapVector<StringRef, SPIRVInstruction>();
    for(auto const  &rec: instrList){
      auto name = rec->getName();
      auto SPIRVOpcode = rec->getValueAsString("Opcode");
      vector<Record *> operands = rec->getValueAsListOfDefs("Args");
      operandsMap.insert({name, {SPIRVOpcode, operands}});

    }
    mappingsList.push_back({oclFunctionName, operandsMap});
  }
  auto OCLFunctionMapRecords = Records.getAllDerivedDefinitions("OCLBuiltinFunctionMapping");
  for(auto const &record: OCLFunctionMapRecords){
    StringRef oclFunctionName = record->getValueAsString("opencl_name");
    StringRef functionName = record->getValueAsString("function");
    functionMappingList.push_back({oclFunctionName, functionName});
  }

  auto OCLMatchingMappingRecords = Records.getAllDerivedDefinitions("OCLMatchingMapping");
  for(auto const &record: OCLMatchingMappingRecords){
    StringRef startsWith = record->getValueAsString("starts_with");
    StringRef handler = record->getValueAsString("handler");
    matchingMappingList.push_back({startsWith, handler});
  }

}

void SPIRVOpenCLBuiltinsEmitter::emitStringMatcher(raw_ostream &OS) {
  vector<StringMatcher::StringPair> validOCLFunctions;
  string matchingLadder;
  raw_string_ostream MatchingStream(matchingLadder);
  for (auto &func : mappingsList) {
    validOCLFunctions.push_back(StringMatcher::StringPair(func.functionName, emitSelectRecord(func)));
  }

  for (auto &func: functionMappingList){
    string functionName;
    raw_string_ostream SS(functionName);
    SS << "return " << func.second <<  "(name, MIRBuilder, OrigRet, retTy, args, TR);";
    validOCLFunctions.push_back(StringMatcher::StringPair(func.first, functionName));
  }
  for (auto &mapping : matchingMappingList){
    string functionName;
    raw_string_ostream MM(functionName);
    MM << "return " << mapping.second <<  "(name, MIRBuilder, OrigRet, retTy, args, TR);\n";
    MatchingStream << "  if(name.startswith(\"" << mapping.first << "\"))  " << functionName;

  }
  OS << "bool generateOpenCLBuiltinMappings(StringRef name, ";
  OS << " MachineIRBuilder &MIRBuilder, Register OrigRet, const SPIRVType *retTy, const SmallVectorImpl<Register> &args," <<
      "SPIRVTypeRegistry *TR) {\n";
  OS << matchingLadder;
  OS << "  auto firstBraceIdx = name.find_first_of('(');\n";
  OS << "  auto nameNoArgs = name.substr(0, firstBraceIdx);\n";
  OS << "  const auto MRI = MIRBuilder.getMRI();\n";
  StringMatcher("nameNoArgs", validOCLFunctions, OS).Emit(0, true);

  OS << "  return false;\n";
  OS << "}\n";

}

string SPIRVOpenCLBuiltinsEmitter::createBaseType(Record* baseType){
  string returnTypeStr;
  raw_string_ostream SS(returnTypeStr);
  auto retTy = baseType->getType()->getAsString();
  if(retTy == "BoolType"){
    SS << "  auto type" << typeCount->getValue() << " = TR->getOrCreateSPIRVBoolType(MIRBuilder);\n";
  }else if(retTy == "IntType"){
    auto bitWidth = baseType->getValueAsInt("bitwidth");
    SS << "  auto type" << typeCount->getValue() << " = TR->getOrCreateSPIRVIntegerType(" << bitWidth << ", MIRBuilder);\n";
  }else if(retTy == "VoidType"){
    int val = typeCount->getValue();
    SS << "  auto contextType" << val << " = Type::getVoidTy(MIRBuilder.getMF().getFunction().getContext());\n";
    SS << "  auto type" << val << " = TR->getOrCreateSPIRVType(contextType" << val << ", MIRBuilder);\n";
  }
  return returnTypeStr;
}
string SPIRVOpenCLBuiltinsEmitter::createVectorReturnType(Record* vectorType){
  string returnTypeStr;
  raw_string_ostream SS(returnTypeStr);
  auto NumElements = vectorType->getValueAsInt("VecWidth");
  auto baseTy = vectorType->getValueAsDef("baseTy");
  SS << "  //baseTy " << baseTy->getType()->getAsString() << "\n";
  string baseTypeStr = createBaseType(baseTy);
  SS << baseTypeStr;
  int vectorVal = typeCount->getValue();
  SS << "  auto type" << vectorVal << " = TR->getOrCreateSPIRVVectorType(type" << vectorVal - 1 << ", " << NumElements <<
           ", MIRBuilder);\n";

  return returnTypeStr;

}
string SPIRVOpenCLBuiltinsEmitter::createInstrReturnType(Record* returnTypeOperand, unsigned count){

  string returnTypeStr;
  raw_string_ostream SS(returnTypeStr);
  if(returnTypeOperand->getValueAsBit("isDefault")){
    SS << "  M" << count << ".addUse(TR->getSPIRVTypeID(retTy));\n";
  } else {
    auto retTy = returnTypeOperand->getValueAsDef("Ty");
    StringRef retTyClass = retTy->getType()->getAsString();
    if(retTyClass == "VectorType"){
      SS << createVectorReturnType(retTy);
    }else {
      SS << createBaseType(retTy);
    }
    SS << "  M" << count << ".addUse(TR->getSPIRVTypeID(type" << typeCount->getValue() - 1 << "));\n";
  }
  return returnTypeStr;

}
string SPIRVOpenCLBuiltinsEmitter::emitSelectRecord(OCLFunction function){
  int operandSize = function.instrList.size();
  string caseBody;
  raw_string_ostream SS(caseBody);
  SS << "{\n";
  if(operandSize > 1)
    SS << "  Register operands ["<< function.instrList.size() <<"];\n";
  int count = 0;
  for(auto instrs: function.instrList){
    SS << "  // Instruction: " << instrs.first << "\n";
    StringRef opcode = instrs.second.Opcode;
    vector<Record *> operands = instrs.second.operands;
    SS << "  auto M" << count << " = MIRBuilder.buildInstr(" << opcode << ");\n";
    bool isVoid = true;
    for(auto operand : operands){
      StringRef operandType = operand->getType()->getAsString();
      SS << "  // Operand "<< operandType <<"\n";

      if (operandType == "OCLDest"){
        isVoid = false;
        if(operand->getValueAsBit("generic")){
          auto genericType = operand->getValueAsDef("type");
          auto genericClass = genericType->getType()->getAsString();
          auto bitwidth = genericType->getValueAsInt("size");
          if(genericClass == "LLTTypeVector"){
            auto numElements = genericType->getValueAsInt("num_elements");
            SS << "  auto dest" << count << " = MRI->createVirtualRegister(LLT::vector(" << numElements << ", " << bitwidth << "));\n";
            SS << "  auto intDest" << count << " = TR->getOrCreateSPIRVIntegerType(" << bitwidth << ", MIRBuilder);\n";
            SS << "  auto vecDest" << count << " TR->getOrCreateSPIRVVectorType(intDest" << count << ", " << numElements << ", MIRBuilder);\n";
            SS << "TR->assignSPIRVTypeToVReg(vecDest" << count << ", dest" << count <<", MIRBuilder);\n";
          }else {
            SS << "  auto dest" << count << " = MRI->createVirtualRegister(&SPIRV::IDRegClass);\n";
            SS << "  TR->assignSPIRVTypeToVReg(TR->getOrCreateSPIRVIntegerType(32, MIRBuilder), dest" << count << ", MIRBuilder);\n";
            SS << "  MRI->setType(dest" << count << " , LLT::scalar(32));\n";
            SS << "  M" << count << ".addDef(dest" << count << ");\n";
          }
        }else
          SS << "  M" << count << ".addDef(OrigRet);\n";
      }
      else if (operandType == "ReturnType"){
        SS << createInstrReturnType(operand, count);
      }
      else if (operandType == "EnumType" || operandType == "OpenCLExt"){
        SS << "  M" << count << ".addImm(" << operand->getValueAsString("val")<< ");\n";
      }
      else if (operandType == "OCLOperand")
        SS << "  M" << count<<".addUse(args[" << operand->getValueAsInt("Index") << "]);\n";
      else if (operandType == "ImmType")
        SS << "  M" << count << ".addImm( " << operand->getValueAsInt("val") << ");\n";
      else if (operandType == "ImmReg"){
        SS << "  M" << count << ".addUse(operands[" << operand->getValueAsInt("Index") << "]);\n";
      }
    }
    if(!isVoid && operandSize > 1) SS << "  operands["<< count <<"] = M" << count << ".getInstr()->getOperand(0).getReg();\n";
    count ++;
  }
  SS << "  return TR->constrainRegOperands(M" << count - 1 << ");\n";
  SS << "}";

  return caseBody;

}
void SPIRVOpenCLBuiltinsEmitter::emitImports(raw_ostream &OS) {
  OS << "#include \"llvm/ADT/StringRef.h\"" << "\n";
  OS << "#include \"SPIRVTypeRegistry.h\"" << "\n";
  OS << "#include \"llvm/ADT/SmallVector.h\"" << "\n";
  OS << "#include \"SPIRVOpenCLBIFs.h\"" << "\n";
  OS << "#include \"SPIRV.h\"" << "\n";
  OS << "#include \"SPIRVEnums.h\"" << "\n";
  OS << "#include \"SPIRVExtInsts.h\"" << "\n";
  OS << "#include \"SPIRVRegisterInfo.h\"" << "\n";
  OS << "#include \"llvm/CodeGen/GlobalISel/MachineIRBuilder.h\"" << "\n";
  OS << "using namespace llvm;\n\n";

}


void SPIRVOpenCLBuiltinsEmitter::run(raw_ostream &OS) {
  emitSourceFileHeader("OpenCLBuiltins data structures", OS);
  emitImports(OS);
  getOCLFunctions();
  emitStringMatcher(OS);
}

namespace llvm {

// The only thing that should be in the llvm namespace is the
// emitter entry point function.

void EmitSPIRVOpenCLBuiltins(RecordKeeper &RK, raw_ostream &OS) {
  // Instantiate the emitter class and invoke run().
  SPIRVOpenCLBuiltinsEmitter(RK).run(OS);
}

} // namespace llvm
