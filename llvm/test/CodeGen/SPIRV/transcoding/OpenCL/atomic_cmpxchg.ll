; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; This test checks that the backend is capable to correctly translate
; atomic_cmpxchg OpenCL C 1.2 built-in function [1] into corresponding SPIR-V
; instruction.

; __kernel void test_atomic_cmpxchg(__global int *p, int cmp, int val) {
;   atomic_cmpxchg(p, cmp, val);
; 
;   __global unsigned int *up = (__global unsigned int *)p;
;   unsigned int ucmp = (unsigned int)cmp;
;   unsigned int uval = (unsigned int)val;
;   atomic_cmpxchg(up, ucmp, uval);
; }

; ModuleID = 'atomic_cmpxchg.cl'
source_filename = "atomic_cmpxchg.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpEntryPoint {{.*}} %[[TEST:[0-9]+]] "test_atomic_cmpxchg"
; CHECK-SPIRV-DAG: %[[UINT:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[UINT_PTR:[0-9]+]] = OpTypePointer CrossWorkgroup %[[UINT]]
;
; In SPIR-V, atomic_cmpxchg is represented as OpAtomicCompareExchange [2],
; which also includes memory scope and two memory semantic arguments. The
; backend applies some default memory order for it and therefore, constants
; below include a bit more information than original source
;
; 0x2 Workgroup
; CHECK-SPIRV-DAG: %[[WORKGROUP_SCOPE:[0-9]+]] = OpConstant %[[UINT]] 2
;
; 0x0 Relaxed
; TODO: do we need CrossWorkgroupMemory here as well?
; CHECK-SPIRV-DAG: %[[RELAXED:[0-9]+]] = OpConstant %[[UINT]] 0
;
; CHECK-SPIRV: %[[TEST]] = OpFunction %{{[0-9]+}}
; CHECK-SPIRV: %[[PTR:[0-9]+]] = OpFunctionParameter %[[UINT_PTR]]
; CHECK-SPIRV: %[[CMP:[0-9]+]] = OpFunctionParameter %[[UINT]]
; CHECK-SPIRV: %[[VAL:[0-9]+]] = OpFunctionParameter %[[UINT]]
; CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %[[UINT]] %[[PTR]] %[[WORKGROUP_SCOPE]] %[[RELAXED]] %[[RELAXED]] %[[VAL]] %[[CMP]]
; CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %[[UINT]] %[[PTR]] %[[WORKGROUP_SCOPE]] %[[RELAXED]] %[[RELAXED]] %[[VAL]] %[[CMP]]

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_atomic_cmpxchg(i32 addrspace(1)* %p, i32 %cmp, i32 %val) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = tail call spir_func i32 @_Z14atomic_cmpxchgPU3AS1Viii(i32 addrspace(1)* %p, i32 %cmp, i32 %val) #2
  %call1 = tail call spir_func i32 @_Z14atomic_cmpxchgPU3AS1Vjjj(i32 addrspace(1)* %p, i32 %cmp, i32 %val) #2
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z14atomic_cmpxchgPU3AS1Viii(i32 addrspace(1)*, i32, i32) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func i32 @_Z14atomic_cmpxchgPU3AS1Vjjj(i32 addrspace(1)*, i32, i32) local_unnamed_addr #1

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="true" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 969b72fb662b9dc2124c6eb7797feb7e3bdd38d5)"}
!3 = !{i32 1, i32 0, i32 0}
!4 = !{!"none", !"none", !"none"}
!5 = !{!"int*", !"int", !"int"}
!6 = !{!"", !"", !""}

; References:
; [1]: https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/atomic_cmpxchg.html
; [2]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#OpAtomicCompareExchange
