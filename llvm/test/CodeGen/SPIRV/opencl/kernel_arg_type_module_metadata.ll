; RUN: llc -O0 %s -o - | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK: %[[TypeSampler:[0-9]+]] = OpTypeSampler
define spir_kernel void @foo(i64 %sampler) #0 {
entry:
  ret void
}

attributes #0 = { nounwind }

!opencl.kernels = !{!0}

!0 = !{void (i64)* @foo, !1, !2, !3, !4, !5, !6}
!1 = !{!"kernel_arg_addr_space", i32 0}
!2 = !{!"kernel_arg_access_qual", !"none"}
!3 = !{!"kernel_arg_type", !"sampler_t"}
!4 = !{!"kernel_arg_type_qual", !""}
!5 = !{!"kernel_arg_base_type", !"sampler_t"}
!6 = !{!"kernel_arg_name", !"sampler"}
