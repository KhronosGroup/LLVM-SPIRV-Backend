; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; void __kernel sample_kernel_read( __global float4 *results,
;     read_only image2d_t image,
;     sampler_t imageSampler,
;     float2 coord,
;     float2 dx,
;     float2 dy)
; {
;   *results = read_imagef( image, imageSampler, coord);
;   *results = read_imagef( image, imageSampler, coord, 3.14f);
;   *results = read_imagef( image, imageSampler, coord, dx, dy);
; }

; ModuleID = 'OpImageSampleExplicitLod_arg.cl'
source_filename = "OpImageSampleExplicitLod_arg.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: %[[float:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV: %[[lodNull:[0-9]+]] = OpConstant %[[float]] 0
; CHECK-SPIRV: %[[lod:[0-9]+]] = OpConstant %[[float]] 1078523331
; CHECK-SPIRV: OpFunctionParameter
; CHECK-SPIRV: OpFunctionParameter
; CHECK-SPIRV: OpFunctionParameter
; CHECK-SPIRV: OpFunctionParameter
; CHECK-SPIRV: %[[dx:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %[[dy:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}

; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} Lod %[[lodNull]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} Lod %[[lod]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} Grad %[[dx]] %[[dy]]

%opencl.image2d_ro_t = type opaque
%opencl.sampler_t = type opaque

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @sample_kernel_read(<4 x float> addrspace(1)* nocapture noundef writeonly %results, %opencl.image2d_ro_t addrspace(1)* %image, %opencl.sampler_t addrspace(2)* %imageSampler, <2 x float> noundef %coord, <2 x float> noundef %dx, <2 x float> noundef %dy) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %call = call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)* %image, %opencl.sampler_t addrspace(2)* %imageSampler, <2 x float> noundef %coord) #2
  store <4 x float> %call, <4 x float> addrspace(1)* %results, align 16, !tbaa !8
  %call1 = call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_ff(%opencl.image2d_ro_t addrspace(1)* %image, %opencl.sampler_t addrspace(2)* %imageSampler, <2 x float> noundef %coord, float noundef 0x40091EB860000000) #2
  store <4 x float> %call1, <4 x float> addrspace(1)* %results, align 16, !tbaa !8
  %call2 = call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_fS1_S1_(%opencl.image2d_ro_t addrspace(1)* %image, %opencl.sampler_t addrspace(2)* %imageSampler, <2 x float> noundef %coord, <2 x float> noundef %dx, <2 x float> noundef %dy) #2
  store <4 x float> %call2, <4 x float> addrspace(1)* %results, align 16, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <2 x float> noundef) local_unnamed_addr #1

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_ff(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <2 x float> noundef, float noundef) local_unnamed_addr #1

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_fS1_S1_(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <2 x float> noundef, <2 x float> noundef, <2 x float> noundef) local_unnamed_addr #1

attributes #0 = { convergent mustprogress nofree norecurse nounwind willreturn "frame-pointer"="none" "min-legal-vector-width"="128" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent mustprogress nofree nounwind readonly willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind readonly willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 1, i32 0, i32 0, i32 0, i32 0}
!4 = !{!"none", !"read_only", !"none", !"none", !"none", !"none"}
!5 = !{!"float4*", !"image2d_t", !"sampler_t", !"float2", !"float2", !"float2"}
!6 = !{!"float __attribute__((ext_vector_type(4)))*", !"image2d_t", !"sampler_t", !"float __attribute__((ext_vector_type(2)))", !"float __attribute__((ext_vector_type(2)))", !"float __attribute__((ext_vector_type(2)))"}
!7 = !{!"", !"", !"", !"", !"", !""}
!8 = !{!9, !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
