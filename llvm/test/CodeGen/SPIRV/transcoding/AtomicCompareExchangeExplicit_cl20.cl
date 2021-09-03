// RUN: clang -cc1 -nostdsysteminc -triple spir-unknown-unknown -O1 -cl-std=CL2.0 -finclude-default-header -emit-llvm %s -o - | sed -Ee 's#target triple = "spir-unknown-unknown"#target triple = "spirv32-unknown-unknown"#' | llc -O0 -global-isel -o - | FileCheck %s --check-prefix=CHECK-SPIRV

__kernel void testAtomicCompareExchangeExplicit_cl20(
    volatile global atomic_int* object,
    global int* expected,
    int desired)
{
  // Values of memory order and memory scope arguments correspond to SPIR-2.0 spec.
  atomic_compare_exchange_strong_explicit(object, expected, desired,
                                          memory_order_release, // 3
                                          memory_order_relaxed  // 0
                                         ); // by default, assume device scope = 2
  atomic_compare_exchange_strong_explicit(object, expected, desired,
                                          memory_order_acq_rel,   // 4
                                          memory_order_relaxed,   // 0
                                          memory_scope_work_group // 1
                                         );
  atomic_compare_exchange_weak_explicit(object, expected, desired,
                                        memory_order_release, // 3
                                        memory_order_relaxed  // 0
                                         ); // by default, assume device scope = 2
  atomic_compare_exchange_weak_explicit(object, expected, desired,
                                        memory_order_acq_rel,   // 4
                                        memory_order_relaxed,   // 0
                                        memory_scope_work_group // 1
                                       );
}

//CHECK-SPIRV: %[[int:[0-9]+]] = OpTypeInt 32 0
//; Constants below correspond to the SPIR-V spec
//CHECK-SPIRV-DAG: %[[DeviceScope:[0-9]+]] = OpConstant %[[int]] 1
//CHECK-SPIRV-DAG: %[[WorkgroupScope:[0-9]+]] = OpConstant %[[int]] 2
//CHECK-SPIRV-DAG: %[[ReleaseMemSem:[0-9]+]] = OpConstant %[[int]] 4
//CHECK-SPIRV-DAG: %[[RelaxedMemSem:[0-9]+]] = OpConstant %[[int]] 0
//CHECK-SPIRV-DAG: %[[AcqRelMemSem:[0-9]+]] = OpConstant %[[int]] 8

//CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %{{[0-9]+}} %{{[0-9]+}} %[[DeviceScope]] %[[ReleaseMemSem]] %[[RelaxedMemSem]]
//CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %{{[0-9]+}} %{{[0-9]+}} %[[WorkgroupScope]] %[[AcqRelMemSem]] %[[RelaxedMemSem]]
//CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchangeWeak %{{[0-9]+}} %{{[0-9]+}} %[[DeviceScope]] %[[ReleaseMemSem]] %[[RelaxedMemSem]]
//CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchangeWeak %{{[0-9]+}} %{{[0-9]+}} %[[WorkgroupScope]] %[[AcqRelMemSem]] %[[RelaxedMemSem]]
