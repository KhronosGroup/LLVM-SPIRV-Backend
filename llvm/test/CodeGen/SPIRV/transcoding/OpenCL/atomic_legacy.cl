// RUN: clang -cc1 -nostdsysteminc %s -triple spir -cl-std=CL1.2 -emit-llvm -fdeclare-opencl-builtins -o - | sed -Ee 's#target triple = "spir"#target triple = "spirv32-unknown-unknown"#' | llc -O0 -global-isel -o - | FileCheck %s --check-prefix=CHECK-SPIRV

// This test checks that the translator is capable to correctly translate
// legacy atomic OpenCL C 1.2 built-in functions [1] into corresponding SPIR-V
// instruction.

__kernel void test_legacy_atomics(__global int *p, int val) {
  atom_add(p, val);     // from cl_khr_global_int32_base_atomics
  atomic_add(p, val);   // from OpenCL C 1.1
}

// CHECK-SPIRV: OpEntryPoint {{.*}} %[[TEST:[0-9]+]] "test_legacy_atomics"
// CHECK-SPIRV-DAG: %[[UINT:[0-9]+]] = OpTypeInt 32 0
// CHECK-SPIRV-DAG: %[[UINT_PTR:[0-9]+]] = OpTypePointer CrossWorkgroup %[[UINT]]
//
// In SPIR-V, atomic_add is represented as OpAtomicIAdd [2], which also includes
// memory scope and memory semantic arguments. The translator applies a default
// memory scope and memory order for it and therefore, constants below include
// a bit more information than original source
//
// 0x2 Workgroup
// CHECK-SPIRV-DAG: %[[WORKGROUP_SCOPE:[0-9]+]] = OpConstant %[[UINT]] 2
//
// 0x0 Relaxed
// CHECK-SPIRV-DAG: %[[RELAXED:[0-9]+]] = OpConstant %[[UINT]] 0
//
// CHECK-SPIRV: %[[TEST]] = OpFunction %{{[0-9]+}}
// CHECK-SPIRV: %[[PTR:[0-9]+]] = OpFunctionParameter %[[UINT_PTR]]
// CHECK-SPIRV: %[[VAL:[0-9]+]] = OpFunctionParameter %[[UINT]]
// CHECK-SPIRV: %{{[0-9]+}} = OpAtomicIAdd %[[UINT]] %[[PTR]] %[[WORKGROUP_SCOPE]] %[[RELAXED]] %[[VAL]]
// CHECK-SPIRV: %{{[0-9]+}} = OpAtomicIAdd %[[UINT]] %[[PTR]] %[[WORKGROUP_SCOPE]] %[[RELAXED]] %[[VAL]]

// References:
// [1]: https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#atomic-legacy
// [2]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#OpAtomicIAdd
