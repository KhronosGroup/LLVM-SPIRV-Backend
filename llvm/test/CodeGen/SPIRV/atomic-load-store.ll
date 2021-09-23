; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

; CHECK-DAG: %[[#Relaxed:]] = OpConstant %[[#]] 0
; CHECK-DAG: %[[#DeviceScope:]] = OpConstant %[[#]] 1
; CHECK-DAG: %[[#Acquire:]] = OpConstant %[[#]] 2
; CHECK-DAG: %[[#Release:]] = OpConstant %[[#]] 4
; CHECK-DAG: %[[#SequentiallyConsistent:]] = OpConstant %[[#]] 16

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; Function Attrs: nounwind
define dso_local spir_func void @test() {
entry:
; CHECK: %[[#PTR:]] = OpVariable %[[#]]
  %0 = alloca i32

; CHECK: OpAtomicStore %[[#PTR]] %[[#DeviceScope]] %[[#Relaxed]] %[[#]]
  store atomic i32 0, i32* %0 monotonic, align 4
; CHECK: OpAtomicStore %[[#PTR]] %[[#DeviceScope]] %[[#Release]] %[[#]]
  store atomic i32 0, i32* %0 release, align 4
; CHECK: OpAtomicStore %[[#PTR]] %[[#DeviceScope]] %[[#SequentiallyConsistent]] %[[#]]
  store atomic i32 0, i32* %0 seq_cst, align 4

; CHECK: %[[#]] = OpAtomicLoad %[[#]] %[[#PTR]] %[[#DeviceScope]] %[[#Relaxed]]
  %1 = load atomic i32, i32* %0 monotonic, align 4
; CHECK: %[[#]] = OpAtomicLoad %[[#]] %[[#PTR]] %[[#DeviceScope]] %[[#Acquire]]
  %2 = load atomic i32, i32* %0 acquire, align 4
; CHECK: %[[#]] = OpAtomicLoad %[[#]] %[[#PTR]] %[[#DeviceScope]] %[[#SequentiallyConsistent]]
  %3 = load atomic i32, i32* %0 seq_cst, align 4
  ret void
}
