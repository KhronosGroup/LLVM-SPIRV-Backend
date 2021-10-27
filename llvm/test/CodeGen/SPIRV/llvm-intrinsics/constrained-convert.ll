; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

; CHECK: OpName %[[sf:[0-9]+]] "conv"
; CHECK: OpName %[[uf:[0-9]+]] "conv1"
; CHECK: OpName %[[fs:[0-9]+]] "conv2"
; CHECK: OpName %[[fu:[0-9]+]] "conv3"
; CHECK: OpName %[[fe:[0-9]+]] "conv4"
; CHECK: OpName %[[ft:[0-9]+]] "conv5"

; CHECK-DAG: OpDecorate %[[sf]] FPRoundingMode RTE
; CHECK-DAG: OpDecorate %[[uf]] FPRoundingMode RTZ
; CHECK-DAG: OpDecorate %[[ft]] FPRoundingMode RTP

; CHECK-NOT: OpDecorate %[[fs]] FPRoundingMode
; CHECK-NOT: OpDecorate %[[fu]] FPRoundingMode
; CHECK-NOT: OpDecorate %[[fe]] FPRoundingMode


;CHECK: %[[sf]] = OpConvertSToF %{{[0-9]+}}
;CHECK: %[[uf]] = OpConvertUToF %{{[0-9]+}}
;CHECK: %[[fs]] = OpConvertFToS %{{[0-9]+}}
;CHECK: %[[fu]] = OpConvertFToU %{{[0-9]+}}
;CHECK: %[[fe]] = OpFConvert %{{[0-9]+}}
;CHECK: %[[ft]] = OpFConvert %{{[0-9]+}}

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-linux"

; Function Attrs: norecurse nounwind strictfp
define dso_local spir_kernel void @test(float %a, i32 %in, i32 %ui) local_unnamed_addr #0 !kernel_arg_addr_space !5 !kernel_arg_access_qual !6 !kernel_arg_type !7 !kernel_arg_base_type !7 !kernel_arg_type_qual !8 !kernel_arg_buffer_location !9 {
entry:
  %conv = tail call float @llvm.experimental.constrained.sitofp.f32.i32(i32 %in, metadata !"round.tonearest", metadata !"fpexcept.ignore") #2
  %conv1 = tail call float @llvm.experimental.constrained.uitofp.f32.i32(i32 %ui, metadata !"round.towardzero", metadata !"fpexcept.ignore") #2
  %conv2 = tail call i32 @llvm.experimental.constrained.fptosi.i32.f32(float %conv1, metadata !"fpexcept.ignore") #2
  %conv3 = tail call i32 @llvm.experimental.constrained.fptoui.i32.f32(float %conv1, metadata !"fpexcept.ignore") #2
  %conv4 = tail call double @llvm.experimental.constrained.fpext.f64.f32(float %conv1, metadata !"fpexcept.ignore") #2
  %conv5 = tail call float @llvm.experimental.constrained.fptrunc.f32.f64(double %conv4, metadata !"round.upward", metadata !"fpexcept.ignore") #2
  ret void
}

; Function Attrs: inaccessiblememonly nounwind willreturn
declare float @llvm.experimental.constrained.sitofp.f32.i32(i32, metadata, metadata) #1

; Function Attrs: inaccessiblememonly nounwind willreturn
declare float @llvm.experimental.constrained.uitofp.f32.i32(i32, metadata, metadata) #1

; Function Attrs: inaccessiblememonly nounwind willreturn
declare i32 @llvm.experimental.constrained.fptosi.i32.f32(float, metadata) #1

; Function Attrs: inaccessiblememonly nounwind willreturn
declare i32 @llvm.experimental.constrained.fptoui.i32.f32(float, metadata) #1

; Function Attrs: inaccessiblememonly nounwind willreturn
declare double @llvm.experimental.constrained.fpext.f64.f32(float, metadata) #1

; Function Attrs: inaccessiblememonly nounwind willreturn
declare float @llvm.experimental.constrained.fptrunc.f32.f64(double, metadata, metadata) #1

attributes #0 = { norecurse nounwind strictfp "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "sycl-module-id"="test2.cl" "uniform-work-group-size"="true" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inaccessiblememonly nounwind willreturn }
attributes #2 = { strictfp }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2, !2}
!spirv.Source = !{!3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 4, i32 100000}
!4 = !{!"clang version 12.0.0 (https://github.com/c199914007/llvm.git f0c85a8adeb49638c01eee1451aa9b35462cbfd5)"}
!5 = !{i32 0, i32 0, i32 0}
!6 = !{!"none", !"none", !"none"}
!7 = !{!"float", !"int", !"uint"}
!8 = !{!"", !"", !""}
!9 = !{i32 -1, i32 -1, i32 -1}
