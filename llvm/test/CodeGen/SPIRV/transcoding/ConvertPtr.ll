; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'ConvertPtr.cl'
source_filename = "ConvertPtr.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; kernel void testConvertPtrToU(global int *a, global unsigned long *res) {
;   res[0] = (unsigned long)&a[0];
; }

; CHECK-SPIRV: OpConvertPtrToU

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define dso_local spir_kernel void @testConvertPtrToU(i32 addrspace(1)* noundef %a, i64 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %0 = ptrtoint i32 addrspace(1)* %a to i32
  %1 = zext i32 %0 to i64
  store i64 %1, i64 addrspace(1)* %res, align 8, !tbaa !7
  ret void
}

; kernel void testConvertUToPtr(unsigned long a) {
;   global unsigned int *res = (global unsigned int *)a;
;   res[0] = 0;
; }

; CHECK-SPIRV: OpConvertUToPtr

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn writeonly
define dso_local spir_kernel void @testConvertUToPtr(i64 noundef %a) local_unnamed_addr #0 !kernel_arg_addr_space !11 !kernel_arg_access_qual !12 !kernel_arg_type !13 !kernel_arg_base_type !13 !kernel_arg_type_qual !14 {
entry:
  %conv = trunc i64 %a to i32
  %0 = inttoptr i32 %conv to i32 addrspace(1)*
  store i32 0, i32 addrspace(1)* %0, align 4, !tbaa !15
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn writeonly "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 1}
!4 = !{!"none", !"none"}
!5 = !{!"int*", !"ulong*"}
!6 = !{!"", !""}
!7 = !{!8, !8, i64 0}
!8 = !{!"long", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{i32 0}
!12 = !{!"none"}
!13 = !{!"ulong"}
!14 = !{!""}
!15 = !{!16, !16, i64 0}
!16 = !{!"int", !9, i64 0}
