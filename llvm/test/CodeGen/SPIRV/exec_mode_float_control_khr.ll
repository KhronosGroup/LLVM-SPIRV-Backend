; RUN: llc -O0 %s -o - | FileCheck %s --check-prefixes=SPV,SPV14,SPV-NEGATIVE

; ModuleID = 'float_control.bc'
source_filename = "float_control.cpp"
target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spirv32-unknown-unknown"

; Function Attrs: noinline norecurse nounwind readnone
define dso_local dllexport spir_kernel void @k_float_controls_0(i32 %ibuf, i32 %obuf) local_unnamed_addr {
entry:
  ret void
}

; Function Attrs: noinline norecurse nounwind readnone
define dso_local dllexport spir_kernel void @k_float_controls_1(i32 %ibuf, i32 %obuf) local_unnamed_addr {
entry:
  ret void
}

; Function Attrs: noinline norecurse nounwind readnone
define dso_local dllexport spir_kernel void @k_float_controls_2(i32 %ibuf, i32 %obuf) local_unnamed_addr {
entry:
  ret void
}

; Function Attrs: noinline norecurse nounwind readnone
define dso_local dllexport spir_kernel void @k_float_controls_3(i32 %ibuf, i32 %obuf) local_unnamed_addr {
entry:
  ret void
}

; Function Attrs: noinline norecurse nounwind readnone
define dso_local dllexport spir_kernel void @k_float_controls_4(i32 %ibuf, i32 %obuf) local_unnamed_addr {
entry:
  ret void
}


!llvm.module.flags = !{!12}
!llvm.ident = !{!13}
!spirv.EntryPoint = !{}
!spirv.ExecutionMode = !{!15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29}

; SPV14-NOT: OpExtension "SPV_KHR_float_controls"
; SPV-NEGATIVE-NOT: OpExtension "SPV_KHR_float_controls"

; SPV-DAG: OpEntryPoint {{.*}} %[[KERNEL0:[0-9]+]] "k_float_controls_0"
; SPV-DAG: OpEntryPoint {{.*}} %[[KERNEL1:[0-9]+]] "k_float_controls_1"
; SPV-DAG: OpEntryPoint {{.*}} %[[KERNEL2:[0-9]+]] "k_float_controls_2"
; SPV-DAG: OpEntryPoint {{.*}} %[[KERNEL3:[0-9]+]] "k_float_controls_3"
; SPV-DAG: OpEntryPoint {{.*}} %[[KERNEL4:[0-9]+]] "k_float_controls_4"
!0 = !{void (i32, i32)* @k_float_controls_0, !"k_float_controls_0", !1, i32 0, !2, !3, !4, i32 0, i32 0}
!1 = !{i32 2, i32 2}
!2 = !{i32 32, i32 36}
!3 = !{i32 0, i32 0}
!4 = !{!"", !""}
!12 = !{i32 1, !"wchar_size", i32 4}
!13 = !{!"clang version 8.0.1"}
!14 = !{i32 1, i32 0}

; SPV-DAG: OpExecutionMode %[[KERNEL0]] DenormPreserve 64
!15 = !{void (i32, i32)* @k_float_controls_0, i32 4459, i32 64}
; SPV-DAG: OpExecutionMode %[[KERNEL0]] DenormPreserve 32
!16 = !{void (i32, i32)* @k_float_controls_0, i32 4459, i32 32}
; SPV-DAG: OpExecutionMode %[[KERNEL0]] DenormPreserve 16
!17 = !{void (i32, i32)* @k_float_controls_0, i32 4459, i32 16}

; SPV-DAG: OpExecutionMode %[[KERNEL1]] DenormFlushToZero 64
!18 = !{void (i32, i32)* @k_float_controls_1, i32 4460, i32 64}
; SPV-DAG: OpExecutionMode %[[KERNEL1]] DenormFlushToZero 32
!19 = !{void (i32, i32)* @k_float_controls_1, i32 4460, i32 32}
; SPV-DAG: OpExecutionMode %[[KERNEL1]] DenormFlushToZero 16
!20 = !{void (i32, i32)* @k_float_controls_1, i32 4460, i32 16}

; SPV-DAG: OpExecutionMode %[[KERNEL2]] SignedZeroInfNanPreserve 64
!21 = !{void (i32, i32)* @k_float_controls_2, i32 4461, i32 64}
; SPV-DAG: OpExecutionMode %[[KERNEL2]] SignedZeroInfNanPreserve 32
!22 = !{void (i32, i32)* @k_float_controls_2, i32 4461, i32 32}
; SPV-DAG: OpExecutionMode %[[KERNEL2]] SignedZeroInfNanPreserve 16
!23 = !{void (i32, i32)* @k_float_controls_2, i32 4461, i32 16}

; SPV-DAG: OpExecutionMode %[[KERNEL3]] RoundingModeRTE 64
!24 = !{void (i32, i32)* @k_float_controls_3, i32 4462, i32 64}
; SPV-DAG: OpExecutionMode %[[KERNEL3]] RoundingModeRTE 32
!25 = !{void (i32, i32)* @k_float_controls_3, i32 4462, i32 32}
; SPV-DAG: OpExecutionMode %[[KERNEL3]] RoundingModeRTE 16
!26 = !{void (i32, i32)* @k_float_controls_3, i32 4462, i32 16}

; SPV-DAG: OpExecutionMode %[[KERNEL4]] RoundingModeRTZ 64
!27 = !{void (i32, i32)* @k_float_controls_4, i32 4463, i32 64}
; SPV-DAG: OpExecutionMode %[[KERNEL4]] RoundingModeRTZ 32
!28 = !{void (i32, i32)* @k_float_controls_4, i32 4463, i32 32}
; SPV-DAG: OpExecutionMode %[[KERNEL4]] RoundingModeRTZ 16
!29 = !{void (i32, i32)* @k_float_controls_4, i32 4463, i32 16}
