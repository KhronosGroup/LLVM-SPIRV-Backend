; RUN: llc -O0 %s -o - | FileCheck %s

; CHECK-DAG: OpCapability Int8
; CHECK-DAG: OpCapability Int16
; CHECK-DAG: OpCapability Int64

; CHECK-DAG: %{{[0-9]+}} = OpTypeInt 8 0
; CHECK-DAG: %{{[0-9]+}} = OpTypeInt 16 0
; CHECK-DAG: %{{[0-9]+}} = OpTypeInt 64 0

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

@a = addrspace(1) global i8 0, align 1
@b = addrspace(1) global i16 0, align 2
@c = addrspace(1) global i64 0, align 8

define spir_kernel void @test_atomic_fn() #0 {
  ret void
}

!opencl.enable.FP_CONTRACT = !{}
!opencl.ocl.version = !{!0}
!opencl.spir.version = !{!0}
!opencl.used.extensions = !{!1}
!opencl.used.optional.core.features = !{!1}
!opencl.compiler.options = !{!1}

!0 = !{i32 2, i32 0}
!1 = !{}
