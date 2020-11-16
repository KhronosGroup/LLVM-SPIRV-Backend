; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV:ConvertPtrToU 
; CHECK-SPIRV:ConvertPtrToU
; CHECK-SPIRV:INotEqual
; CHECK-SPIRV:ConvertPtrToU
; CHECK-SPIRV:ConvertPtrToU
; CHECK-SPIRV:IEqual
; CHECK-SPIRV:ConvertPtrToU
; CHECK-SPIRV:ConvertPtrToU
; CHECK-SPIRV:UGreaterThan
; CHECK-SPIRV:ConvertPtrToU
; CHECK-SPIRV:ConvertPtrToU
; CHECK-SPIRV:ULessThan

; ModuleID = 'ComparePointers.cl'
source_filename = "ComparePointers.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; Function Attrs: convergent noinline norecurse nounwind optnone
define spir_kernel void @test(i32 addrspace(1)* %in, i32 addrspace(1)* %in2) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %in.addr = alloca i32 addrspace(1)*, align 8
  %in2.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %in, i32 addrspace(1)** %in.addr, align 8
  store i32 addrspace(1)* %in2, i32 addrspace(1)** %in2.addr, align 8
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %in.addr, align 8
  %tobool = icmp ne i32 addrspace(1)* %0, null
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  br label %if.end8

if.end:                                           ; preds = %entry
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %in.addr, align 8
  %cmp = icmp eq i32 addrspace(1)* %1, inttoptr (i64 1 to i32 addrspace(1)*)
  br i1 %cmp, label %if.then1, label %if.end2

if.then1:                                         ; preds = %if.end
  br label %if.end8

if.end2:                                          ; preds = %if.end
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %in.addr, align 8
  %3 = load i32 addrspace(1)*, i32 addrspace(1)** %in2.addr, align 8
  %cmp3 = icmp ugt i32 addrspace(1)* %2, %3
  br i1 %cmp3, label %if.then4, label %if.end5

if.then4:                                         ; preds = %if.end2
  br label %if.end8

if.end5:                                          ; preds = %if.end2
  %4 = load i32 addrspace(1)*, i32 addrspace(1)** %in.addr, align 8
  %5 = load i32 addrspace(1)*, i32 addrspace(1)** %in2.addr, align 8
  %cmp6 = icmp ult i32 addrspace(1)* %4, %5
  br i1 %cmp6, label %if.then7, label %if.end8

if.then7:                                         ; preds = %if.end5
  br label %if.end8

if.end8:                                          ; preds = %if.then, %if.then1, %if.then4, %if.then7, %if.end5
  ret void
}

attributes #0 = { convergent noinline norecurse nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 12.0.0"}
!3 = !{i32 1, i32 1}
!4 = !{!"none", !"none"}
!5 = !{!"int*", !"int*"}
!6 = !{!"", !""}
