; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'explicit-conversions.cl'
source_filename = "explicit-conversions.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpSatConvertSToU

; kernel void testSToU(global int2 *a, global uchar2 *res) {
;   res[0] = convert_uchar2_sat(*a);
; }

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @testSToU(<2 x i32> addrspace(1)* nocapture noundef readonly %a, <2 x i8> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %a, align 8, !tbaa !8
  %call = call spir_func <2 x i8> @_Z18convert_uchar2_satDv2_i(<2 x i32> noundef %0) #4
  store <2 x i8> %call, <2 x i8> addrspace(1)* %res, align 2, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <2 x i8> @_Z18convert_uchar2_satDv2_i(<2 x i32> noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpSatConvertUToS

; kernel void testUToS(global uint2 *a, global char2 *res) {
;   res[0] = convert_char2_sat(*a);
; }

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @testUToS(<2 x i32> addrspace(1)* nocapture noundef readonly %a, <2 x i8> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !12 !kernel_arg_type_qual !7 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %a, align 8, !tbaa !8
  %call = call spir_func <2 x i8> @_Z17convert_char2_satDv2_j(<2 x i32> noundef %0) #4
  store <2 x i8> %call, <2 x i8> addrspace(1)* %res, align 2, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <2 x i8> @_Z17convert_char2_satDv2_j(<2 x i32> noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpConvertUToF

; kernel void testUToF(global uint2 *a, global float2 *res) {
;   res[0] = convert_float2_rtz(*a);
; }

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @testUToF(<2 x i32> addrspace(1)* nocapture noundef readonly %a, <2 x float> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !13 !kernel_arg_base_type !14 !kernel_arg_type_qual !7 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %a, align 8, !tbaa !8
  %call = call spir_func <2 x float> @_Z18convert_float2_rtzDv2_j(<2 x i32> noundef %0) #4
  store <2 x float> %call, <2 x float> addrspace(1)* %res, align 8, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <2 x float> @_Z18convert_float2_rtzDv2_j(<2 x i32> noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpConvertFToU

; kernel void testFToUSat(global float2 *a, global uint2 *res) {
;   res[0] = convert_uint2_sat_rtn(*a);
; }

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @testFToUSat(<2 x float> addrspace(1)* nocapture noundef readonly %a, <2 x i32> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !15 !kernel_arg_base_type !16 !kernel_arg_type_qual !7 {
entry:
  %0 = load <2 x float>, <2 x float> addrspace(1)* %a, align 8, !tbaa !8
  %call = call spir_func <2 x i32> @_Z21convert_uint2_sat_rtnDv2_f(<2 x float> noundef %0) #4
  store <2 x i32> %call, <2 x i32> addrspace(1)* %res, align 8, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <2 x i32> @_Z21convert_uint2_sat_rtnDv2_f(<2 x float> noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpUConvert

; kernel void testUToUSat(global uchar *a, global uint *res) {
;   res[0] = convert_uint_sat(*a);
; }

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @testUToUSat(i8 addrspace(1)* nocapture noundef readonly %a, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #2 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !17 !kernel_arg_base_type !17 !kernel_arg_type_qual !7 {
entry:
  %0 = load i8, i8 addrspace(1)* %a, align 1, !tbaa !8
  %call = call spir_func i32 @_Z16convert_uint_sath(i8 noundef zeroext %0) #4
  store i32 %call, i32 addrspace(1)* %res, align 4, !tbaa !18
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func i32 @_Z16convert_uint_sath(i8 noundef zeroext) local_unnamed_addr #1

; CHECK-SPIRV: OpUConvert

; kernel void testUToUSat1(global uint *a, global uchar *res) {
;   res[0] = convert_uchar_sat(*a);
; }

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @testUToUSat1(i32 addrspace(1)* nocapture noundef readonly %a, i8 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #2 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !20 !kernel_arg_base_type !20 !kernel_arg_type_qual !7 {
entry:
  %0 = load i32, i32 addrspace(1)* %a, align 4, !tbaa !18
  %call = call spir_func zeroext i8 @_Z17convert_uchar_satj(i32 noundef %0) #4
  store i8 %call, i8 addrspace(1)* %res, align 1, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func zeroext i8 @_Z17convert_uchar_satj(i32 noundef) local_unnamed_addr #1

; CHECK-SPIRV: OpConvertFToU

; kernel void testFToU(global float3 *a, global uint3 *res) {
;   res[0] = convert_uint3_rtp(*a);
; }

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn
define dso_local spir_kernel void @testFToU(<3 x float> addrspace(1)* nocapture noundef readonly %a, <3 x i32> addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #3 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !21 !kernel_arg_base_type !22 !kernel_arg_type_qual !7 {
entry:
  %castToVec4 = bitcast <3 x float> addrspace(1)* %a to <4 x float> addrspace(1)*
  %loadVec4 = load <4 x float>, <4 x float> addrspace(1)* %castToVec4, align 16
  %extractVec = shufflevector <4 x float> %loadVec4, <4 x float> poison, <3 x i32> <i32 0, i32 1, i32 2>
  %call = call spir_func <3 x i32> @_Z17convert_uint3_rtpDv3_f(<3 x float> noundef %extractVec) #4
  %extractVec1 = shufflevector <3 x i32> %call, <3 x i32> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>
  %storetmp = bitcast <3 x i32> addrspace(1)* %res to <4 x i32> addrspace(1)*
  store <4 x i32> %extractVec1, <4 x i32> addrspace(1)* %storetmp, align 16, !tbaa !8
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func <3 x i32> @_Z17convert_uint3_rtpDv3_f(<3 x float> noundef) local_unnamed_addr #1

attributes #0 = { convergent mustprogress nofree norecurse nounwind willreturn "frame-pointer"="none" "min-legal-vector-width"="64" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent mustprogress nofree nounwind readnone willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent mustprogress nofree norecurse nounwind willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #3 = { convergent mustprogress nofree norecurse nounwind willreturn "frame-pointer"="none" "min-legal-vector-width"="96" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #4 = { convergent nounwind readnone willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 1}
!4 = !{!"none", !"none"}
!5 = !{!"int2*", !"uchar2*"}
!6 = !{!"int __attribute__((ext_vector_type(2)))*", !"uchar __attribute__((ext_vector_type(2)))*"}
!7 = !{!"", !""}
!8 = !{!9, !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!"uint2*", !"char2*"}
!12 = !{!"uint __attribute__((ext_vector_type(2)))*", !"char __attribute__((ext_vector_type(2)))*"}
!13 = !{!"uint2*", !"float2*"}
!14 = !{!"uint __attribute__((ext_vector_type(2)))*", !"float __attribute__((ext_vector_type(2)))*"}
!15 = !{!"float2*", !"uint2*"}
!16 = !{!"float __attribute__((ext_vector_type(2)))*", !"uint __attribute__((ext_vector_type(2)))*"}
!17 = !{!"uchar*", !"uint*"}
!18 = !{!19, !19, i64 0}
!19 = !{!"int", !9, i64 0}
!20 = !{!"uint*", !"uchar*"}
!21 = !{!"float3*", !"uint3*"}
!22 = !{!"float __attribute__((ext_vector_type(3)))*", !"uint __attribute__((ext_vector_type(3)))*"}
