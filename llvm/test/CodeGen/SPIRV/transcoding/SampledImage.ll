; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; constant sampler_t constSampl = CLK_FILTER_LINEAR;
;
; __kernel
; void sample_kernel_float(image2d_t input, float2 coords, global float4 *results, sampler_t argSampl) {
;   *results = read_imagef(input, constSampl, coords);
;   *results = read_imagef(input, argSampl, coords);
;   *results = read_imagef(input, CLK_FILTER_NEAREST|CLK_ADDRESS_REPEAT, coords);
; }
;
; __kernel
; void sample_kernel_int(image2d_t input, float2 coords, global int4 *results, sampler_t argSampl) {
;   *results = read_imagei(input, constSampl, coords);
;   *results = read_imagei(input, argSampl, coords);
;   *results = read_imagei(input, CLK_FILTER_NEAREST|CLK_ADDRESS_REPEAT, coords);
; }

; ModuleID = 'SampledImage.cl'
source_filename = "SampledImage.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%opencl.image2d_ro_t = type opaque
%opencl.sampler_t = type opaque

; CHECK-SPIRV: OpCapability LiteralSampler
; CHECK-SPIRV: OpEntryPoint Kernel %[[sample_kernel_float:[0-9]+]] "sample_kernel_float"
; CHECK-SPIRV: OpEntryPoint Kernel %[[sample_kernel_int:[0-9]+]] "sample_kernel_int"

; CHECK-SPIRV: %[[TypeSampler:[0-9]+]] = OpTypeSampler
; CHECK-SPIRV-DAG: %[[SampledImageTy:[0-9]+]] = OpTypeSampledImage
; CHECK-SPIRV-DAG: %[[ConstSampler1:[0-9]+]] = OpConstantSampler %[[TypeSampler]] None 0 Linear
; CHECK-SPIRV-DAG: %[[ConstSampler2:[0-9]+]] = OpConstantSampler %[[TypeSampler]] Repeat 0 Nearest
; CHECK-SPIRV-DAG: %[[ConstSampler3:[0-9]+]] = OpConstantSampler %[[TypeSampler]] None 0 Linear
; CHECK-SPIRV-DAG: %[[ConstSampler4:[0-9]+]] = OpConstantSampler %[[TypeSampler]] Repeat 0 Nearest

; CHECK-SPIRV: %[[sample_kernel_float]] = OpFunction %{{.*}}
; CHECK-SPIRV: %[[InputImage:[0-9]+]] = OpFunctionParameter %{{.*}}
; CHECK-SPIRV: %[[argSampl:[0-9]+]] = OpFunctionParameter %[[TypeSampler]]

; CHECK-SPIRV: %[[SampledImage1:[0-9]+]] = OpSampledImage %[[SampledImageTy]] %[[InputImage]] %[[ConstSampler1]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SampledImage1]]

; CHECK-SPIRV: %[[SampledImage2:[0-9]+]] = OpSampledImage %[[SampledImageTy]] %[[InputImage]] %[[argSampl]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SampledImage2]]

; CHECK-SPIRV: %[[SampledImage3:[0-9]+]] = OpSampledImage %[[SampledImageTy]] %[[InputImage]] %[[ConstSampler2]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SampledImage3]]

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @sample_kernel_float(%opencl.image2d_ro_t addrspace(1)* %input, <2 x float> noundef %coords, <4 x float> addrspace(1)* nocapture noundef writeonly %results, %opencl.sampler_t addrspace(2)* %argSampl) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %0 = tail call spir_func %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32 32) #2
  %call = tail call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)* %input, %opencl.sampler_t addrspace(2)* %0, <2 x float> noundef %coords) #3
  store <4 x float> %call, <4 x float> addrspace(1)* %results, align 16, !tbaa !8
  %call1 = tail call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)* %input, %opencl.sampler_t addrspace(2)* %argSampl, <2 x float> noundef %coords) #3
  store <4 x float> %call1, <4 x float> addrspace(1)* %results, align 16, !tbaa !8
  %1 = tail call spir_func %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32 22) #2
  %call2 = tail call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)* %input, %opencl.sampler_t addrspace(2)* %1, <2 x float> noundef %coords) #3
  store <4 x float> %call2, <4 x float> addrspace(1)* %results, align 16, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <2 x float> noundef) local_unnamed_addr #1

declare spir_func %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32) local_unnamed_addr

; CHECK-SPIRV: %[[sample_kernel_int]] = OpFunction %{{.*}}
; CHECK-SPIRV: %[[InputImage:[0-9]+]] = OpFunctionParameter %{{.*}}
; CHECK-SPIRV: %[[argSampl:[0-9]+]] = OpFunctionParameter %[[TypeSampler]]

; CHECK-SPIRV: %[[SampledImage4:[0-9]+]] = OpSampledImage %[[SampledImageTy]] %[[InputImage]] %[[ConstSampler3]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SampledImage4]]

; CHECK-SPIRV: %[[SampledImage5:[0-9]+]] = OpSampledImage %[[SampledImageTy]] %[[InputImage]] %[[argSampl]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SampledImage5]]

; CHECK-SPIRV: %[[SampledImage6:[0-9]+]] = OpSampledImage %[[SampledImageTy]] %[[InputImage]] %[[ConstSampler4]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[SampledImage6]]

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @sample_kernel_int(%opencl.image2d_ro_t addrspace(1)* %input, <2 x float> noundef %coords, <4 x i32> addrspace(1)* nocapture noundef writeonly %results, %opencl.sampler_t addrspace(2)* %argSampl) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !12 !kernel_arg_type_qual !7 {
entry:
  %0 = tail call spir_func %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32 32) #2
  %call = tail call spir_func <4 x i32> @_Z11read_imagei14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)* %input, %opencl.sampler_t addrspace(2)* %0, <2 x float> noundef %coords) #3
  store <4 x i32> %call, <4 x i32> addrspace(1)* %results, align 16, !tbaa !8
  %call1 = tail call spir_func <4 x i32> @_Z11read_imagei14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)* %input, %opencl.sampler_t addrspace(2)* %argSampl, <2 x float> noundef %coords) #3
  store <4 x i32> %call1, <4 x i32> addrspace(1)* %results, align 16, !tbaa !8
  %1 = tail call spir_func %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32 22) #2
  %call2 = tail call spir_func <4 x i32> @_Z11read_imagei14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)* %input, %opencl.sampler_t addrspace(2)* %1, <2 x float> noundef %coords) #3
  store <4 x i32> %call2, <4 x i32> addrspace(1)* %results, align 16, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare spir_func <4 x i32> @_Z11read_imagei14ocl_image2d_ro11ocl_samplerDv2_f(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <2 x float> noundef) local_unnamed_addr #1

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="128" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent mustprogress nofree nounwind readonly willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nounwind }
attributes #3 = { convergent nounwind readonly willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 0, i32 1, i32 0}
!4 = !{!"read_only", !"none", !"none", !"none"}
!5 = !{!"image2d_t", !"float2", !"float4*", !"sampler_t"}
!6 = !{!"image2d_t", !"float __attribute__((ext_vector_type(2)))", !"float __attribute__((ext_vector_type(4)))*", !"sampler_t"}
!7 = !{!"", !"", !"", !""}
!8 = !{!9, !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!"image2d_t", !"float2", !"int4*", !"sampler_t"}
!12 = !{!"image2d_t", !"float __attribute__((ext_vector_type(2)))", !"int __attribute__((ext_vector_type(4)))*", !"sampler_t"}
