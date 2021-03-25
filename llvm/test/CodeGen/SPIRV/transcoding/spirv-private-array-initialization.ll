; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV
;
; CHECK-SPIRV-DAG: %[[i32:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[i8:[0-9]+]] = OpTypeInt 8 0
; CHECK-SPIRV-DAG: %[[one:[0-9]+]] = OpConstant %[[i32]] 1
; CHECK-SPIRV-DAG: %[[two:[0-9]+]] = OpConstant %[[i32]] 2
; CHECK-SPIRV-DAG: %[[three:[0-9]+]] = OpConstant %[[i32]] 3
; CHECK-SPIRV-DAG: %[[twelve:[0-9]+]] = OpConstant %[[i32]] 12
; CHECK-SPIRV-DAG: %[[i32x3:[0-9]+]] = OpTypeArray %[[i32]] %[[three]]
; CHECK-SPIRV-DAG: %[[i32x3_ptr:[0-9]+]] = OpTypePointer Function %[[i32x3]]
; CHECK-SPIRV-DAG: %[[const_i32x3_ptr:[0-9]+]] = OpTypePointer UniformConstant %[[i32x3]]
; CHECK-SPIRV-DAG: %[[i8_ptr:[0-9]+]] = OpTypePointer Function %[[i8]]
; CHECK-SPIRV-DAG: %[[const_i8_ptr:[0-9]+]] = OpTypePointer UniformConstant %[[i8]]
; CHECK-SPIRV: %[[test_arr_init:[0-9]+]] = OpConstantComposite %[[i32x3]] %[[one]] %[[two]] %[[three]]
; CHECK-SPIRV: %[[test_arr:[0-9]+]] = OpVariable %[[const_i32x3_ptr]] UniformConstant %[[test_arr_init]]
; CHECK-SPIRV: %[[test_arr2:[0-9]+]] = OpVariable %[[const_i32x3_ptr]] UniformConstant %[[test_arr_init]]
;
; CHECK-SPIRV: %[[arr:[0-9]+]] = OpVariable %[[i32x3_ptr]] Function
; CHECK-SPIRV: %[[arr2:[0-9]+]] = OpVariable %[[i32x3_ptr]] Function
;
; CHECK-SPIRV: %[[arr_i8_ptr:[0-9]+]] = OpBitcast %[[i8_ptr]] %[[arr]]
; CHECK-SPIRV: %[[test_arr_const_i8_ptr:[0-9]+]] = OpBitcast %[[const_i8_ptr]] %[[test_arr]]
; FIXME: check syntax of OpCopyMemorySized
; CHECK-SPIRV: OpCopyMemorySized %[[arr_i8_ptr]] %[[test_arr_const_i8_ptr]] %[[twelve]] Aligned Nontemporal
;
; CHECK-SPIRV: %[[arr2_i8_ptr:[0-9]+]] = OpBitcast %[[i8_ptr]] %[[arr2]]
; CHECK-SPIRV: %[[test_arr2_const_i8_ptr:[0-9]+]] = OpBitcast %[[const_i8_ptr]] %[[test_arr2]]
; FIXME: check syntax of OpCopyMemorySized
; CHECK-SPIRV: OpCopyMemorySized %[[arr2_i8_ptr]] %[[test_arr2_const_i8_ptr]] %[[twelve]] Aligned Nontemporal

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

@__const.test.arr = private unnamed_addr addrspace(2) constant [3 x i32] [i32 1, i32 2, i32 3], align 4
@__const.test.arr2 = private unnamed_addr addrspace(2) constant [3 x i32] [i32 1, i32 2, i32 3], align 4

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @test() #0 {
entry:
  %arr = alloca [3 x i32], align 4
  %arr2 = alloca [3 x i32], align 4
  %0 = bitcast [3 x i32]* %arr to i8*
  call void @llvm.memcpy.p0i8.p2i8.i32(i8* align 4 %0, i8 addrspace(2)* align 4 bitcast ([3 x i32] addrspace(2)* @__const.test.arr to i8 addrspace(2)*), i32 12, i1 false)
  %1 = bitcast [3 x i32]* %arr2 to i8*
  call void @llvm.memcpy.p0i8.p2i8.i32(i8* align 4 %1, i8 addrspace(2)* align 4 bitcast ([3 x i32] addrspace(2)* @__const.test.arr2 to i8 addrspace(2)*), i32 12, i1 false)
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p2i8.i32(i8* nocapture writeonly, i8 addrspace(2)* nocapture readonly, i32, i1) #1

attributes #0 = { convergent noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
