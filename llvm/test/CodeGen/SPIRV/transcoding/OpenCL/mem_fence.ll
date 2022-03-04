; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; This test checks that the backend is capable to correctly translate
; mem_fence OpenCL C 1.2 built-in function [1] into corresponding SPIR-V
; instruction.

; Strictly speaking, this flag is not supported by mem_fence in OpenCL 1.2
; #define CLK_IMAGE_MEM_FENCE 0x04
;
; __kernel void test_mem_fence_const_flags() {
;   mem_fence(CLK_LOCAL_MEM_FENCE);
;   mem_fence(CLK_GLOBAL_MEM_FENCE);
;   mem_fence(CLK_IMAGE_MEM_FENCE);
;
;   mem_fence(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
;   mem_fence(CLK_LOCAL_MEM_FENCE | CLK_IMAGE_MEM_FENCE);
;   mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE | CLK_IMAGE_MEM_FENCE);
;
;   read_mem_fence(CLK_LOCAL_MEM_FENCE);
;   read_mem_fence(CLK_GLOBAL_MEM_FENCE);
;   read_mem_fence(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
;
;   write_mem_fence(CLK_LOCAL_MEM_FENCE);
;   write_mem_fence(CLK_GLOBAL_MEM_FENCE);
;   write_mem_fence(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
; }
;
; __kernel void test_mem_fence_non_const_flags(cl_mem_fence_flags flags) {
  ; FIXME: OpenCL spec doesn't require flags to be compile-time known
  ; mem_fence(flags);
; }

; ModuleID = 'mem_fence.cl'
source_filename = "mem_fence.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpName %[[TEST_CONST_FLAGS:[0-9]+]] "test_mem_fence_const_flags"
; CHECK-SPIRV: %[[UINT:[0-9]+]] = OpTypeInt 32 0
;
; In SPIR-V, mem_fence is represented as OpMemoryBarrier [2] and OpenCL
; cl_mem_fence_flags are represented as part of Memory Semantics [3], which
; also includes memory order constraints. The backend applies some default
; memory order for OpMemoryBarrier and therefore, constants below include a
; bit more information than original source
;
; 0x2 Workgroup
; CHECK-SPIRV-DAG: %[[WG:[0-9]+]] = OpConstant %[[UINT]] 2
;
; 0x2 Acquire + 0x100 WorkgroupMemory
; CHECK-SPIRV-DAG: %[[ACQ_LOCAL:[0-9]+]] = OpConstant %[[UINT]] 258
; 0x2 Acquire + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[ACQ_GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 514
; 0x2 Acquire + 0x100 WorkgroupMemory + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[ACQ_LOCAL_GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 770
;
; 0x4 Release + 0x100 WorkgroupMemory
; CHECK-SPIRV-DAG: %[[REL_LOCAL:[0-9]+]] = OpConstant %[[UINT]] 260
; 0x4 Release + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[REL_GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 516
; 0x4 Release + 0x100 WorkgroupMemory + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[REL_LOCAL_GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 772
;
; 0x8 AcquireRelease + 0x100 WorkgroupMemory
; CHECK-SPIRV-DAG: %[[ACQ_REL_LOCAL:[0-9]+]] = OpConstant %[[UINT]] 264
; 0x8 AcquireRelease + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[ACQ_REL_GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 520
; 0x8 AcquireRelease + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[ACQ_REL_IMAGE:[0-9]+]] = OpConstant %[[UINT]] 2056
; 0x8 AcquireRelease + 0x100 WorkgroupMemory + 0x200 CrossWorkgroupMemory
; CHECK-SPIRV-DAG: %[[ACQ_REL_LOCAL_GLOBAL:[0-9]+]] = OpConstant %[[UINT]] 776
; 0x8 AcquireRelease + 0x100 WorkgroupMemory + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[ACQ_REL_LOCAL_IMAGE:[0-9]+]] = OpConstant %[[UINT]] 2312
; 0x8 AcquireRelease + 0x100 WorkgroupMemory + 0x200 CrossWorkgroupMemory + 0x800 ImageMemory
; CHECK-SPIRV-DAG: %[[ACQ_REL_LOCAL_GLOBAL_IMAGE:[0-9]+]] = OpConstant %[[UINT]] 2824
;
; CHECK-SPIRV: %[[TEST_CONST_FLAGS]] = OpFunction %{{[0-9]+}}
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_REL_LOCAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_REL_GLOBAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_REL_IMAGE]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_REL_LOCAL_GLOBAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_REL_LOCAL_IMAGE]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_REL_LOCAL_GLOBAL_IMAGE]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_LOCAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_GLOBAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[ACQ_LOCAL_GLOBAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[REL_LOCAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[REL_GLOBAL]]
; CHECK-SPIRV: OpMemoryBarrier %[[WG]] %[[REL_LOCAL_GLOBAL]]

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_mem_fence_const_flags() local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !3 !kernel_arg_type !3 !kernel_arg_base_type !3 !kernel_arg_type_qual !3 {
entry:
  tail call spir_func void @_Z9mem_fencej(i32 noundef 1) #3
  tail call spir_func void @_Z9mem_fencej(i32 noundef 2) #3
  tail call spir_func void @_Z9mem_fencej(i32 noundef 4) #3
  tail call spir_func void @_Z9mem_fencej(i32 noundef 3) #3
  tail call spir_func void @_Z9mem_fencej(i32 noundef 5) #3
  tail call spir_func void @_Z9mem_fencej(i32 noundef 7) #3
  tail call spir_func void @_Z14read_mem_fencej(i32 noundef 1) #3
  tail call spir_func void @_Z14read_mem_fencej(i32 noundef 2) #3
  tail call spir_func void @_Z14read_mem_fencej(i32 noundef 3) #3
  tail call spir_func void @_Z15write_mem_fencej(i32 noundef 1) #3
  tail call spir_func void @_Z15write_mem_fencej(i32 noundef 2) #3
  tail call spir_func void @_Z15write_mem_fencej(i32 noundef 3) #3
  ret void
}

; Function Attrs: convergent
declare spir_func void @_Z9mem_fencej(i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z14read_mem_fencej(i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z15write_mem_fencej(i32 noundef) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local spir_kernel void @test_mem_fence_non_const_flags(i32 noundef %flags) local_unnamed_addr #2 !kernel_arg_addr_space !4 !kernel_arg_access_qual !5 !kernel_arg_type !6 !kernel_arg_base_type !7 !kernel_arg_type_qual !8 {
entry:
  ret void
}

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="true" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { mustprogress nofree norecurse nosync nounwind readnone willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="true" }
attributes #3 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{}
!4 = !{i32 0}
!5 = !{!"none"}
!6 = !{!"cl_mem_fence_flags"}
!7 = !{!"uint"}
!8 = !{!""}

; References:
; [1]: https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/mem_fence.html
; [2]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#OpMemoryBarrier
; [3]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#_a_id_memory_semantics__id_a_memory_semantics_lt_id_gt
