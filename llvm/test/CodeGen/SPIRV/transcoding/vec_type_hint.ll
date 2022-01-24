; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; kernel
; __attribute__((vec_type_hint(float4)))
; void test_float() {}

; kernel
; __attribute__((vec_type_hint(double)))
; void test_double() {}

; kernel
; __attribute__((vec_type_hint(uint4)))
; void test_uint() {}

; kernel
; __attribute__((vec_type_hint(int8)))
; void test_int() {}

; ModuleID = 'vec_type_hint.cl'
source_filename = "vec_type_hint.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV: OpEntryPoint {{.*}} %{{[0-9]+}} "test_float"
; CHECK-SPIRV: OpEntryPoint {{.*}} %{{[0-9]+}} "test_double"
; CHECK-SPIRV: OpEntryPoint {{.*}} %{{[0-9]+}} "test_uint"
; CHECK-SPIRV: OpEntryPoint {{.*}} %{{[0-9]+}} "test_int"
; CHECK-SPIRV: OpExecutionMode %{{[0-9]+}} VecTypeHint {{[0-9]+}}
; CHECK-SPIRV: OpExecutionMode %{{[0-9]+}} VecTypeHint {{[0-9]+}}
; CHECK-SPIRV: OpExecutionMode %{{[0-9]+}} VecTypeHint {{[0-9]+}}
; CHECK-SPIRV: OpExecutionMode %{{[0-9]+}} VecTypeHint {{[0-9]+}}

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @test_float() #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 !vec_type_hint !4 {
entry:
  ret void
}

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @test_double() #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 !vec_type_hint !5 {
entry:
  ret void
}

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @test_uint() #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 !vec_type_hint !6 {
entry:
  ret void
}

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @test_int() #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 !vec_type_hint !7 {
entry:
  ret void
}

attributes #0 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{}
!4 = !{<4 x float> undef, i32 0}
!5 = !{double undef, i32 0}
!6 = !{<4 x i32> undef, i32 0}
!7 = !{<8 x i32> undef, i32 1}
