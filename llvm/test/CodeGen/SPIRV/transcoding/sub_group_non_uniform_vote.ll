;; #pragma OPENCL EXTENSION cl_khr_subgroup_non_uniform_vote : enable
;; 
;; kernel void testSubGroupElect(global int* dst){
;; 	dst[0] = sub_group_elect();
;; }
;; 
;; kernel void testSubGroupNonUniformAll(global int* dst){
;; 	dst[0] = sub_group_non_uniform_all(0); 
;; }
;; 
;; kernel void testSubGroupNonUniformAny(global int* dst){
;; 	dst[0] = sub_group_non_uniform_any(0);
;; }
;; 
;; #pragma OPENCL EXTENSION cl_khr_fp16 : enable
;; #pragma OPENCL EXTENSION cl_khr_fp64 : enable
;; kernel void testSubGroupNonUniformAllEqual(global int* dst){
;;     {
;;         char v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         uchar v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         short v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         ushort v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         int v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         uint v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         long v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         ulong v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         float v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         half v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;;     {
;;         double v = 0;
;;         dst[0] = sub_group_non_uniform_all_equal( v );
;;     }
;; }

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'sub_group_non_uniform_vote.cl'
source_filename = "sub_group_non_uniform_vote.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV-DAG: OpCapability GroupNonUniform
; CHECK-SPIRV-DAG: OpCapability GroupNonUniformVote

; CHECK-SPIRV-DAG: %[[bool:[0-9]+]] = OpTypeBool
; CHECK-SPIRV-DAG: %[[char:[0-9]+]] = OpTypeInt 8 0
; CHECK-SPIRV-DAG: %[[short:[0-9]+]] = OpTypeInt 16 0
; CHECK-SPIRV-DAG: %[[int:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV-DAG: %[[long:[0-9]+]] = OpTypeInt 64 0
; CHECK-SPIRV-DAG: %[[half:[0-9]+]] = OpTypeFloat 16
; CHECK-SPIRV-DAG: %[[float:[0-9]+]] = OpTypeFloat 32
; CHECK-SPIRV-DAG: %[[double:[0-9]+]] = OpTypeFloat 64

; CHECK-SPIRV-DAG: %[[false:[0-9]+]] = OpConstantFalse %[[bool]]
; CHECK-SPIRV-DAG: %[[ScopeSubgroup:[0-9]+]] = OpConstant %[[int]] 3
; CHECK-SPIRV-DAG: %[[char_0:[0-9]+]] = OpConstant %[[char]] 0
; CHECK-SPIRV-DAG: %[[short_0:[0-9]+]] = OpConstant %[[short]] 0
; CHECK-SPIRV-DAG: %[[int_0:[0-9]+]] = OpConstant %[[int]] 0
; CHECK-SPIRV-DAG: %[[long_0:[0-9]+]] = OpConstant %[[long]] 0
; CHECK-SPIRV-DAG: %[[half_0:[0-9]+]] = OpConstant %[[half]] 0
; CHECK-SPIRV-DAG: %[[float_0:[0-9]+]] = OpConstant %[[float]] 0
; CHECK-SPIRV-DAG: %[[double_0:[0-9]+]] = OpConstant %[[double]] 0

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformElect %[[bool]] %[[ScopeSubgroup]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testSubGroupElect(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z15sub_group_electv() #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z15sub_group_electv() local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAll %[[bool]] %[[ScopeSubgroup]] %[[false]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testSubGroupNonUniformAll(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z25sub_group_non_uniform_alli(i32 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z25sub_group_non_uniform_alli(i32) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAny %[[bool]] %[[ScopeSubgroup]] %[[false]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testSubGroupNonUniformAny(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z25sub_group_non_uniform_anyi(i32 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z25sub_group_non_uniform_anyi(i32) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformAllEqual %[[bool]] %[[ScopeSubgroup]] %[[double_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testSubGroupNonUniformAllEqual(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equalc(i8 signext 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !7
  %3 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equalh(i8 zeroext 0) #2
  store i32 %3, i32 addrspace(1)* %0, align 4, !tbaa !7
  %4 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equals(i16 signext 0) #2
  store i32 %4, i32 addrspace(1)* %0, align 4, !tbaa !7
  %5 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equalt(i16 zeroext 0) #2
  store i32 %5, i32 addrspace(1)* %0, align 4, !tbaa !7
  %6 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equali(i32 0) #2
  store i32 %6, i32 addrspace(1)* %0, align 4, !tbaa !7
  %7 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equalj(i32 0) #2
  store i32 %7, i32 addrspace(1)* %0, align 4, !tbaa !7
  %8 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equall(i64 0) #2
  store i32 %8, i32 addrspace(1)* %0, align 4, !tbaa !7
  %9 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equalm(i64 0) #2
  store i32 %9, i32 addrspace(1)* %0, align 4, !tbaa !7
  %10 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equalf(float 0.000000e+00) #2
  store i32 %10, i32 addrspace(1)* %0, align 4, !tbaa !7
  %11 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equalDh(half 0xH0000) #2
  store i32 %11, i32 addrspace(1)* %0, align 4, !tbaa !7
  %12 = tail call spir_func i32 @_Z31sub_group_non_uniform_all_equald(double 0.000000e+00) #2
  store i32 %12, i32 addrspace(1)* %0, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equalc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equalh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equals(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equalt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equali(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equalj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equall(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equalm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equalf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equalDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_all_equald(double) local_unnamed_addr #1

attributes #0 = { convergent nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "uniform-work-group-size"="true" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { convergent "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{!"clang version 9.0.1 (https://github.com/llvm/llvm-project.git cb6d58d1dcf36a29ae5dd24ff891d6552f00bac7)"}
!3 = !{i32 1}
!4 = !{!"none"}
!5 = !{!"int*"}
!6 = !{!""}
!7 = !{!8, !8, i64 0}
!8 = !{!"int", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
