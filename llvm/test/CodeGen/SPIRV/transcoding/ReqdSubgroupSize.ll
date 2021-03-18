; Check translation of intel_reqd_sub_group_size metadata to SubgroupSize
; execution mode and back. The IR is producded from the following OpenCL C code:
; kernel __attribute__((intel_reqd_sub_group_size(8)))
; void foo() {}

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpCapability SubgroupDispatch
; CHECK-SPIRV: OpEntryPoint Kernel %[[kernel:[0-9]+]] "foo"
; FIXME: operands of OpExecutionMode
; CHECK-SPIRV: OpExecutionMode %[[kernel]] SubgroupSize 8

; ModuleID = 'ReqdSubgroupSize.ll'
source_filename = "ReqdSubgroupSize.ll"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: norecurse nounwind readnone
define spir_kernel void @foo() local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 !intel_reqd_sub_group_size !5 {
entry:
  ret void
}

attributes #0 = { norecurse nounwind readnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!opencl.compiler.options = !{!3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{!"clang version 6.0.0 (cfe/trunk)"}
!5 = !{i32 8}
