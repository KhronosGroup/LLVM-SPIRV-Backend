; RUN: llc -O0 %s -o - | FileCheck %s

target triple = "spirv32-unknown-unknown"

!opencl.ocl.version = !{!0}
!0 = !{i32 1, i32 2}

; CHECK: OpSource OpenCL_C 102000
