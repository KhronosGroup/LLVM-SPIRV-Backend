; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'DivRem.cl'
source_filename = "DivRem.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV-DAG: %[[int:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[int2:[0-9]+]] = OpTypeVector %[[int]] 2
; CHECK-SPIRV-DAG: %[[float:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV-DAG: %[[float2:[0-9]+]] = OpTypeVector %[[float]] 2

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpSDiv %[[int2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; kernel void testSDiv(int2 a, int2 b, global int2 *res) {
;   res[0] = a / b;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define dso_local spir_kernel void @testSDiv(<2 x i32> noundef %a, <2 x i32> noundef %b, <2 x i32> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %div = sdiv <2 x i32> %a, %b
  store <2 x i32> %div, <2 x i32> addrspace(1)* %res, align 8, !tbaa !8
  ret void
}

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpUDiv %[[int2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; kernel void testUDiv(uint2 a, uint2 b, global uint2 *res) {
;   res[0] = a / b;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define dso_local spir_kernel void @testUDiv(<2 x i32> noundef %a, <2 x i32> noundef %b, <2 x i32> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !12 !kernel_arg_type_qual !7 {
entry:
  %div = udiv <2 x i32> %a, %b
  store <2 x i32> %div, <2 x i32> addrspace(1)* %res, align 8, !tbaa !8
  ret void
}

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpFDiv %[[float2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; kernel void testFDiv(float2 a, float2 b, global float2 *res) {
;   res[0] = a / b;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define dso_local spir_kernel void @testFDiv(<2 x float> noundef %a, <2 x float> noundef %b, <2 x float> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !13 !kernel_arg_base_type !14 !kernel_arg_type_qual !7 {
entry:
  %div = fdiv <2 x float> %a, %b, !fpmath !15
  store <2 x float> %div, <2 x float> addrspace(1)* %res, align 8, !tbaa !8
  ret void
}

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpSRem %[[int2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; kernel void testSRem(int2 a, int2 b, global int2 *res) {
;   res[0] = a % b;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define dso_local spir_kernel void @testSRem(<2 x i32> noundef %a, <2 x i32> noundef %b, <2 x i32> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %rem = srem <2 x i32> %a, %b
  store <2 x i32> %rem, <2 x i32> addrspace(1)* %res, align 8, !tbaa !8
  ret void
}

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpUMod %[[int2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; kernel void testUMod(uint2 a, uint2 b, global uint2 *res) {
;   res[0] = a % b;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define dso_local spir_kernel void @testUMod(<2 x i32> noundef %a, <2 x i32> noundef %b, <2 x i32> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !12 !kernel_arg_type_qual !7 {
entry:
  %rem = urem <2 x i32> %a, %b
  store <2 x i32> %rem, <2 x i32> addrspace(1)* %res, align 8, !tbaa !8
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn writeonly "frame-pointer"="none" "min-legal-vector-width"="64" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 0, i32 0, i32 1}
!4 = !{!"none", !"none", !"none"}
!5 = !{!"int2", !"int2", !"int2*"}
!6 = !{!"int __attribute__((ext_vector_type(2)))", !"int __attribute__((ext_vector_type(2)))", !"int __attribute__((ext_vector_type(2)))*"}
!7 = !{!"", !"", !""}
!8 = !{!9, !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!"uint2", !"uint2", !"uint2*"}
!12 = !{!"uint __attribute__((ext_vector_type(2)))", !"uint __attribute__((ext_vector_type(2)))", !"uint __attribute__((ext_vector_type(2)))*"}
!13 = !{!"float2", !"float2", !"float2*"}
!14 = !{!"float __attribute__((ext_vector_type(2)))", !"float __attribute__((ext_vector_type(2)))", !"float __attribute__((ext_vector_type(2)))*"}
!15 = !{float 2.500000e+00}
