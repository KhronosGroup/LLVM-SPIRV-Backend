; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=SPV

; The IR was generated from the following source:
; void __kernel K(global float* A, int B) {
;   bool Cmp = B > 0;
;   A[0] = Cmp;
; }
; Command line:
; clang -x cl -cl-std=CL2.0 -target spir64 -emit-llvm -S -c test.cl


; SPV-DAG: OpName %[[s1:[0-9]+]] "s1"
; SPV-DAG: OpName %[[s2:[0-9]+]] "s2"
; SPV-DAG: OpName %[[s3:[0-9]+]] "s3"
; SPV-DAG: OpName %[[s4:[0-9]+]] "s4"
; SPV-DAG: OpName %[[s5:[0-9]+]] "s5"
; SPV-DAG: OpName %[[s6:[0-9]+]] "s6"
; SPV-DAG: OpName %[[s7:[0-9]+]] "s7"
; SPV-DAG: OpName %[[s8:[0-9]+]] "s8"
; SPV-DAG: OpName %[[z1:[0-9]+]] "z1"
; SPV-DAG: OpName %[[z2:[0-9]+]] "z2"
; SPV-DAG: OpName %[[z3:[0-9]+]] "z3"
; SPV-DAG: OpName %[[z4:[0-9]+]] "z4"
; SPV-DAG: OpName %[[z5:[0-9]+]] "z5"
; SPV-DAG: OpName %[[z6:[0-9]+]] "z6"
; SPV-DAG: OpName %[[z7:[0-9]+]] "z7"
; SPV-DAG: OpName %[[z8:[0-9]+]] "z8"
; SPV-DAG: OpName %[[ufp1:[0-9]+]] "ufp1"
; SPV-DAG: OpName %[[ufp2:[0-9]+]] "ufp2"
; SPV-DAG: OpName %[[sfp1:[0-9]+]] "sfp1"
; SPV-DAG: OpName %[[sfp2:[0-9]+]] "sfp2"
; SPV-DAG: %[[int_32:[0-9]+]] = OpTypeInt 32 0
; SPV-DAG: %[[int_8:[0-9]+]] = OpTypeInt 8 0
; SPV-DAG: %[[int_16:[0-9]+]] = OpTypeInt 16 0
; SPV-DAG: %[[int_64:[0-9]+]] = OpTypeInt 64 0
; SPV-DAG: %[[zero_32:[0-9]+]] = OpConstant %[[int_32]] 0
; SPV-DAG: %[[one_32:[0-9]+]] = OpConstant %[[int_32]] 1
; SPV-DAG: %[[zero_8:[0-9]+]] = OpConstantNull %[[int_8]]
; SPV-DAG: %[[mone_8:[0-9]+]] = OpConstant %[[int_8]] 255
; SPV-DAG: %[[zero_16:[0-9]+]] = OpConstantNull %[[int_16]]
; SPV-DAG: %[[mone_16:[0-9]+]] = OpConstant %[[int_16]] 65535
; SPV-DAG: %[[mone_32:[0-9]+]] = OpConstant %[[int_32]] 4294967295
; SPV-DAG: %[[zero_64:[0-9]+]] = OpConstantNull %[[int_64]]
; SPV-DAG: %[[mone_64:[0-9]+]] = OpConstant %[[int_64]] 4294967295 4294967295
; SPV-DAG: %[[one_8:[0-9]+]] = OpConstant %[[int_8]] 1
; SPV-DAG: %[[one_16:[0-9]+]] = OpConstant %[[int_16]] 1
; SPV-DAG: %[[one_64:[0-9]+]] = OpConstant %[[int_64]] 1 0
; SPV-DAG: %[[void:[0-9]+]] = OpTypeVoid
; SPV-DAG: %[[float:[0-9]+]] = OpTypeFloat 32
; SPV-DAG: %[[bool:[0-9]+]] = OpTypeBool
; SPV-DAG: %[[vec_8:[0-9]+]] = OpTypeVector %[[int_8]] 2
; SPV-DAG: %[[vec_1:[0-9]+]] = OpTypeVector %[[bool]] 2
; SPV-DAG: %[[vec_16:[0-9]+]] = OpTypeVector %[[int_16]] 2
; SPV-DAG: %[[vec_32:[0-9]+]] = OpTypeVector %[[int_32]] 2
; SPV-DAG: %[[vec_64:[0-9]+]] = OpTypeVector %[[int_64]] 2
; SPV-DAG: %[[vec_float:[0-9]+]] = OpTypeVector %[[float]] 2
; SPV-DAG: %[[zeros_8:[0-9]+]] = OpConstantNull %[[vec_8]]
; SPV-DAG: %[[mones_8:[0-9]+]] = OpConstantComposite %[[vec_8]] %[[mone_8]] %[[mone_8]]
; SPV-DAG: %[[zeros_16:[0-9]+]] = OpConstantNull %[[vec_16]]
; SPV-DAG: %[[mones_16:[0-9]+]] = OpConstantComposite %[[vec_16]] %[[mone_16]] %[[mone_16]]
; SPV-DAG: %[[zeros_32:[0-9]+]] = OpConstantNull %[[vec_32]]
; SPV-DAG: %[[mones_32:[0-9]+]] = OpConstantComposite %[[vec_32]] %[[mone_32]] %[[mone_32]]
; SPV-DAG: %[[zeros_64:[0-9]+]] = OpConstantNull %[[vec_64]]
; SPV-DAG: %[[mones_64:[0-9]+]] = OpConstantComposite %[[vec_64]] %[[mone_64]] %[[mone_64]]
; SPV-DAG: %[[ones_8:[0-9]+]] = OpConstantComposite %[[vec_8]] %[[one_8]] %[[one_8]]
; SPV-DAG: %[[ones_16:[0-9]+]] = OpConstantComposite %[[vec_16]] %[[one_16]] %[[one_16]]
; SPV-DAG: %[[ones_32:[0-9]+]] = OpConstantComposite %[[vec_32]] %[[one_32]] %[[one_32]]
; SPV-DAG: %[[ones_64:[0-9]+]] = OpConstantComposite %[[vec_64]] %[[one_64]] %[[one_64]]


target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

; SPV-DAG: OpFunction
; SPV-DAG: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; SPV-DAG: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; SPV-DAG: %[[i1s:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; SPV-DAG: %[[i1v:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}

; Function Attrs: nofree norecurse nounwind writeonly
define dso_local spir_kernel void @K(float addrspace(1)* nocapture %A, i32 %B, i1 %i1s, <2 x i1> %i1v) local_unnamed_addr #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:


; SPV-DAG: %[[cmp_res:[0-9]+]] = OpSGreaterThan %[[bool]] %[[B]] %[[zero_32]]
  %cmp = icmp sgt i32 %B, 0
; SPV-DAG: %[[select_res:[0-9]+]] = OpSelect %[[int_32]] %[[cmp_res]] %[[one_32]] %[[zero_32]]
; SPV-DAG: %[[utof_res:[0-9]+]] = OpConvertUToF %[[float]] %[[select_res]]
  %conv = uitofp i1 %cmp to float
; SPV-DAG: OpStore %[[A]] %[[utof_res]]
  store float %conv, float addrspace(1)* %A, align 4;

; SPV-DAG: %[[s1]] = OpSelect %[[int_8]] %[[i1s]] %[[mone_8]] %[[zero_8]]
  %s1 = sext i1 %i1s to i8
; SPV-DAG: %[[s2]] = OpSelect %[[int_16]] %[[i1s]] %[[mone_16]] %[[zero_16]]
  %s2 = sext i1 %i1s to i16
; SPV-DAG: %[[s3]] = OpSelect %[[int_32]] %[[i1s]] %[[mone_32]] %[[zero_32]]
  %s3 = sext i1 %i1s to i32
; SPV-DAG: %[[s4]] = OpSelect %[[int_64]] %[[i1s]] %[[mone_64]] %[[zero_64]]
  %s4 = sext i1 %i1s to i64
; SPV-DAG: %[[s5]] = OpSelect %[[vec_8]] %[[i1v]] %[[mones_8]] %[[zeros_8]]
  %s5 = sext <2 x i1> %i1v to <2 x i8>
; SPV-DAG: %[[s6]] = OpSelect %[[vec_16]] %[[i1v]] %[[mones_16]] %[[zeros_16]]
  %s6 = sext <2 x i1> %i1v to <2 x i16>
; SPV-DAG: %[[s7]] = OpSelect %[[vec_32]] %[[i1v]] %[[mones_32]] %[[zeros_32]]
  %s7 = sext <2 x i1> %i1v to <2 x i32>
; SPV-DAG: %[[s8]] = OpSelect %[[vec_64]] %[[i1v]] %[[mones_64]] %[[zeros_64]]
  %s8 = sext <2 x i1> %i1v to <2 x i64>
; SPV-DAG: %[[z1]] = OpSelect %[[int_8]] %[[i1s]] %[[one_8]] %[[zero_8]]
  %z1 = zext i1 %i1s to i8
; SPV-DAG: %[[z2]] = OpSelect %[[int_16]] %[[i1s]] %[[one_16]] %[[zero_16]]
  %z2 = zext i1 %i1s to i16
; SPV-DAG: %[[z3]] = OpSelect %[[int_32]] %[[i1s]] %[[one_32]] %[[zero_32]]
  %z3 = zext i1 %i1s to i32
; SPV-DAG: %[[z4]] = OpSelect %[[int_64]] %[[i1s]] %[[one_64]] %[[zero_64]]
  %z4 = zext i1 %i1s to i64
; SPV-DAG: %[[z5]] = OpSelect %[[vec_8]] %[[i1v]] %[[ones_8]] %[[zeros_8]]
  %z5 = zext <2 x i1> %i1v to <2 x i8>
; SPV-DAG: %[[z6]] = OpSelect %[[vec_16]] %[[i1v]] %[[ones_16]] %[[zeros_16]]
  %z6 = zext <2 x i1> %i1v to <2 x i16>
; SPV-DAG: %[[z7]] = OpSelect %[[vec_32]] %[[i1v]] %[[ones_32]] %[[zeros_32]]
  %z7 = zext <2 x i1> %i1v to <2 x i32>
; SPV-DAG: %[[z8]] = OpSelect %[[vec_64]] %[[i1v]] %[[ones_64]] %[[zeros_64]]
  %z8 = zext <2 x i1> %i1v to <2 x i64>
; SPV-DAG: %[[ufp1_res:[0-9]+]] = OpSelect %[[int_32]] %[[i1s]] %[[one_32]] %[[zero_32]]
; SPV-DAG: %[[ufp1]] = OpConvertUToF %[[float]] %[[ufp1_res]]
  %ufp1 = uitofp i1 %i1s to float
; SPV-DAG: %[[ufp2_res:[0-9]+]] = OpSelect %[[vec_32]] %[[i1v]] %[[ones_32]] %[[zeros_32]]
; SPV-DAG: %[[ufp2]] = OpConvertUToF %[[vec_float]] %[[ufp2_res]]
  %ufp2 = uitofp <2 x i1> %i1v to <2 x float>
; SPV-DAG: %[[sfp1_res:[0-9]+]] = OpSelect %[[int_32]] %[[i1s]] %[[one_32]] %[[zero_32]]
; SPV-DAG: %[[sfp1]] = OpConvertSToF %[[float]] %[[sfp1_res]]
  %sfp1 = sitofp i1 %i1s to float
; SPV-DAG: %[[sfp2_res:[0-9]+]] = OpSelect %[[vec_32]] %[[i1v]] %[[ones_32]] %[[zeros_32]]
; SPV-DAG: %[[sfp2]] = OpConvertSToF %[[vec_float]] %[[sfp2_res]]
  %sfp2 = sitofp <2 x i1> %i1v to <2 x float>
  ret void
}


attributes #0 = { nofree norecurse nounwind writeonly "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{i32 1, i32 0}
!3 = !{!"none", !"none"}
!4 = !{!"float*", !"int"}
!5 = !{!"", !""}
