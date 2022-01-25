; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'group_ops.cl'
source_filename = "group_ops.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV-DAG: %[[int:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[float:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV-DAG: %[[ScopeWorkgroup:[0-9]+]] = OpConstant %[[int]] 2
; CHECK-SPIRV-DAG: %[[ScopeSubgroup:[0-9]+]] = OpConstant %[[int]] 3

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupFMax %[[float]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupFMax(float a, global float *res) {
;   res[0] = work_group_reduce_max(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupFMax(float noundef %a, float addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func float @_Z21work_group_reduce_maxf(float noundef %a) #2
  store float %call, float addrspace(1)* %res, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare spir_func float @_Z21work_group_reduce_maxf(float noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupFMin %[[float]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupFMin(float a, global float *res) {
;   res[0] = work_group_reduce_min(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupFMin(float noundef %a, float addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func float @_Z21work_group_reduce_minf(float noundef %a) #2
  store float %call, float addrspace(1)* %res, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare spir_func float @_Z21work_group_reduce_minf(float noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupFAdd %[[float]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupFAdd(float a, global float *res) {
;   res[0] = work_group_reduce_add(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupFAdd(float noundef %a, float addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func float @_Z21work_group_reduce_addf(float noundef %a) #2
  store float %call, float addrspace(1)* %res, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare spir_func float @_Z21work_group_reduce_addf(float noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupFMax %[[float]] %[[ScopeWorkgroup]] InclusiveScan
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupScanInclusiveFMax(float a, global float *res) {
;   res[0] = work_group_scan_inclusive_max(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupScanInclusiveFMax(float noundef %a, float addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func float @_Z29work_group_scan_inclusive_maxf(float noundef %a) #2
  store float %call, float addrspace(1)* %res, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare spir_func float @_Z29work_group_scan_inclusive_maxf(float noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupFMax %[[float]] %[[ScopeWorkgroup]] ExclusiveScan
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupScanExclusiveFMax(float a, global float *res) {
;   res[0] = work_group_scan_exclusive_max(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupScanExclusiveFMax(float noundef %a, float addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func float @_Z29work_group_scan_exclusive_maxf(float noundef %a) #2
  store float %call, float addrspace(1)* %res, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare spir_func float @_Z29work_group_scan_exclusive_maxf(float noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupSMax %[[int]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupSMax(int a, global int *res) {
;   res[0] = work_group_reduce_max(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupSMax(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !11 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z21work_group_reduce_maxi(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z21work_group_reduce_maxi(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupSMin %[[int]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupSMin(int a, global int *res) {
;   res[0] = work_group_reduce_min(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupSMin(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !11 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z21work_group_reduce_mini(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z21work_group_reduce_mini(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupIAdd %[[int]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupIAddSigned(int a, global int *res) {
;   res[0] = work_group_reduce_add(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupIAddSigned(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !11 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z21work_group_reduce_addi(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z21work_group_reduce_addi(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupIAdd %[[int]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupIAddUnsigned(uint a, global uint *res) {
;   res[0] = work_group_reduce_add(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupIAddUnsigned(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z21work_group_reduce_addj(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z21work_group_reduce_addj(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupUMax %[[int]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupUMax(uint a, global uint *res) {
;   res[0] = work_group_reduce_max(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupUMax(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z21work_group_reduce_maxj(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z21work_group_reduce_maxj(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupUMax %[[int]] %[[ScopeSubgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; #pragma OPENCL EXTENSION cl_khr_subgroups: enable
; kernel void testSubGroupUMax(uint a, global uint *res) {
;   res[0] = sub_group_reduce_max(a);
; }
; #pragma OPENCL EXTENSION cl_khr_subgroups: disable

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testSubGroupUMax(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z20sub_group_reduce_maxj(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z20sub_group_reduce_maxj(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupUMax %[[int]] %[[ScopeWorkgroup]] InclusiveScan
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupScanInclusiveUMax(uint a, global uint *res) {
;   res[0] = work_group_scan_inclusive_max(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupScanInclusiveUMax(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z29work_group_scan_inclusive_maxj(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z29work_group_scan_inclusive_maxj(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupUMax %[[int]] %[[ScopeWorkgroup]] ExclusiveScan
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupScanExclusiveUMax(uint a, global uint *res) {
;   res[0] = work_group_scan_exclusive_max(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupScanExclusiveUMax(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z29work_group_scan_exclusive_maxj(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z29work_group_scan_exclusive_maxj(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupUMin %[[int]] %[[ScopeWorkgroup]] Reduce
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupUMin(uint a, global uint *res) {
;   res[0] = work_group_reduce_min(a);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupUMin(i32 noundef %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func i32 @_Z21work_group_reduce_minj(i32 noundef %a) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z21work_group_reduce_minj(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupBroadcast %[[int]] %[[ScopeWorkgroup]]
; CHECK-SPIRV: OpFunctionEnd

; kernel void testWorkGroupBroadcast(uint a, global size_t *id, global int *res) {
;   res[0] = work_group_broadcast(a, *id);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testWorkGroupBroadcast(i32 noundef %a, i32 addrspace(1)* nocapture noundef readonly %id, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !15 !kernel_arg_access_qual !16 !kernel_arg_type !17 !kernel_arg_base_type !18 !kernel_arg_type_qual !19 {
entry:
  %0 = load i32, i32 addrspace(1)* %id, align 4, !tbaa !12
  %call = call spir_func i32 @_Z20work_group_broadcastjj(i32 noundef %a, i32 noundef %0) #2
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z20work_group_broadcastjj(i32 noundef, i32 noundef) local_unnamed_addr #1

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 0, i32 1}
!4 = !{!"none", !"none"}
!5 = !{!"float", !"float*"}
!6 = !{!"", !""}
!7 = !{!8, !8, i64 0}
!8 = !{!"float", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!"int", !"int*"}
!12 = !{!13, !13, i64 0}
!13 = !{!"int", !9, i64 0}
!14 = !{!"uint", !"uint*"}
!15 = !{i32 0, i32 1, i32 1}
!16 = !{!"none", !"none", !"none"}
!17 = !{!"uint", !"size_t*", !"int*"}
!18 = !{!"uint", !"uint*", !"int*"}
!19 = !{!"", !"", !""}
