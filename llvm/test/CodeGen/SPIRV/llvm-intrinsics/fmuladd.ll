; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefixes=COMMON,REPLACE

; COMMON-NOT: llvm.fmuladd

; COMMON: %[[f32:[0-9]+]] = OpTypeFloat 32
; COMMON: %[[f64:[0-9]+]] = OpTypeFloat 64
;
; REPLACE: %{{[0-9]+}} = OpExtInst %[[f32]] %{{[0-9]+}} mad
; REPLACE: %{{[0-9]+}} = OpExtInst %[[f64]] %{{[0-9]+}} mad

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

; Function Attrs: nounwind
define spir_func void @foo(float %a, float %b, float %c, double %x, double %y, double %z) #0 {
entry:
  %0 = call float @llvm.fmuladd.f32(float %a, float %b, float %c)
  %1 = call double @llvm.fmuladd.f64(double %x, double %y, double %z)
ret void
}

; Function Attrs: nounwind readnone
declare float @llvm.fmuladd.f32(float, float, float) #1

; Function Attrs: nounwind readnone
declare double @llvm.fmuladd.f64(double, double, double) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!3}
!opencl.compiler.options = !{!2}

!0 = !{i32 1, i32 2}
!1 = !{i32 2, i32 0}
!2 = !{}
!3 = !{!"cl_doubles"}
