;; Test CL opaque types
;;
;; // cl-types.cl
;; // CL source code for generating LLVM IR.
;; // Command for compilation:
;; //  clang -cc1 -x cl -cl-std=CL2.0 -triple spir-unknonw-unknown -emit-llvm cl-types.cl
;; void kernel foo(
;;  read_only pipe int a,
;;  write_only pipe int b,
;;  read_only image1d_t c1,
;;  read_only image2d_t d1,
;;  read_only image3d_t e1,
;;  read_only image2d_array_t f1,
;;  read_only image1d_buffer_t g1,
;;  write_only image1d_t c2,
;;  read_write image2d_t d3,
;;  sampler_t s
;; ){
;; }

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV-DAG: OpCapability Sampled1D
; CHECK-SPIRV-DAG: OpCapability SampledBuffer
; CHECK-SPIRV-DAG: %[[VOID:[0-9]+]] = OpTypeVoid
; CHECK-SPIRV-DAG: %[[PIPE_RD:[0-9]+]] = OpTypePipe 0
; CHECK-SPIRV-DAG: %[[PIPE_WR:[0-9]+]] = OpTypePipe 1
; CHECK-SPIRV-DAG: %[[IMG1D_RD:[0-9]+]] = OpTypeImage %[[VOID]] 0 0 0 0 0 0 0
; CHECK-SPIRV-DAG: %[[IMG2D_RD:[0-9]+]] = OpTypeImage %[[VOID]] 1 0 0 0 0 0 0
; CHECK-SPIRV-DAG: %[[IMG3D_RD:[0-9]+]] = OpTypeImage %[[VOID]] 2 0 0 0 0 0 0
; CHECK-SPIRV-DAG: %[[IMG2DA_RD:[0-9]+]] = OpTypeImage %[[VOID]] 1 0 1 0 0 0 0
; CHECK-SPIRV-DAG: %[[IMG1DB_RD:[0-9]+]] = OpTypeImage %[[VOID]] 5 0 0 0 0 0 0
; CHECK-SPIRV-DAG: %[[IMG1D_WR:[0-9]+]] = OpTypeImage %[[VOID]] 0 0 0 0 0 0 1
; CHECK-SPIRV-DAG: %[[IMG2D_RW:[0-9]+]] = OpTypeImage %[[VOID]] 1 0 0 0 0 0 2
; CHECK-SPIRV-DAG: %[[SAMP:[0-9]+]] = OpTypeSampler
; CHECK-SPIRV-DAG: %[[SAMPIMG:[0-9]+]] = OpTypeSampledImage %[[IMG2D_RD]]

; CHECK-SPIRV: %[[SAMP_CONST:[0-9]+]] = OpConstantSampler %[[SAMP]] 0 0 1

; ModuleID = 'cl-types.cl'
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%opencl.pipe_ro_t = type opaque
%opencl.pipe_wo_t = type opaque
%opencl.image3d_ro_t = type opaque
%opencl.image2d_array_ro_t = type opaque
%opencl.image1d_buffer_ro_t = type opaque
%opencl.image1d_ro_t = type opaque
%opencl.image1d_wo_t = type opaque
%opencl.image2d_rw_t = type opaque
%opencl.image2d_ro_t = type opaque
%opencl.sampler_t = type opaque

; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[PIPE_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[PIPE_WR]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG1D_RD]]
; CHECK-SPIRV: %[[IMG_ARG:[0-9]+]] = OpFunctionParameter %[[IMG2D_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG3D_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG2DA_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG1DB_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG1D_WR]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG2D_RW]]
; CHECK-SPIRV: %[[SAMP_ARG:[0-9]+]] = OpFunctionParameter %[[SAMP]]

; Function Attrs: nounwind readnone
define spir_kernel void @foo(
  %opencl.pipe_ro_t addrspace(1)* nocapture %a,
  %opencl.pipe_wo_t addrspace(1)* nocapture %b,
  %opencl.image1d_ro_t addrspace(1)* nocapture %c1,
  %opencl.image2d_ro_t addrspace(1)* nocapture %d1,
  %opencl.image3d_ro_t addrspace(1)* nocapture %e1,
  %opencl.image2d_array_ro_t addrspace(1)* nocapture %f1,
  %opencl.image1d_buffer_ro_t addrspace(1)* nocapture %g1,
  %opencl.image1d_wo_t addrspace(1)* nocapture %c2,
  %opencl.image2d_rw_t addrspace(1)* nocapture %d3,
  %opencl.sampler_t addrspace(2)* %s) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
; CHECK-SPIRV: %[[SAMPIMG_VAR1:[0-9]+]] = OpSampledImage %[[SAMPIMG]] %[[IMG_ARG]] %[[SAMP_ARG]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SAMPIMG_VAR1]]
  %.tmp = call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv4_if(%opencl.image2d_ro_t addrspace(1)* %d1, %opencl.sampler_t addrspace(2)* %s, <4 x i32> zeroinitializer, float 1.000000e+00)

; CHECK-SPIRV: %[[SAMPIMG_VAR2:[0-9]+]] = OpSampledImage %[[SAMPIMG]] %[[IMG_ARG]] %[[SAMP_CONST]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SAMPIMG_VAR2]]
  %0 = call %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32 32)
  %.tmp2 = call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv4_if(%opencl.image2d_ro_t addrspace(1)* %d1, %opencl.sampler_t addrspace(2)* %0, <4 x i32> zeroinitializer, float 1.000000e+00)
  ret void
}

declare spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv4_if(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <4 x i32>, float) #1

declare %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32)

attributes #0 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!9}
!opencl.compiler.options = !{!8}

!1 = !{i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 0}
!2 = !{!"read_only", !"write_only", !"read_only", !"read_only", !"read_only", !"read_only", !"read_only", !"write_only", !"read_write", !"none"}
!3 = !{!"int", !"int", !"image1d_t", !"image2d_t", !"image3d_t", !"image2d_array_t", !"image1d_buffer_t", !"image1d_t", !"image2d_t", !"sampler_t"}
!4 = !{!"int", !"int", !"image1d_t", !"image2d_t", !"image3d_t", !"image2d_array_t", !"image1d_buffer_t", !"image1d_t", !"image2d_t", !"sampler_t"}
!5 = !{!"pipe", !"pipe", !"", !"", !"", !"", !"", !"", !"", !""}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 0}
!8 = !{}
!9 = !{!"cl_images"}
