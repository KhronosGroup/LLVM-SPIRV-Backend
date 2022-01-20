; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; int load (volatile atomic_int* obj, memory_order order, memory_scope scope) {
;   return atomic_load_explicit(obj, order, scope);
; }

; ModuleID = 'atomic_explicit_arguments.cl'
source_filename = "atomic_explicit_arguments.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpName %[[LOAD:[0-9]+]] "load"
; CHECK-SPIRV: OpName %[[TRANS_MEM_SCOPE:[0-9]+]] "__translate_ocl_memory_scope"
; CHECK-SPIRV: OpName %[[TRANS_MEM_ORDER:[0-9]+]] "__translate_ocl_memory_order"

; CHECK-SPIRV: %[[int:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[ZERO:[0-9]+]] = OpConstant %[[int]] 0
; CHECK-SPIRV-DAG: %[[ONE:[0-9]+]] = OpConstant %[[int]] 1
; CHECK-SPIRV-DAG: %[[TWO:[0-9]+]] = OpConstant %[[int]] 2
; CHECK-SPIRV-DAG: %[[THREE:[0-9]+]] = OpConstant %[[int]] 3
; CHECK-SPIRV-DAG: %[[FOUR:[0-9]+]] = OpConstant %[[int]] 4
; CHECK-SPIRV-DAG: %[[EIGHT:[0-9]+]] = OpConstant %[[int]] 8
; CHECK-SPIRV-DAG: %[[SIXTEEN:[0-9]+]] = OpConstant %[[int]] 16

; CHECK-SPIRV: %[[LOAD]] = OpFunction %{{[0-9]+}}
; CHECK-SPIRV: %[[OBJECT:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %[[OCL_ORDER:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %[[OCL_SCOPE:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}

; CHECK-SPIRV: %[[SPIRV_SCOPE:[0-9]+]] = OpFunctionCall %[[int]] %[[TRANS_MEM_SCOPE]] %[[OCL_SCOPE]]
; CHECK-SPIRV: %[[SPIRV_ORDER:[0-9]+]] = OpFunctionCall %[[int]] %[[TRANS_MEM_ORDER]] %[[OCL_ORDER]]
; CHECK-SPIRV: %{{[0-9]+}} = OpAtomicLoad %[[int]] %[[OBJECT]] %[[SPIRV_SCOPE]] %[[SPIRV_ORDER]]

; CHECK-SPIRV: %[[TRANS_MEM_SCOPE]] = OpFunction %[[int]]
; CHECK-SPIRV: %[[KEY:[0-9]+]] = OpFunctionParameter %[[int]]
; CHECK-SPIRV: OpSwitch %[[KEY]] %[[CASE_2:[0-9]+]] 0 %[[CASE_0:[0-9]+]] 1 %[[CASE_1:[0-9]+]] 2 %[[CASE_2]] 3 %[[CASE_3:[0-9]+]] 4 %[[CASE_4:[0-9]+]]
; CHECK-SPIRV: %[[CASE_0]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[FOUR]]
; CHECK-SPIRV: %[[CASE_1]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[TWO]]
; CHECK-SPIRV: %[[CASE_2]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[ONE]]
; CHECK-SPIRV: %[[CASE_3]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[ZERO]]
; CHECK-SPIRV: %[[CASE_4]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[THREE]]
; CHECK-SPIRV: OpFunctionEnd

; CHECK-SPIRV: %[[TRANS_MEM_ORDER]] = OpFunction %[[int]]
; CHECK-SPIRV: %[[KEY:[0-9]+]] = OpFunctionParameter %[[int]]
; CHECK-SPIRV: OpSwitch %[[KEY]] %[[CASE_5:[0-9]+]] 0 %[[CASE_0:[0-9]+]] 2 %[[CASE_2:[0-9]+]] 3 %[[CASE_3:[0-9]+]] 4 %[[CASE_4:[0-9]+]] 5 %[[CASE_5]]
; CHECK-SPIRV: %[[CASE_0]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[ZERO]]
; CHECK-SPIRV: %[[CASE_2]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[TWO]]
; CHECK-SPIRV: %[[CASE_3]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[FOUR]]
; CHECK-SPIRV: %[[CASE_4]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[EIGHT]]
; CHECK-SPIRV: %[[CASE_5]] = OpLabel
; CHECK-SPIRV: OpReturnValue %[[SIXTEEN]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent norecurse nounwind
define dso_local spir_func i32 @load(i32 addrspace(4)* noundef %obj, i32 noundef %order, i32 noundef %scope) local_unnamed_addr #0 {
entry:
  %call = tail call spir_func i32 @_Z20atomic_load_explicitPU3AS4VU7_Atomici12memory_order12memory_scope(i32 addrspace(4)* noundef %obj, i32 noundef %order, i32 noundef %scope) #2
  ret i32 %call
}

; Function Attrs: convergent
declare spir_func i32 @_Z20atomic_load_explicitPU3AS4VU7_Atomici12memory_order12memory_scope(i32 addrspace(4)* noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
