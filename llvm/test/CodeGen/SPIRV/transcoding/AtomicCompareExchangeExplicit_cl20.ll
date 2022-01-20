; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; __kernel void testAtomicCompareExchangeExplicit_cl20(
;     volatile global atomic_int* object,
;     global int* expected,
;     int desired)
; {
  ; Values of memory order and memory scope arguments correspond to SPIR-2.0 spec.
;   atomic_compare_exchange_strong_explicit(object, expected, desired,
;                                           memory_order_release, // 3
;                                           memory_order_relaxed  // 0
;                                          ); // by default, assume device scope = 2
;   atomic_compare_exchange_strong_explicit(object, expected, desired,
;                                           memory_order_acq_rel,   // 4
;                                           memory_order_relaxed,   // 0
;                                           memory_scope_work_group // 1
;                                          );
;   atomic_compare_exchange_weak_explicit(object, expected, desired,
;                                         memory_order_release, // 3
;                                         memory_order_relaxed  // 0
;                                          ); // by default, assume device scope = 2
;   atomic_compare_exchange_weak_explicit(object, expected, desired,
;                                         memory_order_acq_rel,   // 4
;                                         memory_order_relaxed,   // 0
;                                         memory_scope_work_group // 1
;                                        );
; }

; ModuleID = 'AtomicCompareExchangeExplicit_cl20.cl'
source_filename = "AtomicCompareExchangeExplicit_cl20.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

;CHECK-SPIRV: %[[int:[0-9]+]] = OpTypeInt 32 0
; Constants below correspond to the SPIR-V spec
;CHECK-SPIRV-DAG: %[[DeviceScope:[0-9]+]] = OpConstant %[[int]] 1
;CHECK-SPIRV-DAG: %[[WorkgroupScope:[0-9]+]] = OpConstant %[[int]] 2
;CHECK-SPIRV-DAG: %[[ReleaseMemSem:[0-9]+]] = OpConstant %[[int]] 4
;CHECK-SPIRV-DAG: %[[RelaxedMemSem:[0-9]+]] = OpConstant %[[int]] 0
;CHECK-SPIRV-DAG: %[[AcqRelMemSem:[0-9]+]] = OpConstant %[[int]] 8

;CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %{{[0-9]+}} %{{[0-9]+}} %[[DeviceScope]] %[[ReleaseMemSem]] %[[RelaxedMemSem]]
;CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %{{[0-9]+}} %{{[0-9]+}} %[[WorkgroupScope]] %[[AcqRelMemSem]] %[[RelaxedMemSem]]
;CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchangeWeak %{{[0-9]+}} %{{[0-9]+}} %[[DeviceScope]] %[[ReleaseMemSem]] %[[RelaxedMemSem]]
;CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchangeWeak %{{[0-9]+}} %{{[0-9]+}} %[[WorkgroupScope]] %[[AcqRelMemSem]] %[[RelaxedMemSem]]

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testAtomicCompareExchangeExplicit_cl20(i32 addrspace(1)* noundef %object, i32 addrspace(1)* noundef %expected, i32 noundef %desired) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %0 = addrspacecast i32 addrspace(1)* %object to i32 addrspace(4)*
  %1 = addrspacecast i32 addrspace(1)* %expected to i32 addrspace(4)*
  %call = call spir_func zeroext i1 @_Z39atomic_compare_exchange_strong_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_(i32 addrspace(4)* noundef %0, i32 addrspace(4)* noundef %1, i32 noundef %desired, i32 noundef 3, i32 noundef 0) #2
  %call1 = call spir_func zeroext i1 @_Z39atomic_compare_exchange_strong_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_12memory_scope(i32 addrspace(4)* noundef %0, i32 addrspace(4)* noundef %1, i32 noundef %desired, i32 noundef 4, i32 noundef 0, i32 noundef 1) #2
  %call2 = call spir_func zeroext i1 @_Z37atomic_compare_exchange_weak_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_(i32 addrspace(4)* noundef %0, i32 addrspace(4)* noundef %1, i32 noundef %desired, i32 noundef 3, i32 noundef 0) #2
  %call3 = call spir_func zeroext i1 @_Z37atomic_compare_exchange_weak_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_12memory_scope(i32 addrspace(4)* noundef %0, i32 addrspace(4)* noundef %1, i32 noundef %desired, i32 noundef 4, i32 noundef 0, i32 noundef 1) #2
  ret void
}

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z39atomic_compare_exchange_strong_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_(i32 addrspace(4)* noundef, i32 addrspace(4)* noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z39atomic_compare_exchange_strong_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_12memory_scope(i32 addrspace(4)* noundef, i32 addrspace(4)* noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z37atomic_compare_exchange_weak_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_(i32 addrspace(4)* noundef, i32 addrspace(4)* noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z37atomic_compare_exchange_weak_explicitPU3AS4VU7_AtomiciPU3AS4ii12memory_orderS4_12memory_scope(i32 addrspace(4)* noundef, i32 addrspace(4)* noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

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
!3 = !{i32 1, i32 1, i32 0}
!4 = !{!"none", !"none", !"none"}
!5 = !{!"atomic_int*", !"int*", !"int"}
!6 = !{!"_Atomic(int)*", !"int*", !"int"}
!7 = !{!"volatile", !"", !""}
