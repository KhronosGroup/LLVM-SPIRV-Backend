// RUN: clang -cc1 -nostdsysteminc -triple spir -cl-std=cl2.0 %s -finclude-default-header -emit-llvm -o - | sed -Ee 's#target triple = "spir"#target triple = "spirv32-unknown-unknown"#' | llc -O0 -global-isel -o - | FileCheck %s --check-prefix=CHECK-SPIRV

int load (volatile atomic_int* obj, memory_order order, memory_scope scope) {
  return atomic_load_explicit(obj, order, scope);
}

// CHECK-SPIRV: OpName %[[LOAD:[0-9]+]] "load"
// CHECK-SPIRV: OpName %[[TRANS_MEM_SCOPE:[0-9]+]] "__translate_ocl_memory_scope"
// CHECK-SPIRV: OpName %[[TRANS_MEM_ORDER:[0-9]+]] "__translate_ocl_memory_order"

// CHECK-SPIRV: %[[int:[0-9]+]] = OpTypeInt 32 0
// CHECK-SPIRV-DAG: %[[ZERO:[0-9]+]] = OpConstant %[[int]] 0
// CHECK-SPIRV-DAG: %[[ONE:[0-9]+]] = OpConstant %[[int]] 1
// CHECK-SPIRV-DAG: %[[TWO:[0-9]+]] = OpConstant %[[int]] 2
// CHECK-SPIRV-DAG: %[[THREE:[0-9]+]] = OpConstant %[[int]] 3
// CHECK-SPIRV-DAG: %[[FOUR:[0-9]+]] = OpConstant %[[int]] 4
// CHECK-SPIRV-DAG: %[[EIGHT:[0-9]+]] = OpConstant %[[int]] 8
// CHECK-SPIRV-DAG: %[[SIXTEEN:[0-9]+]] = OpConstant %[[int]] 16

// CHECK-SPIRV: %[[LOAD]] = OpFunction %{{[0-9]+}}
// CHECK-SPIRV: %[[OBJECT:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
// CHECK-SPIRV: %[[OCL_ORDER:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
// CHECK-SPIRV: %[[OCL_SCOPE:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}

// CHECK-SPIRV: %[[SPIRV_SCOPE:[0-9]+]] = OpFunctionCall %[[int]] %[[TRANS_MEM_SCOPE]] %[[OCL_SCOPE]]
// CHECK-SPIRV: %[[SPIRV_ORDER:[0-9]+]] = OpFunctionCall %[[int]] %[[TRANS_MEM_ORDER]] %[[OCL_ORDER]]
// CHECK-SPIRV: %{{[0-9]+}} = OpAtomicLoad %[[int]] %[[OBJECT]] %[[SPIRV_SCOPE]] %[[SPIRV_ORDER]]

// CHECK-SPIRV: %[[TRANS_MEM_SCOPE]] = OpFunction %[[int]]
// CHECK-SPIRV: %[[KEY:[0-9]+]] = OpFunctionParameter %[[int]]
// CHECK-SPIRV: OpSwitch %[[KEY]] %[[CASE_2:[0-9]+]] 0 %[[CASE_0:[0-9]+]] 1 %[[CASE_1:[0-9]+]] 2 %[[CASE_2]] 3 %[[CASE_3:[0-9]+]] 4 %[[CASE_4:[0-9]+]]
// CHECK-SPIRV: %[[CASE_0]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[FOUR]]
// CHECK-SPIRV: %[[CASE_1]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[TWO]]
// CHECK-SPIRV: %[[CASE_2]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[ONE]]
// CHECK-SPIRV: %[[CASE_3]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[ZERO]]
// CHECK-SPIRV: %[[CASE_4]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[THREE]]
// CHECK-SPIRV: OpFunctionEnd

// CHECK-SPIRV: %[[TRANS_MEM_ORDER]] = OpFunction %[[int]]
// CHECK-SPIRV: %[[KEY:[0-9]+]] = OpFunctionParameter %[[int]]
// CHECK-SPIRV: OpSwitch %[[KEY]] %[[CASE_5:[0-9]+]] 0 %[[CASE_0:[0-9]+]] 2 %[[CASE_2:[0-9]+]] 3 %[[CASE_3:[0-9]+]] 4 %[[CASE_4:[0-9]+]] 5 %[[CASE_5]]
// CHECK-SPIRV: %[[CASE_0]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[ZERO]]
// CHECK-SPIRV: %[[CASE_2]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[TWO]]
// CHECK-SPIRV: %[[CASE_3]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[FOUR]]
// CHECK-SPIRV: %[[CASE_4]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[EIGHT]]
// CHECK-SPIRV: %[[CASE_5]] = OpLabel
// CHECK-SPIRV: OpReturnValue %[[SIXTEEN]]
// CHECK-SPIRV: OpFunctionEnd
