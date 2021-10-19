//===-- SPIRVOpenCLBuiltins.td - Mapping between OpenCL Builtins and the equivelant SPIRV instructions ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class Type<string _Name> {
  string Name = _Name;
}

class BoolType: Type<"bool">;

class IntType<int _bitwidth> : Type<"int">{
  int bitwidth = _bitwidth;
}
class FloatType<int _bitwidth> : Type<"float"> {
  int bitwidth = _bitwidth;
}

class VoidType: Type<"void">;
class VectorType<Type _Ty, int _VecWidth> : Type<_Ty.Name> {
  int VecWidth = _VecWidth;
  Type baseTy = _Ty;
}

class ReturnType<Type _Ty=Type<"">, bit _isDefault=0> : Type<"returnType">{
  Type Ty = _Ty;
  bit isDefault = _isDefault;
}

class ImmType<int _val> : Type<"">{
 int val = _val;
}

class EnumType<string _enum_val, string _enum_class> : Type<"EnumType"> {
  string val = ""#_enum_class#"::"#_enum_val#"";

}

class OpenCLExt<string function> : EnumType<function, "OpenCL_std">;
class OCLOperand<int _ArgIdx>: Type<""> {
  int Index = _ArgIdx;
}
class LLT<int _size> {
  int size = _size;
}
class LLTVector<int _num_elements, int _size> : LLT<_size> {
  int num_elements = _num_elements;
}
class LLTInt<int _size> : LLT<_size>;

class OCLDest<bit _generic=0, LLT _type=LLTInt<32>> : Type<"Destination">{
  bit generic = _generic;
  LLT type = _type;
}

class ImmReg<int _ArgIdx>: Type<"IntermediateValue">{
  int Index = _ArgIdx;
}
class OCLPair<string _instr, list<Type> _Args>{
  string Opcode = "SPIRV::"#_instr#"";
  list<Type> Args = _Args;
}
class OCLBuiltinMapping<string _opencl_name, list<OCLPair> _InstrList> {
  string opencl_name = _opencl_name;
  list<OCLPair> InstrList = _InstrList;
}

// Function should be in SPIRVOpenCLBifs.cpp and
// it should have the format bool func(StringRef name, MachineIRBuilder &MIRBuilder, Register OrigRet, 
// const SPIRVType *retTy, const SmallVectorImpl<Register> &args,
//      "SPIRVTypeRegistry *TR)
// function should return a boolean
// you need to put the function name only
class OCLBuiltinFunctionMapping<string _opencl_name, string _function> {
  string opencl_name = _opencl_name;
  string function = _function;
}

class OCLMatchingMapping<string _starts_with, string _handler> {
  string starts_with = _starts_with;
  string handler = _handler;
}
def void : VoidType;
def voidRet : ReturnType<void>;

def return: OCLDest;
def defaultReturnType: ReturnType<Type<"">, 1>;
def Arg0 : OCLOperand<0>;
def Arg1 : OCLOperand<1>;
def acos : OpenCLExt<"acos">;
def bool: BoolType;
def boolRet : ReturnType<bool>;
def p2 : OCLPair<"OpFOrdGreaterThanEqual", [return, boolRet, Arg0, Arg1]>;

def: OCLBuiltinMapping<"__spirv_FOrdGreaterThanEqual", [p2]>;

def const1: OCLPair<"OpConstantI" , [OCLDest<1>, ReturnType<IntType<32>, 0>, ImmType<1>]>;
def const0: OCLPair<"OpConstantI" , [OCLDest<1>, ReturnType<IntType<32>, 0>, ImmType<0>]>;
def ford_comapre: OCLPair<"OpFOrdGreaterThanEqual", [OCLDest<1>, boolRet, Arg0, Arg1]>;
def ford_select: OCLPair<"OpSelectSISCond", [return, defaultReturnType, ImmReg<2>, ImmReg<0>, ImmReg<1>]>;
def: OCLBuiltinMapping<"isgreaterequal", [const1, const0, ford_comapre, ford_select]>;


// 
def write_img: OCLPair<"OpImageWrite", [Arg0, Arg1, OCLOperand<2>]>;
foreach n = ["f", "ui", "i"] in {
  def: OCLBuiltinFunctionMapping<"read_image"#n#"", "handleReadImage">;
  def: OCLBuiltinMapping<"write_image"#n#"", [write_img]>;
  
}


// vloadn
foreach v = [2, 4, 8, 16] in {
    def: OCLBuiltinMapping<"vload"#v#"",
      [OCLPair<"OpExtInst",
      [return, defaultReturnType, ImmType<0>, OpenCLExt<"vloadn">, Arg0, Arg1, ImmType<v>]>]>;
}

// vstoren
foreach v = [2, 4, 8, 16] in {
    def: OCLBuiltinMapping<"vstore"#v#"(size_t, const "#mode#" gentype)",
      [OCLPair<"OpExtInst",
      [return, voidRet, ImmType<0>, OpenCLExt<"vstoren">, Arg0, Arg1]>]>;
}

def: OCLBuiltinFunctionMapping<"get_group_id", "handleWorkGroup">;
def: OCLBuiltinFunctionMapping<"get_enqueued_local_size", "handleWorkGroup">;
def: OCLBuiltinFunctionMapping<"get_num_groups", "handleWorkGroup">;
def: OCLBuiltinFunctionMapping<"work_group_barrier", "handleBarrier">;
def: OCLBuiltinFunctionMapping<"__translate_sampler_initializer", "handleSamplerLiteral">;

def: OCLMatchingMapping<"convert", "handleConvert">;
def: OCLMatchingMapping<"atom", "handleAtomic">;
def: OCLMatchingMapping<"barrier", "handleBarrier">;
def: OCLMatchingMapping<"dot", "handleDot">;
def: OCLMatchingMapping<"get_local", "handleLocal">;
def: OCLMatchingMapping<"get_global", "handleGlobal">;
def: OCLMatchingMapping<"get_image", "handleImageQuery">;


