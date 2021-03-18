; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: %[[bool:[0-9]+]] = OpTypeBool
; CHECK-SPIRV: %[[bool2:[0-9]+]] = OpTypeVector %[[bool]] 2

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpFUnordEqual %[[bool2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: nounwind
define spir_kernel void @testFUnordEqual(<2 x float> %a, <2 x float> %b) #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_type_qual !5 !kernel_arg_base_type !4 {
entry:
  %0 = fcmp ueq <2 x float> %a, %b
  ret void
}

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpFUnordGreaterThan %[[bool2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: nounwind
define spir_kernel void @testFUnordGreaterThan(<2 x float> %a, <2 x float> %b) #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_type_qual !5 !kernel_arg_base_type !4 {
entry:
  %0 = fcmp ugt <2 x float> %a, %b
  ret void
}

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpFUnordGreaterThanEqual %[[bool2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: nounwind
define spir_kernel void @testFUnordGreaterThanEqual(<2 x float> %a, <2 x float> %b) #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_type_qual !5 !kernel_arg_base_type !4 {
entry:
  %0 = fcmp uge <2 x float> %a, %b
  ret void
}

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpFUnordLessThan %[[bool2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: nounwind
define spir_kernel void @testFUnordLessThan(<2 x float> %a, <2 x float> %b) #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_type_qual !5 !kernel_arg_base_type !4 {
entry:
  %0 = fcmp ult <2 x float> %a, %b
  ret void
}

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV-NEXT: %[[A:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT: %[[B:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpFUnordLessThanEqual %[[bool2]] %[[A]] %[[B]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: nounwind
define spir_kernel void @testFUnordLessThanEqual(<2 x float> %a, <2 x float> %b) #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_type_qual !5 !kernel_arg_base_type !4 {
entry:
  %0 = fcmp ule <2 x float> %a, %b
  ret void
}

attributes #0 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!1}
!opencl.used.optional.core.features = !{!1}

!0 = !{i32 2, i32 0}
!1 = !{}
!2 = !{i32 0, i32 0}
!3 = !{!"none", !"none"}
!4 = !{!"float2", !"float2"}
!5 = !{!"", !""}
