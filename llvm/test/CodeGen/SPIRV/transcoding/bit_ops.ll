; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; kernel void testBitOperations(int a, int b, int c, global int *res) {
;   *res = (a & b) | (0x6F ^ c);
;   *res += popcount(b);
; }

; CHECK-SPIRV: OpBitwiseAnd
; CHECK-SPIRV: OpBitwiseXor
; CHECK-SPIRV: OpBitwiseOr
; CHECK-SPIRV: OpBitCount

; ModuleID = 'bit_ops.cl'
source_filename = "bit_ops.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: convergent mustprogress nofree norecurse nounwind willreturn writeonly
define dso_local spir_kernel void @testBitOperations(i32 noundef %a, i32 noundef %b, i32 noundef %c, i32 addrspace(1)* nocapture noundef writeonly %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %and = and i32 %b, %a
  %xor = xor i32 %c, 111
  %or = or i32 %xor, %and
  %call = call spir_func i32 @_Z8popcounti(i32 noundef %b) #2
  %add = add nsw i32 %call, %or
  store i32 %add, i32 addrspace(1)* %res, align 4, !tbaa !7
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func i32 @_Z8popcounti(i32 noundef) local_unnamed_addr #1

attributes #0 = { convergent mustprogress nofree norecurse nounwind willreturn writeonly "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent mustprogress nofree nounwind readnone willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind readnone willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 0, i32 0, i32 0, i32 1}
!4 = !{!"none", !"none", !"none", !"none"}
!5 = !{!"int", !"int", !"int", !"int*"}
!6 = !{!"", !"", !"", !""}
!7 = !{!8, !8, i64 0}
!8 = !{!"int", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
