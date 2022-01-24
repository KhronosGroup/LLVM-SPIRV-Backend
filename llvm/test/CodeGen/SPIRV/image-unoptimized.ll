; RUN: llc -O0 %s -o - | FileCheck %s

; ModuleID = 'image-unoptimized.cl'
source_filename = "image-unoptimized.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK: %[[TypeImage:[0-9]+]] = OpTypeImage
; CHECK: %[[TypeSampler:[0-9]+]] = OpTypeSampler
; CHECK-DAG: %[[TypeImagePtr:[0-9]+]] = OpTypePointer {{.*}} %[[TypeImage]]
; CHECK-DAG: %[[TypeSamplerPtr:[0-9]+]] = OpTypePointer {{.*}} %[[TypeSampler]]

; CHECK: %[[srcimg:[0-9]+]] = OpFunctionParameter %[[TypeImage]]
; CHECK: %[[sampler:[0-9]+]] = OpFunctionParameter %[[TypeSampler]]

; CHECK: %[[srcimg_addr:[0-9]+]] = OpVariable %[[TypeImagePtr]]
; CHECK: %[[sampler_addr:[0-9]+]] = OpVariable %[[TypeSamplerPtr]]

; CHECK: OpStore %[[srcimg_addr]] %[[srcimg]]
; CHECK: OpStore %[[sampler_addr]] %[[sampler]]

; CHECK: %[[srcimg_val:[0-9]+]] = OpLoad %{{[0-9]+}} %[[srcimg_addr]]
; CHECK: %[[sampler_val:[0-9]+]] = OpLoad %{{[0-9]+}} %[[sampler_addr]]

; CHECK: %{{[0-9]+}} = OpSampledImage %{{[0-9]+}} %[[srcimg_val]] %[[sampler_val]]
; CHECK-NEXT: OpImageSampleExplicitLod

; CHECK: %[[srcimg_val:[0-9]+]] = OpLoad %{{[0-9]+}} %[[srcimg_addr]]
; CHECK: %{{[0-9]+}} = OpImageQuerySizeLod %{{[0-9]+}} %[[srcimg_val]]

; Excerpt from opencl-c-base.h
; typedef float float4 __attribute__((ext_vector_type(4)));
; typedef int int2 __attribute__((ext_vector_type(2)));
; typedef __SIZE_TYPE__ size_t;
;
; Excerpt from opencl-c.h to speed up compilation.
; #define __ovld __attribute__((overloadable))
; #define __purefn __attribute__((pure))
; #define __cnfn __attribute__((const))
; size_t __ovld __cnfn get_global_id(unsigned int dimindx);
; int __ovld __cnfn get_image_width(read_only image2d_t image);
; float4 __purefn __ovld read_imagef(read_only image2d_t image, sampler_t sampler, int2 coord);
;
;
; __kernel void test_fn(image2d_t srcimg, sampler_t sampler, global float4 *results) {
;   int tid_x = get_global_id(0);
;   int tid_y = get_global_id(1);
;   results[tid_x + tid_y * get_image_width(srcimg)] = read_imagef(srcimg, sampler, (int2){tid_x, tid_y});
; }

%opencl.image2d_ro_t = type opaque
%opencl.sampler_t = type opaque

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @test_fn(%opencl.image2d_ro_t addrspace(1)* %srcimg, %opencl.sampler_t addrspace(2)* %sampler, <4 x float> addrspace(1)* noundef %results) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %srcimg.addr = alloca %opencl.image2d_ro_t addrspace(1)*, align 4
  %sampler.addr = alloca %opencl.sampler_t addrspace(2)*, align 4
  %results.addr = alloca <4 x float> addrspace(1)*, align 4
  %tid_x = alloca i32, align 4
  %tid_y = alloca i32, align 4
  %.compoundliteral = alloca <2 x i32>, align 8
  store %opencl.image2d_ro_t addrspace(1)* %srcimg, %opencl.image2d_ro_t addrspace(1)** %srcimg.addr, align 4
  store %opencl.sampler_t addrspace(2)* %sampler, %opencl.sampler_t addrspace(2)** %sampler.addr, align 4
  store <4 x float> addrspace(1)* %results, <4 x float> addrspace(1)** %results.addr, align 4
  %call = call spir_func i32 @_Z13get_global_idj(i32 noundef 0) #3
  store i32 %call, i32* %tid_x, align 4
  %call1 = call spir_func i32 @_Z13get_global_idj(i32 noundef 1) #3
  store i32 %call1, i32* %tid_y, align 4
  %0 = load %opencl.image2d_ro_t addrspace(1)*, %opencl.image2d_ro_t addrspace(1)** %srcimg.addr, align 4
  %1 = load %opencl.sampler_t addrspace(2)*, %opencl.sampler_t addrspace(2)** %sampler.addr, align 4
  %2 = load i32, i32* %tid_x, align 4
  %vecinit = insertelement <2 x i32> undef, i32 %2, i32 0
  %3 = load i32, i32* %tid_y, align 4
  %vecinit2 = insertelement <2 x i32> %vecinit, i32 %3, i32 1
  store <2 x i32> %vecinit2, <2 x i32>* %.compoundliteral, align 8
  %4 = load <2 x i32>, <2 x i32>* %.compoundliteral, align 8
  %call3 = call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_i(%opencl.image2d_ro_t addrspace(1)* %0, %opencl.sampler_t addrspace(2)* %1, <2 x i32> noundef %4) #4
  %5 = load <4 x float> addrspace(1)*, <4 x float> addrspace(1)** %results.addr, align 4
  %6 = load i32, i32* %tid_x, align 4
  %7 = load i32, i32* %tid_y, align 4
  %8 = load %opencl.image2d_ro_t addrspace(1)*, %opencl.image2d_ro_t addrspace(1)** %srcimg.addr, align 4
  %call4 = call spir_func i32 @_Z15get_image_width14ocl_image2d_ro(%opencl.image2d_ro_t addrspace(1)* %8) #3
  %mul = mul nsw i32 %7, %call4
  %add = add nsw i32 %6, %mul
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %5, i32 %add
  store <4 x float> %call3, <4 x float> addrspace(1)* %arrayidx, align 16
  ret void
}

; Function Attrs: convergent nounwind readnone willreturn
declare spir_func i32 @_Z13get_global_idj(i32 noundef) #1

; Function Attrs: convergent nounwind readonly willreturn
declare spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_i(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <2 x i32> noundef) #2

; Function Attrs: convergent nounwind readnone willreturn
declare spir_func i32 @_Z15get_image_width14ocl_image2d_ro(%opencl.image2d_ro_t addrspace(1)*) #1

attributes #0 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="128" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="true" }
attributes #1 = { convergent nounwind readnone willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind readonly willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { convergent nounwind readnone willreturn }
attributes #4 = { convergent nounwind readonly willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 0, i32 1}
!4 = !{!"read_only", !"none", !"none"}
!5 = !{!"image2d_t", !"sampler_t", !"float4*"}
!6 = !{!"image2d_t", !"sampler_t", !"float __attribute__((ext_vector_type(4)))*"}
!7 = !{!"", !"", !""}
