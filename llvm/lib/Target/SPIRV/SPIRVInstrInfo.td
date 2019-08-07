//===-- SPIRVInstrInfo.td - Target Description for SPIR-V Target ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the SPIR-V instructions in TableGen format.
//
// TODO Auto-generate this from the SPIR-V Machine-readable Grammar:
//  - https://www.khronos.org/registry/spir-v/specs/unified1/MachineReadableGrammar.html
//  - https://github.com/KhronosGroup/SPIRV-Headers/blob/master/include/spirv/unified1/spirv.core.grammar.json
//
//===----------------------------------------------------------------------===//

include "SPIRVInstrFormats.td"
include "SPIRVEnums.td"

//Codegen only metadata instructions
let isCodeGenOnly=1 in {
  def ASSIGN_TYPE: Pseudo<(outs), (ins TYPE:$type, ID:$id)>;
}

class BinOp<string name, bits<16> opCode>
                : Op<opCode, (outs ID:$dst), (ins TYPE:$type, ID:$src, ID:$src2),
                  "$dst = "#name#" $type $src $src2">;

class UnOp<string name, bits<16> opCode>
                : Op<opCode, (outs ID:$dst), (ins TYPE:$type, ID:$src),
                  "$dst = "#name#" $type $src">;

class SimpleOp<string name, bits<16> opCode>: Op<opCode, (outs), (ins), name>;

//3.32.1 Miscellaneous Instructions

def OpNop: SimpleOp<"OpNop", 0>;
def OpUndef: Op<1, (outs ID:$res), (ins TYPE:$type), "$res = OpUndef $type">;
def OpSizeOf: Op<321, (outs ID:$res), (ins TYPE:$ty, ID:$ptr), "$res = OpSizeOf $ty $ptr">;

//3.32.2 Debug Instructions

def OpSourceContinued: Op<2, (outs), (ins StringImm:$str, variable_ops),
                  "OpSourceContinued $str">;
def OpSource: Op<3, (outs), (ins SourceLanguage:$lang, i32imm:$version, variable_ops),
                  "OpSource $lang $version">;
def OpSourceExtension: Op<4, (outs), (ins StringImm:$extension, variable_ops),
                  "OpSourceExtension $extension">;
def OpName: Op<5, (outs), (ins ANY:$tar, StringImm:$name, variable_ops), "OpName $tar $name">;
def OpMemberName: Op<6, (outs), (ins TYPE:$ty, i32imm:$mem, StringImm:$name, variable_ops),
                  "OpMemberName $ty $mem $name">;
def OpString: Op<7, (outs ID:$r), (ins StringImm:$s, variable_ops), "$r = OpString $s">;
def OpLine: Op<8, (outs), (ins ID:$file, i32imm:$ln, i32imm:$col), "OpLine $file $ln $col">;
def OpNoLine: Op<317, (outs), (ins), "OpNoLine">;
def OpModuleProcessed: Op<330, (outs), (ins StringImm:$process, variable_ops),
                  "OpModuleProcessed $process">;

//3.32.3 Annotation Instructions

def OpDecorate: Op<71, (outs), (ins ANY:$target, Decoration:$dec, variable_ops),
                  "OpDecorate $target $dec">;
def OpMemberDecorate: Op<72, (outs), (ins TYPE:$t, i32imm:$m, Decoration:$d, variable_ops),
                  "OpMemberDecorate $t $m $d">;

// TODO Currently some deprecated opcodes are missing: OpDecorationGroup,
// OpGroupDecorate and OpGroupMemberDecorate

def OpDecorateId: Op<332, (outs), (ins ANY:$target, Decoration:$dec, variable_ops),
                  "OpDecorateId $target $dec">;
def OpDecorateString: Op<5632, (outs), (ins ANY:$t, Decoration:$d, StringImm:$s, variable_ops),
                  "OpDecorateString $t $d $s">;
def OpMemberDecorateString: Op<5633, (outs),
                  (ins TYPE:$ty, i32imm:$mem, Decoration:$dec, StringImm:$str, variable_ops),
                  "OpMemberDecorateString $ty $mem $dec $str">;

//3.32.4 Extension Instructions

def OpExtension: Op<10, (outs), (ins StringImm:$name, variable_ops), "OpExtension $name">;
def OpExtInstImport: Op<11, (outs ID:$res), (ins StringImm:$extInstsName, variable_ops),
                  "$res = OpExtInstImport $extInstsName">;
def OpExtInst: Op<12, (outs ID:$res), (ins TYPE:$ty, ID:$set, ExtInst:$inst, variable_ops),
                  "$res = OpExtInst $ty $set $inst">;

//3.32.5 Mode-Setting Instructions

def OpMemoryModel: Op<14, (outs), (ins AddressingModel:$addr, MemoryModel:$mem),
                  "OpMemoryModel $addr $mem">;
def OpEntryPoint: Op<15, (outs),
                  (ins ExecutionModel:$model, ID:$entry, StringImm:$name, variable_ops),
                  "OpEntryPoint $model $entry $name">;
def OpExecutionMode: Op<16, (outs), (ins ID:$entry, ExecutionMode:$mode, variable_ops),
                  "OpExecutionMode $entry $mode">;
def OpCapability: Op<17, (outs), (ins Capability:$cap), "OpCapability $cap">;
def OpExecutionModeId: Op<331, (outs), (ins ID:$entry, ExecutionMode:$mode, variable_ops),
                  "OpExecutionModeId $entry $mode">;

//3.32.6 Type-Declaration Instructions


def OpTypeVoid: Op<19, (outs TYPE:$type), (ins), "$type = OpTypeVoid">;
def OpTypeBool: Op<20, (outs TYPE:$type), (ins), "$type = OpTypeBool">;
def OpTypeInt: Op<21, (outs TYPE:$type), (ins i32imm:$width, i32imm:$signedness),
                  "$type = OpTypeInt $width $signedness">;
def OpTypeFloat: Op<22, (outs TYPE:$type), (ins i32imm:$width),
                  "$type = OpTypeFloat $width">;
def OpTypeVector: Op<23, (outs TYPE:$type), (ins TYPE:$compType, i32imm:$compCount),
                  "$type = OpTypeVector $compType $compCount">;
def OpTypeMatrix: Op<24, (outs TYPE:$type), (ins TYPE:$colType, i32imm:$colCount),
                  "$type = OpTypeMatrix $colType $colCount">;
def OpTypeImage: Op<25, (outs TYPE:$res), (ins TYPE:$sampTy, Dim:$dim, i32imm:$depth,
      i32imm:$arrayed, i32imm:$MS, i32imm:$sampled, ImageFormat:$imFormat, variable_ops),
                  "$res = OpTypeImage $sampTy $dim $depth $arrayed $MS $sampled $imFormat">;
def OpTypeSampler: Op<26, (outs TYPE:$res), (ins), "$res = OpTypeSampler">;
def OpTypeSampledImage: Op<27, (outs TYPE:$res), (ins TYPE:$imageType),
                  "$res = OpTypeSampledImage $imageType">;
def OpTypeArray: Op<28, (outs TYPE:$type), (ins TYPE:$elementType, ID:$length),
                  "$type = OpTypeArray $elementType $length">;
def OpTypeRuntimeArray: Op<29, (outs TYPE:$type), (ins TYPE:$elementType),
                  "$type = OpTypeRuntimeArray $elementType">;
def OpTypeStruct: Op<30, (outs TYPE:$res), (ins variable_ops), "$res = OpTypeStruct">;
def OpTypeOpaque: Op<31, (outs TYPE:$res), (ins StringImm:$name, variable_ops),
                  "$res = OpTypeOpaque $name">;
def OpTypePointer: Op<32, (outs TYPE:$res), (ins StorageClass:$storage, TYPE:$type),
                  "$res = OpTypePointer $storage $type">;
def OpTypeFunction: Op<33, (outs TYPE:$funcType), (ins TYPE:$returnType, variable_ops),
                  "$funcType = OpTypeFunction $returnType">;
def OpTypeEvent: Op<34, (outs TYPE:$res), (ins), "$res = OpTypeEvent">;
def OpTypeDeviceEvent: Op<35, (outs TYPE:$res), (ins), "$res = OpTypeDeviceEvent">;
def OpTypeReserveId: Op<36, (outs TYPE:$res), (ins), "$res = OpTypeReserveId">;
def OpTypeQueue: Op<37, (outs TYPE:$res), (ins), "$res = OpTypeQueue">;
def OpTypePipe: Op<38, (outs TYPE:$res), (ins AccessQualifier:$a), "$res = OpTypePipe $a">;
def OpTypeForwardPointer: Op<39, (outs), (ins TYPE:$ptrType, StorageClass:$storageClass),
                  "OpTypeForwardPointer $ptrType $storageClass">;
def OpTypePipeStorage: Op<322, (outs TYPE:$res), (ins), "$res = OpTypePipeStorage">;
def OpTypeNamedBarrier: Op<327, (outs TYPE:$res), (ins), "$res = OpTypeNamedBarrier">;
def OpTypeAccelerationStructureNV: Op<5341, (outs TYPE:$res), (ins),
                  "$res = OpTypeAccelerationStructureNV">;
def OpTypeCooperativeMatrixNV: Op<5358, (outs TYPE:$res),
                  (ins TYPE:$compType, ID:$scope, ID:$rows, ID:$cols),
                  "$res = OpTypeCooperativeMatrixNV $compType $scope $rows $cols">;

//3.32.7 Constant-Creation Instructions

def OpConstantTrue: Op<41, (outs ID:$res), (ins TYPE:$ty), "$res = OpConstantTrue $ty">;
def OpConstantFalse: Op<42, (outs ID:$res), (ins TYPE:$ty), "$res = OpConstantFalse $ty">;
def OpConstant: Op<43, (outs ID:$res), (ins TYPE:$ty, variable_ops), "$res = OpConstant $ty">;
def OpConstantComposite: Op<44, (outs ID:$res), (ins TYPE:$type, variable_ops),
                  "$res = OpConstantComposite $type">;
def OpConstantSampler: Op<45, (outs ID:$res),
                  (ins TYPE:$t, SamplerAddressingMode:$s, i32imm:$p, SamplerFilterMode:$f),
                  "$res = OpConstantSampler $t $s $p $f">;
def OpConstantNull: Op<46, (outs ID:$res), (ins TYPE:$ty), "$res = OpConstantNull $ty">;

def OpSpecConstantTrue: Op<48, (outs ID:$r), (ins TYPE:$t), "$r= OpSpecConstantTrue $t">;
def OpSpecConstantFalse: Op<49, (outs ID:$r), (ins TYPE:$t), "$r= OpSpecConstantFalse $t">;
def OpSpecConstant: Op<50, (outs ID:$res), (ins TYPE:$type, i32imm:$imm, variable_ops),
                  "$res = OpSpecConstant $type $imm">;
def OpSpecConstantComposite: Op<51, (outs ID:$res), (ins TYPE:$type, variable_ops),
                  "$res = OpSpecConstantComposite $type">;
def OpSpecConstantOp: Op<52, (outs ID:$res), (ins TYPE:$t, i32imm:$c, ID:$o, variable_ops),
                  "$res = OpSpecConstantOp $t $c $o">;

//3.32.8 Memory Instructions

def OpVariable: Op<59, (outs ID:$res), (ins TYPE:$type, StorageClass:$sc, variable_ops),
                  "$res = OpVariable $type $sc">;
def OpImageTexelPointer: Op<60, (outs ID:$res),
                  (ins TYPE:$resType, ID:$image, ID:$coord, ID:$sample),
                  "$res = OpImageTexelPointer $resType $image $coord $sample">;
def OpLoad: Op<61, (outs ID:$res), (ins TYPE:$resType, ID:$pointer, variable_ops),
                  "$res = OpLoad $resType $pointer">;
def OpStore: Op<62, (outs), (ins ID:$pointer, ID:$objectToStore, variable_ops),
                  "OpStore $pointer $objectToStore">;
def OpCopyMemory: Op<63, (outs), (ins ID:$dest, ID:$src, variable_ops),
                  "OpCopyMemory $dest $src">;
def OpCopyMemorySized: Op<64, (outs), (ins ID:$dest, ID:$src, ID:$size, variable_ops),
                  "OpCopyMemorySized $dest $src $size">;
def OpAccessChain: Op<65, (outs ID:$res), (ins TYPE:$type, ID:$base, variable_ops),
                  "$res = OpAccessChain $type $base">;
def OpInBoundsAccessChain: Op<66, (outs ID:$res),
                  (ins TYPE:$type, ID:$base, variable_ops),
                  "$res = OpInBoundsAccessChain $type $base">;
def OpPtrAccessChain: Op<67, (outs ID:$res),
                  (ins TYPE:$type, ID:$base, ID:$element, variable_ops),
                  "$res = OpPtrAccessChain $type $base $element">;
def OpArrayLength: Op<68, (outs ID:$res), (ins TYPE:$resTy, ID:$struct, i32imm:$arrayMember),
                  "$res = OpArrayLength $resTy $struct $arrayMember">;
def OpGenericPtrMemSemantics: Op<69, (outs ID:$res), (ins TYPE:$resType, ID:$pointer),
                  "$res = OpGenericPtrMemSemantics $resType $pointer">;
def OpInBoundsPtrAccessChain: Op<70, (outs ID:$res),
                  (ins TYPE:$type, ID:$base, ID:$element, variable_ops),
                  "$res = OpInBoundsPtrAccessChain $type $base $element">;
def OpPtrEqual: Op<401, (outs ID:$res), (ins TYPE:$resType, ID:$a, ID:$b),
                  "$res = OpPtrEqual $resType $a $b">;
def OpPtrNotEqual: Op<402, (outs ID:$res), (ins TYPE:$resType, ID:$a, ID:$b),
                  "$res = OpPtrNotEqual $resType $a $b">;
def OpPtrDiff: Op<403, (outs ID:$res), (ins TYPE:$resType, ID:$a, ID:$b),
                  "$res = OpPtrDiff $resType $a $b">;

//3.32.9 Function Instructions

def OpFunction: Op<54, (outs ID:$func),
                  (ins TYPE:$resType, FunctionControl:$funcControl, TYPE:$funcType),
                  "$func = OpFunction $resType $funcControl $funcType">;
def OpFunctionParameter: Op<55, (outs ID:$arg), (ins TYPE:$type),
                  "$arg = OpFunctionParameter $type">;
def OpFunctionEnd: Op<56, (outs), (ins), "OpFunctionEnd"> {
  let isTerminator=1;
}
def OpFunctionCall: Op<57, (outs ID:$res), (ins TYPE:$resType, ID:$function, variable_ops),
                  "$res = OpFunctionCall $resType $function">;

//3.32.10 Image Instructions

def OpSampledImage: BinOp<"OpSampledImage", 86>;

def OpImageSampleImplicitLod: Op<87, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, variable_ops),
                  "$res = OpImageSampleImplicitLod $type $sampledImage $coord">;
def OpImageSampleExplicitLod: Op<88, (outs ID:$res),
                  (ins TYPE:$ty, ID:$sImage, ID:$uv, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSampleExplicitLod $ty $sImage $uv $op $i">;

def OpImageSampleDrefImplicitLod: Op<89, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, ID:$dref, variable_ops),
                  "$res = OpImageSampleDrefImplicitLod $type $sampledImage $dref $coord">;
def OpImageSampleDrefExplicitLod: Op<90, (outs ID:$res),
                  (ins TYPE:$ty, ID:$im, ID:$uv, ID:$d, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSampleDrefExplicitLod $ty $im $uv $d $op $i">;

def OpImageSampleProjImplicitLod: Op<91, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, variable_ops),
                  "$res = OpImageSampleProjImplicitLod $type $sampledImage $coord">;
def OpImageSampleProjExplicitLod: Op<92, (outs ID:$res),
                  (ins TYPE:$ty, ID:$im, ID:$uv, ID:$d, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSampleProjExplicitLod $ty $im $uv $op $i">;

def OpImageSampleProjDrefImplicitLod: Op<93, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, ID:$dref, variable_ops),
                  "$res = OpImageSampleProjDrefImplicitLod $type $sampledImage $dref $coord">;
def OpImageSampleProjDrefExplicitLod: Op<94, (outs ID:$res),
                  (ins TYPE:$ty, ID:$im, ID:$uv, ID:$d, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSampleProjDrefExplicitLod $ty $im $uv $d $op $i">;

def OpImageFetch: Op<95, (outs ID:$res),
                  (ins TYPE:$type, ID:$image, ID:$coord, variable_ops),
                  "$res = OpImageFetch $type $image $coord">;
def OpImageGather: Op<96, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, ID:$component, variable_ops),
                  "$res = OpImageGather $type $sampledImage $coord $component">;
def OpImageDrefGather: Op<97, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, ID:$dref, variable_ops),
                  "$res = OpImageDrefGather $type $sampledImage $coord $dref">;

def OpImageRead: Op<98, (outs ID:$res),
                  (ins TYPE:$type, ID:$image, ID:$coord, variable_ops),
                  "$res = OpImageRead $type $image $coord">;
def OpImageWrite: Op<99, (outs), (ins ID:$image, ID:$coord, ID:$texel, variable_ops),
                  "OpImageWrite $image $coord $texel">;

def OpImage: UnOp<"OpImage", 100>;
def OpImageQueryFormat: UnOp<"OpImageQueryFormat", 101>;
def OpImageQueryOrder: UnOp<"OpImageQueryOrder", 102>;
def OpImageQuerySizeLod: BinOp<"OpImageQuerySizeLod", 103>;
def OpImageQuerySize: UnOp<"OpImageQuerySize", 104>;
def OpImageQueryLod: BinOp<"OpImageQueryLod", 105>;
def OpImageQueryLevels: UnOp<"OpImageQueryLevels", 106>;
def OpImageQuerySamples: UnOp<"OpImageQuerySamples", 107>;


def OpImageSparseSampleImplicitLod: Op<305, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, variable_ops),
                  "$res = OpImageSparseSampleImplicitLod $type $sampledImage $coord">;
def OpImageSparseSampleExplicitLod: Op<306, (outs ID:$res),
                  (ins TYPE:$ty, ID:$sImage, ID:$uv, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSparseSampleExplicitLod $ty $sImage $uv $op $i">;

def OpImageSparseSampleDrefImplicitLod: Op<307, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImg, ID:$coord, ID:$dref, variable_ops),
                  "$res = OpImageSparseSampleDrefImplicitLod $type $sampledImg $dref $coord">;
def OpImageSparseSampleDrefExplicitLod: Op<308, (outs ID:$res),
                  (ins TYPE:$ty, ID:$im, ID:$uv, ID:$d, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSparseSampleDrefExplicitLod $ty $im $uv $d $op $i">;

def OpImageSparseSampleProjImplicitLod: Op<309, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, variable_ops),
                  "$res = OpImageSparseSampleProjImplicitLod $type $sampledImage $coord">;
def OpImageSparseSampleProjExplicitLod: Op<310, (outs ID:$res),
                  (ins TYPE:$ty, ID:$im, ID:$uv, ID:$d, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSparseSampleProjExplicitLod $ty $im $uv $op $i">;

def OpImageSparseSampleProjDrefImplicitLod: Op<311, (outs ID:$res),
                  (ins TYPE:$type, ID:$sImage, ID:$coord, ID:$dref, variable_ops),
                  "$res = OpImageSparseSampleProjDrefImplicitLod $type $sImage $dref $coord">;
def OpImageSparseSampleProjDrefExplicitLod: Op<312, (outs ID:$res),
                  (ins TYPE:$ty, ID:$im, ID:$uv, ID:$d, ImageOperand:$op, ID:$i, variable_ops),
                  "$res = OpImageSparseSampleProjDrefExplicitLod $ty $im $uv $d $op $i">;

def OpImageSparseFetch: Op<313, (outs ID:$res),
                  (ins TYPE:$type, ID:$image, ID:$coord, variable_ops),
                  "$res = OpImageSparseFetch $type $image $coord">;
def OpImageSparseGather: Op<314, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, ID:$component, variable_ops),
                  "$res = OpImageSparseGather $type $sampledImage $coord $component">;
def OpImageSparseDrefGather: Op<315, (outs ID:$res),
                  (ins TYPE:$type, ID:$sampledImage, ID:$coord, ID:$dref, variable_ops),
                  "$res = OpImageSparseDrefGather $type $sampledImage $coord $dref">;

def OpImageSparseTexelsResident: UnOp<"OpImageSparseTexelsResident", 316>;

def OpImageSparseRead: Op<320, (outs ID:$res),
                  (ins TYPE:$type, ID:$image, ID:$coord, variable_ops),
                  "$res = OpImageSparseRead $type $image $coord">;

def OpImageSampleFootprintNV: Op<5283, (outs ID:$res),
                  (ins TYPE:$ty, ID:$sImg, ID:$uv, ID:$granularity, ID:$coarse, variable_ops),
                  "$res = OpImageSampleFootprintNV $ty $sImg $uv $granularity $coarse">;

//3.32.11 Conversion instructions

def OpConvertFToU : UnOp<"OpConvertFToU", 109>;
def OpConvertFToS : UnOp<"OpConvertFToS", 110>;
def OpConvertSToF : UnOp<"OpConvertSToF", 111>;
def OpConvertUToF : UnOp<"OpConvertUToF", 112>;

def OpUConvert : UnOp<"OpUConvert", 113>;
def OpSConvert : UnOp<"OpSConvert", 114>;
def OpFConvert : UnOp<"OpFConvert", 115>;

def OpQuantizeToF16 : UnOp<"OpQuantizeToF16", 116>;
def OpConvertPtrToU : UnOp<"OpConvertPtrToU", 117>;

def OpSatConvertSToU : UnOp<"OpSatConvertSToU", 118>;
def OpSatConvertUToS : UnOp<"OpSatConvertUToS", 119>;

def OpConvertUToPtr : UnOp<"OpConvertUToPtr", 120>;
def OpPtrCastToGeneric : UnOp<"OpPtrCastToGeneric", 121>;
def OpGenericCastToPtr : UnOp<"OpGenericCastToPtr", 122>;
def OpGenericCastToPtrExplicit : Op<123, (outs ID:$r), (ins TYPE:$t, ID:$p, StorageClass:$s),
                              "$r = OpGenericCastToPtrExplicit $t $p $s">;
def OpBitcast : UnOp<"OpBitcast", 124>;

//3.32.12 Composite Instructions

def OpVectorExtractDynamic: Op<77, (outs ID:$res), (ins TYPE:$type, ID:$vec, ID:$idx),
                  "$res = OpVectorExtractDynamic $type $vec $idx">;
def OpVectorInsertDynamic: Op<78, (outs ID:$r), (ins TYPE:$ty, ID:$vec, ID:$comp, ID:$idx),
                  "$r = OpVectorInsertDynamic $ty $vec $comp $idx">;
def OpVectorShuffle: Op<79, (outs ID:$res), (ins TYPE:$ty, ID:$v1, ID:$v2, variable_ops),
                  "$res = OpVectorShuffle $ty $v1 $v2">;
def OpCompositeConstruct: Op<80, (outs ID:$res), (ins TYPE:$type, variable_ops),
                  "$res = OpCompositeConstruct $type">;
def OpCompositeExtract: Op<81, (outs ID:$res), (ins TYPE:$type, ID:$base, variable_ops),
                  "$res = OpCompositeExtract $type $base">;
def OpCompositeInsert: Op<82, (outs ID:$r), (ins TYPE:$ty, ID:$obj, ID:$base, variable_ops),
                  "$r = OpCompositeInsert $ty $obj $base">;
def OpCopyObject: UnOp<"OpCopyObject", 83>;
def OpTranspose: UnOp<"OpTranspose", 84>;
def OpCopyLogical: UnOp<"OpCopyLogical", 400>;


//3.32.13 Arithmetic Instructions

def OpSNegate: UnOp<"OpSNegate", 126>;
def OpFNegate: UnOp<"OpFNegate", 127>;

def OpIAdd: BinOp<"OpIAdd", 128>;
def OpFAdd: BinOp<"OpFAdd", 129>;

def OpISub: BinOp<"OpISub", 130>;
def OpFSub: BinOp<"OpFSub", 131>;

def OpIMul: BinOp<"OpIMul", 132>;
def OpFMul: BinOp<"OpFMul", 133>;

def OpUDiv: BinOp<"OpUDiv", 134>;
def OpSDiv: BinOp<"OpSDiv", 135>;
def OpFDiv: BinOp<"OpFDiv", 136>;

def OpUMod: BinOp<"OpUMod", 137>;

def OpSRem : BinOp<"OpSRem", 138>;
def OpSMod: BinOp<"OpSMod", 139>;

def OpFRem: BinOp<"OpFRem", 140>;
def OpFMod: BinOp<"OpFMod", 141>;

def OpVectorTimesScalar: BinOp<"OpVectorTimesScalar", 142>;
def OpMatrixTimesScalar: BinOp<"OpMatrixTimesScalar", 143>;
def OpVectorTimesMatrix: BinOp<"OpVectorTimesMatrix", 144>;
def OpMatrixTimesVector: BinOp<"OpMatrixTimesVector", 145>;
def OpMatrixTimesMatrix: BinOp<"OpMatrixTimesMatrix", 146>;

def OpOuterProduct: BinOp<"OpOuterProduct", 147>;
def OpDot: BinOp<"OpDot", 148>;

def OpIAddCarry: BinOp<"OpIAddCarry", 149>;
def OpISubBorrow: BinOp<"OpISubBorrow", 150>;
def OpUMulExtended: BinOp<"OpUMulExtended", 151>;
def OpSMulExtended: BinOp<"OpSMulExtended", 152>;

//3.32.14 Bit Instructions

def OpShiftRightLogical: BinOp<"OpShiftRightLogical", 194>;
def OpShiftRightArithmetic: BinOp<"OpShiftRightArithmetic", 195>;
def OpShiftLeftLogical: BinOp<"OpShiftLeftLogical", 196>;

def OpBitwiseOr: BinOp<"OpBitwiseOr", 197>;
def OpBitwiseXor: BinOp<"OpBitwiseXor", 198>;
def OpBitwiseAnd: BinOp<"OpBitwiseAnd", 199>;
def OpNot: UnOp<"OpNot", 200>;

def OpBitFieldInsert: Op<201, (outs ID:$res),
                  (ins TYPE:$ty, ID:$base, ID:$insert, ID:$offset, ID:$count),
                  "$res = OpBitFieldInsert $ty $base $insert $offset $count">;
def OpBitFieldSExtract: Op<202, (outs ID:$res),
                  (ins TYPE:$ty, ID:$base, ID:$offset, ID:$count),
                  "$res = OpBitFieldSExtract $ty $base $offset $count">;
def OpBitFieldUExtract: Op<203, (outs ID:$res),
                  (ins TYPE:$ty, ID:$base, ID:$offset, ID:$count),
                  "$res = OpBitFieldUExtract $ty $base $offset $count">;
def OpBitReverse: Op<204, (outs ID:$r), (ins TYPE:$ty, ID:$b), "$r = OpBitReverse $ty $b">;
def OpBitCount: Op<205, (outs ID:$r), (ins TYPE:$ty, ID:$b), "$r = OpBitCount $ty $b">;

//3.32.15 Relational and Logical Instructions

def OpAny: Op<154, (outs ID:$res), (ins ID:$vec), "$res = OpAny $vec">;
def OpAll: Op<155, (outs ID:$res), (ins ID:$vec), "$res = OpAll $vec">;

def OpIsNan: UnOp<"OpIsNan", 156>;
def OpIsInf: UnOp<"OpIsInf", 157>;
def OpIsFinite: UnOp<"OpIsFinite", 158>;
def OpIsNormal: UnOp<"OpIsNormal", 159>;
def OpSignBitSet: UnOp<"OpSignBitSet", 160>;

def OpLessOrGreater: BinOp<"OpLessOrGreater", 161>;
def OpOrdered: BinOp<"OpOrdered", 162>;
def OpUnordered: BinOp<"OpUnordered", 163>;

def OpLogicalEqual: BinOp<"OpLogicalEqual", 164>;
def OpLogicalNotEqual: BinOp<"OpLogicalNotEqual", 165>;
def OpLogicalOr: BinOp<"OpLogicalOr", 166>;
def OpLogicalAnd: BinOp<"OpLogicalAnd", 167>;
def OpLogicalNot: UnOp<"OpLogicalNot", 168>;

def OpSelect: Op<169, (outs ID:$res), (ins TYPE:$type, ID:$cond, ID:$true, ID:$false),
                  "$res = OpSelect $type $cond $true $false">;

def OpIEqual: BinOp<"OpIEqual", 170>;
def OpINotEqual: BinOp<"OpINotEqual", 171>;

def OpUGreaterThan: BinOp<"OpUGreaterThan", 172>;
def OpSGreaterThan: BinOp<"OpSGreaterThan", 173>;
def OpUGreaterThanEqual: BinOp<"OpUGreaterThanEqual", 174>;
def OpSGreaterThanEqual: BinOp<"OpSGreaterThanEqual", 175>;
def OpULessThan: BinOp<"OpULessThan", 176>;
def OpSLessThan: BinOp<"OpSLessThan", 177>;
def OpULessThanEqual: BinOp<"OpULessThanEqual", 178>;
def OpSLessThanEqual: BinOp<"OpSLessThanEqual", 179>;

def OpFOrdEqual: BinOp<"OpFOrdEqual", 180>;
def OpFUnordEqual: BinOp<"OpFUnordEqual", 181>;
def OpFOrdNotEqual: BinOp<"OpFOrdNotEqual", 182>;
def OpFUnordNotEqual: BinOp<"OpFUnordNotEqual", 183>;

def OpFOrdLessThan: BinOp<"OpFOrdLessThan", 184>;
def OpFUnordLessThan: BinOp<"OpFUnordLessThan", 185>;
def OpFOrdGreaterThan: BinOp<"OpFOrdGreaterThan", 186>;
def OpFUnordGreaterThan: BinOp<"OpFUnordGreaterThan", 187>;

def OpFOrdLessThanEqual: BinOp<"OpFOrdLessThanEqual", 188>;
def OpFUnordLessThanEqual: BinOp<"OpFUnordLessThanEqual", 189>;
def OpFOrdGreaterThanEqual: BinOp<"OpFOrdGreaterThanEqual", 190>;
def OpFUnordGreaterThanEqual: BinOp<"OpFUnordGreaterThanEqual", 191>;

//3.32.16 Derivative Instructions

def OpDPdx: UnOp<"OpDPdx", 207>;
def OpDPdy: UnOp<"OpDPdy", 208>;
def OpFwidth: UnOp<"OpFwidth", 209>;

def OpDPdxFine: UnOp<"OpDPdxFine", 210>;
def OpDPdyFine: UnOp<"OpDPdyFine", 211>;
def OpFwidthFine: UnOp<"OpFwidthFine", 212>;

def OpDPdxCoarse: UnOp<"OpDPdxCoarse", 213>;
def OpDPdyCoarse: UnOp<"OpDPdyCoarse", 214>;
def OpFwidthCoarse: UnOp<"OpFwidthCoarse", 215>;


//3.32.17 Control-Flow Instructions

def OpPhi: Op<245, (outs ID:$res), (ins TYPE:$type, ID:$var0, ID:$block0, variable_ops),
                  "$res = OpPhi $type $var0 $block0">;
def OpLoopMerge: Op<246, (outs), (ins ID:$merge, ID:$continue, LoopControl:$lc, variable_ops),
                  "OpLoopMerge $merge $merge $continue $lc">;
def OpSelectionMerge: Op<247, (outs), (ins ID:$merge, SelectionControl:$sc),
                  "OpSelectionMerge $merge $sc">;
def OpLabel: Op<248, (outs ID:$label), (ins), "$label = OpLabel">;
let isTerminator=1 in {
def OpBranch: Op<249, (outs), (ins ID:$label), "OpBranch $label">;
def OpBranchConditional: Op<250, (outs), (ins ID:$cond, ID:$true, ID:$false, variable_ops),
                  "OpBranchConditional $cond $true $false">;
def OpSwitch: Op<251, (outs), (ins ID:$sel, ID:$dflt, variable_ops), "OpSwitch $sel $dflt">;
}
let isReturn = 1, hasDelaySlot=0, isBarrier = 0, isTerminator=1, isNotDuplicable = 1 in {
  def OpKill: SimpleOp<"OpKill", 252>;
  def OpReturn: SimpleOp<"OpReturn", 253>;
  def OpReturnValue: Op<254, (outs), (ins ID:$ret), "OpReturnValue $ret">;
  def OpUnreachable: SimpleOp<"OpUnreachable", 255>;
}
def OpLifetimeStart: Op<256, (outs), (ins ID:$ptr, i32imm:$sz), "OpLifetimeStart $ptr, $sz">;
def OpLifetimeStop: Op<257, (outs), (ins ID:$ptr, i32imm:$sz), "OpLifetimeStop $ptr, $sz">;

//3.32.18 Atomic Instructions

class AtomicOp<string name, bits<16> opCode>: Op<opCode, (outs ID:$res),
                  (ins TYPE:$ty, ID:$ptr, ID:$sc, ID:$sem),
                  "$res = "#name#" $ty $ptr $sc $sem">;

class AtomicOpVal<string name, bits<16> opCode>: Op<opCode, (outs ID:$res),
                  (ins TYPE:$ty, ID:$ptr, ID:$sc, ID:$sem, ID:$val),
                  "$res = "#name#" $ty $ptr $sc $sem $val">;

def OpAtomicLoad: AtomicOp<"OpAtomicLoad", 227>;

def OpAtomicStore: Op<228, (outs), (ins ID:$ptr, ID:$sc, ID:$sem, ID:$val),
                  "OpStore $ptr $sc $sem $val">;
def OpAtomicExchange: Op<229, (outs ID:$res),
                  (ins TYPE:$ty, ID:$ptr, ID:$sc, ID:$sem, ID:$val),
                  "$res = OpAtomicExchange $ty $ptr $sc $sem $val">;
def OpAtomicCompareExchange: Op<230, (outs ID:$res),
                  (ins TYPE:$ty, ID:$ptr, ID:$sc, ID:$eq,
                   ID:$neq, ID:$val, ID:$cmp),
                  "$res = OpAtomicCompareExchange $ty $ptr $sc $eq $neq $val $cmp">;
// TODO Currently the following deprecated opcode is missing: OpAtomicCompareExchangeWeak

def OpAtomicIIncrement: AtomicOp<"OpAtomicIIncrement", 232>;
def OpAtomicIDecrement: AtomicOp<"OpAtomicIDecrement", 233>;

def OpAtomicIAdd: AtomicOpVal<"OpAtomicIAdd", 234>;
def OpAtomicISub: AtomicOpVal<"OpAtomicISub", 235>;

def OpAtomicSMin: AtomicOpVal<"OpAtomicSMin", 236>;
def OpAtomicUMin: AtomicOpVal<"OpAtomicUMin", 237>;
def OpAtomicSMax: AtomicOpVal<"OpAtomicSMax", 238>;
def OpAtomicUMax: AtomicOpVal<"OpAtomicUMax", 239>;

def OpAtomicAnd: AtomicOpVal<"OpAtomicAnd", 240>;
def OpAtomicOr: AtomicOpVal<"OpAtomicOr", 241>;
def OpAtomicXor: AtomicOpVal<"OpAtomicXor", 242>;


def OpAtomicFlagTestAndSet: AtomicOp<"OpAtomicFlagTestAndSet", 318>;
def OpAtomicFlagClear: Op<319, (outs), (ins ID:$ptr, ID:$sc, ID:$sem),
                  "OpAtomicFlagClear $ptr $sc $sem">;

//3.32.19 Primitive Instructions

def OpEmitVertex: SimpleOp<"OpEmitVertex", 218>;
def OpEndPrimitive: SimpleOp<"OpEndPrimitive", 219>;
def OpEmitStreamVertex: Op<220, (outs), (ins ID:$stream), "OpEmitStreamVertex $stream">;
def OpEndStreamPrimitive: Op<221, (outs), (ins ID:$stream), "OpEndStreamPrimitive $stream">;

//3.32.20 Barrier Instructions

def OpControlBarrier: Op<224, (outs), (ins ID:$exec, ID:$mem, ID:$sem),
                  "OpControlBarrier $exec $mem $sem">;
def OpMemoryBarrier: Op<225, (outs), (ins ID:$mem, ID:$sem),
                  "OpMemoryBarrier $mem $sem">;
def OpNamedBarrierInitialize: UnOp<"OpNamedBarrierInitialize", 328>;
def OpMemoryNamedBarrier: Op<329, (outs), (ins ID:$barr, ID:$mem, ID:$sem),
                  "OpMemoryNamedBarrier $barr $mem $sem">;

// TODO Complete this list, or auto-generate it, to include later sections such as
// 3.32.21. Group and Subgroup Instructions,
// 3.32.22. Device-Side Enqueue Instructions,
// 3.32.23. Pipe Instructions,
// 3.32.24. Non-Uniform Instructions,
// and possibly 3.32.25. Reserved Instructions.

