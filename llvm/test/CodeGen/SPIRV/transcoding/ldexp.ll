; Check that backend converts scalar arg to vector for ldexp math instructions

; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'ldexp.cl'
source_filename = "ldexp.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; #pragma OPENCL EXTENSION cl_khr_fp16 : enable
; #pragma OPENCL EXTENSION cl_khr_fp64 : enable

; __kernel void test_kernel_half(half3 x, int k, __global half3* ret) {
;    *ret = ldexp(x, k);
; }

; CHECK-SPIRV: %{{.*}} ldexp

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn writeonly
define dso_local spir_kernel void @test_kernel_half(<3 x half> noundef %x, i32 noundef %k, <3 x half> addrspace(1)* nocapture noundef writeonly %ret) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %call = call spir_func <3 x half> @_Z5ldexpDv3_Dhi(<3 x half> noundef %x, i32 noundef %k) #4
  %extractVec2 = shufflevector <3 x half> %call, <3 x half> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>
  %storetmp3 = bitcast <3 x half> addrspace(1)* %ret to <4 x half> addrspace(1)*
  store <4 x half> %extractVec2, <4 x half> addrspace(1)* %storetmp3, align 8, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <3 x half> @_Z5ldexpDv3_Dhi(<3 x half> noundef, i32 noundef) local_unnamed_addr #1

; __kernel void test_kernel_float(float3 x, int k, __global float3* ret) {
;    *ret = ldexp(x, k);
; }

; CHECK-SPIRV: %{{.*}} ldexp

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn writeonly
define dso_local spir_kernel void @test_kernel_float(<3 x float> noundef %x, i32 noundef %k, <3 x float> addrspace(1)* nocapture noundef writeonly %ret) local_unnamed_addr #2 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !12 !kernel_arg_type_qual !7 {
entry:
  %call = call spir_func <3 x float> @_Z5ldexpDv3_fi(<3 x float> noundef %x, i32 noundef %k) #4
  %extractVec2 = shufflevector <3 x float> %call, <3 x float> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>
  %storetmp3 = bitcast <3 x float> addrspace(1)* %ret to <4 x float> addrspace(1)*
  store <4 x float> %extractVec2, <4 x float> addrspace(1)* %storetmp3, align 16, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <3 x float> @_Z5ldexpDv3_fi(<3 x float> noundef, i32 noundef) local_unnamed_addr #1

; __kernel void test_kernel_double(double3 x, int k, __global double3* ret) {
;    *ret = ldexp(x, k);
; }

; CHECK-SPIRV: %{{.*}} ldexp

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn writeonly
define dso_local spir_kernel void @test_kernel_double(<3 x double> noundef %x, i32 noundef %k, <3 x double> addrspace(1)* nocapture noundef writeonly %ret) local_unnamed_addr #3 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !13 !kernel_arg_base_type !14 !kernel_arg_type_qual !7 {
entry:
  %call = call spir_func <3 x double> @_Z5ldexpDv3_di(<3 x double> noundef %x, i32 noundef %k) #4
  %extractVec2 = shufflevector <3 x double> %call, <3 x double> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>
  %storetmp3 = bitcast <3 x double> addrspace(1)* %ret to <4 x double> addrspace(1)*
  store <4 x double> %extractVec2, <4 x double> addrspace(1)* %storetmp3, align 32, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <3 x double> @_Z5ldexpDv3_di(<3 x double> noundef, i32 noundef) local_unnamed_addr #1

attributes #0 = { convergent mustprogress nofree norecurse nounwind willreturn writeonly "frame-pointer"="none" "min-legal-vector-width"="48" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent mustprogress nofree nounwind readnone willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent mustprogress nofree norecurse nounwind willreturn writeonly "frame-pointer"="none" "min-legal-vector-width"="96" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #3 = { convergent mustprogress nofree norecurse nounwind willreturn writeonly "frame-pointer"="none" "min-legal-vector-width"="192" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #4 = { convergent nounwind readnone willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 0, i32 0, i32 1}
!4 = !{!"none", !"none", !"none"}
!5 = !{!"half3", !"int", !"half3*"}
!6 = !{!"half __attribute__((ext_vector_type(3)))", !"int", !"half __attribute__((ext_vector_type(3)))*"}
!7 = !{!"", !"", !""}
!8 = !{!9, !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!"float3", !"int", !"float3*"}
!12 = !{!"float __attribute__((ext_vector_type(3)))", !"int", !"float __attribute__((ext_vector_type(3)))*"}
!13 = !{!"double3", !"int", !"double3*"}
!14 = !{!"double __attribute__((ext_vector_type(3)))", !"int", !"double __attribute__((ext_vector_type(3)))*"}
