; ModuleID = 'bitcast.bc'
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; RUN: llc -O0 %s -o - | FileCheck %s

; Check the bitcast is translated back to bitcast

; CHECK: OpTypeInt 16
; CHECK: OpIAdd

; Function Attrs: nounwind
define i16 @test_fn(i16 %arg0, i16 %arg1) {
entry:
  %0 = add i16 %arg0, %arg1
  ret i16 %0
}

; Function Attrs: nounwind readnone
declare spir_func i64 @_Z13get_global_idj(i32) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!6}
!opencl.used.extensions = !{!7}
!opencl.used.optional.core.features = !{!7}
!opencl.compiler.options = !{!7}
!llvm.ident = !{!8}

!1 = !{i32 1, i32 1}
!2 = !{!"none", !"none"}
!3 = !{!"char2*", !"short*"}
!4 = !{!"", !""}
!5 = !{!"char2*", !"short*"}
!6 = !{i32 2, i32 0}
!7 = !{}
!8 = !{!"clang version 3.4 "}
!9 = !{!10, !10, i64 0}
!10 = !{!"omnipotent char", !11, i64 0}
!11 = !{!"Simple C/C++ TBAA"}
!12 = !{!13, !13, i64 0}
!13 = !{!"short", !10, i64 0}
