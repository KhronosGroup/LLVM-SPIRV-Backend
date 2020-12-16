; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

; CHECK: %[[Int:[0-9]+]] = OpTypeInt 32 0
; CHECK-DAG: %[[Scope_Device:[0-9]+]] = OpConstant %[[Int]] 1 {{$}}
; CHECK-DAG: %[[MemSem_Relaxed:[0-9]+]] = OpConstant %[[Int]] 0
; CHECK-DAG: %[[MemSem_Acquire:[0-9]+]] = OpConstant %[[Int]] 2
; CHECK-DAG: %[[MemSem_Release:[0-9]+]] = OpConstant %[[Int]] 4 {{$}}
; CHECK-DAG: %[[MemSem_AcquireRelease:[0-9]+]] = OpConstant %[[Int]] 8
; CHECK-DAG: %[[MemSem_SequentiallyConsistent:[0-9]+]] = OpConstant %[[Int]] 16
; CHECK-DAG: %[[Value:[0-9]+]] = OpConstant %[[Int]] 42
; CHECK: %[[Float:[0-9]+]] = OpTypeFloat 32
; CHECK: %[[Pointer:[0-9]+]] = OpVariable %{{[0-9]+}}
; CHECK: %[[FPPointer:[0-9]+]] = OpVariable %{{[0-9]+}}
; CHECK: %[[FPValue:[0-9]+]] = OpConstant %[[Float]] 1109917696

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

@ui = common dso_local addrspace(1) global i32 0, align 4
@f = common dso_local local_unnamed_addr addrspace(1) global float 0.000000e+00, align 4

; Function Attrs: nounwind
define dso_local spir_func void @test_atomicrmw() local_unnamed_addr #0 {
entry:
  %0 = atomicrmw xchg i32 addrspace(1)* @ui, i32 42 acq_rel
; CHECK: %{{[0-9]+}} = OpAtomicExchange %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_AcquireRelease]] %[[Value]]

  %1 = atomicrmw xchg float addrspace(1)* @f, float 42.000000e+00 seq_cst
; CHECK: %{{[0-9]+}} = OpAtomicExchange %[[Float]] %[[FPPointer]] %[[Scope_Device]] %[[MemSem_SequentiallyConsistent]] %[[FPValue]]

  %2 = atomicrmw add i32 addrspace(1)* @ui, i32 42 monotonic
; CHECK: %{{[0-9]+}} = OpAtomicIAdd %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_Relaxed]] %[[Value]]

  %3 = atomicrmw sub i32 addrspace(1)* @ui, i32 42 acquire
; CHECK: %{{[0-9]+}} = OpAtomicISub %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_Acquire]] %[[Value]]

  %4 = atomicrmw or i32 addrspace(1)* @ui, i32 42 release
; CHECK: %{{[0-9]+}} = OpAtomicOr %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_Release]] %[[Value]]

  %5 = atomicrmw xor i32 addrspace(1)* @ui, i32 42 acq_rel
; CHECK: %{{[0-9]+}} = OpAtomicXor %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_AcquireRelease]] %[[Value]]

  %6 = atomicrmw and i32 addrspace(1)* @ui, i32 42 seq_cst
; CHECK: %{{[0-9]+}} = OpAtomicAnd %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_SequentiallyConsistent]] %[[Value]]

  %7 = atomicrmw max i32 addrspace(1)* @ui, i32 42 monotonic
; CHECK: %{{[0-9]+}} = OpAtomicSMax %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_Relaxed]] %[[Value]]

  %8 = atomicrmw min i32 addrspace(1)* @ui, i32 42 acquire
; CHECK: %{{[0-9]+}} = OpAtomicSMin %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_Acquire]] %[[Value]]

  %9 = atomicrmw umax i32 addrspace(1)* @ui, i32 42 release
; CHECK: %{{[0-9]+}} = OpAtomicUMax %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_Release]] %[[Value]]

  %10 = atomicrmw umin i32 addrspace(1)* @ui, i32 42 acq_rel
; CHECK: %{{[0-9]+}} = OpAtomicUMin %[[Int]] %[[Pointer]] %[[Scope_Device]] %[[MemSem_AcquireRelease]] %[[Value]]

  ret void
}

attributes #0 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 11.0.0 (https://github.com/llvm/llvm-project.git 20c5968e0953d978be4d9d1062801e8758c393b5)"}
