; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: 3 Name [[#r1:]] "r1"
; CHECK-SPIRV: 3 Name [[#r2:]] "r2"
; CHECK-SPIRV: 3 Name [[#r3:]] "r3"
; CHECK-SPIRV: 3 Name [[#r4:]] "r4"
; CHECK-SPIRV: 3 Name [[#r5:]] "r5"
; CHECK-SPIRV: 3 Name [[#r6:]] "r6"
; CHECK-SPIRV: 3 Name [[#r7:]] "r7"
; CHECK-SPIRV-NOT: 4 Decorate [[#r1]] FPFastMathMode
; CHECK-SPIRV-DAG: 4 Decorate [[#r2]] FPFastMathMode 1
; CHECK-SPIRV-DAG: 4 Decorate [[#r3]] FPFastMathMode 2
; CHECK-SPIRV-DAG: 4 Decorate [[#r4]] FPFastMathMode 4
; CHECK-SPIRV-DAG: 4 Decorate [[#r5]] FPFastMathMode 8
; CHECK-SPIRV-DAG: 4 Decorate [[#r6]] FPFastMathMode 16
; CHECK-SPIRV-DAG: 4 Decorate [[#r7]] FPFastMathMode 3
; CHECK-SPIRV: 3 TypeFloat [[float:[0-9]+]] 32
; CHECK-SPIRV: 5 FMul [[float]] [[#r1]]
; CHECK-SPIRV: 5 FMul [[float]] [[#r2]]
; CHECK-SPIRV: 5 FMul [[float]] [[#r3]]
; CHECK-SPIRV: 5 FMul [[float]] [[#r4]]
; CHECK-SPIRV: 5 FMul [[float]] [[#r5]]
; CHECK-SPIRV: 5 FMul [[float]] [[#r6]]
; CHECK-SPIRV: 5 FMul [[float]] [[#r7]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @testFMul(float %a, float %b) local_unnamed_addr #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %r1 = fmul float %a, %b
  %r2 = fmul nnan float %a, %b
  %r3 = fmul ninf float %a, %b
  %r4 = fmul nsz float %a, %b
  %r5 = fmul arcp float %a, %b
  %r6 = fmul fast float %a, %b
  %r7 = fmul nnan ninf float %a, %b
  ret void
}

attributes #0 = { convergent nounwind writeonly "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{i32 0, i32 0}
!3 = !{!"none", !"none"}
!4 = !{!"float", !"float"}
!5 = !{!"", !""}
