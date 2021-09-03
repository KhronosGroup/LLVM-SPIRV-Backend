// RUN: clang -cc1 -nostdsysteminc -triple spir-unknown-unknown -O1 -cl-std=CL2.0 -finclude-default-header -emit-llvm %s -o - | sed -Ee 's#target triple = "spir-unknown-unknown"#target triple = "spirv32-unknown-unknown"#' | llc -O0 -global-isel -o - | FileCheck %s --check-prefix=CHECK-SPIRV

kernel void testAtomicFlag(global int *res) {
  atomic_flag f;

  *res = atomic_flag_test_and_set(&f);
  *res += atomic_flag_test_and_set_explicit(&f, memory_order_seq_cst);
  *res += atomic_flag_test_and_set_explicit(&f, memory_order_seq_cst, memory_scope_work_group);

  atomic_flag_clear(&f);
  atomic_flag_clear_explicit(&f, memory_order_seq_cst);
  atomic_flag_clear_explicit(&f, memory_order_seq_cst, memory_scope_work_group);
}

// CHECK-SPIRV: OpAtomicFlagTestAndSet
// CHECK-SPIRV: OpAtomicFlagTestAndSet
// CHECK-SPIRV: OpAtomicFlagTestAndSet
// CHECK-SPIRV: OpAtomicFlagClear
// CHECK-SPIRV: OpAtomicFlagClear
// CHECK-SPIRV: OpAtomicFlagClear
