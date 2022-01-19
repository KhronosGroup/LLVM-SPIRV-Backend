; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: %[[Int:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[MemScope_Device:[0-9]+]] = OpConstant %[[Int]] 1
; CHECK-SPIRV-DAG: %[[MemSemEqual_SeqCst:[0-9]+]] = OpConstant %[[Int]] 16
; CHECK-SPIRV-DAG: %[[MemSemUnequal_Acquire:[0-9]+]] = OpConstant %[[Int]] 2
; CHECK-SPIRV-DAG: %[[Constant_456:[0-9]+]] = OpConstant %[[Int]] 456
; CHECK-SPIRV-DAG: %[[Constant_128:[0-9]+]] = OpConstant %[[Int]] 128
; CHECK-SPIRV-DAG: %[[Bool:[0-9]+]] = OpTypeBool
; CHECK-SPIRV-DAG: %[[Struct:[0-9]+]] = OpTypeStruct %[[Int]] %[[Bool]]
; CHECK-SPIRV-DAG: %[[UndefStruct:[0-9]+]] = OpUndef %[[Struct]]

; CHECK-SPIRV: %[[Pointer:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %[[Value_ptr:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %[[Comparator:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}

; CHECK-SPIRV: %[[Value:[0-9]+]] = OpLoad %[[Int]] %[[Value_ptr]]
; CHECK-SPIRV: %[[Res:[0-9]+]] = OpAtomicCompareExchange %[[Int]] %[[Pointer]] %[[MemScope_Device]]
; CHECK-SPIRV-SAME:                  %[[MemSemEqual_SeqCst]] %[[MemSemUnequal_Acquire]] %[[Value]] %[[Comparator]]
; CHECK-SPIRV: %[[Success:[0-9]+]] = OpIEqual %{{[0-9]+}} %[[Res]] %[[Comparator]]
; CHECK-SPIRV: %[[Composite_0:[0-9]+]] = OpCompositeInsert %[[Struct]] %[[Res]] %[[UndefStruct]] 0
; CHECK-SPIRV: %[[Composite_1:[0-9]+]] = OpCompositeInsert %[[Struct]] %[[Success]] %[[Composite_0]] 1
; CHECK-SPIRV: %{{[0-9]+}} = OpCompositeExtract %[[Bool]] %[[Composite_1]] 1

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: nounwind
define dso_local spir_func void @test(i32* %ptr, i32* %value_ptr, i32 %comparator) local_unnamed_addr #0 {
entry:
  %0 = load i32, i32* %value_ptr, align 4
  %1 = cmpxchg i32* %ptr, i32 %comparator, i32 %0 seq_cst acquire
  %2 = extractvalue { i32, i1 } %1, 1
  br i1 %2, label %cmpxchg.continue, label %cmpxchg.store_expected

cmpxchg.store_expected:                           ; preds = %entry
  %3 = extractvalue { i32, i1 } %1, 0
  store i32 %3, i32* %value_ptr, align 4
  br label %cmpxchg.continue

cmpxchg.continue:                                 ; preds = %cmpxchg.store_expected, %entry
  ret void
}

; CHECK-SPIRV: %[[Ptr:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV: %[[Store_ptr:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}

; CHECK-SPIRV: %[[Res_1:[0-9]+]] = OpAtomicCompareExchange %[[Int]] %[[Ptr]] %[[MemScope_Device]]
; CHECK-SPIRV-SAME:                  %[[MemSemEqual_SeqCst]] %[[MemSemUnequal_Acquire]] %[[Constant_456]] %[[Constant_128]]
; CHECK-SPIRV: %[[Success_1:[0-9]+]] = OpIEqual %{{[0-9]+}} %[[Res_1]] %[[Constant_128]]
; CHECK-SPIRV: %[[Composite:[0-9]+]] = OpCompositeInsert %[[Struct]] %[[Res_1]] %[[UndefStruct]] 0
; CHECK-SPIRV: %[[Composite_1:[0-9]+]] = OpCompositeInsert %[[Struct]] %[[Success_1]] %[[Composite]] 1
; CHECK-SPIRV: OpStore %[[Store_ptr]] %[[Composite_1]]

; Function Attrs: nounwind
define dso_local spir_func void @test2(i32* %ptr, {i32, i1}* %store_ptr) local_unnamed_addr #0 {
entry:
  %0 = cmpxchg i32* %ptr, i32 128, i32 456 seq_cst acquire
  store { i32, i1 } %0, { i32, i1 }* %store_ptr, align 4
  ret void
}

attributes #0 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 11.0.0 (https://github.com/llvm/llvm-project.git cfebd7774229885e7ec88ae9ef1f4ae819cce1d2)"}
