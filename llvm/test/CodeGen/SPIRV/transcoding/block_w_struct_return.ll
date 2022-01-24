; RUN: llc -O0 %s -o - | FileCheck %s --check-prefixes=CHECK-SPIRV,CHECK-SPIRV1_1,CHECK-SPIRV1_4

; kernel void block_ret_struct(__global int* res)
; {
;   struct A {
;       int a;
;   };
;   struct A (^kernelBlock)(struct A) = ^struct A(struct A a)
;   {
;     a.a = 6;
;     return a;
;   };
;   size_t tid = get_global_id(0);
;   res[tid] = -1;
;   struct A aa;
;   aa.a = 5;
;   res[tid] = kernelBlock(aa).a - 6;
; }

; ModuleID = 'block_w_struct_return.cl'
source_filename = "block_w_struct_return.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV1_4: OpEntryPoint Kernel %[[#]] "block_ret_struct" [[#InterdaceId1:]] [[#InterdaceId2:]]
; CHECK-SPIRV1_4: OpName %[[#InterdaceId1]] "__block_literal_global"
; CHECK-SPIRV1_4: OpName %[[#InterdaceId2]] "__spirv_BuiltInGlobalInvocationId"

; CHECK-SPIRV1_1: OpEntryPoint Kernel %[[#]] "block_ret_struct" [[#InterdaceId1:]]
; CHECK-SPIRV1_1: OpName %[[#InterdaceId1]] "__spirv_BuiltInGlobalInvocationId"

; CHECK-SPIRV: OpName %[[BlockInv:[0-9]+]] "__block_ret_struct_block_invoke"

; CHECK-SPIRV: %[[IntTy:[0-9]+]] = OpTypeInt 32
; CHECK-SPIRV: %[[Int8Ty:[0-9]+]] = OpTypeInt 8
; CHECK-SPIRV: %[[Int8Ptr:[0-9]+]] = OpTypePointer Generic %[[Int8Ty]]
; CHECK-SPIRV: %[[StructTy:[0-9]+]] = OpTypeStruct %[[IntTy]]
; CHECK-SPIRV: %[[StructPtrTy:[0-9]+]] = OpTypePointer Function %[[StructTy]]

; CHECK-SPIRV: %[[StructArg:[0-9]+]] = OpVariable %[[StructPtrTy]] Function
; CHECK-SPIRV: %[[StructRet:[0-9]+]] = OpVariable %[[StructPtrTy]] Function
; CHECK-SPIRV: %[[BlockLit:[0-9]+]] = OpPtrCastToGeneric %[[Int8Ptr]] %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionCall %{{[0-9]+}} %[[BlockInv]] %[[StructRet]] %[[BlockLit]] %[[StructArg]]

%struct.__opencl_block_literal_generic = type { i32, i32, i8 addrspace(4)* }
%struct.A = type { i32 }

@__block_literal_global = internal addrspace(1) constant { i32, i32, i8 addrspace(4)* } { i32 12, i32 4, i8 addrspace(4)* addrspacecast (i8* bitcast (void (%struct.A*, i8 addrspace(4)*, %struct.A*)* @__block_ret_struct_block_invoke to i8*) to i8 addrspace(4)*) }, align 4 #0

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @block_ret_struct(i32 addrspace(1)* noundef %res) #1 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %res.addr = alloca i32 addrspace(1)*, align 4
  %kernelBlock = alloca %struct.__opencl_block_literal_generic addrspace(4)*, align 4
  %tid = alloca i32, align 4
  %aa = alloca %struct.A, align 4
  %tmp = alloca %struct.A, align 4
  store i32 addrspace(1)* %res, i32 addrspace(1)** %res.addr, align 4, !tbaa !7
  %0 = bitcast %struct.__opencl_block_literal_generic addrspace(4)** %kernelBlock to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %0) #6
  store %struct.__opencl_block_literal_generic addrspace(4)* addrspacecast (%struct.__opencl_block_literal_generic addrspace(1)* bitcast ({ i32, i32, i8 addrspace(4)* } addrspace(1)* @__block_literal_global to %struct.__opencl_block_literal_generic addrspace(1)*) to %struct.__opencl_block_literal_generic addrspace(4)*), %struct.__opencl_block_literal_generic addrspace(4)** %kernelBlock, align 4, !tbaa !11
  %1 = bitcast i32* %tid to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %1) #6
  %call = call spir_func i32 @_Z13get_global_idj(i32 noundef 0) #7
  store i32 %call, i32* %tid, align 4, !tbaa !12
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %res.addr, align 4, !tbaa !7
  %3 = load i32, i32* %tid, align 4, !tbaa !12
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %2, i32 %3
  store i32 -1, i32 addrspace(1)* %arrayidx, align 4, !tbaa !12
  %4 = bitcast %struct.A* %aa to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %4) #6
  %a = getelementptr inbounds %struct.A, %struct.A* %aa, i32 0, i32 0
  store i32 5, i32* %a, align 4, !tbaa !14
  call spir_func void @__block_ret_struct_block_invoke(%struct.A* sret(%struct.A) align 4 %tmp, i8 addrspace(4)* noundef addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32, i8 addrspace(4)* } addrspace(1)* @__block_literal_global to i8 addrspace(1)*) to i8 addrspace(4)*), %struct.A* noundef byval(%struct.A) align 4 %aa) #8
  %a1 = getelementptr inbounds %struct.A, %struct.A* %tmp, i32 0, i32 0
  %5 = load i32, i32* %a1, align 4, !tbaa !14
  %sub = sub nsw i32 %5, 6
  %6 = load i32 addrspace(1)*, i32 addrspace(1)** %res.addr, align 4, !tbaa !7
  %7 = load i32, i32* %tid, align 4, !tbaa !12
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %6, i32 %7
  store i32 %sub, i32 addrspace(1)* %arrayidx2, align 4, !tbaa !12
  %8 = bitcast %struct.A* %aa to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %8) #6
  %9 = bitcast i32* %tid to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %9) #6
  %10 = bitcast %struct.__opencl_block_literal_generic addrspace(4)** %kernelBlock to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %10) #6
  ret void
}

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #2

; Function Attrs: convergent nounwind
define internal spir_func void @__block_ret_struct_block_invoke(%struct.A* noalias sret(%struct.A) align 4 %agg.result, i8 addrspace(4)* noundef %.block_descriptor, %struct.A* noundef byval(%struct.A) align 4 %a) #3 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32, i8 addrspace(4)* }> addrspace(4)*
  %a1 = getelementptr inbounds %struct.A, %struct.A* %a, i32 0, i32 0
  store i32 6, i32* %a1, align 4, !tbaa !14
  %0 = bitcast %struct.A* %agg.result to i8*
  %1 = bitcast %struct.A* %a to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 4 %0, i8* align 4 %1, i32 4, i1 false), !tbaa.struct !16
  ret void
}

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i32(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i32, i1 immarg) #4

; Function Attrs: convergent nounwind readnone willreturn
declare spir_func i32 @_Z13get_global_idj(i32 noundef) #5

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #2

attributes #0 = { "objc_arc_inert" }
attributes #1 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #2 = { argmemonly nofree nosync nounwind willreturn }
attributes #3 = { convergent nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #4 = { argmemonly nofree nounwind willreturn }
attributes #5 = { convergent nounwind readnone willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #6 = { nounwind }
attributes #7 = { convergent nounwind readnone willreturn }
attributes #8 = { convergent }

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
!8 = !{!"any pointer", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!9, !9, i64 0}
!12 = !{!13, !13, i64 0}
!13 = !{!"int", !9, i64 0}
!14 = !{!15, !13, i64 0}
!15 = !{!"A", !13, i64 0}
!16 = !{i64 0, i64 4, !12}
