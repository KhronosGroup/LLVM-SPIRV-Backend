; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK: OpSource OpenCL_C 131072

; Int64Atomics capability must be declared only if atomic builtins have 64-bit integers arguments.
; CHECK-NOT: OpCapability Int64Atomics

; CHECK: %[[int:[0-9]+]] = OpTypeInt 32 0
; CHECK-DAG: %[[SequentiallyConsistent_MS:[0-9]+]] = OpConstant %[[int]] 16
; CHECK-DAG: %[[DeviceScope:[0-9]+]] = OpConstant %[[int]] 1
; CHECK-DAG: %[[int_ptr:[0-9]+]] = OpTypePointer Generic %[[int]]
; CHECK-DAG: %[[bool:[0-9]+]] = OpTypeBool

; Function Attrs: nounwind
define spir_func void @test(i32 addrspace(4)* %object, i32 addrspace(4)* %expected, i32 %desired) #0 {
; CHECK: %[[object:[0-9]+]] = OpFunctionParameter %[[int_ptr]]
; CHECK: %[[expected:[0-9]+]] = OpFunctionParameter %[[int_ptr]]
; CHECK: %[[desired:[0-9]+]] = OpFunctionParameter %[[int]]

entry:
  %object.addr = alloca i32 addrspace(4)*, align 4
  %expected.addr = alloca i32 addrspace(4)*, align 4
  %desired.addr = alloca i32, align 4
  %strong_res = alloca i8, align 1
  %res = alloca i8, align 1
  %weak_res = alloca i8, align 1
  store i32 addrspace(4)* %object, i32 addrspace(4)** %object.addr, align 4
  store i32 addrspace(4)* %expected, i32 addrspace(4)** %expected.addr, align 4
  store i32 %desired, i32* %desired.addr, align 4
  %0 = load i32 addrspace(4)*, i32 addrspace(4)** %object.addr, align 4
  %1 = load i32 addrspace(4)*, i32 addrspace(4)** %expected.addr, align 4
  %2 = load i32, i32* %desired.addr, align 4
; CHECK: OpStore %[[object_addr:[0-9]+]] %[[object]]
; CHECK: OpStore %[[expected_addr:[0-9]+]] %[[expected]]
; CHECK: OpStore %[[desired_addr:[0-9]+]] %[[desired]]
; CHECK: %[[Pointer:[0-9]+]] = OpLoad %[[int_ptr]] %[[object_addr]]
; CHECK: %[[exp:[0-9]+]] = OpLoad %[[int_ptr]] %[[expected_addr]]
; CHECK: %[[Value:[0-9]+]] = OpLoad %[[int]] %[[desired_addr]]
; CHECK: %[[Comparator:[0-9]+]] = OpLoad %[[int]] %[[exp]]
; CHECK-NEXT: %[[Result:[0-9]+]] = OpAtomicCompareExchange %[[int]] %[[Pointer]] %[[DeviceScope]] %[[SequentiallyConsistent_MS]] %[[SequentiallyConsistent_MS]] %[[Value]] %[[Comparator]]
  %call = call spir_func zeroext i1 @_Z30atomic_compare_exchange_strongPVU3AS4U7_AtomiciPU3AS4ii(i32 addrspace(4)* %0, i32 addrspace(4)* %1, i32 %2)
; CHECK-NEXT: OpStore %[[exp]] %[[Result]]
; CHECK-NEXT: %[[CallRes:[0-9]+]] = OpIEqual %[[bool]] %[[Result]] %[[Comparator]]
; CHECK-NOT: %[[Result]]
  %frombool = zext i1 %call to i8
  store i8 %frombool, i8* %strong_res, align 1
  %3 = load i8, i8* %strong_res, align 1
  %tobool = trunc i8 %3 to i1
  %lnot = xor i1 %tobool, true
  %frombool1 = zext i1 %lnot to i8
  store i8 %frombool1, i8* %res, align 1
  %4 = load i32 addrspace(4)*, i32 addrspace(4)** %object.addr, align 4
  %5 = load i32 addrspace(4)*, i32 addrspace(4)** %expected.addr, align 4
  %6 = load i32, i32* %desired.addr, align 4

; CHECK: %[[Pointer:[0-9]+]] = OpLoad %[[int_ptr]] %[[object_addr]]
; CHECK: %[[exp:[0-9]+]] = OpLoad %[[int_ptr]] %[[expected_addr]]
; CHECK: %[[Value:[0-9]+]] = OpLoad %[[int]] %[[desired_addr]]
; CHECK: %[[ComparatorWeak:[0-9]+]] = OpLoad %[[int]] %[[exp]]
  %call2 = call spir_func zeroext i1 @_Z28atomic_compare_exchange_weakPVU3AS4U7_AtomiciPU3AS4ii(i32 addrspace(4)* %4, i32 addrspace(4)* %5, i32 %6)
; CHECK-NEXT: %[[Result:[0-9]+]] = OpAtomicCompareExchangeWeak %[[int]] %[[Pointer]] %[[DeviceScope]] %[[SequentiallyConsistent_MS]] %[[SequentiallyConsistent_MS]] %[[Value]] %[[ComparatorWeak]]
; CHECK-NEXT: OpStore %[[exp]] %[[Result]]
; CHECK-NEXT: %[[CallRes:[0-9]+]] = OpIEqual %[[bool]] %[[Result]] %[[ComparatorWeak]]
; CHECK-NOT: %[[Result]]

  %frombool3 = zext i1 %call2 to i8
  store i8 %frombool3, i8* %weak_res, align 1
  %7 = load i8, i8* %weak_res, align 1
  %tobool4 = trunc i8 %7 to i1
  %lnot5 = xor i1 %tobool4, true
  %frombool6 = zext i1 %lnot5 to i8
  store i8 %frombool6, i8* %res, align 1
  ret void
}

declare spir_func zeroext i1 @_Z30atomic_compare_exchange_strongPVU3AS4U7_AtomiciPU3AS4ii(i32 addrspace(4)*, i32 addrspace(4)*, i32) #1

declare spir_func zeroext i1 @_Z28atomic_compare_exchange_weakPVU3AS4U7_AtomiciPU3AS4ii(i32 addrspace(4)*, i32 addrspace(4)*, i32) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}


!0 = !{i32 1, i32 2}
!1 = !{i32 2, i32 0}
!2 = !{}
