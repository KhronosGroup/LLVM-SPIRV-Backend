; RUN: llc -O0 -mtriple=spirv64-unknown-unknown %s -o - | FileCheck %s

; __kernel void test_fn( const __global char *src)
; {
;     wait_group_events(0, NULL);
; }

; CHECK-NOT: OpCapability Groups
; CHECK: OpGroupWaitEvents

define dso_local spir_kernel void @test_fn(ptr addrspace(1) %src) {
  %src.addr = alloca ptr addrspace(1), align 8
  store ptr addrspace(1) %src, ptr %src.addr, align 8
  call spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32 0, ptr addrspace(4) null)
  ret void
}

declare spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32, ptr addrspace(4))
