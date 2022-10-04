; RUN: llc -O0 %s -o - | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK: %[[TypeSampler:[0-9]+]] = OpTypeSampler
define spir_kernel void @foo(i64 %sampler) #0 !kernel_arg_addr_space !7 !kernel_arg_access_qual !8 !kernel_arg_type !9 !kernel_arg_type_qual !10 !kernel_arg_base_type !9 {
entry:
  ret void
}

attributes #0 = { nounwind }

!7 = !{i32 0}
!8 = !{!"none"}
!9 = !{!"sampler_t"}
!10 = !{!""}
