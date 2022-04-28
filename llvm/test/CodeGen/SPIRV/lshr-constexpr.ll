; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV-DAG: %[[type_int32:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[type_int64:[0-9]+]] = OpTypeInt 64 0
; CHECK-SPIRV: %[[type_vec:[0-9]+]] = OpTypeVector %[[type_int32]] 2
; CHECK-SPIRV: %[[const1:[0-9]+]] = OpConstant %[[type_int32]] 1
; CHECK-SPIRV: %[[vec_const:[0-9]+]] = OpConstantComposite %[[type_vec]] %[[const1]] %[[const1]]
; CHECK-SPIRV: %[[const32:[0-9]+]] = OpConstant %[[type_int64]] 32 0

; CHECK-SPIRV: %[[bitcast_res:[0-9]+]] = OpBitcast %[[type_int64]] %[[vec_const]]
; CHECK-SPIRV: %[[shift_res:[0-9]+]] = OpShiftRightLogical %[[type_int64]] %[[bitcast_res]] %[[const32]]


; Function Attrs: nounwind ssp uwtable
define void @foo(i64* %arg) {
entry:
  %0 = lshr i64 bitcast (<2 x i32> <i32 1, i32 1> to i64), 32
  store i64 %0, i64* %arg
  ret void
}
