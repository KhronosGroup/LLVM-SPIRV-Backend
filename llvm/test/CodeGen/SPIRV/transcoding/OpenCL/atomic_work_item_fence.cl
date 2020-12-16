// RUN: clang -cc1 -nostdsysteminc %s -triple spir -cl-std=CL2.0 -emit-llvm -o - | sed -Ee 's#target triple = "spir"#target triple = "spirv32-unknown-unknown"#' | llc -O0 -global-isel -o - | FileCheck %s --check-prefix=CHECK-SPIRV

// This test checks that the translator is capable to correctly translate
// atomic_work_item_fence OpenCL C 2.0 built-in function [1] into corresponding
// SPIR-V instruction [3].
//
// Forward declarations and defines below are based on the following sources:
// - llvm/llvm-project [1]:
//   - clang/lib/Headers/opencl-c-base.h
//   - clang/lib/Headers/opencl-c.h
// - OpenCL C 2.0 reference pages [2]
// TODO: remove these and switch to using -fdeclare-opencl-builtins once
// atomic_work_item_fence is supported by this flag

typedef unsigned int cl_mem_fence_flags;

#define CLK_LOCAL_MEM_FENCE 0x01
#define CLK_GLOBAL_MEM_FENCE 0x02
#define CLK_IMAGE_MEM_FENCE 0x04

typedef enum memory_scope {
  memory_scope_work_item = __OPENCL_MEMORY_SCOPE_WORK_ITEM,
  memory_scope_work_group = __OPENCL_MEMORY_SCOPE_WORK_GROUP,
  memory_scope_device = __OPENCL_MEMORY_SCOPE_DEVICE,
  memory_scope_all_svm_devices = __OPENCL_MEMORY_SCOPE_ALL_SVM_DEVICES,
  memory_scope_sub_group = __OPENCL_MEMORY_SCOPE_SUB_GROUP
} memory_scope;

typedef enum memory_order
{
  memory_order_relaxed = __ATOMIC_RELAXED,
  memory_order_acquire = __ATOMIC_ACQUIRE,
  memory_order_release = __ATOMIC_RELEASE,
  memory_order_acq_rel = __ATOMIC_ACQ_REL,
  memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

void __attribute__((overloadable)) atomic_work_item_fence(cl_mem_fence_flags, memory_order, memory_scope);

__kernel void test_mem_fence_const_flags() {
  atomic_work_item_fence(CLK_LOCAL_MEM_FENCE, memory_order_relaxed, memory_scope_work_item);
  atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire, memory_scope_work_group);
  atomic_work_item_fence(CLK_IMAGE_MEM_FENCE, memory_order_release, memory_scope_device);
  atomic_work_item_fence(CLK_LOCAL_MEM_FENCE, memory_order_acq_rel, memory_scope_all_svm_devices);
  atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_seq_cst, memory_scope_sub_group);
  atomic_work_item_fence(CLK_IMAGE_MEM_FENCE | CLK_LOCAL_MEM_FENCE, memory_order_acquire, memory_scope_sub_group);
}

__kernel void test_mem_fence_non_const_flags(cl_mem_fence_flags flags, memory_order order, memory_scope scope) {
  // FIXME: OpenCL spec doesn't require flags to be compile-time known
  // atomic_work_item_fence(flags, order, scope);
}

// CHECK-SPIRV: OpEntryPoint Kernel %[[TEST_CONST_FLAGS:[0-9]+]] "test_mem_fence_const_flags"
// CHECK-SPIRV: %[[UINT:[0-9]+]] = OpTypeInt 32 0
//
// 0x0 Relaxed + 0x100 WorkgroupMemory
// CHECK-SPIRV-DAG: %[[LOCAL_RELAXED:[0-9]+]] = OpConstant %[[UINT]] 256
// 0x2 Acquire + 0x200 CrossWorkgroupMemory
// CHECK-SPIRV-DAG: %[[GLOBAL_ACQUIRE:[0-9]+]] = OpConstant %[[UINT]] 514
// 0x4 Release + 0x800 ImageMemory
// CHECK-SPIRV-DAG: %[[IMAGE_RELEASE:[0-9]+]] = OpConstant %[[UINT]] 2052
// 0x8 AcquireRelease + 0x100 WorkgroupMemory
// CHECK-SPIRV-DAG: %[[LOCAL_ACQ_REL:[0-9]+]] = OpConstant %[[UINT]] 264
// 0x10 SequentiallyConsistent + 0x200 CrossWorkgroupMemory
// CHECK-SPIRV-DAG: %[[GLOBAL_SEQ_CST:[0-9]+]] = OpConstant %[[UINT]] 528
// 0x2 Acquire + 0x100 WorkgroupMemory + 0x800 ImageMemory
// CHECK-SPIRV-DAG: %[[LOCAL_IMAGE_ACQUIRE:[0-9]+]] = OpConstant %[[UINT]] 2306
//
// Scopes [4]:
// 4 Invocation
// CHECK-SPIRV-DAG: %[[SCOPE_INVOCATION:[0-9]+]] = OpConstant %[[UINT]] 4
// 2 Workgroup
// CHECK-SPIRV-DAG: %[[SCOPE_WORK_GROUP:[0-9]+]] = OpConstant %[[UINT]] 2
// 1 Device
// CHECK-SPIRV-DAG: %[[SCOPE_DEVICE:[0-9]+]] = OpConstant %[[UINT]] 1
// 0 CrossDevice
// CHECK-SPIRV-DAG: %[[SCOPE_CROSS_DEVICE:[0-9]+]] = OpConstant %[[UINT]] 0
// 3 Subgroup
// CHECK-SPIRV-DAG: %[[SCOPE_SUBGROUP:[0-9]+]] = OpConstant %[[UINT]] 3
//
// CHECK-SPIRV: %[[TEST_CONST_FLAGS]] = OpFunction %{{[0-9]+}}
// CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_INVOCATION]] %[[LOCAL_RELAXED]]
// CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_WORK_GROUP]] %[[GLOBAL_ACQUIRE]]
// CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_DEVICE]] %[[IMAGE_RELEASE]]
// CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_CROSS_DEVICE]] %[[LOCAL_ACQ_REL]]
// CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_SUBGROUP]] %[[GLOBAL_SEQ_CST]]
// CHECK-SPIRV: OpMemoryBarrier %[[SCOPE_SUBGROUP]] %[[LOCAL_IMAGE_ACQUIRE]]

// References:
// [1]: https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/atomic_work_item_fence.html
// [2]: https://github.com/llvm/llvm-project
// [3]: https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#OpMemoryBarrier
