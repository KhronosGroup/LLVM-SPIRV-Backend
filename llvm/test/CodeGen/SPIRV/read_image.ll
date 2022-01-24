; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'read_image.cl'
source_filename = "read_image.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV: %[[IntTy:[0-9]+]] = OpTypeInt
; CHECK-SPIRV: %[[IVecTy:[0-9]+]] = OpTypeVector %[[IntTy]]
; CHECK-SPIRV: %[[FloatTy:[0-9]+]] = OpTypeFloat
; CHECK-SPIRV: %[[FVecTy:[0-9]+]] = OpTypeVector %[[FloatTy]]
; CHECK-SPIRV: OpImageRead %[[IVecTy]]
; CHECK-SPIRV: OpImageRead %[[FVecTy]]

; __kernel void kernelA(__read_only image3d_t input) {
;   uint4 c = read_imageui(input, (int4)(0, 0, 0, 0));
; }
;
; __kernel void kernelB(__read_only image3d_t input) {
;   float4 f = read_imagef(input, (int4)(0, 0, 0, 0));
; }

%opencl.image3d_ro_t = type opaque

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @kernelA(%opencl.image3d_ro_t addrspace(1)* %input) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %input.addr = alloca %opencl.image3d_ro_t addrspace(1)*, align 8
  %c = alloca <4 x i32>, align 16
  %.compoundliteral = alloca <4 x i32>, align 16
  store %opencl.image3d_ro_t addrspace(1)* %input, %opencl.image3d_ro_t addrspace(1)** %input.addr, align 8
  %0 = load %opencl.image3d_ro_t addrspace(1)*, %opencl.image3d_ro_t addrspace(1)** %input.addr, align 8
  store <4 x i32> zeroinitializer, <4 x i32>* %.compoundliteral, align 16
  %1 = load <4 x i32>, <4 x i32>* %.compoundliteral, align 16
  %call = call spir_func <4 x i32> @_Z12read_imageui14ocl_image3d_roDv4_i(%opencl.image3d_ro_t addrspace(1)* %0, <4 x i32> noundef %1) #2
  store <4 x i32> %call, <4 x i32>* %c, align 16
  ret void
}

; Function Attrs: convergent nounwind readonly willreturn
declare spir_func <4 x i32> @_Z12read_imageui14ocl_image3d_roDv4_i(%opencl.image3d_ro_t addrspace(1)*, <4 x i32> noundef) #1

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @kernelB(%opencl.image3d_ro_t addrspace(1)* %input) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %input.addr = alloca %opencl.image3d_ro_t addrspace(1)*, align 8
  %f = alloca <4 x float>, align 16
  %.compoundliteral = alloca <4 x i32>, align 16
  store %opencl.image3d_ro_t addrspace(1)* %input, %opencl.image3d_ro_t addrspace(1)** %input.addr, align 8
  %0 = load %opencl.image3d_ro_t addrspace(1)*, %opencl.image3d_ro_t addrspace(1)** %input.addr, align 8
  store <4 x i32> zeroinitializer, <4 x i32>* %.compoundliteral, align 16
  %1 = load <4 x i32>, <4 x i32>* %.compoundliteral, align 16
  %call = call spir_func <4 x float> @_Z11read_imagef14ocl_image3d_roDv4_i(%opencl.image3d_ro_t addrspace(1)* %0, <4 x i32> noundef %1) #2
  store <4 x float> %call, <4 x float>* %f, align 16
  ret void
}

; Function Attrs: convergent nounwind readonly willreturn
declare spir_func <4 x float> @_Z11read_imagef14ocl_image3d_roDv4_i(%opencl.image3d_ro_t addrspace(1)*, <4 x i32> noundef) #1

attributes #0 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="128" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent nounwind readonly willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind readonly willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1}
!4 = !{!"read_only"}
!5 = !{!"image3d_t"}
!6 = !{!""}
