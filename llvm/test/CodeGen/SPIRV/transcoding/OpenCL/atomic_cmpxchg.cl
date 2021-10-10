// RUN: clang -cc1 -nostdsysteminc %s -triple spir -cl-std=CL1.2 -emit-llvm -fdeclare-opencl-builtins -o - | sed -Ee 's#target triple = "spir"#target triple = "spirv32-unknown-unknown"#' | llc -O0 -global-isel -o - | FileCheck %s --check-prefix=CHECK-SPIRV

// This test checks that the translator is capable to correctly translate
// atomic_cmpxchg OpenCL C 1.2 built-in function [1] into corresponding SPIR-V
// instruction.

__kernel void test_atomic_cmpxchg(__global int *p, int cmp, int val) {
  atomic_cmpxchg(p, cmp, val);

  __global unsigned int *up = (__global unsigned int *)p;
  unsigned int ucmp = (unsigned int)cmp;
  unsigned int uval = (unsigned int)val;
  atomic_cmpxchg(up, ucmp, uval);
}

// CHECK-SPIRV: OpEntryPoint Kernel %[[TEST:[0-9]+]] "test_atomic_cmpxchg"
// CHECK-SPIRV-DAG: %[[UINT:[0-9]+]] = OpTypeInt 32 0
// CHECK-SPIRV-DAG: %[[UINT_PTR:[0-9]+]] = OpTypePointer CrossWorkgroup %[[UINT]]
//
// In SPIR-V, atomic_cmpxchg is represented as OpAtomicCompareExchange [2],
// which also includes memory scope and two memory semantic arguments. The
// translator applies some default memory order for it and therefore, constants
// below include a bit more information than original source
//
// 0x2 Workgroup
// CHECK-SPIRV-DAG: %[[SCOPE:[0-9]+]] = OpConstant %[[UINT]] 2
//
// 0x0 Relaxed
// TODO: do we need CrossWorkgroupMemory here as well?
// CHECK-SPIRV-DAG: %[[CST:[0-9]+]] = OpConstant %[[UINT]] 0
//
// CHECK-SPIRV: %[[TEST]] = OpFunction %{{[0-9]+}}
// CHECK-SPIRV: %[[PTR:[0-9]+]] = OpFunctionParameter %[[UINT_PTR]]
// CHECK-SPIRV: %[[CMP:[0-9]+]] = OpFunctionParameter %[[UINT]]
// CHECK-SPIRV: %[[VAL:[0-9]+]] = OpFunctionParameter %[[UINT]]
// CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %[[UINT]] %[[PTR]] %[[SCOPE]] %[[CST]] %[[CST]] %[[VAL]] %[[CMP]]
// CHECK-SPIRV: %{{[0-9]+}} = OpAtomicCompareExchange %[[UINT]] %[[PTR]] %[[SCOPE]] %[[CST]] %[[CST]] %[[VAL]] %[[CMP]]

// References:
// [1]: https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/atomic_cmpxchg.html
// [2]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#OpAtomicCompareExchange
