; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpName %[[vec:[0-9]+]] "vec"
; CHECK-SPIRV: OpName %[[index:[0-9]+]] "index"
; CHECK-SPIRV: OpName %[[res:[0-9]+]] "res"

; CHECK-SPIRV: %[[float:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV: %[[float2:[0-9]+]] = OpTypeVector %[[float]] 2

; CHECK-SPIRV: %[[res]] = OpVectorExtractDynamic %[[float]] %[[vec]] %[[index]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @test(float addrspace(1)* nocapture %out, <2 x float> %vec, i32 %index) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %res = extractelement <2 x float> %vec, i32 %index
  store float %res, float addrspace(1)* %out, align 4
  ret void
}

attributes #0 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!8}

!1 = !{i32 1, i32 0}
!2 = !{!"none", !"none", !"none"}
!3 = !{!"float*", !"float2", !"int"}
!4 = !{!"float*", !"float2", !"int"}
!5 = !{!"", !"", !""}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 0}
!8 = !{}
