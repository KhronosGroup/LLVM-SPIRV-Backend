; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'enqueue_marker.cl'
source_filename = "enqueue_marker.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; kernel void test_enqueue_marker(global int *out) {
;   queue_t queue = get_default_queue();
;
;   clk_event_t waitlist, evt;
;
  ; CHECK-SPIRV: OpEnqueueMarker
;   *out = enqueue_marker(queue, 1, &waitlist, &evt);
; }

%opencl.queue_t = type opaque
%opencl.clk_event_t = type opaque

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @test_enqueue_marker(i32 addrspace(1)* noundef %out) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %out.addr = alloca i32 addrspace(1)*, align 4
  %queue = alloca %opencl.queue_t*, align 4
  %waitlist = alloca %opencl.clk_event_t*, align 4
  %evt = alloca %opencl.clk_event_t*, align 4
  store i32 addrspace(1)* %out, i32 addrspace(1)** %out.addr, align 4
  %call = call spir_func %opencl.queue_t* @_Z17get_default_queuev() #2
  store %opencl.queue_t* %call, %opencl.queue_t** %queue, align 4
  %0 = load %opencl.queue_t*, %opencl.queue_t** %queue, align 4
  %waitlist.ascast = addrspacecast %opencl.clk_event_t** %waitlist to %opencl.clk_event_t* addrspace(4)*
  %evt.ascast = addrspacecast %opencl.clk_event_t** %evt to %opencl.clk_event_t* addrspace(4)*
  %call1 = call spir_func i32 @_Z14enqueue_marker9ocl_queuejPU3AS4K12ocl_clkeventPU3AS4S0_(%opencl.queue_t* %0, i32 noundef 1, %opencl.clk_event_t* addrspace(4)* noundef %waitlist.ascast, %opencl.clk_event_t* addrspace(4)* noundef %evt.ascast) #2
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %out.addr, align 4
  store i32 %call1, i32 addrspace(1)* %1, align 4
  ret void
}

; Function Attrs: convergent
declare spir_func %opencl.queue_t* @_Z17get_default_queuev() #1

; Function Attrs: convergent
declare spir_func i32 @_Z14enqueue_marker9ocl_queuejPU3AS4K12ocl_clkeventPU3AS4S0_(%opencl.queue_t*, i32 noundef, %opencl.clk_event_t* addrspace(4)* noundef, %opencl.clk_event_t* addrspace(4)* noundef) #1

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
!5 = !{!"int*"}
!6 = !{!""}
