; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

; ModuleID = 'test.cl'
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK: OpName %[[WD:[0-9]+]] "__spirv_BuiltInWorkDim"
; CHECK: OpName %[[GS:[0-9]+]] "__spirv_BuiltInGlobalSize"
; CHECK: OpName %[[GII:[0-9]+]] "__spirv_BuiltInGlobalInvocationId"
; CHECK: OpName %[[WS:[0-9]+]] "__spirv_BuiltInWorkgroupSize"
; CHECK: OpName %[[EWS:[0-9]+]] "__spirv_BuiltInEnqueuedWorkgroupSize"
; CHECK: OpName %[[LLI:[0-9]+]] "__spirv_BuiltInLocalInvocationId"
; CHECK: OpName %[[NW:[0-9]+]] "__spirv_BuiltInNumWorkgroups"
; CHECK: OpName %[[WI:[0-9]+]] "__spirv_BuiltInWorkgroupId"
; CHECK: OpName %[[GO:[0-9]+]] "__spirv_BuiltInGlobalOffset"
; CHECK: OpName %[[GLI:[0-9]+]] "__spirv_BuiltInGlobalLinearId"
; CHECK: OpName %[[LLII:[0-9]+]] "__spirv_BuiltInLocalInvocationIndex"
; CHECK: OpName %[[SS:[0-9]+]] "__spirv_BuiltInSubgroupSize"
; CHECK: OpName %[[SMS:[0-9]+]] "__spirv_BuiltInSubgroupMaxSize"
; CHECK: OpName %[[NS:[0-9]+]] "__spirv_BuiltInNumSubgroups"
; CHECK: OpName %[[NES:[0-9]+]] "__spirv_BuiltInNumEnqueuedSubgroups"
; CHECK: OpName %[[SI:[0-9]+]] "__spirv_BuiltInSubgroupId"
; CHECK: OpName %[[SLII:[0-9]+]] "__spirv_BuiltInSubgroupLocalInvocationId"

; CHECK: OpDecorate %[[NW]] BuiltIn NumWorkgroups
; CHECK: OpDecorate %[[WS]] BuiltIn WorkgroupSize
; CHECK: OpDecorate %[[WI]] BuiltIn WorkgroupId
; CHECK: OpDecorate %[[LLI]] BuiltIn LocalInvocationId
; CHECK: OpDecorate %[[GII]] BuiltIn GlobalInvocationId
; CHECK: OpDecorate %[[LLII]] BuiltIn LocalInvocationIndex
; CHECK: OpDecorate %[[WD]] BuiltIn WorkDim
; CHECK: OpDecorate %[[GS]] BuiltIn GlobalSize
; CHECK: OpDecorate %[[EWS]] BuiltIn EnqueuedWorkgroupSize
; CHECK: OpDecorate %[[GO]] BuiltIn GlobalOffset
; CHECK: OpDecorate %[[GLI]] BuiltIn GlobalLinearId
; CHECK: OpDecorate %[[SS]] BuiltIn SubgroupSize
; CHECK: OpDecorate %[[SMS]] BuiltIn SubgroupMaxSize
; CHECK: OpDecorate %[[NS]] BuiltIn NumSubgroups
; CHECK: OpDecorate %[[NES]] BuiltIn NumEnqueuedSubgroups
; CHECK: OpDecorate %[[SI]] BuiltIn SubgroupId
; CHECK: OpDecorate %[[SLII]] BuiltIn SubgroupLocalInvocationId
@__spirv_BuiltInWorkDim = external addrspace(1) global i32
@__spirv_BuiltInGlobalSize = external addrspace(1) global <3 x i32>
@__spirv_BuiltInGlobalInvocationId = external addrspace(1) global <3 x i32>
@__spirv_BuiltInWorkgroupSize = external addrspace(1) global <3 x i32>
@__spirv_BuiltInEnqueuedWorkgroupSize = external addrspace(1) global <3 x i32>
@__spirv_BuiltInLocalInvocationId = external addrspace(1) global <3 x i32>
@__spirv_BuiltInNumWorkgroups = external addrspace(1) global <3 x i32>
@__spirv_BuiltInWorkgroupId = external addrspace(1) global <3 x i32>
@__spirv_BuiltInGlobalOffset = external addrspace(1) global <3 x i32>
@__spirv_BuiltInGlobalLinearId = external addrspace(1) global i32
@__spirv_BuiltInLocalInvocationIndex = external addrspace(1) global i32
@__spirv_BuiltInSubgroupSize = external addrspace(1) global i32
@__spirv_BuiltInSubgroupMaxSize = external addrspace(1) global i32
@__spirv_BuiltInNumSubgroups = external addrspace(1) global i32
@__spirv_BuiltInNumEnqueuedSubgroups = external addrspace(1) global i32
@__spirv_BuiltInSubgroupId = external addrspace(1) global i32
@__spirv_BuiltInSubgroupLocalInvocationId = external addrspace(1) global i32

; Function Attrs: nounwind readnone
define spir_kernel void @_Z1wv() #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
entry:
  ret void
}

attributes #0 = { alwaysinline nounwind readonly "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!8}
!opencl.compiler.options = !{!8}
!llvm.ident = !{!9}
!spirv.Source = !{!10}
!spirv.String = !{!11}

!0 = !{}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 1}
!8 = !{}
!9 = !{!"clang version 3.6.1 "}
!10 = !{i32 3, i32 200000, !11}
!11 = !{!"test.cl"}
!12 = !{!13, !13, i64 0}
!13 = !{!"int", !14, i64 0}
!14 = !{!"omnipotent char", !15, i64 0}
!15 = !{!"Simple C/C++ TBAA"}
