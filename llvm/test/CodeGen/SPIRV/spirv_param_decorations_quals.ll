; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @k(i32 addrspace(1)* %a) #0 !kernel_arg_addr_space !4 !kernel_arg_access_qual !5 !kernel_arg_type !6 !kernel_arg_type_qual !7 !kernel_arg_base_type !6 !spirv.ParameterDecorations !10 {
entry:
  ret void
}

; CHECK-SPIRV: OpDecorate %[[PId:[0-9]+]] Volatile
; CHECK-SPIRV: OpDecorate %[[PId]] FuncParamAttr NoAlias
; CHECK-SPIRV: %[[PId]] = OpFunctionParameter %{{[0-9]+}}

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{!"clang version 14.0.0"}
!4 = !{i32 0, i32 0, i32 0}
!5 = !{!"none"}
!6 = !{!"int*"}
!7 = !{!"volatile"}
!8 = !{i32 38, i32 4} ; FuncParamAttr NoAlias
!9 = !{!8}
!10 = !{!9}
