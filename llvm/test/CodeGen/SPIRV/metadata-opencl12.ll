; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

target triple = "spirv32-unknown-unknown"

!opencl.ocl.version = !{!0}
!0 = !{i32 1, i32 2}

; We assume the SPIR-V 2.2 environment spec's version format: 0|Maj|Min|Rev|
; CHECK: OpSource OpenCL_C 66048

