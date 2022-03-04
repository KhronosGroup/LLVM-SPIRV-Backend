; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; This test checks that the backend is capable to correctly translate
; sub_group_barrier built-in function [1] from cl_khr_subgroups extension into
; corresponding SPIR-V instruction.

; __kernel void test_barrier_const_flags() {
;   work_group_barrier(CLK_LOCAL_MEM_FENCE);
;   work_group_barrier(CLK_GLOBAL_MEM_FENCE);
;   work_group_barrier(CLK_IMAGE_MEM_FENCE);
;
;   work_group_barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
;   work_group_barrier(CLK_LOCAL_MEM_FENCE | CLK_IMAGE_MEM_FENCE);
;   work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE | CLK_IMAGE_MEM_FENCE);
;
;   work_group_barrier(CLK_LOCAL_MEM_FENCE, memory_scope_work_item);
;   work_group_barrier(CLK_LOCAL_MEM_FENCE, memory_scope_work_group);
;   work_group_barrier(CLK_LOCAL_MEM_FENCE, memory_scope_device);
;   work_group_barrier(CLK_LOCAL_MEM_FENCE, memory_scope_all_svm_devices);
;   work_group_barrier(CLK_LOCAL_MEM_FENCE, memory_scope_sub_group);
;
  ; barrier should also work (preserved for backward compatibility)
;   barrier(CLK_GLOBAL_MEM_FENCE);
; }
;
; __kernel void test_barrier_non_const_flags(cl_mem_fence_flags flags, memory_scope scope) {
  ; FIXME: OpenCL spec doesn't require flags to be compile-time known
  ; work_group_barrier(flags);
  ; work_group_barrier(flags, scope);
; }

; ModuleID = 'work_group_barrier.cl'
source_filename = "work_group_barrier.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpName %[[TEST_CONST_FLAGS:[0-9]+]] "test_barrier_const_flags"
; CHECK-SPIRV: %[[UINT:[0-9]+]] = OpTypeInt 32 0
;
; In SPIR-V, barrier is represented as OpControlBarrier [2] and OpenCL
; cl_mem_fence_flags are represented as part of Memory Semantics [3], which
; also includes memory order constraints. The backend applies some default
; memory order for OpControlBarrier and therefore, constants below include a
; bit more information than original source
;
; 0x10 SequentiallyConsistent + 0x100 WorkgroupMemory
; CHECK-SPIRV-DAG: %[[LOCAL:[0-9]+]] = OpConstant %[[UINT]] 272
; 0x10 SequentiallyConsistent + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 528
; 0x10 SequentiallyConsistent + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[IMAGE:[0-9]+]] = OpConstant %[[UINT]] 2064
; 0x10 SequentiallyConsistent + 0x100 WorkgroupMemory + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[LOCAL_GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 784
; 0x10 SequentiallyConsistent + 0x100 WorkgroupMemory + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[LOCAL_IMAGE:[0-9]+]] = OpConstant %[[UINT]] 2320
; 0x10 SequentiallyConsistent + 0x100 WorkgroupMemory + 0x200 CrossWorkgroupMemory + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[LOCAL_GLOBAL_IMAGE:[0-9]+]] = OpConstant %[[UINT]] 2832
;
; Scopes [4]:
; 2 Workgroup
; CHECK-SPIRV-DAG: %[[SCOPE_WORK_GROUP:[0-9]+]] = OpConstant %[[UINT]] 2
; 4 Invocation
; CHECK-SPIRV-DAG: %[[SCOPE_INVOCATION:[0-9]+]] = OpConstant %[[UINT]] 4
; 1 Device
; CHECK-SPIRV-DAG: %[[SCOPE_DEVICE:[0-9]+]] = OpConstant %[[UINT]] 1
; 0 CrossDevice
; CHECK-SPIRV-DAG: %[[SCOPE_CROSS_DEVICE:[0-9]+]] = OpConstant %[[UINT]] 0
; 3 Subgroup
; CHECK-SPIRV-DAG: %[[SCOPE_SUBGROUP:[0-9]+]] = OpConstant %[[UINT]] 3
;
; CHECK-SPIRV: %[[TEST_CONST_FLAGS]] = OpFunction %{{[0-9]+}}
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[LOCAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[GLOBAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[IMAGE]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[LOCAL_GLOBAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[LOCAL_IMAGE]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[LOCAL_GLOBAL_IMAGE]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_INVOCATION]] %[[LOCAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[LOCAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_DEVICE]] %[[LOCAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_CROSS_DEVICE]] %[[LOCAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_SUBGROUP]] %[[LOCAL]]
; CHECK-SPIRV: OpControlBarrier %[[SCOPE_WORK_GROUP]] %[[SCOPE_WORK_GROUP]] %[[GLOBAL]]

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_barrier_const_flags() local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 {
entry:
  tail call spir_func void @_Z18work_group_barrierj(i32 noundef 1) #3
  tail call spir_func void @_Z18work_group_barrierj(i32 noundef 2) #3
  tail call spir_func void @_Z18work_group_barrierj(i32 noundef 4) #3
  tail call spir_func void @_Z18work_group_barrierj(i32 noundef 3) #3
  tail call spir_func void @_Z18work_group_barrierj(i32 noundef 5) #3
  tail call spir_func void @_Z18work_group_barrierj(i32 noundef 7) #3
  tail call spir_func void @_Z18work_group_barrierj12memory_scope(i32 noundef 1, i32 noundef 0) #3
  tail call spir_func void @_Z18work_group_barrierj12memory_scope(i32 noundef 1, i32 noundef 1) #3
  tail call spir_func void @_Z18work_group_barrierj12memory_scope(i32 noundef 1, i32 noundef 2) #3
  tail call spir_func void @_Z18work_group_barrierj12memory_scope(i32 noundef 1, i32 noundef 3) #3
  tail call spir_func void @_Z18work_group_barrierj12memory_scope(i32 noundef 1, i32 noundef 4) #3
  tail call spir_func void @_Z7barrierj(i32 noundef 2) #3
  ret void
}

; Function Attrs: convergent
declare spir_func void @_Z18work_group_barrierj(i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z18work_group_barrierj12memory_scope(i32 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z7barrierj(i32 noundef) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local spir_kernel void @test_barrier_non_const_flags(i32 noundef %flags, i32 noundef %scope) local_unnamed_addr #2 !kernel_arg_addr_space !4 !kernel_arg_access_qual !5 !kernel_arg_type !6 !kernel_arg_base_type !7 !kernel_arg_type_qual !8 {
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
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{}
!4 = !{i32 0, i32 0}
!5 = !{!"none", !"none"}
!6 = !{!"cl_mem_fence_flags", !"memory_scope"}
!7 = !{!"uint", !"enum memory_scope"}
!8 = !{!"", !""}

; References:
; [1]: https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/work_group_barrier.html
; [2]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#OpControlBarrier
; [3]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#_a_id_memory_semantics__id_a_memory_semantics_lt_id_gt
