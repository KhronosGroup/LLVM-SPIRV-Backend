;; #pragma OPENCL EXTENSION cl_khr_subgroup_non_uniform_arithmetic : enable
;; #pragma OPENCL EXTENSION cl_khr_fp16 : enable
;; #pragma OPENCL EXTENSION cl_khr_fp64 : enable
;; 
;; kernel void testNonUniformArithmeticChar(global char* dst)
;; {
;;     char v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticUChar(global uchar* dst)
;; {
;;     uchar v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticShort(global short* dst)
;; {
;;     short v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticUShort(global ushort* dst)
;; {
;;     ushort v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticInt(global int* dst)
;; {
;;     int v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticUInt(global uint* dst)
;; {
;;     uint v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticLong(global long* dst)
;; {
;;     long v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticULong(global ulong* dst)
;; {
;;     ulong v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticFloat(global float* dst)
;; {
;;     float v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticHalf(global half* dst)
;; {
;;     half v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformArithmeticDouble(global double* dst)
;; {
;;     double v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_add(v);
;;     dst[1] = sub_group_non_uniform_reduce_mul(v);
;;     dst[2] = sub_group_non_uniform_reduce_min(v);
;;     dst[3] = sub_group_non_uniform_reduce_max(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_add(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_mul(v);
;;     dst[6] = sub_group_non_uniform_scan_inclusive_min(v);
;;     dst[7] = sub_group_non_uniform_scan_inclusive_max(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_add(v);
;;     dst[9] = sub_group_non_uniform_scan_exclusive_mul(v);
;;     dst[10] = sub_group_non_uniform_scan_exclusive_min(v);
;;     dst[11] = sub_group_non_uniform_scan_exclusive_max(v);
;; }
;; 
;; kernel void testNonUniformBitwiseChar(global char* dst)
;; {
;;     char v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; kernel void testNonUniformBitwiseUChar(global uchar* dst)
;; {
;;     uchar v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; kernel void testNonUniformBitwiseShort(global short* dst)
;; {
;;     short v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; kernel void testNonUniformBitwiseUShort(global ushort* dst)
;; {
;;     ushort v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; kernel void testNonUniformBitwiseInt(global int* dst)
;; {
;;     int v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; kernel void testNonUniformBitwiseUInt(global uint* dst)
;; {
;;     uint v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; kernel void testNonUniformBitwiseLong(global long* dst)
;; {
;;     long v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; kernel void testNonUniformBitwiseULong(global ulong* dst)
;; {
;;     ulong v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_xor(v);
;; }
;; 
;; kernel void testNonUniformLogical(global int* dst)
;; {
;;     int v = 0;
;;     dst[0] = sub_group_non_uniform_reduce_logical_and(v);
;;     dst[1] = sub_group_non_uniform_reduce_logical_or(v);
;;     dst[2] = sub_group_non_uniform_reduce_logical_xor(v);
;;     dst[3] = sub_group_non_uniform_scan_inclusive_logical_and(v);
;;     dst[4] = sub_group_non_uniform_scan_inclusive_logical_or(v);
;;     dst[5] = sub_group_non_uniform_scan_inclusive_logical_xor(v);
;;     dst[6] = sub_group_non_uniform_scan_exclusive_logical_and(v);
;;     dst[7] = sub_group_non_uniform_scan_exclusive_logical_or(v);
;;     dst[8] = sub_group_non_uniform_scan_exclusive_logical_xor(v);
;; }

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV-DAG: OpCapability GroupNonUniformArithmetic

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

; ModuleID = 'sub_group_non_uniform_arithmetic.cl'
source_filename = "sub_group_non_uniform_arithmetic.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticChar(i8 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func signext i8 @_Z32sub_group_non_uniform_reduce_addc(i8 signext 0) #2
  store i8 %2, i8 addrspace(1)* %0, align 1, !tbaa !7
  %3 = tail call spir_func signext i8 @_Z32sub_group_non_uniform_reduce_mulc(i8 signext 0) #2
  %4 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 1
  store i8 %3, i8 addrspace(1)* %4, align 1, !tbaa !7
  %5 = tail call spir_func signext i8 @_Z32sub_group_non_uniform_reduce_minc(i8 signext 0) #2
  %6 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 2
  store i8 %5, i8 addrspace(1)* %6, align 1, !tbaa !7
  %7 = tail call spir_func signext i8 @_Z32sub_group_non_uniform_reduce_maxc(i8 signext 0) #2
  %8 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 3
  store i8 %7, i8 addrspace(1)* %8, align 1, !tbaa !7
  %9 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_addc(i8 signext 0) #2
  %10 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 4
  store i8 %9, i8 addrspace(1)* %10, align 1, !tbaa !7
  %11 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_mulc(i8 signext 0) #2
  %12 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 5
  store i8 %11, i8 addrspace(1)* %12, align 1, !tbaa !7
  %13 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_minc(i8 signext 0) #2
  %14 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 6
  store i8 %13, i8 addrspace(1)* %14, align 1, !tbaa !7
  %15 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_maxc(i8 signext 0) #2
  %16 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 7
  store i8 %15, i8 addrspace(1)* %16, align 1, !tbaa !7
  %17 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_addc(i8 signext 0) #2
  %18 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 8
  store i8 %17, i8 addrspace(1)* %18, align 1, !tbaa !7
  %19 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_mulc(i8 signext 0) #2
  %20 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 9
  store i8 %19, i8 addrspace(1)* %20, align 1, !tbaa !7
  %21 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_minc(i8 signext 0) #2
  %22 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 10
  store i8 %21, i8 addrspace(1)* %22, align 1, !tbaa !7
  %23 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_maxc(i8 signext 0) #2
  %24 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 11
  store i8 %23, i8 addrspace(1)* %24, align 1, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z32sub_group_non_uniform_reduce_addc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z32sub_group_non_uniform_reduce_mulc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z32sub_group_non_uniform_reduce_minc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z32sub_group_non_uniform_reduce_maxc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_addc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_mulc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_minc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_maxc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_addc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_mulc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_minc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_maxc(i8 signext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticUChar(i8 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !10 !kernel_arg_base_type !10 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_addh(i8 zeroext 0) #2
  store i8 %2, i8 addrspace(1)* %0, align 1, !tbaa !7
  %3 = tail call spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_mulh(i8 zeroext 0) #2
  %4 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 1
  store i8 %3, i8 addrspace(1)* %4, align 1, !tbaa !7
  %5 = tail call spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_minh(i8 zeroext 0) #2
  %6 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 2
  store i8 %5, i8 addrspace(1)* %6, align 1, !tbaa !7
  %7 = tail call spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_maxh(i8 zeroext 0) #2
  %8 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 3
  store i8 %7, i8 addrspace(1)* %8, align 1, !tbaa !7
  %9 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_addh(i8 zeroext 0) #2
  %10 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 4
  store i8 %9, i8 addrspace(1)* %10, align 1, !tbaa !7
  %11 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_mulh(i8 zeroext 0) #2
  %12 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 5
  store i8 %11, i8 addrspace(1)* %12, align 1, !tbaa !7
  %13 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_minh(i8 zeroext 0) #2
  %14 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 6
  store i8 %13, i8 addrspace(1)* %14, align 1, !tbaa !7
  %15 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_maxh(i8 zeroext 0) #2
  %16 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 7
  store i8 %15, i8 addrspace(1)* %16, align 1, !tbaa !7
  %17 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_addh(i8 zeroext 0) #2
  %18 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 8
  store i8 %17, i8 addrspace(1)* %18, align 1, !tbaa !7
  %19 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_mulh(i8 zeroext 0) #2
  %20 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 9
  store i8 %19, i8 addrspace(1)* %20, align 1, !tbaa !7
  %21 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_minh(i8 zeroext 0) #2
  %22 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 10
  store i8 %21, i8 addrspace(1)* %22, align 1, !tbaa !7
  %23 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_maxh(i8 zeroext 0) #2
  %24 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 11
  store i8 %23, i8 addrspace(1)* %24, align 1, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_addh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_mulh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_minh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_maxh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_addh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_mulh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_minh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_maxh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_addh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_mulh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_minh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_maxh(i8 zeroext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticShort(i16 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !11 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func signext i16 @_Z32sub_group_non_uniform_reduce_adds(i16 signext 0) #2
  store i16 %2, i16 addrspace(1)* %0, align 2, !tbaa !12
  %3 = tail call spir_func signext i16 @_Z32sub_group_non_uniform_reduce_muls(i16 signext 0) #2
  %4 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 1
  store i16 %3, i16 addrspace(1)* %4, align 2, !tbaa !12
  %5 = tail call spir_func signext i16 @_Z32sub_group_non_uniform_reduce_mins(i16 signext 0) #2
  %6 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 2
  store i16 %5, i16 addrspace(1)* %6, align 2, !tbaa !12
  %7 = tail call spir_func signext i16 @_Z32sub_group_non_uniform_reduce_maxs(i16 signext 0) #2
  %8 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 3
  store i16 %7, i16 addrspace(1)* %8, align 2, !tbaa !12
  %9 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_adds(i16 signext 0) #2
  %10 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 4
  store i16 %9, i16 addrspace(1)* %10, align 2, !tbaa !12
  %11 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_muls(i16 signext 0) #2
  %12 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 5
  store i16 %11, i16 addrspace(1)* %12, align 2, !tbaa !12
  %13 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_mins(i16 signext 0) #2
  %14 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 6
  store i16 %13, i16 addrspace(1)* %14, align 2, !tbaa !12
  %15 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_maxs(i16 signext 0) #2
  %16 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 7
  store i16 %15, i16 addrspace(1)* %16, align 2, !tbaa !12
  %17 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_adds(i16 signext 0) #2
  %18 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 8
  store i16 %17, i16 addrspace(1)* %18, align 2, !tbaa !12
  %19 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_muls(i16 signext 0) #2
  %20 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 9
  store i16 %19, i16 addrspace(1)* %20, align 2, !tbaa !12
  %21 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_mins(i16 signext 0) #2
  %22 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 10
  store i16 %21, i16 addrspace(1)* %22, align 2, !tbaa !12
  %23 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_maxs(i16 signext 0) #2
  %24 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 11
  store i16 %23, i16 addrspace(1)* %24, align 2, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z32sub_group_non_uniform_reduce_adds(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z32sub_group_non_uniform_reduce_muls(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z32sub_group_non_uniform_reduce_mins(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z32sub_group_non_uniform_reduce_maxs(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_adds(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_muls(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_mins(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_maxs(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_adds(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_muls(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_mins(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_maxs(i16 signext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticUShort(i16 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_addt(i16 zeroext 0) #2
  store i16 %2, i16 addrspace(1)* %0, align 2, !tbaa !12
  %3 = tail call spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_mult(i16 zeroext 0) #2
  %4 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 1
  store i16 %3, i16 addrspace(1)* %4, align 2, !tbaa !12
  %5 = tail call spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_mint(i16 zeroext 0) #2
  %6 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 2
  store i16 %5, i16 addrspace(1)* %6, align 2, !tbaa !12
  %7 = tail call spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_maxt(i16 zeroext 0) #2
  %8 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 3
  store i16 %7, i16 addrspace(1)* %8, align 2, !tbaa !12
  %9 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_addt(i16 zeroext 0) #2
  %10 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 4
  store i16 %9, i16 addrspace(1)* %10, align 2, !tbaa !12
  %11 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_mult(i16 zeroext 0) #2
  %12 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 5
  store i16 %11, i16 addrspace(1)* %12, align 2, !tbaa !12
  %13 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_mint(i16 zeroext 0) #2
  %14 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 6
  store i16 %13, i16 addrspace(1)* %14, align 2, !tbaa !12
  %15 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_maxt(i16 zeroext 0) #2
  %16 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 7
  store i16 %15, i16 addrspace(1)* %16, align 2, !tbaa !12
  %17 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_addt(i16 zeroext 0) #2
  %18 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 8
  store i16 %17, i16 addrspace(1)* %18, align 2, !tbaa !12
  %19 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_mult(i16 zeroext 0) #2
  %20 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 9
  store i16 %19, i16 addrspace(1)* %20, align 2, !tbaa !12
  %21 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_mint(i16 zeroext 0) #2
  %22 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 10
  store i16 %21, i16 addrspace(1)* %22, align 2, !tbaa !12
  %23 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_maxt(i16 zeroext 0) #2
  %24 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 11
  store i16 %23, i16 addrspace(1)* %24, align 2, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_addt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_mult(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_mint(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_maxt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_addt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_mult(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_mint(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_maxt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_addt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_mult(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_mint(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_maxt(i16 zeroext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticInt(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !15 !kernel_arg_base_type !15 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_addi(i32 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !16
  %3 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_muli(i32 0) #2
  %4 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1
  store i32 %3, i32 addrspace(1)* %4, align 4, !tbaa !16
  %5 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_mini(i32 0) #2
  %6 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 2
  store i32 %5, i32 addrspace(1)* %6, align 4, !tbaa !16
  %7 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_maxi(i32 0) #2
  %8 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 3
  store i32 %7, i32 addrspace(1)* %8, align 4, !tbaa !16
  %9 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_addi(i32 0) #2
  %10 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 4
  store i32 %9, i32 addrspace(1)* %10, align 4, !tbaa !16
  %11 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_muli(i32 0) #2
  %12 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 5
  store i32 %11, i32 addrspace(1)* %12, align 4, !tbaa !16
  %13 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_mini(i32 0) #2
  %14 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 6
  store i32 %13, i32 addrspace(1)* %14, align 4, !tbaa !16
  %15 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_maxi(i32 0) #2
  %16 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 7
  store i32 %15, i32 addrspace(1)* %16, align 4, !tbaa !16
  %17 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_addi(i32 0) #2
  %18 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 8
  store i32 %17, i32 addrspace(1)* %18, align 4, !tbaa !16
  %19 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_muli(i32 0) #2
  %20 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 9
  store i32 %19, i32 addrspace(1)* %20, align 4, !tbaa !16
  %21 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_mini(i32 0) #2
  %22 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 10
  store i32 %21, i32 addrspace(1)* %22, align 4, !tbaa !16
  %23 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_maxi(i32 0) #2
  %24 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 11
  store i32 %23, i32 addrspace(1)* %24, align 4, !tbaa !16
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_addi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_muli(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_mini(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_maxi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_addi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_muli(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_mini(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_maxi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_addi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_muli(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_mini(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_maxi(i32) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticUInt(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !18 !kernel_arg_base_type !18 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_addj(i32 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !16
  %3 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_mulj(i32 0) #2
  %4 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1
  store i32 %3, i32 addrspace(1)* %4, align 4, !tbaa !16
  %5 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_minj(i32 0) #2
  %6 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 2
  store i32 %5, i32 addrspace(1)* %6, align 4, !tbaa !16
  %7 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_maxj(i32 0) #2
  %8 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 3
  store i32 %7, i32 addrspace(1)* %8, align 4, !tbaa !16
  %9 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_addj(i32 0) #2
  %10 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 4
  store i32 %9, i32 addrspace(1)* %10, align 4, !tbaa !16
  %11 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_mulj(i32 0) #2
  %12 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 5
  store i32 %11, i32 addrspace(1)* %12, align 4, !tbaa !16
  %13 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_minj(i32 0) #2
  %14 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 6
  store i32 %13, i32 addrspace(1)* %14, align 4, !tbaa !16
  %15 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_maxj(i32 0) #2
  %16 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 7
  store i32 %15, i32 addrspace(1)* %16, align 4, !tbaa !16
  %17 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_addj(i32 0) #2
  %18 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 8
  store i32 %17, i32 addrspace(1)* %18, align 4, !tbaa !16
  %19 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_mulj(i32 0) #2
  %20 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 9
  store i32 %19, i32 addrspace(1)* %20, align 4, !tbaa !16
  %21 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_minj(i32 0) #2
  %22 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 10
  store i32 %21, i32 addrspace(1)* %22, align 4, !tbaa !16
  %23 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_maxj(i32 0) #2
  %24 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 11
  store i32 %23, i32 addrspace(1)* %24, align 4, !tbaa !16
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_addj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_mulj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_minj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_maxj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_addj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_mulj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_minj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_maxj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_addj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_mulj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_minj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_maxj(i32) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMin %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformSMax %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticLong(i64 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !19 !kernel_arg_base_type !19 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_addl(i64 0) #2
  store i64 %2, i64 addrspace(1)* %0, align 8, !tbaa !20
  %3 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_mull(i64 0) #2
  %4 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 1
  store i64 %3, i64 addrspace(1)* %4, align 8, !tbaa !20
  %5 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_minl(i64 0) #2
  %6 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 2
  store i64 %5, i64 addrspace(1)* %6, align 8, !tbaa !20
  %7 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_maxl(i64 0) #2
  %8 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 3
  store i64 %7, i64 addrspace(1)* %8, align 8, !tbaa !20
  %9 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_addl(i64 0) #2
  %10 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 4
  store i64 %9, i64 addrspace(1)* %10, align 8, !tbaa !20
  %11 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_mull(i64 0) #2
  %12 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 5
  store i64 %11, i64 addrspace(1)* %12, align 8, !tbaa !20
  %13 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_minl(i64 0) #2
  %14 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 6
  store i64 %13, i64 addrspace(1)* %14, align 8, !tbaa !20
  %15 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_maxl(i64 0) #2
  %16 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 7
  store i64 %15, i64 addrspace(1)* %16, align 8, !tbaa !20
  %17 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_addl(i64 0) #2
  %18 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 8
  store i64 %17, i64 addrspace(1)* %18, align 8, !tbaa !20
  %19 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_mull(i64 0) #2
  %20 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 9
  store i64 %19, i64 addrspace(1)* %20, align 8, !tbaa !20
  %21 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_minl(i64 0) #2
  %22 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 10
  store i64 %21, i64 addrspace(1)* %22, align 8, !tbaa !20
  %23 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_maxl(i64 0) #2
  %24 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 11
  store i64 %23, i64 addrspace(1)* %24, align 8, !tbaa !20
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_addl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_mull(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_minl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_maxl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_addl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_mull(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_minl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_maxl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_addl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_mull(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_minl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_maxl(i64) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIAdd %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformIMul %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMin %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformUMax %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticULong(i64 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !22 !kernel_arg_base_type !22 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_addm(i64 0) #2
  store i64 %2, i64 addrspace(1)* %0, align 8, !tbaa !20
  %3 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_mulm(i64 0) #2
  %4 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 1
  store i64 %3, i64 addrspace(1)* %4, align 8, !tbaa !20
  %5 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_minm(i64 0) #2
  %6 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 2
  store i64 %5, i64 addrspace(1)* %6, align 8, !tbaa !20
  %7 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_maxm(i64 0) #2
  %8 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 3
  store i64 %7, i64 addrspace(1)* %8, align 8, !tbaa !20
  %9 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_addm(i64 0) #2
  %10 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 4
  store i64 %9, i64 addrspace(1)* %10, align 8, !tbaa !20
  %11 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_mulm(i64 0) #2
  %12 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 5
  store i64 %11, i64 addrspace(1)* %12, align 8, !tbaa !20
  %13 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_minm(i64 0) #2
  %14 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 6
  store i64 %13, i64 addrspace(1)* %14, align 8, !tbaa !20
  %15 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_maxm(i64 0) #2
  %16 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 7
  store i64 %15, i64 addrspace(1)* %16, align 8, !tbaa !20
  %17 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_addm(i64 0) #2
  %18 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 8
  store i64 %17, i64 addrspace(1)* %18, align 8, !tbaa !20
  %19 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_mulm(i64 0) #2
  %20 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 9
  store i64 %19, i64 addrspace(1)* %20, align 8, !tbaa !20
  %21 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_minm(i64 0) #2
  %22 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 10
  store i64 %21, i64 addrspace(1)* %22, align 8, !tbaa !20
  %23 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_maxm(i64 0) #2
  %24 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 11
  store i64 %23, i64 addrspace(1)* %24, align 8, !tbaa !20
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_addm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_mulm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_minm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_maxm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_addm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_mulm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_minm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_maxm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_addm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_mulm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_minm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_maxm(i64) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[float]] %[[ScopeSubgroup]] Reduce %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[float]] %[[ScopeSubgroup]] Reduce %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[float]] %[[ScopeSubgroup]] Reduce %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[float]] %[[ScopeSubgroup]] Reduce %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[float]] %[[ScopeSubgroup]] InclusiveScan %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[float]] %[[ScopeSubgroup]] InclusiveScan %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[float]] %[[ScopeSubgroup]] InclusiveScan %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[float]] %[[ScopeSubgroup]] InclusiveScan %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[float]] %[[ScopeSubgroup]] ExclusiveScan %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[float]] %[[ScopeSubgroup]] ExclusiveScan %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[float]] %[[ScopeSubgroup]] ExclusiveScan %[[float_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[float]] %[[ScopeSubgroup]] ExclusiveScan %[[float_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticFloat(float addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !23 !kernel_arg_base_type !23 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func float @_Z32sub_group_non_uniform_reduce_addf(float 0.000000e+00) #2
  store float %2, float addrspace(1)* %0, align 4, !tbaa !24
  %3 = tail call spir_func float @_Z32sub_group_non_uniform_reduce_mulf(float 0.000000e+00) #2
  %4 = getelementptr inbounds float, float addrspace(1)* %0, i64 1
  store float %3, float addrspace(1)* %4, align 4, !tbaa !24
  %5 = tail call spir_func float @_Z32sub_group_non_uniform_reduce_minf(float 0.000000e+00) #2
  %6 = getelementptr inbounds float, float addrspace(1)* %0, i64 2
  store float %5, float addrspace(1)* %6, align 4, !tbaa !24
  %7 = tail call spir_func float @_Z32sub_group_non_uniform_reduce_maxf(float 0.000000e+00) #2
  %8 = getelementptr inbounds float, float addrspace(1)* %0, i64 3
  store float %7, float addrspace(1)* %8, align 4, !tbaa !24
  %9 = tail call spir_func float @_Z40sub_group_non_uniform_scan_inclusive_addf(float 0.000000e+00) #2
  %10 = getelementptr inbounds float, float addrspace(1)* %0, i64 4
  store float %9, float addrspace(1)* %10, align 4, !tbaa !24
  %11 = tail call spir_func float @_Z40sub_group_non_uniform_scan_inclusive_mulf(float 0.000000e+00) #2
  %12 = getelementptr inbounds float, float addrspace(1)* %0, i64 5
  store float %11, float addrspace(1)* %12, align 4, !tbaa !24
  %13 = tail call spir_func float @_Z40sub_group_non_uniform_scan_inclusive_minf(float 0.000000e+00) #2
  %14 = getelementptr inbounds float, float addrspace(1)* %0, i64 6
  store float %13, float addrspace(1)* %14, align 4, !tbaa !24
  %15 = tail call spir_func float @_Z40sub_group_non_uniform_scan_inclusive_maxf(float 0.000000e+00) #2
  %16 = getelementptr inbounds float, float addrspace(1)* %0, i64 7
  store float %15, float addrspace(1)* %16, align 4, !tbaa !24
  %17 = tail call spir_func float @_Z40sub_group_non_uniform_scan_exclusive_addf(float 0.000000e+00) #2
  %18 = getelementptr inbounds float, float addrspace(1)* %0, i64 8
  store float %17, float addrspace(1)* %18, align 4, !tbaa !24
  %19 = tail call spir_func float @_Z40sub_group_non_uniform_scan_exclusive_mulf(float 0.000000e+00) #2
  %20 = getelementptr inbounds float, float addrspace(1)* %0, i64 9
  store float %19, float addrspace(1)* %20, align 4, !tbaa !24
  %21 = tail call spir_func float @_Z40sub_group_non_uniform_scan_exclusive_minf(float 0.000000e+00) #2
  %22 = getelementptr inbounds float, float addrspace(1)* %0, i64 10
  store float %21, float addrspace(1)* %22, align 4, !tbaa !24
  %23 = tail call spir_func float @_Z40sub_group_non_uniform_scan_exclusive_maxf(float 0.000000e+00) #2
  %24 = getelementptr inbounds float, float addrspace(1)* %0, i64 11
  store float %23, float addrspace(1)* %24, align 4, !tbaa !24
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func float @_Z32sub_group_non_uniform_reduce_addf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z32sub_group_non_uniform_reduce_mulf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z32sub_group_non_uniform_reduce_minf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z32sub_group_non_uniform_reduce_maxf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_inclusive_addf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_inclusive_mulf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_inclusive_minf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_inclusive_maxf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_exclusive_addf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_exclusive_mulf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_exclusive_minf(float) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func float @_Z40sub_group_non_uniform_scan_exclusive_maxf(float) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[half]] %[[ScopeSubgroup]] Reduce %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[half]] %[[ScopeSubgroup]] Reduce %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[half]] %[[ScopeSubgroup]] Reduce %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[half]] %[[ScopeSubgroup]] Reduce %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[half]] %[[ScopeSubgroup]] InclusiveScan %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[half]] %[[ScopeSubgroup]] InclusiveScan %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[half]] %[[ScopeSubgroup]] InclusiveScan %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[half]] %[[ScopeSubgroup]] InclusiveScan %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[half]] %[[ScopeSubgroup]] ExclusiveScan %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[half]] %[[ScopeSubgroup]] ExclusiveScan %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[half]] %[[ScopeSubgroup]] ExclusiveScan %[[half_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[half]] %[[ScopeSubgroup]] ExclusiveScan %[[half_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticHalf(half addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !26 !kernel_arg_base_type !26 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func half @_Z32sub_group_non_uniform_reduce_addDh(half 0xH0000) #2
  store half %2, half addrspace(1)* %0, align 2, !tbaa !27
  %3 = tail call spir_func half @_Z32sub_group_non_uniform_reduce_mulDh(half 0xH0000) #2
  %4 = getelementptr inbounds half, half addrspace(1)* %0, i64 1
  store half %3, half addrspace(1)* %4, align 2, !tbaa !27
  %5 = tail call spir_func half @_Z32sub_group_non_uniform_reduce_minDh(half 0xH0000) #2
  %6 = getelementptr inbounds half, half addrspace(1)* %0, i64 2
  store half %5, half addrspace(1)* %6, align 2, !tbaa !27
  %7 = tail call spir_func half @_Z32sub_group_non_uniform_reduce_maxDh(half 0xH0000) #2
  %8 = getelementptr inbounds half, half addrspace(1)* %0, i64 3
  store half %7, half addrspace(1)* %8, align 2, !tbaa !27
  %9 = tail call spir_func half @_Z40sub_group_non_uniform_scan_inclusive_addDh(half 0xH0000) #2
  %10 = getelementptr inbounds half, half addrspace(1)* %0, i64 4
  store half %9, half addrspace(1)* %10, align 2, !tbaa !27
  %11 = tail call spir_func half @_Z40sub_group_non_uniform_scan_inclusive_mulDh(half 0xH0000) #2
  %12 = getelementptr inbounds half, half addrspace(1)* %0, i64 5
  store half %11, half addrspace(1)* %12, align 2, !tbaa !27
  %13 = tail call spir_func half @_Z40sub_group_non_uniform_scan_inclusive_minDh(half 0xH0000) #2
  %14 = getelementptr inbounds half, half addrspace(1)* %0, i64 6
  store half %13, half addrspace(1)* %14, align 2, !tbaa !27
  %15 = tail call spir_func half @_Z40sub_group_non_uniform_scan_inclusive_maxDh(half 0xH0000) #2
  %16 = getelementptr inbounds half, half addrspace(1)* %0, i64 7
  store half %15, half addrspace(1)* %16, align 2, !tbaa !27
  %17 = tail call spir_func half @_Z40sub_group_non_uniform_scan_exclusive_addDh(half 0xH0000) #2
  %18 = getelementptr inbounds half, half addrspace(1)* %0, i64 8
  store half %17, half addrspace(1)* %18, align 2, !tbaa !27
  %19 = tail call spir_func half @_Z40sub_group_non_uniform_scan_exclusive_mulDh(half 0xH0000) #2
  %20 = getelementptr inbounds half, half addrspace(1)* %0, i64 9
  store half %19, half addrspace(1)* %20, align 2, !tbaa !27
  %21 = tail call spir_func half @_Z40sub_group_non_uniform_scan_exclusive_minDh(half 0xH0000) #2
  %22 = getelementptr inbounds half, half addrspace(1)* %0, i64 10
  store half %21, half addrspace(1)* %22, align 2, !tbaa !27
  %23 = tail call spir_func half @_Z40sub_group_non_uniform_scan_exclusive_maxDh(half 0xH0000) #2
  %24 = getelementptr inbounds half, half addrspace(1)* %0, i64 11
  store half %23, half addrspace(1)* %24, align 2, !tbaa !27
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func half @_Z32sub_group_non_uniform_reduce_addDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z32sub_group_non_uniform_reduce_mulDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z32sub_group_non_uniform_reduce_minDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z32sub_group_non_uniform_reduce_maxDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_inclusive_addDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_inclusive_mulDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_inclusive_minDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_inclusive_maxDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_exclusive_addDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_exclusive_mulDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_exclusive_minDh(half) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func half @_Z40sub_group_non_uniform_scan_exclusive_maxDh(half) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[double]] %[[ScopeSubgroup]] Reduce %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[double]] %[[ScopeSubgroup]] Reduce %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[double]] %[[ScopeSubgroup]] Reduce %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[double]] %[[ScopeSubgroup]] Reduce %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[double]] %[[ScopeSubgroup]] InclusiveScan %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[double]] %[[ScopeSubgroup]] InclusiveScan %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[double]] %[[ScopeSubgroup]] InclusiveScan %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[double]] %[[ScopeSubgroup]] InclusiveScan %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFAdd %[[double]] %[[ScopeSubgroup]] ExclusiveScan %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMul %[[double]] %[[ScopeSubgroup]] ExclusiveScan %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMin %[[double]] %[[ScopeSubgroup]] ExclusiveScan %[[double_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformFMax %[[double]] %[[ScopeSubgroup]] ExclusiveScan %[[double_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformArithmeticDouble(double addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !29 !kernel_arg_base_type !29 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func double @_Z32sub_group_non_uniform_reduce_addd(double 0.000000e+00) #2
  store double %2, double addrspace(1)* %0, align 8, !tbaa !30
  %3 = tail call spir_func double @_Z32sub_group_non_uniform_reduce_muld(double 0.000000e+00) #2
  %4 = getelementptr inbounds double, double addrspace(1)* %0, i64 1
  store double %3, double addrspace(1)* %4, align 8, !tbaa !30
  %5 = tail call spir_func double @_Z32sub_group_non_uniform_reduce_mind(double 0.000000e+00) #2
  %6 = getelementptr inbounds double, double addrspace(1)* %0, i64 2
  store double %5, double addrspace(1)* %6, align 8, !tbaa !30
  %7 = tail call spir_func double @_Z32sub_group_non_uniform_reduce_maxd(double 0.000000e+00) #2
  %8 = getelementptr inbounds double, double addrspace(1)* %0, i64 3
  store double %7, double addrspace(1)* %8, align 8, !tbaa !30
  %9 = tail call spir_func double @_Z40sub_group_non_uniform_scan_inclusive_addd(double 0.000000e+00) #2
  %10 = getelementptr inbounds double, double addrspace(1)* %0, i64 4
  store double %9, double addrspace(1)* %10, align 8, !tbaa !30
  %11 = tail call spir_func double @_Z40sub_group_non_uniform_scan_inclusive_muld(double 0.000000e+00) #2
  %12 = getelementptr inbounds double, double addrspace(1)* %0, i64 5
  store double %11, double addrspace(1)* %12, align 8, !tbaa !30
  %13 = tail call spir_func double @_Z40sub_group_non_uniform_scan_inclusive_mind(double 0.000000e+00) #2
  %14 = getelementptr inbounds double, double addrspace(1)* %0, i64 6
  store double %13, double addrspace(1)* %14, align 8, !tbaa !30
  %15 = tail call spir_func double @_Z40sub_group_non_uniform_scan_inclusive_maxd(double 0.000000e+00) #2
  %16 = getelementptr inbounds double, double addrspace(1)* %0, i64 7
  store double %15, double addrspace(1)* %16, align 8, !tbaa !30
  %17 = tail call spir_func double @_Z40sub_group_non_uniform_scan_exclusive_addd(double 0.000000e+00) #2
  %18 = getelementptr inbounds double, double addrspace(1)* %0, i64 8
  store double %17, double addrspace(1)* %18, align 8, !tbaa !30
  %19 = tail call spir_func double @_Z40sub_group_non_uniform_scan_exclusive_muld(double 0.000000e+00) #2
  %20 = getelementptr inbounds double, double addrspace(1)* %0, i64 9
  store double %19, double addrspace(1)* %20, align 8, !tbaa !30
  %21 = tail call spir_func double @_Z40sub_group_non_uniform_scan_exclusive_mind(double 0.000000e+00) #2
  %22 = getelementptr inbounds double, double addrspace(1)* %0, i64 10
  store double %21, double addrspace(1)* %22, align 8, !tbaa !30
  %23 = tail call spir_func double @_Z40sub_group_non_uniform_scan_exclusive_maxd(double 0.000000e+00) #2
  %24 = getelementptr inbounds double, double addrspace(1)* %0, i64 11
  store double %23, double addrspace(1)* %24, align 8, !tbaa !30
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func double @_Z32sub_group_non_uniform_reduce_addd(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z32sub_group_non_uniform_reduce_muld(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z32sub_group_non_uniform_reduce_mind(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z32sub_group_non_uniform_reduce_maxd(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_inclusive_addd(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_inclusive_muld(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_inclusive_mind(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_inclusive_maxd(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_exclusive_addd(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_exclusive_muld(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_exclusive_mind(double) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func double @_Z40sub_group_non_uniform_scan_exclusive_maxd(double) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseChar(i8 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func signext i8 @_Z32sub_group_non_uniform_reduce_andc(i8 signext 0) #2
  store i8 %2, i8 addrspace(1)* %0, align 1, !tbaa !7
  %3 = tail call spir_func signext i8 @_Z31sub_group_non_uniform_reduce_orc(i8 signext 0) #2
  %4 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 1
  store i8 %3, i8 addrspace(1)* %4, align 1, !tbaa !7
  %5 = tail call spir_func signext i8 @_Z32sub_group_non_uniform_reduce_xorc(i8 signext 0) #2
  %6 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 2
  store i8 %5, i8 addrspace(1)* %6, align 1, !tbaa !7
  %7 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_andc(i8 signext 0) #2
  %8 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 3
  store i8 %7, i8 addrspace(1)* %8, align 1, !tbaa !7
  %9 = tail call spir_func signext i8 @_Z39sub_group_non_uniform_scan_inclusive_orc(i8 signext 0) #2
  %10 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 4
  store i8 %9, i8 addrspace(1)* %10, align 1, !tbaa !7
  %11 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_xorc(i8 signext 0) #2
  %12 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 5
  store i8 %11, i8 addrspace(1)* %12, align 1, !tbaa !7
  %13 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_andc(i8 signext 0) #2
  %14 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 6
  store i8 %13, i8 addrspace(1)* %14, align 1, !tbaa !7
  %15 = tail call spir_func signext i8 @_Z39sub_group_non_uniform_scan_exclusive_orc(i8 signext 0) #2
  %16 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 7
  store i8 %15, i8 addrspace(1)* %16, align 1, !tbaa !7
  %17 = tail call spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_xorc(i8 signext 0) #2
  %18 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 8
  store i8 %17, i8 addrspace(1)* %18, align 1, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z32sub_group_non_uniform_reduce_andc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z31sub_group_non_uniform_reduce_orc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z32sub_group_non_uniform_reduce_xorc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_andc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z39sub_group_non_uniform_scan_inclusive_orc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_inclusive_xorc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_andc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z39sub_group_non_uniform_scan_exclusive_orc(i8 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i8 @_Z40sub_group_non_uniform_scan_exclusive_xorc(i8 signext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[char]] %[[ScopeSubgroup]] Reduce %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[char]] %[[ScopeSubgroup]] InclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[char]] %[[ScopeSubgroup]] ExclusiveScan %[[char_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseUChar(i8 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !10 !kernel_arg_base_type !10 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_andh(i8 zeroext 0) #2
  store i8 %2, i8 addrspace(1)* %0, align 1, !tbaa !7
  %3 = tail call spir_func zeroext i8 @_Z31sub_group_non_uniform_reduce_orh(i8 zeroext 0) #2
  %4 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 1
  store i8 %3, i8 addrspace(1)* %4, align 1, !tbaa !7
  %5 = tail call spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_xorh(i8 zeroext 0) #2
  %6 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 2
  store i8 %5, i8 addrspace(1)* %6, align 1, !tbaa !7
  %7 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_andh(i8 zeroext 0) #2
  %8 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 3
  store i8 %7, i8 addrspace(1)* %8, align 1, !tbaa !7
  %9 = tail call spir_func zeroext i8 @_Z39sub_group_non_uniform_scan_inclusive_orh(i8 zeroext 0) #2
  %10 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 4
  store i8 %9, i8 addrspace(1)* %10, align 1, !tbaa !7
  %11 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_xorh(i8 zeroext 0) #2
  %12 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 5
  store i8 %11, i8 addrspace(1)* %12, align 1, !tbaa !7
  %13 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_andh(i8 zeroext 0) #2
  %14 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 6
  store i8 %13, i8 addrspace(1)* %14, align 1, !tbaa !7
  %15 = tail call spir_func zeroext i8 @_Z39sub_group_non_uniform_scan_exclusive_orh(i8 zeroext 0) #2
  %16 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 7
  store i8 %15, i8 addrspace(1)* %16, align 1, !tbaa !7
  %17 = tail call spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_xorh(i8 zeroext 0) #2
  %18 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 8
  store i8 %17, i8 addrspace(1)* %18, align 1, !tbaa !7
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_andh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z31sub_group_non_uniform_reduce_orh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z32sub_group_non_uniform_reduce_xorh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_andh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z39sub_group_non_uniform_scan_inclusive_orh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_inclusive_xorh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_andh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z39sub_group_non_uniform_scan_exclusive_orh(i8 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i8 @_Z40sub_group_non_uniform_scan_exclusive_xorh(i8 zeroext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseShort(i16 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !11 !kernel_arg_base_type !11 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func signext i16 @_Z32sub_group_non_uniform_reduce_ands(i16 signext 0) #2
  store i16 %2, i16 addrspace(1)* %0, align 2, !tbaa !12
  %3 = tail call spir_func signext i16 @_Z31sub_group_non_uniform_reduce_ors(i16 signext 0) #2
  %4 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 1
  store i16 %3, i16 addrspace(1)* %4, align 2, !tbaa !12
  %5 = tail call spir_func signext i16 @_Z32sub_group_non_uniform_reduce_xors(i16 signext 0) #2
  %6 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 2
  store i16 %5, i16 addrspace(1)* %6, align 2, !tbaa !12
  %7 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_ands(i16 signext 0) #2
  %8 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 3
  store i16 %7, i16 addrspace(1)* %8, align 2, !tbaa !12
  %9 = tail call spir_func signext i16 @_Z39sub_group_non_uniform_scan_inclusive_ors(i16 signext 0) #2
  %10 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 4
  store i16 %9, i16 addrspace(1)* %10, align 2, !tbaa !12
  %11 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_xors(i16 signext 0) #2
  %12 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 5
  store i16 %11, i16 addrspace(1)* %12, align 2, !tbaa !12
  %13 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_ands(i16 signext 0) #2
  %14 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 6
  store i16 %13, i16 addrspace(1)* %14, align 2, !tbaa !12
  %15 = tail call spir_func signext i16 @_Z39sub_group_non_uniform_scan_exclusive_ors(i16 signext 0) #2
  %16 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 7
  store i16 %15, i16 addrspace(1)* %16, align 2, !tbaa !12
  %17 = tail call spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_xors(i16 signext 0) #2
  %18 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 8
  store i16 %17, i16 addrspace(1)* %18, align 2, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z32sub_group_non_uniform_reduce_ands(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z31sub_group_non_uniform_reduce_ors(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z32sub_group_non_uniform_reduce_xors(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_ands(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z39sub_group_non_uniform_scan_inclusive_ors(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_inclusive_xors(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_ands(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z39sub_group_non_uniform_scan_exclusive_ors(i16 signext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func signext i16 @_Z40sub_group_non_uniform_scan_exclusive_xors(i16 signext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[short]] %[[ScopeSubgroup]] Reduce %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[short]] %[[ScopeSubgroup]] InclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[short]] %[[ScopeSubgroup]] ExclusiveScan %[[short_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseUShort(i16 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !14 !kernel_arg_base_type !14 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_andt(i16 zeroext 0) #2
  store i16 %2, i16 addrspace(1)* %0, align 2, !tbaa !12
  %3 = tail call spir_func zeroext i16 @_Z31sub_group_non_uniform_reduce_ort(i16 zeroext 0) #2
  %4 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 1
  store i16 %3, i16 addrspace(1)* %4, align 2, !tbaa !12
  %5 = tail call spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_xort(i16 zeroext 0) #2
  %6 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 2
  store i16 %5, i16 addrspace(1)* %6, align 2, !tbaa !12
  %7 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_andt(i16 zeroext 0) #2
  %8 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 3
  store i16 %7, i16 addrspace(1)* %8, align 2, !tbaa !12
  %9 = tail call spir_func zeroext i16 @_Z39sub_group_non_uniform_scan_inclusive_ort(i16 zeroext 0) #2
  %10 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 4
  store i16 %9, i16 addrspace(1)* %10, align 2, !tbaa !12
  %11 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_xort(i16 zeroext 0) #2
  %12 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 5
  store i16 %11, i16 addrspace(1)* %12, align 2, !tbaa !12
  %13 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_andt(i16 zeroext 0) #2
  %14 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 6
  store i16 %13, i16 addrspace(1)* %14, align 2, !tbaa !12
  %15 = tail call spir_func zeroext i16 @_Z39sub_group_non_uniform_scan_exclusive_ort(i16 zeroext 0) #2
  %16 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 7
  store i16 %15, i16 addrspace(1)* %16, align 2, !tbaa !12
  %17 = tail call spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_xort(i16 zeroext 0) #2
  %18 = getelementptr inbounds i16, i16 addrspace(1)* %0, i64 8
  store i16 %17, i16 addrspace(1)* %18, align 2, !tbaa !12
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_andt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z31sub_group_non_uniform_reduce_ort(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z32sub_group_non_uniform_reduce_xort(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_andt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z39sub_group_non_uniform_scan_inclusive_ort(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_inclusive_xort(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_andt(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z39sub_group_non_uniform_scan_exclusive_ort(i16 zeroext) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func zeroext i16 @_Z40sub_group_non_uniform_scan_exclusive_xort(i16 zeroext) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseInt(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !15 !kernel_arg_base_type !15 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_andi(i32 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !16
  %3 = tail call spir_func i32 @_Z31sub_group_non_uniform_reduce_ori(i32 0) #2
  %4 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1
  store i32 %3, i32 addrspace(1)* %4, align 4, !tbaa !16
  %5 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_xori(i32 0) #2
  %6 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 2
  store i32 %5, i32 addrspace(1)* %6, align 4, !tbaa !16
  %7 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_andi(i32 0) #2
  %8 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 3
  store i32 %7, i32 addrspace(1)* %8, align 4, !tbaa !16
  %9 = tail call spir_func i32 @_Z39sub_group_non_uniform_scan_inclusive_ori(i32 0) #2
  %10 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 4
  store i32 %9, i32 addrspace(1)* %10, align 4, !tbaa !16
  %11 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_xori(i32 0) #2
  %12 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 5
  store i32 %11, i32 addrspace(1)* %12, align 4, !tbaa !16
  %13 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_andi(i32 0) #2
  %14 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 6
  store i32 %13, i32 addrspace(1)* %14, align 4, !tbaa !16
  %15 = tail call spir_func i32 @_Z39sub_group_non_uniform_scan_exclusive_ori(i32 0) #2
  %16 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 7
  store i32 %15, i32 addrspace(1)* %16, align 4, !tbaa !16
  %17 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_xori(i32 0) #2
  %18 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 8
  store i32 %17, i32 addrspace(1)* %18, align 4, !tbaa !16
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_andi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_reduce_ori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_xori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_andi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z39sub_group_non_uniform_scan_inclusive_ori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_xori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_andi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z39sub_group_non_uniform_scan_exclusive_ori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_xori(i32) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[int]] %[[ScopeSubgroup]] Reduce %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[int]] %[[ScopeSubgroup]] InclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[int]] %[[ScopeSubgroup]] ExclusiveScan %[[int_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseUInt(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !18 !kernel_arg_base_type !18 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_andj(i32 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !16
  %3 = tail call spir_func i32 @_Z31sub_group_non_uniform_reduce_orj(i32 0) #2
  %4 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1
  store i32 %3, i32 addrspace(1)* %4, align 4, !tbaa !16
  %5 = tail call spir_func i32 @_Z32sub_group_non_uniform_reduce_xorj(i32 0) #2
  %6 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 2
  store i32 %5, i32 addrspace(1)* %6, align 4, !tbaa !16
  %7 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_andj(i32 0) #2
  %8 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 3
  store i32 %7, i32 addrspace(1)* %8, align 4, !tbaa !16
  %9 = tail call spir_func i32 @_Z39sub_group_non_uniform_scan_inclusive_orj(i32 0) #2
  %10 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 4
  store i32 %9, i32 addrspace(1)* %10, align 4, !tbaa !16
  %11 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_xorj(i32 0) #2
  %12 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 5
  store i32 %11, i32 addrspace(1)* %12, align 4, !tbaa !16
  %13 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_andj(i32 0) #2
  %14 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 6
  store i32 %13, i32 addrspace(1)* %14, align 4, !tbaa !16
  %15 = tail call spir_func i32 @_Z39sub_group_non_uniform_scan_exclusive_orj(i32 0) #2
  %16 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 7
  store i32 %15, i32 addrspace(1)* %16, align 4, !tbaa !16
  %17 = tail call spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_xorj(i32 0) #2
  %18 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 8
  store i32 %17, i32 addrspace(1)* %18, align 4, !tbaa !16
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_andj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z31sub_group_non_uniform_reduce_orj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z32sub_group_non_uniform_reduce_xorj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_andj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z39sub_group_non_uniform_scan_inclusive_orj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_inclusive_xorj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_andj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z39sub_group_non_uniform_scan_exclusive_orj(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_scan_exclusive_xorj(i32) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseLong(i64 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !19 !kernel_arg_base_type !19 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_andl(i64 0) #2
  store i64 %2, i64 addrspace(1)* %0, align 8, !tbaa !20
  %3 = tail call spir_func i64 @_Z31sub_group_non_uniform_reduce_orl(i64 0) #2
  %4 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 1
  store i64 %3, i64 addrspace(1)* %4, align 8, !tbaa !20
  %5 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_xorl(i64 0) #2
  %6 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 2
  store i64 %5, i64 addrspace(1)* %6, align 8, !tbaa !20
  %7 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_andl(i64 0) #2
  %8 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 3
  store i64 %7, i64 addrspace(1)* %8, align 8, !tbaa !20
  %9 = tail call spir_func i64 @_Z39sub_group_non_uniform_scan_inclusive_orl(i64 0) #2
  %10 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 4
  store i64 %9, i64 addrspace(1)* %10, align 8, !tbaa !20
  %11 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_xorl(i64 0) #2
  %12 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 5
  store i64 %11, i64 addrspace(1)* %12, align 8, !tbaa !20
  %13 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_andl(i64 0) #2
  %14 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 6
  store i64 %13, i64 addrspace(1)* %14, align 8, !tbaa !20
  %15 = tail call spir_func i64 @_Z39sub_group_non_uniform_scan_exclusive_orl(i64 0) #2
  %16 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 7
  store i64 %15, i64 addrspace(1)* %16, align 8, !tbaa !20
  %17 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_xorl(i64 0) #2
  %18 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 8
  store i64 %17, i64 addrspace(1)* %18, align 8, !tbaa !20
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_andl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z31sub_group_non_uniform_reduce_orl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_xorl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_andl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z39sub_group_non_uniform_scan_inclusive_orl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_xorl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_andl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z39sub_group_non_uniform_scan_exclusive_orl(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_xorl(i64) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[long]] %[[ScopeSubgroup]] Reduce %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[long]] %[[ScopeSubgroup]] InclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseAnd %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseOr  %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformBitwiseXor %[[long]] %[[ScopeSubgroup]] ExclusiveScan %[[long_0]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformBitwiseULong(i64 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !22 !kernel_arg_base_type !22 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_andm(i64 0) #2
  store i64 %2, i64 addrspace(1)* %0, align 8, !tbaa !20
  %3 = tail call spir_func i64 @_Z31sub_group_non_uniform_reduce_orm(i64 0) #2
  %4 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 1
  store i64 %3, i64 addrspace(1)* %4, align 8, !tbaa !20
  %5 = tail call spir_func i64 @_Z32sub_group_non_uniform_reduce_xorm(i64 0) #2
  %6 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 2
  store i64 %5, i64 addrspace(1)* %6, align 8, !tbaa !20
  %7 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_andm(i64 0) #2
  %8 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 3
  store i64 %7, i64 addrspace(1)* %8, align 8, !tbaa !20
  %9 = tail call spir_func i64 @_Z39sub_group_non_uniform_scan_inclusive_orm(i64 0) #2
  %10 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 4
  store i64 %9, i64 addrspace(1)* %10, align 8, !tbaa !20
  %11 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_xorm(i64 0) #2
  %12 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 5
  store i64 %11, i64 addrspace(1)* %12, align 8, !tbaa !20
  %13 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_andm(i64 0) #2
  %14 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 6
  store i64 %13, i64 addrspace(1)* %14, align 8, !tbaa !20
  %15 = tail call spir_func i64 @_Z39sub_group_non_uniform_scan_exclusive_orm(i64 0) #2
  %16 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 7
  store i64 %15, i64 addrspace(1)* %16, align 8, !tbaa !20
  %17 = tail call spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_xorm(i64 0) #2
  %18 = getelementptr inbounds i64, i64 addrspace(1)* %0, i64 8
  store i64 %17, i64 addrspace(1)* %18, align 8, !tbaa !20
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_andm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z31sub_group_non_uniform_reduce_orm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z32sub_group_non_uniform_reduce_xorm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_andm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z39sub_group_non_uniform_scan_inclusive_orm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_inclusive_xorm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_andm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z39sub_group_non_uniform_scan_exclusive_orm(i64) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i64 @_Z40sub_group_non_uniform_scan_exclusive_xorm(i64) local_unnamed_addr #1

; CHECK-SPIRV-LABEL: OpFunction
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalAnd %[[bool]] %[[ScopeSubgroup]] Reduce %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalOr  %[[bool]] %[[ScopeSubgroup]] Reduce %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalXor %[[bool]] %[[ScopeSubgroup]] Reduce %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalAnd %[[bool]] %[[ScopeSubgroup]] InclusiveScan %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalOr  %[[bool]] %[[ScopeSubgroup]] InclusiveScan %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalXor %[[bool]] %[[ScopeSubgroup]] InclusiveScan %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalAnd %[[bool]] %[[ScopeSubgroup]] ExclusiveScan %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalOr  %[[bool]] %[[ScopeSubgroup]] ExclusiveScan %[[false]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGroupNonUniformLogicalXor %[[bool]] %[[ScopeSubgroup]] ExclusiveScan %[[false]]
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: convergent nounwind
define dso_local spir_kernel void @testNonUniformLogical(i32 addrspace(1)* nocapture) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !15 !kernel_arg_base_type !15 !kernel_arg_type_qual !6 {
  %2 = tail call spir_func i32 @_Z40sub_group_non_uniform_reduce_logical_andi(i32 0) #2
  store i32 %2, i32 addrspace(1)* %0, align 4, !tbaa !16
  %3 = tail call spir_func i32 @_Z39sub_group_non_uniform_reduce_logical_ori(i32 0) #2
  %4 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1
  store i32 %3, i32 addrspace(1)* %4, align 4, !tbaa !16
  %5 = tail call spir_func i32 @_Z40sub_group_non_uniform_reduce_logical_xori(i32 0) #2
  %6 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 2
  store i32 %5, i32 addrspace(1)* %6, align 4, !tbaa !16
  %7 = tail call spir_func i32 @_Z48sub_group_non_uniform_scan_inclusive_logical_andi(i32 0) #2
  %8 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 3
  store i32 %7, i32 addrspace(1)* %8, align 4, !tbaa !16
  %9 = tail call spir_func i32 @_Z47sub_group_non_uniform_scan_inclusive_logical_ori(i32 0) #2
  %10 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 4
  store i32 %9, i32 addrspace(1)* %10, align 4, !tbaa !16
  %11 = tail call spir_func i32 @_Z48sub_group_non_uniform_scan_inclusive_logical_xori(i32 0) #2
  %12 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 5
  store i32 %11, i32 addrspace(1)* %12, align 4, !tbaa !16
  %13 = tail call spir_func i32 @_Z48sub_group_non_uniform_scan_exclusive_logical_andi(i32 0) #2
  %14 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 6
  store i32 %13, i32 addrspace(1)* %14, align 4, !tbaa !16
  %15 = tail call spir_func i32 @_Z47sub_group_non_uniform_scan_exclusive_logical_ori(i32 0) #2
  %16 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 7
  store i32 %15, i32 addrspace(1)* %16, align 4, !tbaa !16
  %17 = tail call spir_func i32 @_Z48sub_group_non_uniform_scan_exclusive_logical_xori(i32 0) #2
  %18 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 8
  store i32 %17, i32 addrspace(1)* %18, align 4, !tbaa !16
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_reduce_logical_andi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z39sub_group_non_uniform_reduce_logical_ori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z40sub_group_non_uniform_reduce_logical_xori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z48sub_group_non_uniform_scan_inclusive_logical_andi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z47sub_group_non_uniform_scan_inclusive_logical_ori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z48sub_group_non_uniform_scan_inclusive_logical_xori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z48sub_group_non_uniform_scan_exclusive_logical_andi(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z47sub_group_non_uniform_scan_exclusive_logical_ori(i32) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func i32 @_Z48sub_group_non_uniform_scan_exclusive_logical_xori(i32) local_unnamed_addr #1

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
!5 = !{!"char*"}
!6 = !{!""}
!7 = !{!8, !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = !{!"uchar*"}
!11 = !{!"short*"}
!12 = !{!13, !13, i64 0}
!13 = !{!"short", !8, i64 0}
!14 = !{!"ushort*"}
!15 = !{!"int*"}
!16 = !{!17, !17, i64 0}
!17 = !{!"int", !8, i64 0}
!18 = !{!"uint*"}
!19 = !{!"long*"}
!20 = !{!21, !21, i64 0}
!21 = !{!"long", !8, i64 0}
!22 = !{!"ulong*"}
!23 = !{!"float*"}
!24 = !{!25, !25, i64 0}
!25 = !{!"float", !8, i64 0}
!26 = !{!"half*"}
!27 = !{!28, !28, i64 0}
!28 = !{!"half", !8, i64 0}
!29 = !{!"double*"}
!30 = !{!31, !31, i64 0}
!31 = !{!"double", !8, i64 0}