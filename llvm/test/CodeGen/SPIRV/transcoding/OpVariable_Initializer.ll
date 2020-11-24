; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV-NOT: undef
; CHECK-SPIRV: %{{[0-9]+}} = OpVariable %{{[0-9]+}} %{{[0-9]+}}

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

@test_atomic_fn.L = internal addrspace(3) global [64 x i32] undef, align 4

; Function Attrs: nounwind
define spir_kernel void @test_atomic_fn() #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
  ret void
}

attributes #0 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!6}
!opencl.spir.version = !{!7}
!opencl.ocl.version = !{!8}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!spirv.Generator = !{!10}

!0 = !{}
!6 = !{i32 3, i32 100000}
!7 = !{i32 1, i32 2}
!8 = !{i32 1, i32 0}
!10 = !{i16 7, i16 0}
