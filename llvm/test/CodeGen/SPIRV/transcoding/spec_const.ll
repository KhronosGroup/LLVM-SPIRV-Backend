; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV-NOT: OpCapability Matrix
; CHECK-SPIRV-NOT: OpCapability Shader
; CHECK-SPIRV: OpCapability Kernel

; CHECK-SPIRV-DAG: OpDecorate %[[SC0:[0-9]+]] SpecId 0
; CHECK-SPIRV-DAG: OpDecorate %[[SC1:[0-9]+]] SpecId 1
; CHECK-SPIRV-DAG: OpDecorate %[[SC2:[0-9]+]] SpecId 2
; CHECK-SPIRV-DAG: OpDecorate %[[SC3:[0-9]+]] SpecId 3
; CHECK-SPIRV-DAG: OpDecorate %[[SC4:[0-9]+]] SpecId 4
; CHECK-SPIRV-DAG: OpDecorate %[[SC5:[0-9]+]] SpecId 5
; CHECK-SPIRV-DAG: OpDecorate %[[SC6:[0-9]+]] SpecId 6
; CHECK-SPIRV-DAG: OpDecorate %[[SC7:[0-9]+]] SpecId 7

; CHECK-SPIRV-DAG: %[[SC0]] = OpSpecConstantFalse %{{[0-9]+}}
; CHECK-SPIRV-DAG: %[[SC1]] = OpSpecConstant {{[0-9]+}} 100
; CHECK-SPIRV-DAG: %[[SC2]] = OpSpecConstant {{[0-9]+}} 1
; CHECK-SPIRV-DAG: %[[SC3]] = OpSpecConstant {{[0-9]+}} 2
; CHECK-SPIRV-DAG: %[[SC4]] = OpSpecConstant {{[0-9]+}} 3 0
; CHECK-SPIRV-DAG: %[[SC5]] = OpSpecConstant {{[0-9]+}} 14336
; CHECK-SPIRV-DAG: %[[SC6]] = OpSpecConstant {{[0-9]+}} 1067450368
; CHECK-SPIRV-DAG: %[[SC7]] = OpSpecConstant {{[0-9]+}} 0 1073807360

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"
; Function Attrs: nofree norecurse nounwind writeonly
 define spir_kernel void @foo(i8 addrspace(1)* nocapture %b, i8 addrspace(1)* nocapture %c, i16 addrspace(1)* nocapture %s, i32 addrspace(1)* nocapture %i, i64 addrspace(1)* nocapture %l, half addrspace(1)* nocapture %h, float addrspace(1)* nocapture %f, double addrspace(1)* nocapture %d) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %0 = call i1 @_Z20__spirv_SpecConstantib(i32 0, i1 false)
  %conv = zext i1 %0 to i8
  store i8 %conv, i8 addrspace(1)* %b, align 1

  %1 = call i8 @_Z20__spirv_SpecConstantia(i32 1, i8 100)
  store i8 %1, i8 addrspace(1)* %c, align 1

  %2 = call i16 @_Z20__spirv_SpecConstantis(i32 2, i16 1)
  store i16 %2, i16 addrspace(1)* %s, align 2

  %3 = call i32 @_Z20__spirv_SpecConstantii(i32 3, i32 2)
  store i32 %3, i32 addrspace(1)* %i, align 4

  %4 = call i64 @_Z20__spirv_SpecConstantix(i32 4, i64 3)
  store i64 %4, i64 addrspace(1)* %l, align 8

  %5 = call half @_Z20__spirv_SpecConstantih(i32 5, half 0xH3800)
  store half %5, half addrspace(1)* %h, align 2

  %6 = call float @_Z20__spirv_SpecConstantif(i32 6, float 1.250000e+00)
  store float %6, float addrspace(1)* %f, align 4

  %7 = call double @_Z20__spirv_SpecConstantid(i32 7, double 2.125000e+00)
  store double %7, double addrspace(1)* %d, align 8
  ret void
}

declare i1 @_Z20__spirv_SpecConstantib(i32, i1)
declare i8 @_Z20__spirv_SpecConstantia(i32, i8)
declare i16 @_Z20__spirv_SpecConstantis(i32, i16)
declare i32 @_Z20__spirv_SpecConstantii(i32, i32)
declare i64 @_Z20__spirv_SpecConstantix(i32, i64)
declare half @_Z20__spirv_SpecConstantih(i32, half)
declare float @_Z20__spirv_SpecConstantif(i32, float)
declare double @_Z20__spirv_SpecConstantid(i32, double)

attributes #0 = { nofree norecurse nounwind writeonly "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!opencl.used.extensions = !{!7}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 11.0.0 (https://github.com/llvm/llvm-project.git f11016b41a94d2bad8824d5e2833d141fda24502)"}
!3 = !{i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1}
!4 = !{!"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!5 = !{!"bool*", !"char*", !"short*", !"int*", !"long*", !"half*", !"float*", !"double*"}
!6 = !{!"", !"", !"", !"", !"", !"", !"", !""}
!7 = !{!"cl_khr_fp16"}
