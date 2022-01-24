; RUN: llc -O0 %s -o - | FileCheck %s

; __kernel void test_fn( const __global char *src)
; {
; 	wait_group_events(0, NULL);
; }

; ModuleID = 'event_no_group_cap.cl'
source_filename = "event_no_group_cap.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK-NOT:OpCapability Groups
; CHECK:OpGroupWaitEvents

%opencl.event_t = type opaque

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @test_fn(i8 addrspace(1)* noundef %src) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %src.addr = alloca i8 addrspace(1)*, align 8
  store i8 addrspace(1)* %src, i8 addrspace(1)** %src.addr, align 8
  call spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32 noundef 0, %opencl.event_t* addrspace(4)* noundef null) #2
  ret void
}

; Function Attrs: convergent
declare spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32 noundef, %opencl.event_t* addrspace(4)* noundef) #1

attributes #0 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1}
!4 = !{!"none"}
!5 = !{!"char*"}
!6 = !{!"const"}
