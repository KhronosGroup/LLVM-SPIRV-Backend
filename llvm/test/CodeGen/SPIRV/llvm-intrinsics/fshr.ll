; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV: OpName %[[NAME_FSHR_FUNC_32:[0-9]+]] "spirv.llvm_fshr_i32"
; CHECK-SPIRV: OpName %[[NAME_FSHR_FUNC_16:[0-9]+]] "spirv.llvm_fshr_i16"
; CHECK-SPIRV: OpName %[[NAME_FSHR_FUNC_VEC_INT_16:[0-9]+]] "spirv.llvm_fshr_v2i16"
; CHECK-SPIRV: %[[TYPE_INT_32:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV: %[[TYPE_INT_16:[0-9]+]] = OpTypeInt 16 0
; CHECK-SPIRV-DAG: %[[CONST_ROTATE_32:[0-9]+]] = OpConstant %[[TYPE_INT_32]] 8
; CHECK-SPIRV-DAG: %[[CONST_ROTATE_16:[0-9]+]] = OpConstant %[[TYPE_INT_16]] 8
; CHECK-SPIRV-DAG: %[[CONST_TYPE_SIZE_32:[0-9]+]] = OpConstant %[[TYPE_INT_32]] 32
; CHECK-SPIRV: %[[TYPE_ORIG_FUNC_32:[0-9]+]] = OpTypeFunction %[[TYPE_INT_32]] %[[TYPE_INT_32]] %[[TYPE_INT_32]]
; CHECK-SPIRV: %[[TYPE_FSHR_FUNC_32:[0-9]+]] = OpTypeFunction %[[TYPE_INT_32]] %[[TYPE_INT_32]] %[[TYPE_INT_32]] %[[TYPE_INT_32]]
; CHECK-SPIRV: %[[TYPE_ORIG_FUNC_16:[0-9]+]] = OpTypeFunction %[[TYPE_INT_16]] %[[TYPE_INT_16]] %[[TYPE_INT_16]]
; CHECK-SPIRV: %[[TYPE_FSHR_FUNC_16:[0-9]+]] = OpTypeFunction %[[TYPE_INT_16]] %[[TYPE_INT_16]] %[[TYPE_INT_16]] %[[TYPE_INT_16]]
; CHECK-SPIRV: %[[TYPE_VEC_INT_16:[0-9]+]] = OpTypeVector %[[TYPE_INT_16]] 2
; CHECK-SPIRV: %[[TYPE_ORIG_FUNC_VEC_INT_16:[0-9]+]] = OpTypeFunction %[[TYPE_VEC_INT_16]] %[[TYPE_VEC_INT_16]] %[[TYPE_VEC_INT_16]]
; CHECK-SPIRV: %[[TYPE_FSHR_FUNC_VEC_INT_16:[0-9]+]] = OpTypeFunction %[[TYPE_VEC_INT_16]] %[[TYPE_VEC_INT_16]] %[[TYPE_VEC_INT_16]] %[[TYPE_VEC_INT_16]]
; CHECK-SPIRV: %[[CONST_ROTATE_VEC_INT_16:[0-9]+]] = OpConstantComposite %[[TYPE_VEC_INT_16]] %[[CONST_ROTATE_16]] %[[CONST_ROTATE_16]]

; Function Attrs: nounwind readnone
; CHECK-SPIRV: %{{[0-9]+}} = OpFunction %[[TYPE_INT_32]] {{.*}} %[[TYPE_ORIG_FUNC_32]]
; CHECK-SPIRV: %[[X:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_32]]
; CHECK-SPIRV: %[[Y:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_32]]
define spir_func i32 @Test_i32(i32 %x, i32 %y) local_unnamed_addr #0 {
entry:
  ; CHECK-SPIRV: %[[CALL_32_X_Y:[0-9]+]] = OpFunctionCall %[[TYPE_INT_32]] %[[NAME_FSHR_FUNC_32]] %[[X]] %[[Y]] %[[CONST_ROTATE_32]]
  %0 = call i32 @llvm.fshr.i32(i32 %x, i32 %y, i32 8)
  ; CHECK-SPIRV: %[[CALL_32_Y_X:[0-9]+]] = OpFunctionCall %[[TYPE_INT_32]] %[[NAME_FSHR_FUNC_32]] %[[Y]] %[[X]] %[[CONST_ROTATE_32]]
  %1 = call i32 @llvm.fshr.i32(i32 %y, i32 %x, i32 8)
  ; CHECK-SPIRV: %[[ADD_32:[0-9]+]] = OpIAdd %[[TYPE_INT_32]] %[[CALL_32_X_Y]] %[[CALL_32_Y_X]]
  %sum = add i32 %0, %1
  ; CHECK-SPIRV: OpReturnValue %[[ADD_32]]
  ret i32 %sum
}

; CHECK-SPIRV: %[[NAME_FSHR_FUNC_32]] = OpFunction %[[TYPE_INT_32]] {{.*}} %[[TYPE_FSHR_FUNC_32]]
; CHECK-SPIRV: %[[X_ARG:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_32]]
; CHECK-SPIRV: %[[Y_ARG:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_32]]
; CHECK-SPIRV: %[[ROT:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_32]]

; CHECK-SPIRV: %[[ROTATE_MOD_SIZE:[0-9]+]] = OpUMod %[[TYPE_INT_32]] %[[ROT]] %[[CONST_TYPE_SIZE_32]]
; CHECK-SPIRV: %[[Y_SHIFT_RIGHT:[0-9]+]] = OpShiftRightLogical %[[TYPE_INT_32]] %[[Y_ARG]] %[[ROTATE_MOD_SIZE]]
; CHECK-SPIRV: %[[NEG_ROTATE:[0-9]+]] = OpISub %[[TYPE_INT_32]] %[[CONST_TYPE_SIZE_32]] %[[ROTATE_MOD_SIZE]]
; CHECK-SPIRV: %[[X_SHIFT_LEFT:[0-9]+]] = OpShiftLeftLogical %[[TYPE_INT_32]] %[[X_ARG]] %[[NEG_ROTATE]]
; CHECK-SPIRV: %[[FSHR_RESULT:[0-9]+]] = OpBitwiseOr %[[TYPE_INT_32]] %[[Y_SHIFT_RIGHT]] %[[X_SHIFT_LEFT]]
; CHECK-SPIRV: OpReturnValue %[[FSHR_RESULT]]

; Function Attrs: nounwind readnone
; CHECK-SPIRV: %{{[0-9]+}} = OpFunction %[[TYPE_INT_16]] {{.*}} %[[TYPE_ORIG_FUNC_16]]
; CHECK-SPIRV: %[[X:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_16]]
; CHECK-SPIRV: %[[Y:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_16]]
define spir_func i16 @Test_i16(i16 %x, i16 %y) local_unnamed_addr #0 {
entry:
  ; CHECK-SPIRV: %[[CALL_16:[0-9]+]] = OpFunctionCall %[[TYPE_INT_16]] %[[NAME_FSHR_FUNC_16]] %[[X]] %[[Y]] %[[CONST_ROTATE_16]]
  %0 = call i16 @llvm.fshr.i16(i16 %x, i16 %y, i16 8)
  ; CHECK-SPIRV: OpReturnValue %[[CALL_16]]
  ret i16 %0
}

; Just check that the function for i16 was generated as such - we've checked the logic for another type.
; CHECK-SPIRV: %[[NAME_FSHR_FUNC_16]] = OpFunction %[[TYPE_INT_16]] {{.*}} %[[TYPE_FSHR_FUNC_16]]
; CHECK-SPIRV: %[[X_ARG:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_16]]
; CHECK-SPIRV: %[[Y_ARG:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_16]]
; CHECK-SPIRV: %[[ROT:[0-9]+]] = OpFunctionParameter %[[TYPE_INT_16]]

; CHECK-SPIRV: %{{[0-9]+}} = OpFunction %[[TYPE_VEC_INT_16]] {{.*}} %[[TYPE_ORIG_FUNC_VEC_INT_16]]
; CHECK-SPIRV: %[[X:[0-9]+]] = OpFunctionParameter %[[TYPE_VEC_INT_16]]
; CHECK-SPIRV: %[[Y:[0-9]+]] = OpFunctionParameter %[[TYPE_VEC_INT_16]]
define spir_func <2 x i16> @Test_v2i16(<2 x i16> %x, <2 x i16> %y) local_unnamed_addr #0 {
entry:
  ; CHECK-SPIRV: %[[CALL_VEC_INT_16:[0-9]+]] = OpFunctionCall %[[TYPE_VEC_INT_16]] %[[NAME_FSHR_FUNC_VEC_INT_16]] %[[X]] %[[Y]] %[[CONST_ROTATE_VEC_INT_16]]
  %0 = call <2 x i16> @llvm.fshr.v2i16(<2 x i16> %x, <2 x i16> %y, <2 x i16> <i16 8, i16 8>)
  ; CHECK-SPIRV: OpReturnValue %[[CALL_VEC_INT_16]]
  ret <2 x i16> %0
}

; Just check that the function for v2i16 was generated as such - we've checked the logic for another type.
; CHECK-SPIRV: %[[NAME_FSHR_FUNC_VEC_INT_16]] = OpFunction %[[TYPE_VEC_INT_16]] {{.*}} %[[TYPE_FSHR_FUNC_VEC_INT_16]]
; CHECK-SPIRV: %[[X_ARG:[0-9]+]] = OpFunctionParameter %[[TYPE_VEC_INT_16]]
; CHECK-SPIRV: %[[Y_ARG:[0-9]+]] = OpFunctionParameter %[[TYPE_VEC_INT_16]]
; CHECK-SPIRV: %[[ROT:[0-9]+]] = OpFunctionParameter %[[TYPE_VEC_INT_16]]

; Function Attrs: nounwind readnone speculatable willreturn
declare i32 @llvm.fshr.i32(i32, i32, i32) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare i16 @llvm.fshr.i16(i16, i16, i16) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare <2 x i16> @llvm.fshr.v2i16(<2 x i16>, <2 x i16>, <2 x i16>) #1

attributes #0 = { nounwind readnone "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
