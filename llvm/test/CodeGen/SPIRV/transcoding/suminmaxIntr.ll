; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

; CHECK: OpName %[[l:[0-9]+]] "l"
; CHECK: OpName %[[g:[0-9]+]] "g"
; CHECK: OpName %[[lv:[0-9]+]] "lv"
; CHECK: OpName %[[gv:[0-9]+]] "gv"
; CHECK: OpName %[[smax:[0-9]+]] "smax"
; CHECK: OpName %[[smaxv:[0-9]+]] "smaxv"
; CHECK: OpName %[[smin:[0-9]+]] "smin"
; CHECK: OpName %[[sminv:[0-9]+]] "sminv"
; CHECK: OpName %[[umax:[0-9]+]] "umax"
; CHECK: OpName %[[umaxv:[0-9]+]] "umaxv"
; CHECK: OpName %[[umin:[0-9]+]] "umin"
; CHECK: OpName %[[uminv:[0-9]+]] "uminv"

; CHECK: %[[resg:[0-9]+]] = OpSGreaterThan %{{[0-9]+}} %[[l]] %[[g]]
; CHECK: %[[smax]] = OpSelect %{{[0-9]+}} %[[resg]] %[[l]] %[[g]]
; CHECK: %[[resgv:[0-9]+]] = OpSGreaterThan %{{[0-9]+}} %[[lv]] %[[gv]]
; CHECK: %[[smaxv]] = OpSelect %{{[0-9]+}} %[[resgv]] %[[lv]] %[[gv]]
; CHECK: %[[resl:[0-9]+]] = OpSLessThan %{{[0-9]+}} %[[l]] %[[g]]
; CHECK: %[[smin]] = OpSelect %{{[0-9]+}} %[[resl]] %[[l]] %[[g]]
; CHECK: %[[reslv:[0-9]+]] = OpSLessThan %{{[0-9]+}} %[[lv]] %[[gv]]
; CHECK: %[[sminv]] = OpSelect %{{[0-9]+}} %[[reslv]] %[[lv]] %[[gv]]
; CHECK: %[[reug:[0-9]+]] = OpUGreaterThan %{{[0-9]+}} %[[l]] %[[g]]
; CHECK: %[[umax]] = OpSelect %{{[0-9]+}} %[[reug]] %[[l]] %[[g]]
; CHECK: %[[reugv:[0-9]+]] = OpUGreaterThan %{{[0-9]+}} %[[lv]] %[[gv]]
; CHECK: %[[umaxv]] = OpSelect %{{[0-9]+}} %[[reugv]] %[[lv]] %[[gv]]
; CHECK: %[[reul:[0-9]+]] = OpULessThan %{{[0-9]+}} %[[l]] %[[g]]
; CHECK: %[[umin]] = OpSelect %{{[0-9]+}} %[[reul]] %[[l]] %[[g]]
; CHECK: %[[reusv:[0-9]+]] = OpULessThan %{{[0-9]+}} %[[lv]] %[[gv]]
; CHECK: %[[uminv]] = OpSelect %{{[0-9]+}} %[[reusv]] %[[lv]] %[[gv]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @test(i32 %l, i32 %g, <4 x i32> %lv, <4 x i32> %gv) #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
entry:
  %smax = call i32 @llvm.smax.i32(i32 %l, i32 %g) #1
  %smaxv = call <4 x i32> @llvm.smax.v4i32(<4 x i32> %lv, <4 x i32> %gv) #1
  %smin = call i32 @llvm.smin.i32(i32 %l,i32 %g)  #1
  %sminv = call <4 x i32> @llvm.smin.v4i32(<4 x i32> %lv, <4 x i32> %gv) #1
  %umax = call i32 @llvm.umax.i32(i32 %l, i32 %g) #1
  %umaxv = call <4 x i32> @llvm.umax.v4i32(<4 x i32> %lv, <4 x i32> %gv) #1
  %umin = call i32 @llvm.umin.i32(i32 %l, i32 %g) #1
  %uminv = call <4 x i32> @llvm.umin.v4i32(<4 x i32> %lv, <4 x i32> %gv) #1
  ret void
}

declare i32 @llvm.smax.i32(i32 %a, i32 %b)
declare <4 x i32> @llvm.smax.v4i32(<4 x i32> %a, <4 x i32> %b)

declare i32 @llvm.smin.i32(i32 %a, i32 %b)
declare <4 x i32> @llvm.smin.v4i32(<4 x i32> %a, <4 x i32> %b)

declare i32 @llvm.umax.i32(i32 %a, i32 %b)
declare <4 x i32> @llvm.umax.v4i32(<4 x i32> %a, <4 x i32> %b)

declare i32 @llvm.umin.i32(i32 %a, i32 %b)
declare <4 x i32> @llvm.umin.v4i32(<4 x i32> %a, <4 x i32> %b)

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!opencl.compiler.options = !{!0}

!0 = !{}
!1 = !{i32 1, i32 2}
!2 = !{i32 2, i32 0}
