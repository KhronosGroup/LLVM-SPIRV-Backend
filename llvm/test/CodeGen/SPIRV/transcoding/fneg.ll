; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpName %[[#r1:]] "r1"
; CHECK-SPIRV: OpName %[[#r2:]] "r2"
; CHECK-SPIRV: OpName %[[#r3:]] "r3"
; CHECK-SPIRV: OpName %[[#r4:]] "r4"
; CHECK-SPIRV: OpName %[[#r5:]] "r5"
; CHECK-SPIRV: OpName %[[#r6:]] "r6"
; CHECK-SPIRV: OpName %[[#r7:]] "r7"
; CHECK-SPIRV-NOT: OpDecorate %{{.*}} FPFastMathMode
; CHECK-SPIRV: %[[float:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV: %[[#r1]] = OpFNegate %[[float]]
; CHECK-SPIRV: %[[#r2]] = OpFNegate %[[float]]
; CHECK-SPIRV: %[[#r3]] = OpFNegate %[[float]]
; CHECK-SPIRV: %[[#r4]] = OpFNegate %[[float]]
; CHECK-SPIRV: %[[#r5]] = OpFNegate %[[float]]
; CHECK-SPIRV: %[[#r6]] = OpFNegate %[[float]]
; CHECK-SPIRV: %[[#r7]] = OpFNegate %[[float]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @testFNeg(float %a) local_unnamed_addr #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %r1 = fneg float %a
  %r2 = fneg nnan float %a
  %r3 = fneg ninf float %a
  %r4 = fneg nsz float %a
  %r5 = fneg arcp float %a
  %r6 = fneg fast float %a
  %r7 = fneg nnan ninf float %a
  ret void
}

attributes #0 = { convergent nounwind writeonly "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{i32 0}
!3 = !{!"none"}
!4 = !{!"float"}
!5 = !{!""}
