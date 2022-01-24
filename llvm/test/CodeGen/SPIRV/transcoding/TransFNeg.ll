; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpFNegate
; CHECK-SPIRV: OpFNegate
; CHECK-SPIRV: OpFNegate
; CHECK-SPIRV: OpFNegate

; #pragma OPENCL EXTENSION cl_khr_fp64 : enable
; #pragma OPENCL EXTENSION cl_khr_fp16 : enable
;
; __kernel void foo(double a1, __global half *h, __global float *b0, __global double *b1, __global double8 *d) {
;    *h = -*h;
;    *b0 = -*b0;
;    *b1 = -a1;
;    *d = -*d;
; }

; ModuleID = 'TransFNeg.cl'
source_filename = "TransFNeg.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @foo(double noundef %a1, half addrspace(1)* noundef %h, float addrspace(1)* noundef %b0, double addrspace(1)* noundef %b1, <8 x double> addrspace(1)* noundef %d) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
entry:
  %a1.addr = alloca double, align 8
  %h.addr = alloca half addrspace(1)*, align 4
  %b0.addr = alloca float addrspace(1)*, align 4
  %b1.addr = alloca double addrspace(1)*, align 4
  %d.addr = alloca <8 x double> addrspace(1)*, align 4
  store double %a1, double* %a1.addr, align 8
  store half addrspace(1)* %h, half addrspace(1)** %h.addr, align 4
  store float addrspace(1)* %b0, float addrspace(1)** %b0.addr, align 4
  store double addrspace(1)* %b1, double addrspace(1)** %b1.addr, align 4
  store <8 x double> addrspace(1)* %d, <8 x double> addrspace(1)** %d.addr, align 4
  %0 = load half addrspace(1)*, half addrspace(1)** %h.addr, align 4
  %1 = load half, half addrspace(1)* %0, align 2
  %fneg = fneg half %1
  %2 = load half addrspace(1)*, half addrspace(1)** %h.addr, align 4
  store half %fneg, half addrspace(1)* %2, align 2
  %3 = load float addrspace(1)*, float addrspace(1)** %b0.addr, align 4
  %4 = load float, float addrspace(1)* %3, align 4
  %fneg1 = fneg float %4
  %5 = load float addrspace(1)*, float addrspace(1)** %b0.addr, align 4
  store float %fneg1, float addrspace(1)* %5, align 4
  %6 = load double, double* %a1.addr, align 8
  %fneg2 = fneg double %6
  %7 = load double addrspace(1)*, double addrspace(1)** %b1.addr, align 4
  store double %fneg2, double addrspace(1)* %7, align 8
  %8 = load <8 x double> addrspace(1)*, <8 x double> addrspace(1)** %d.addr, align 4
  %9 = load <8 x double>, <8 x double> addrspace(1)* %8, align 64
  %fneg3 = fneg <8 x double> %9
  %10 = load <8 x double> addrspace(1)*, <8 x double> addrspace(1)** %d.addr, align 4
  store <8 x double> %fneg3, <8 x double> addrspace(1)* %10, align 64
  ret void
}

attributes #0 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 0, i32 1, i32 1, i32 1, i32 1}
!4 = !{!"none", !"none", !"none", !"none", !"none"}
!5 = !{!"double", !"half*", !"float*", !"double*", !"double8*"}
!6 = !{!"double", !"half*", !"float*", !"double*", !"double __attribute__((ext_vector_type(8)))*"}
!7 = !{!"", !"", !"", !"", !""}
