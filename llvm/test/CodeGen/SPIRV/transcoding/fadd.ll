; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpName %[[#r1:]] "r1"
; CHECK-SPIRV: OpName %[[#r2:]] "r2"
; CHECK-SPIRV: OpName %[[#r3:]] "r3"
; CHECK-SPIRV: OpName %[[#r4:]] "r4"
; CHECK-SPIRV: OpName %[[#r5:]] "r5"
; CHECK-SPIRV: OpName %[[#r6:]] "r6"
; CHECK-SPIRV: OpName %[[#r7:]] "r7"
; CHECK-SPIRV-NOT: OpDecorate %[[#r1]] FPFastMathMode
; CHECK-SPIRV-DAG: OpDecorate %[[#r2]] FPFastMathMode NotNaN
; CHECK-SPIRV-DAG: OpDecorate %[[#r3]] FPFastMathMode NotInf
; CHECK-SPIRV-DAG: OpDecorate %[[#r4]] FPFastMathMode NSZ
; CHECK-SPIRV-DAG: OpDecorate %[[#r5]] FPFastMathMode AllowRecip
; CHECK-SPIRV-DAG: OpDecorate %[[#r6]] FPFastMathMode NotNaN|NotInf|NSZ|AllowRecip|Fast
; CHECK-SPIRV-DAG: OpDecorate %[[#r7]] FPFastMathMode NotNaN|NotInf
; CHECK-SPIRV: %[[float:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV: %[[#r1]] = OpFAdd %[[float]]
; CHECK-SPIRV: %[[#r2]] = OpFAdd %[[float]]
; CHECK-SPIRV: %[[#r3]] = OpFAdd %[[float]]
; CHECK-SPIRV: %[[#r4]] = OpFAdd %[[float]]
; CHECK-SPIRV: %[[#r5]] = OpFAdd %[[float]]
; CHECK-SPIRV: %[[#r6]] = OpFAdd %[[float]]
; CHECK-SPIRV: %[[#r7]] = OpFAdd %[[float]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @testFAdd(float %a, float %b) local_unnamed_addr #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !3 !kernel_arg_type !4 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %r1 = fadd float %a, %b
  %r2 = fadd nnan float %a, %b
  %r3 = fadd ninf float %a, %b
  %r4 = fadd nsz float %a, %b
  %r5 = fadd arcp float %a, %b
  %r6 = fadd fast float %a, %b
  %r7 = fadd nnan ninf float %a, %b
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
