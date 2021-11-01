; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; This test checks that the backend is capable to correctly translate
; atomic_work_item_fence OpenCL C 2.0 built-in function [1] into corresponding
; SPIR-V instruction [2].

; __kernel void test_mem_fence_const_flags() {
;   atomic_work_item_fence(CLK_LOCAL_MEM_FENCE, memory_order_relaxed, memory_scope_work_item);
;   atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire, memory_scope_work_group);
;   atomic_work_item_fence(CLK_IMAGE_MEM_FENCE, memory_order_release, memory_scope_device);
;   atomic_work_item_fence(CLK_LOCAL_MEM_FENCE, memory_order_acq_rel, memory_scope_all_svm_devices);
;   atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_seq_cst, memory_scope_sub_group);
;   atomic_work_item_fence(CLK_IMAGE_MEM_FENCE | CLK_LOCAL_MEM_FENCE, memory_order_acquire, memory_scope_sub_group);
; }

; __kernel void test_mem_fence_non_const_flags(cl_mem_fence_flags flags, memory_order order, memory_scope scope) {
;   // FIXME: OpenCL spec doesn't require flags to be compile-time known
;   // atomic_work_item_fence(flags, order, scope);
; }

; ModuleID = 'atomic_work_item_fence.cl'
source_filename = "atomic_work_item_fence.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpEntryPoint {{.*}} %[[TEST_CONST_FLAGS:[0-9]+]] "test_mem_fence_const_flags"
; CHECK-SPIRV: %[[UINT:[0-9]+]] = OpTypeInt 32 0
;
; 0x0 Relaxed + 0x100 WorkgroupMemory
; CHECK-SPIRV-DAG: %[[LOCAL_RELAXED:[0-9]+]] = OpConstant %[[UINT]] 256
; 0x2 Acquire + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[GLOBAL_ACQUIRE:[0-9]+]] = OpConstant %[[UINT]] 514
; 0x4 Release + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[IMAGE_RELEASE:[0-9]+]] = OpConstant %[[UINT]] 2052
; 0x8 AcquireRelease + 0x100 WorkgroupMemory
; CHECK-SPIRV-DAG: %[[LOCAL_ACQ_REL:[0-9]+]] = OpConstant %[[UINT]] 264
; 0x10 SequentiallyConsistent + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[GLOBAL_SEQ_CST:[0-9]+]] = OpConstant %[[UINT]] 528
; 0x2 Acquire + 0x100 WorkgroupMemory + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[LOCAL_IMAGE_ACQUIRE:[0-9]+]] = OpConstant %[[UINT]] 2306
;
; Scopes [4]:
; 4 Invocation
; CHECK-SPIRV-DAG: %[[SCOPE_INVOCATION:[0-9]+]] = OpConstant %[[UINT]] 4
; 2 Workgroup
; CHECK-SPIRV-DAG: %[[SCOPE_WORK_GROUP:[0-9]+]] = OpConstant %[[UINT]] 2
; 1 Device
; CHECK-SPIRV-DAG: %[[SCOPE_DEVICE:[0-9]+]] = OpConstant %[[UINT]] 1
; 0 CrossDevice
; CHECK-SPIRV-DAG: %[[SCOPE_CROSS_DEVICE:[0-9]+]] = OpConstant %[[UINT]] 0
; 3 Subgroup
; CHECK-SPIRV-DAG: %[[SCOPE_SUBGROUP:[0-9]+]] = OpConstant %[[UINT]] 3
;
; CHECK-SPIRV: %[[TEST_CONST_FLAGS]] = OpFunction %{{[0-9]+}}
; CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_INVOCATION]] %[[LOCAL_RELAXED]]
; CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_WORK_GROUP]] %[[GLOBAL_ACQUIRE]]
; CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_DEVICE]] %[[IMAGE_RELEASE]]
; CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_CROSS_DEVICE]] %[[LOCAL_ACQ_REL]]
; CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_SUBGROUP]] %[[GLOBAL_SEQ_CST]]
; CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_SUBGROUP]] %[[LOCAL_IMAGE_ACQUIRE]]

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_mem_fence_const_flags() local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 {
entry:
  tail call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 1, i32 0, i32 0) #3
  tail call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 2, i32 2, i32 1) #3
  tail call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 4, i32 3, i32 2) #3
  tail call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 1, i32 4, i32 3) #3
  tail call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 2, i32 5, i32 4) #3
  tail call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 5, i32 2, i32 4) #3
  ret void
}

; Function Attrs: convergent
declare spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32, i32, i32) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local spir_kernel void @test_mem_fence_non_const_flags(i32 %flags, i32 %order, i32 %scope) local_unnamed_addr #2 !kernel_arg_addr_space !4 !kernel_arg_access_qual !5 !kernel_arg_type !6 !kernel_arg_base_type !7 !kernel_arg_type_qual !8 {
entry:
  ret void
}

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { mustprogress nofree norecurse nosync nounwind readnone willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #3 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 969b72fb662b9dc2124c6eb7797feb7e3bdd38d5)"}
!3 = !{}
!4 = !{i32 0, i32 0, i32 0}
!5 = !{!"none", !"none", !"none"}
!6 = !{!"cl_mem_fence_flags", !"memory_order", !"memory_scope"}
!7 = !{!"uint", !"enum memory_order", !"enum memory_scope"}
!8 = !{!"", !"", !""}

; References:
; [1]: https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/atomic_work_item_fence.html
; [2]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#OpMemoryBarrier
