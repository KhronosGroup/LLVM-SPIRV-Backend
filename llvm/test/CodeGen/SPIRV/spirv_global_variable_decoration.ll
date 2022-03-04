; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

@v1 = addrspace(1) global i32 42, !spirv.Decorations !2
@v2 = addrspace(1) global float 1.0, !spirv.Decorations !4

; CHECK-SPIRV: OpDecorate %[[PId1:[0-9]+]] Constant
; CHECK-SPIRV: OpDecorate %[[PId2:[0-9]+]] Constant
; CHECK-SPIRV: OpDecorate %[[PId2]] Binding 1
; CHECK-SPIRV: %[[PId1]] = OpVariable %{{[0-9]+}}
; CHECK-SPIRV: %[[PId2]] = OpVariable %{{[0-9]+}}

!1 = !{i32 22}
!2 = !{!1}
!3 = !{i32 33, i32 1}
!4 = !{!1, !3}
