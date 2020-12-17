; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpName %[[v:[0-9]+]] "v"
; CHECK-SPIRV: OpName %[[index:[0-9]+]] "index"
; CHECK-SPIRV: OpName %[[res:[0-9]+]] "res"

; CHECK-SPIRV-DAG: %[[int16:[0-9]+]] = OpTypeInt 16
; CHECK-SPIRV-DAG: %[[int32:[0-9]+]] = OpTypeInt 32
; CHECK-SPIRV-DAG: %[[int16_2:[0-9]+]] = OpTypeVector %[[int16]] 2

; CHECK-SPIRV: %[[undef:[0-9]+]] = OpUndef %[[int16_2]]

; CHECK-SPIRV-DAG: %[[const1:[0-9]+]] = OpConstant %[[int16]] 4
; CHECK-SPIRV-DAG: %[[idx1:[0-9]+]] = OpConstant %[[int32]] 0
; CHECK-SPIRV-DAG: %[[const2:[0-9]+]] = OpConstant %[[int16]] 8
; CHECK-SPIRV-DAG: %[[idx2:[0-9]+]] = OpConstant %[[int32]] 1

; CHECK-SPIRV: %[[vec1:[0-9]+]] = OpVectorInsertDynamic %[[int16_2]] %[[undef]] %[[const1]] %[[idx1]]
; CHECK-SPIRV: %[[vec2:[0-9]+]] = OpVectorInsertDynamic %[[int16_2]] %[[vec1]] %[[const2]] %[[idx2]]
; CHECK-SPIRV: %[[res]] = OpVectorInsertDynamic %[[int16_2]] %[[vec2]] %[[v]] %[[index]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @test(<2 x i16>* nocapture %out, i16 %v, i32 %index) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %vec1 = insertelement <2 x i16> undef, i16 4, i32 0
  %vec2 = insertelement <2 x i16> %vec1, i16 8, i32 1
  %res = insertelement <2 x i16> %vec2, i16 %v, i32 %index
  store <2 x i16> %res, <2 x i16>* %out, align 4
  ret void
}

attributes #0 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!8}

!1 = !{i32 1, i32 0}
!2 = !{!"none", !"none", !"none"}
!3 = !{!"float*", !"float2", !"int"}
!4 = !{!"float*", !"float2", !"int"}
!5 = !{!"", !"", !""}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 0}
!8 = !{}
