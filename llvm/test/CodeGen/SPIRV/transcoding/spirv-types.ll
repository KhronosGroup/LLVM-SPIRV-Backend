;; Test SPIR-V opaque types
;;
; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpCapability Float16
; CHECK-SPIRV: OpCapability ImageBasic
; CHECK-SPIRV: OpCapability ImageReadWrite
; CHECK-SPIRV: OpCapability Pipes
; CHECK-SPIRV: OpCapability DeviceEnqueue

; CHECK-SPIRV-DAG: %[[VOID:[0-9]+]] = OpTypeVoid
; CHECK-SPIRV-DAG: %[[INT:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[HALF:[0-9]+]] = OpTypeFloat 16
; CHECK-SPIRV-DAG: %[[FLOAT:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV-DAG: %[[PIPE_RD:[0-9]+]] = OpTypePipe 0
; CHECK-SPIRV-DAG: %[[PIPE_WR:[0-9]+]] = OpTypePipe 1
; CHECK-SPIRV-DAG: %[[IMG1D_RD:[0-9]+]] = OpTypeImage %[[VOID]] 1D 0 0 0 0 Unknown ReadOnly
; CHECK-SPIRV-DAG: %[[IMG2D_RD:[0-9]+]] = OpTypeImage %[[INT]] 2D 0 0 0 0 Unknown ReadOnly
; CHECK-SPIRV-DAG: %[[IMG3D_RD:[0-9]+]] = OpTypeImage %[[INT]] 3D 0 0 0 0 Unknown ReadOnly
; CHECK-SPIRV-DAG: %[[IMG2DD_RD:[0-9]+]] = OpTypeImage %[[FLOAT]] 2D 1 0 0 0 Unknown ReadOnly
; CHECK-SPIRV-DAG: %[[IMG2DA_RD:[0-9]+]] = OpTypeImage %[[HALF]] 2D 0 1 0 0 Unknown ReadOnly
; CHECK-SPIRV-DAG: %[[IMG1DB_RD:[0-9]+]] = OpTypeImage %[[FLOAT]] Buffer 0 0 0 0 Unknown ReadOnly
; CHECK-SPIRV-DAG: %[[IMG1D_WR:[0-9]+]] = OpTypeImage %[[VOID]] 1D 0 0 0 0 Unknown WriteOnly
; CHECK-SPIRV-DAG: %[[IMG2D_RW:[0-9]+]] = OpTypeImage %[[VOID]] 2D 0 0 0 0 Unknown ReadWrite
; CHECK-SPIRV-DAG: %[[DEVEVENT:[0-9]+]] = OpTypeDeviceEvent
; CHECK-SPIRV-DAG: %[[EVENT:[0-9]+]] = OpTypeEvent
; CHECK-SPIRV-DAG: %[[QUEUE:[0-9]+]] = OpTypeQueue
; CHECK-SPIRV-DAG: %[[RESID:[0-9]+]] = OpTypeReserveId
; CHECK-SPIRV-DAG: %[[SAMP:[0-9]+]] = OpTypeSampler
; CHECK-SPIRV-DAG: %[[SAMPIMG:[0-9]+]] = OpTypeSampledImage %[[IMG2DD_RD]]

; ModuleID = 'cl-types.cl'
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%spirv.Pipe._0 = type opaque ; read_only pipe
%spirv.Pipe._1 = type opaque ; write_only pipe
%spirv.Image._void_0_0_0_0_0_0_0 = type opaque ; read_only image1d_ro_t
%spirv.Image._int_1_0_0_0_0_0_0 = type opaque ; read_only image2d_ro_t
%spirv.Image._uint_2_0_0_0_0_0_0 = type opaque ; read_only image3d_ro_t
%spirv.Image._float_1_1_0_0_0_0_0 = type opaque; read_only image2d_depth_ro_t
%spirv.Image._half_1_0_1_0_0_0_0 = type opaque ; read_only image2d_array_ro_t
%spirv.Image._float_5_0_0_0_0_0_0 = type opaque ; read_only image1d_buffer_ro_t
%spirv.Image._void_0_0_0_0_0_0_1 = type opaque ; write_only image1d_wo_t
%spirv.Image._void_1_0_0_0_0_0_2 = type opaque ; read_write image2d_rw_t
%spirv.DeviceEvent          = type opaque ; clk_event_t
%spirv.Event                = type opaque ; event_t
%spirv.Queue                = type opaque ; queue_t
%spirv.ReserveId            = type opaque ; reserve_id_t
%spirv.Sampler              = type opaque ; sampler_t
%spirv.SampledImage._float_1_1_0_0_0_0_0 = type opaque

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[PIPE_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[PIPE_WR]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG1D_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG2D_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG3D_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG2DA_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG1DB_RD]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG1D_WR]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[IMG2D_RW]]

; Function Attrs: nounwind readnone
define spir_kernel void @foo(
  %spirv.Pipe._0 addrspace(1)* nocapture %a,
  %spirv.Pipe._1 addrspace(1)* nocapture %b,
  %spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)* nocapture %c1,
  %spirv.Image._int_1_0_0_0_0_0_0 addrspace(1)* nocapture %d1,
  %spirv.Image._uint_2_0_0_0_0_0_0 addrspace(1)* nocapture %e1,
  %spirv.Image._half_1_0_1_0_0_0_0 addrspace(1)* nocapture %f1,
  %spirv.Image._float_5_0_0_0_0_0_0 addrspace(1)* nocapture %g1,
  %spirv.Image._void_0_0_0_0_0_0_1 addrspace(1)* nocapture %c2,
  %spirv.Image._void_1_0_0_0_0_0_2 addrspace(1)* nocapture %d3) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  ret void
}

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[DEVEVENT]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[EVENT]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[QUEUE]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionParameter %[[RESID]]

define spir_func void @bar(
  %spirv.DeviceEvent * %a,
  %spirv.Event * %b,
  %spirv.Queue * %c,
  %spirv.ReserveId * %d) {
  ret void
}

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %[[IMG_ARG:[0-9]+]] = OpFunctionParameter %[[IMG2DD_RD]]
; CHECK-SPIRV: %[[SAMP_ARG:[0-9]+]] = OpFunctionParameter %[[SAMP]]
; CHECK-SPIRV: %[[SAMPIMG_VAR:[0-9]+]] = OpSampledImage %[[SAMPIMG]] %[[IMG_ARG]] %[[SAMP_ARG]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SAMPIMG_VAR]]

define spir_func void @test_sampler(%spirv.Image._float_1_1_0_0_0_0_0 addrspace(1)* %srcimg.coerce,
                                    %spirv.Sampler addrspace(1)* %s.coerce) {
  %1 = tail call spir_func %spirv.SampledImage._float_1_1_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS1K34__spirv_Image__float_1_1_0_0_0_0_0PU3AS1K15__spirv_Sampler(%spirv.Image._float_1_1_0_0_0_0_0 addrspace(1)* %srcimg.coerce, %spirv.Sampler addrspace(1)* %s.coerce) #1
  %2 = tail call spir_func <4 x float> @_Z38__spirv_ImageSampleExplicitLod_Rfloat4PU3AS120__spirv_SampledImageDv4_iif(%spirv.SampledImage._float_1_1_0_0_0_0_0 addrspace(1)* %1, <4 x i32> zeroinitializer, i32 2, float 1.000000e+00) #1
  ret void
}

declare spir_func %spirv.SampledImage._float_1_1_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS1K34__spirv_Image__float_1_1_0_0_0_0_0PU3AS1K15__spirv_Sampler(%spirv.Image._float_1_1_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(1)*)

declare spir_func <4 x float> @_Z38__spirv_ImageSampleExplicitLod_Rfloat4PU3AS120__spirv_SampledImageDv4_iif(%spirv.SampledImage._float_1_1_0_0_0_0_0 addrspace(1)*, <4 x i32>, i32, float)

attributes #0 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!9}
!opencl.compiler.options = !{!8}

!1 = !{i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1}
!2 = !{!"read_only", !"write_only", !"read_only", !"read_only", !"read_only", !"read_only", !"read_only", !"write_only", !"read_write"}
!3 = !{!"int", !"int", !"image1d_t", !"image2d_t", !"image3d_t", !"image2d_array_t", !"image1d_buffer_t", !"image1d_t", !"image2d_t"}
!4 = !{!"int", !"int", !"image1d_t", !"image2d_t", !"image3d_t", !"image2d_array_t", !"image1d_buffer_t", !"image1d_t", !"image2d_t"}
!5 = !{!"pipe", !"pipe", !"", !"", !"", !"", !"", !"", !""}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 0}
!8 = !{!"cl_khr_fp16"}
!9 = !{!"cl_images"}
