; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @k(float %a, float %b, float %c) #0 !kernel_arg_addr_space !4 !kernel_arg_access_qual !5 !kernel_arg_type !6 !kernel_arg_type_qual !7 !kernel_arg_base_type !6 !spirv.ParameterDecorations !14 {
entry:
  ret void
}

; CHECK-SPIRV: OpDecorate %[[PId1:[0-9]+]] Restrict
; CHECK-SPIRV: OpDecorate %[[PId1]] FPRoundingMode RTP
; CHECK-SPIRV: OpDecorate %[[PId2:[0-9]+]] Volatile
; CHECK-SPIRV: %[[PId1]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %[[PId2]] = OpFunctionParameter %{{[0-9]+}}

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{!"clang version 14.0.0"}
!4 = !{i32 0, i32 0, i32 0}
!5 = !{!"none", !"none", !"none"}
!6 = !{!"float", !"float", !"float"}
!7 = !{!"", !"", !""}
!8 = !{i32 19}
!9 = !{i32 39, i32 2}
!10 = !{i32 21}
!11 = !{!8, !9}
!12 = !{}
!13 = !{!10}
!14 = !{!11, !12, !13}
