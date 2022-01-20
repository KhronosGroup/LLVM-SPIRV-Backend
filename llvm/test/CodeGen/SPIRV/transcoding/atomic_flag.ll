; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; kernel void testAtomicFlag(global int *res) {
;   atomic_flag f;

;   *res = atomic_flag_test_and_set(&f);
;   *res += atomic_flag_test_and_set_explicit(&f, memory_order_seq_cst);
;   *res += atomic_flag_test_and_set_explicit(&f, memory_order_seq_cst, memory_scope_work_group);

;   atomic_flag_clear(&f);
;   atomic_flag_clear_explicit(&f, memory_order_seq_cst);
;   atomic_flag_clear_explicit(&f, memory_order_seq_cst, memory_scope_work_group);
; }

; ModuleID = 'atomic_flag.cl'
source_filename = "atomic_flag.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpAtomicFlagTestAndSet
; CHECK-SPIRV: OpAtomicFlagTestAndSet
; CHECK-SPIRV: OpAtomicFlagTestAndSet
; CHECK-SPIRV: OpAtomicFlagClear
; CHECK-SPIRV: OpAtomicFlagClear
; CHECK-SPIRV: OpAtomicFlagClear

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @testAtomicFlag(i32 addrspace(1)* nocapture noundef %res) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %f = alloca i32, align 4
  %0 = bitcast i32* %f to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %0) #3
  %f.ascast = addrspacecast i32* %f to i32 addrspace(4)*
  %call = call spir_func zeroext i1 @_Z24atomic_flag_test_and_setPU3AS4VU7_Atomici(i32 addrspace(4)* noundef %f.ascast) #4
  %conv = zext i1 %call to i32
  store i32 %conv, i32 addrspace(1)* %res, align 4, !tbaa !7
  %call2 = call spir_func zeroext i1 @_Z33atomic_flag_test_and_set_explicitPU3AS4VU7_Atomici12memory_order(i32 addrspace(4)* noundef %f.ascast, i32 noundef 5) #4
  %conv3 = zext i1 %call2 to i32
  %1 = load i32, i32 addrspace(1)* %res, align 4, !tbaa !7
  %add = add nsw i32 %1, %conv3
  store i32 %add, i32 addrspace(1)* %res, align 4, !tbaa !7
  %call5 = call spir_func zeroext i1 @_Z33atomic_flag_test_and_set_explicitPU3AS4VU7_Atomici12memory_order12memory_scope(i32 addrspace(4)* noundef %f.ascast, i32 noundef 5, i32 noundef 1) #4
  %conv6 = zext i1 %call5 to i32
  %2 = load i32, i32 addrspace(1)* %res, align 4, !tbaa !7
  %add7 = add nsw i32 %2, %conv6
  store i32 %add7, i32 addrspace(1)* %res, align 4, !tbaa !7
  call spir_func void @_Z17atomic_flag_clearPU3AS4VU7_Atomici(i32 addrspace(4)* noundef %f.ascast) #4
  call spir_func void @_Z26atomic_flag_clear_explicitPU3AS4VU7_Atomici12memory_order(i32 addrspace(4)* noundef %f.ascast, i32 noundef 5) #4
  call spir_func void @_Z26atomic_flag_clear_explicitPU3AS4VU7_Atomici12memory_order12memory_scope(i32 addrspace(4)* noundef %f.ascast, i32 noundef 5, i32 noundef 1) #4
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %0) #3
  ret void
}

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z24atomic_flag_test_and_setPU3AS4VU7_Atomici(i32 addrspace(4)* noundef) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z33atomic_flag_test_and_set_explicitPU3AS4VU7_Atomici12memory_order(i32 addrspace(4)* noundef, i32 noundef) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z33atomic_flag_test_and_set_explicitPU3AS4VU7_Atomici12memory_order12memory_scope(i32 addrspace(4)* noundef, i32 noundef, i32 noundef) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func void @_Z17atomic_flag_clearPU3AS4VU7_Atomici(i32 addrspace(4)* noundef) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func void @_Z26atomic_flag_clear_explicitPU3AS4VU7_Atomici12memory_order(i32 addrspace(4)* noundef, i32 noundef) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func void @_Z26atomic_flag_clear_explicitPU3AS4VU7_Atomici12memory_order12memory_scope(i32 addrspace(4)* noundef, i32 noundef, i32 noundef) local_unnamed_addr #2

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { argmemonly mustprogress nofree nosync nounwind willreturn }
attributes #2 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { nounwind }
attributes #4 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1}
!4 = !{!"none"}
!5 = !{!"int*"}
!6 = !{!""}
!7 = !{!8, !8, i64 0}
!8 = !{!"int", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
