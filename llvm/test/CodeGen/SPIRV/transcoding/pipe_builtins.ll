; Pipe built-ins are mangled accordingly to SPIR2.0/C++ ABI.

; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'pipe_builtins.cl'
source_filename = "pipe_builtins.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknonw-unknown"

; CHECK-SPIRV-DAG: %[[ROPipeTy:[0-9]+]] = OpTypePipe ReadOnly
; CHECK-SPIRV-DAG: %[[WOPipeTy:[0-9]+]] = OpTypePipe WriteOnly

; #pragma OPENCL EXTENSION cl_khr_subgroups : enable

%opencl.reserve_id_t = type opaque
%opencl.pipe_wo_t = type opaque
%opencl.pipe_ro_t = type opaque

@test_pipe_workgroup_write_char.res_id = internal unnamed_addr addrspace(3) global %opencl.reserve_id_t* undef, align 8
@test_pipe_workgroup_read_char.res_id = internal unnamed_addr addrspace(3) global %opencl.reserve_id_t* undef, align 8

; __kernel void test_pipe_convenience_write_uint(__global uint *src, __write_only pipe uint out_pipe)
; {
  ; CHECK-SPIRV: OpFunction
  ; CHECK-SPIRV-NEXT:  OpFunctionParameter
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[WOPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpWritePipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   int gid = get_global_id(0);
;   write_pipe(out_pipe, &src[gid]);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_convenience_write_uint(i32 addrspace(1)* noundef %src, %opencl.pipe_wo_t addrspace(1)* %out_pipe) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %0 = shl i64 %call, 32
  %idxprom = ashr exact i64 %0, 32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %idxprom
  %1 = bitcast i32 addrspace(1)* %arrayidx to i8 addrspace(1)*
  %2 = addrspacecast i8 addrspace(1)* %1 to i8 addrspace(4)*
  %3 = tail call spir_func i32 @__write_pipe_2(%opencl.pipe_wo_t addrspace(1)* %out_pipe, i8 addrspace(4)* %2, i32 4, i32 4) #5
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func i64 @_Z13get_global_idj(i32 noundef) local_unnamed_addr #1

declare spir_func i32 @__write_pipe_2(%opencl.pipe_wo_t addrspace(1)*, i8 addrspace(4)*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_convenience_read_uint(__read_only pipe uint in_pipe, __global uint *dst)
; {
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[ROPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpReadPipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   int gid = get_global_id(0);
;   read_pipe(in_pipe, &dst[gid]);
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_convenience_read_uint(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i32 addrspace(1)* noundef %dst) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !7 !kernel_arg_type !8 !kernel_arg_base_type !8 !kernel_arg_type_qual !9 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %0 = shl i64 %call, 32
  %idxprom = ashr exact i64 %0, 32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %idxprom
  %1 = bitcast i32 addrspace(1)* %arrayidx to i8 addrspace(1)*
  %2 = addrspacecast i8 addrspace(1)* %1 to i8 addrspace(4)*
  %3 = tail call spir_func i32 @__read_pipe_2(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i8 addrspace(4)* %2, i32 4, i32 4) #5
  ret void
}

declare spir_func i32 @__read_pipe_2(%opencl.pipe_ro_t addrspace(1)*, i8 addrspace(4)*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_write(__global int *src, __write_only pipe int out_pipe)
; {
  ; CHECK-SPIRV-NEXT:  OpFunctionParameter
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[WOPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpReserveWritePipePackets %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: %{{[0-9]+}} = OpReservedWritePipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: OpCommitWritePipe %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   int gid = get_global_id(0);
;   reserve_id_t res_id;
;   res_id = reserve_write_pipe(out_pipe, 1);
;   if(is_valid_reserve_id(res_id))
;   {
;     write_pipe(out_pipe, res_id, 0, &src[gid]);
;     commit_write_pipe(out_pipe, res_id);
;   }
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_write(i32 addrspace(1)* noundef %src, %opencl.pipe_wo_t addrspace(1)* %out_pipe) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !10 !kernel_arg_base_type !10 !kernel_arg_type_qual !6 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %0 = tail call spir_func %opencl.reserve_id_t* @__reserve_write_pipe(%opencl.pipe_wo_t addrspace(1)* %out_pipe, i32 1, i32 4, i32 4) #5
  %call1 = tail call spir_func zeroext i1 @_Z19is_valid_reserve_id13ocl_reserveid(%opencl.reserve_id_t* %0) #6
  br i1 %call1, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %1 = shl i64 %call, 32
  %idxprom = ashr exact i64 %1, 32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %idxprom
  %2 = bitcast i32 addrspace(1)* %arrayidx to i8 addrspace(1)*
  %3 = addrspacecast i8 addrspace(1)* %2 to i8 addrspace(4)*
  %4 = tail call spir_func i32 @__write_pipe_4(%opencl.pipe_wo_t addrspace(1)* %out_pipe, %opencl.reserve_id_t* %0, i32 0, i8 addrspace(4)* %3, i32 4, i32 4) #5
  tail call spir_func void @__commit_write_pipe(%opencl.pipe_wo_t addrspace(1)* %out_pipe, %opencl.reserve_id_t* %0, i32 4, i32 4) #5
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

declare spir_func %opencl.reserve_id_t* @__reserve_write_pipe(%opencl.pipe_wo_t addrspace(1)*, i32, i32, i32) local_unnamed_addr

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z19is_valid_reserve_id13ocl_reserveid(%opencl.reserve_id_t*) local_unnamed_addr #2

declare spir_func i32 @__write_pipe_4(%opencl.pipe_wo_t addrspace(1)*, %opencl.reserve_id_t*, i32, i8 addrspace(4)*, i32, i32) local_unnamed_addr

declare spir_func void @__commit_write_pipe(%opencl.pipe_wo_t addrspace(1)*, %opencl.reserve_id_t*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_query_functions(__write_only pipe int out_pipe, __global int *num_packets, __global int *max_packets)
; {
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[WOPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpGetMaxPipePackets %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: %{{[0-9]+}} = OpGetNumPipePackets %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   *max_packets = get_pipe_max_packets(out_pipe);
;   *num_packets = get_pipe_num_packets(out_pipe);
; }

; Function Attrs: norecurse nounwind
define dso_local spir_kernel void @test_pipe_query_functions(%opencl.pipe_wo_t addrspace(1)* %out_pipe, i32 addrspace(1)* nocapture noundef writeonly %num_packets, i32 addrspace(1)* nocapture noundef writeonly %max_packets) local_unnamed_addr #3 !kernel_arg_addr_space !11 !kernel_arg_access_qual !12 !kernel_arg_type !13 !kernel_arg_base_type !13 !kernel_arg_type_qual !14 {
entry:
  %0 = tail call spir_func i32 @__get_pipe_max_packets_wo(%opencl.pipe_wo_t addrspace(1)* %out_pipe, i32 4, i32 4) #5
  store i32 %0, i32 addrspace(1)* %max_packets, align 4, !tbaa !15
  %1 = tail call spir_func i32 @__get_pipe_num_packets_wo(%opencl.pipe_wo_t addrspace(1)* %out_pipe, i32 4, i32 4) #5
  store i32 %1, i32 addrspace(1)* %num_packets, align 4, !tbaa !15
  ret void
}

declare spir_func i32 @__get_pipe_max_packets_wo(%opencl.pipe_wo_t addrspace(1)*, i32, i32) local_unnamed_addr

declare spir_func i32 @__get_pipe_num_packets_wo(%opencl.pipe_wo_t addrspace(1)*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_read(__read_only pipe int in_pipe, __global int *dst)
; {
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[ROPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpReserveReadPipePackets %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: %{{[0-9]+}} = OpReservedReadPipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: OpCommitReadPipe %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   int gid = get_global_id(0);
;   reserve_id_t res_id;
;   res_id = reserve_read_pipe(in_pipe, 1);
;   if(is_valid_reserve_id(res_id))
;   {
;     read_pipe(in_pipe, res_id, 0, &dst[gid]);
;     commit_read_pipe(in_pipe, res_id);
;   }
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_read(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i32 addrspace(1)* noundef %dst) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !7 !kernel_arg_type !19 !kernel_arg_base_type !19 !kernel_arg_type_qual !9 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %0 = tail call spir_func %opencl.reserve_id_t* @__reserve_read_pipe(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i32 1, i32 4, i32 4) #5
  %call1 = tail call spir_func zeroext i1 @_Z19is_valid_reserve_id13ocl_reserveid(%opencl.reserve_id_t* %0) #6
  br i1 %call1, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %1 = shl i64 %call, 32
  %idxprom = ashr exact i64 %1, 32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %idxprom
  %2 = bitcast i32 addrspace(1)* %arrayidx to i8 addrspace(1)*
  %3 = addrspacecast i8 addrspace(1)* %2 to i8 addrspace(4)*
  %4 = tail call spir_func i32 @__read_pipe_4(%opencl.pipe_ro_t addrspace(1)* %in_pipe, %opencl.reserve_id_t* %0, i32 0, i8 addrspace(4)* %3, i32 4, i32 4) #5
  tail call spir_func void @__commit_read_pipe(%opencl.pipe_ro_t addrspace(1)* %in_pipe, %opencl.reserve_id_t* %0, i32 4, i32 4) #5
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

declare spir_func %opencl.reserve_id_t* @__reserve_read_pipe(%opencl.pipe_ro_t addrspace(1)*, i32, i32, i32) local_unnamed_addr

declare spir_func i32 @__read_pipe_4(%opencl.pipe_ro_t addrspace(1)*, %opencl.reserve_id_t*, i32, i8 addrspace(4)*, i32, i32) local_unnamed_addr

declare spir_func void @__commit_read_pipe(%opencl.pipe_ro_t addrspace(1)*, %opencl.reserve_id_t*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_workgroup_write_char(__global char *src, __write_only pipe char out_pipe)
; {
  ; CHECK-SPIRV-NEXT:  OpFunctionParameter
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[WOPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpGroupReserveWritePipePackets %{{[0-9]+}} %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: OpGroupCommitWritePipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   int gid = get_global_id(0);
;   __local reserve_id_t res_id;

;   res_id = work_group_reserve_write_pipe(out_pipe, get_local_size(0));
;   if(is_valid_reserve_id(res_id))
;   {
;     write_pipe(out_pipe, res_id, get_local_id(0), &src[gid]);
;     work_group_commit_write_pipe(out_pipe, res_id);
;   }
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_workgroup_write_char(i8 addrspace(1)* noundef %src, %opencl.pipe_wo_t addrspace(1)* %out_pipe) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !20 !kernel_arg_base_type !20 !kernel_arg_type_qual !6 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %call1 = tail call spir_func i64 @_Z14get_local_sizej(i32 noundef 0) #4
  %0 = trunc i64 %call1 to i32
  %1 = tail call spir_func %opencl.reserve_id_t* @__work_group_reserve_write_pipe(%opencl.pipe_wo_t addrspace(1)* %out_pipe, i32 %0, i32 1, i32 1) #5
  store %opencl.reserve_id_t* %1, %opencl.reserve_id_t* addrspace(3)* @test_pipe_workgroup_write_char.res_id, align 8, !tbaa !21
  %call2 = tail call spir_func zeroext i1 @_Z19is_valid_reserve_id13ocl_reserveid(%opencl.reserve_id_t* %1) #6
  br i1 %call2, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %2 = load %opencl.reserve_id_t*, %opencl.reserve_id_t* addrspace(3)* @test_pipe_workgroup_write_char.res_id, align 8, !tbaa !21
  %call3 = tail call spir_func i64 @_Z12get_local_idj(i32 noundef 0) #4
  %3 = shl i64 %call, 32
  %idxprom = ashr exact i64 %3, 32
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 %idxprom
  %4 = addrspacecast i8 addrspace(1)* %arrayidx to i8 addrspace(4)*
  %5 = trunc i64 %call3 to i32
  %6 = tail call spir_func i32 @__write_pipe_4(%opencl.pipe_wo_t addrspace(1)* %out_pipe, %opencl.reserve_id_t* %2, i32 %5, i8 addrspace(4)* %4, i32 1, i32 1) #5
  %7 = load %opencl.reserve_id_t*, %opencl.reserve_id_t* addrspace(3)* @test_pipe_workgroup_write_char.res_id, align 8, !tbaa !21
  tail call spir_func void @__work_group_commit_write_pipe(%opencl.pipe_wo_t addrspace(1)* %out_pipe, %opencl.reserve_id_t* %7, i32 1, i32 1) #5
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func i64 @_Z14get_local_sizej(i32 noundef) local_unnamed_addr #1

declare spir_func %opencl.reserve_id_t* @__work_group_reserve_write_pipe(%opencl.pipe_wo_t addrspace(1)*, i32, i32, i32) local_unnamed_addr

; Function Attrs: convergent mustprogress nofree nounwind readnone willreturn
declare spir_func i64 @_Z12get_local_idj(i32 noundef) local_unnamed_addr #1

declare spir_func void @__work_group_commit_write_pipe(%opencl.pipe_wo_t addrspace(1)*, %opencl.reserve_id_t*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_workgroup_read_char(__read_only pipe char in_pipe, __global char *dst)
; {
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[ROPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpGroupReserveReadPipePackets %{{[0-9]+}} %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: OpGroupCommitReadPipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   int gid = get_global_id(0);
;   __local reserve_id_t res_id;

;   res_id = work_group_reserve_read_pipe(in_pipe, get_local_size(0));
;   if(is_valid_reserve_id(res_id))
;   {
;     read_pipe(in_pipe, res_id, get_local_id(0), &dst[gid]);
;     work_group_commit_read_pipe(in_pipe, res_id);
;   }
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_workgroup_read_char(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i8 addrspace(1)* noundef %dst) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !7 !kernel_arg_type !23 !kernel_arg_base_type !23 !kernel_arg_type_qual !9 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %call1 = tail call spir_func i64 @_Z14get_local_sizej(i32 noundef 0) #4
  %0 = trunc i64 %call1 to i32
  %1 = tail call spir_func %opencl.reserve_id_t* @__work_group_reserve_read_pipe(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i32 %0, i32 1, i32 1) #5
  store %opencl.reserve_id_t* %1, %opencl.reserve_id_t* addrspace(3)* @test_pipe_workgroup_read_char.res_id, align 8, !tbaa !21
  %call2 = tail call spir_func zeroext i1 @_Z19is_valid_reserve_id13ocl_reserveid(%opencl.reserve_id_t* %1) #6
  br i1 %call2, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %2 = load %opencl.reserve_id_t*, %opencl.reserve_id_t* addrspace(3)* @test_pipe_workgroup_read_char.res_id, align 8, !tbaa !21
  %call3 = tail call spir_func i64 @_Z12get_local_idj(i32 noundef 0) #4
  %3 = shl i64 %call, 32
  %idxprom = ashr exact i64 %3, 32
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %dst, i64 %idxprom
  %4 = addrspacecast i8 addrspace(1)* %arrayidx to i8 addrspace(4)*
  %5 = trunc i64 %call3 to i32
  %6 = tail call spir_func i32 @__read_pipe_4(%opencl.pipe_ro_t addrspace(1)* %in_pipe, %opencl.reserve_id_t* %2, i32 %5, i8 addrspace(4)* %4, i32 1, i32 1) #5
  %7 = load %opencl.reserve_id_t*, %opencl.reserve_id_t* addrspace(3)* @test_pipe_workgroup_read_char.res_id, align 8, !tbaa !21
  tail call spir_func void @__work_group_commit_read_pipe(%opencl.pipe_ro_t addrspace(1)* %in_pipe, %opencl.reserve_id_t* %7, i32 1, i32 1) #5
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

declare spir_func %opencl.reserve_id_t* @__work_group_reserve_read_pipe(%opencl.pipe_ro_t addrspace(1)*, i32, i32, i32) local_unnamed_addr

declare spir_func void @__work_group_commit_read_pipe(%opencl.pipe_ro_t addrspace(1)*, %opencl.reserve_id_t*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_subgroup_write_uint(__global uint *src, __write_only pipe uint out_pipe)
; {
  ; CHECK-SPIRV-NEXT:  OpFunctionParameter
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[WOPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpGroupReserveWritePipePackets %{{[0-9]+}} %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: OpGroupCommitWritePipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}

;   int gid = get_global_id(0);
;   reserve_id_t res_id;

;   res_id = sub_group_reserve_write_pipe(out_pipe, get_sub_group_size());
;   if(is_valid_reserve_id(res_id))
;   {
;     write_pipe(out_pipe, res_id, get_sub_group_local_id(), &src[gid]);
;     sub_group_commit_write_pipe(out_pipe, res_id);
;   }
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_subgroup_write_uint(i32 addrspace(1)* noundef %src, %opencl.pipe_wo_t addrspace(1)* %out_pipe) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %call1 = tail call spir_func i32 @_Z18get_sub_group_sizev() #6
  %0 = tail call spir_func %opencl.reserve_id_t* @__sub_group_reserve_write_pipe(%opencl.pipe_wo_t addrspace(1)* %out_pipe, i32 %call1, i32 4, i32 4) #5
  %call2 = tail call spir_func zeroext i1 @_Z19is_valid_reserve_id13ocl_reserveid(%opencl.reserve_id_t* %0) #6
  br i1 %call2, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %call3 = tail call spir_func i32 @_Z22get_sub_group_local_idv() #6
  %1 = shl i64 %call, 32
  %idxprom = ashr exact i64 %1, 32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %idxprom
  %2 = bitcast i32 addrspace(1)* %arrayidx to i8 addrspace(1)*
  %3 = addrspacecast i8 addrspace(1)* %2 to i8 addrspace(4)*
  %4 = tail call spir_func i32 @__write_pipe_4(%opencl.pipe_wo_t addrspace(1)* %out_pipe, %opencl.reserve_id_t* %0, i32 %call3, i8 addrspace(4)* %3, i32 4, i32 4) #5
  tail call spir_func void @__sub_group_commit_write_pipe(%opencl.pipe_wo_t addrspace(1)* %out_pipe, %opencl.reserve_id_t* %0, i32 4, i32 4) #5
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @_Z18get_sub_group_sizev() local_unnamed_addr #2

declare spir_func %opencl.reserve_id_t* @__sub_group_reserve_write_pipe(%opencl.pipe_wo_t addrspace(1)*, i32, i32, i32) local_unnamed_addr

; Function Attrs: convergent
declare spir_func i32 @_Z22get_sub_group_local_idv() local_unnamed_addr #2

declare spir_func void @__sub_group_commit_write_pipe(%opencl.pipe_wo_t addrspace(1)*, %opencl.reserve_id_t*, i32, i32) local_unnamed_addr

; CHECK-SPIRV: OpFunction
; __kernel void test_pipe_subgroup_read_uint(__read_only pipe uint in_pipe, __global uint *dst)
; {
  ; CHECK-SPIRV-NEXT:  %[[PipeArgID:[0-9]+]] = OpFunctionParameter %[[ROPipeTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpGroupReserveReadPipePackets %{{[0-9]+}} %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV: OpGroupCommitReadPipe %{{[0-9]+}} %[[PipeArgID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ; CHECK-SPIRV-LABEL: OpFunctionEnd

;   int gid = get_global_id(0);
;   reserve_id_t res_id;

;   res_id = sub_group_reserve_read_pipe(in_pipe, get_sub_group_size());
;   if(is_valid_reserve_id(res_id))
;   {
;     read_pipe(in_pipe, res_id, get_sub_group_local_id(), &dst[gid]);
;     sub_group_commit_read_pipe(in_pipe, res_id);
;   }
; }

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test_pipe_subgroup_read_uint(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i32 addrspace(1)* noundef %dst) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !7 !kernel_arg_type !8 !kernel_arg_base_type !8 !kernel_arg_type_qual !9 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0) #4
  %call1 = tail call spir_func i32 @_Z18get_sub_group_sizev() #6
  %0 = tail call spir_func %opencl.reserve_id_t* @__sub_group_reserve_read_pipe(%opencl.pipe_ro_t addrspace(1)* %in_pipe, i32 %call1, i32 4, i32 4) #5
  %call2 = tail call spir_func zeroext i1 @_Z19is_valid_reserve_id13ocl_reserveid(%opencl.reserve_id_t* %0) #6
  br i1 %call2, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %call3 = tail call spir_func i32 @_Z22get_sub_group_local_idv() #6
  %1 = shl i64 %call, 32
  %idxprom = ashr exact i64 %1, 32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %idxprom
  %2 = bitcast i32 addrspace(1)* %arrayidx to i8 addrspace(1)*
  %3 = addrspacecast i8 addrspace(1)* %2 to i8 addrspace(4)*
  %4 = tail call spir_func i32 @__read_pipe_4(%opencl.pipe_ro_t addrspace(1)* %in_pipe, %opencl.reserve_id_t* %0, i32 %call3, i8 addrspace(4)* %3, i32 4, i32 4) #5
  tail call spir_func void @__sub_group_commit_read_pipe(%opencl.pipe_ro_t addrspace(1)* %in_pipe, %opencl.reserve_id_t* %0, i32 4, i32 4) #5
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

declare spir_func %opencl.reserve_id_t* @__sub_group_reserve_read_pipe(%opencl.pipe_ro_t addrspace(1)*, i32, i32, i32) local_unnamed_addr

declare spir_func void @__sub_group_commit_read_pipe(%opencl.pipe_ro_t addrspace(1)*, %opencl.reserve_id_t*, i32, i32) local_unnamed_addr

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent mustprogress nofree nounwind readnone willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #4 = { convergent nounwind readnone willreturn }
attributes #5 = { nounwind }
attributes #6 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 1}
!4 = !{!"none", !"write_only"}
!5 = !{!"uint*", !"uint"}
!6 = !{!"", !"pipe"}
!7 = !{!"read_only", !"none"}
!8 = !{!"uint", !"uint*"}
!9 = !{!"pipe", !""}
!10 = !{!"int*", !"int"}
!11 = !{i32 1, i32 1, i32 1}
!12 = !{!"write_only", !"none", !"none"}
!13 = !{!"int", !"int*", !"int*"}
!14 = !{!"pipe", !"", !""}
!15 = !{!16, !16, i64 0}
!16 = !{!"int", !17, i64 0}
!17 = !{!"omnipotent char", !18, i64 0}
!18 = !{!"Simple C/C++ TBAA"}
!19 = !{!"int", !"int*"}
!20 = !{!"char*", !"char"}
!21 = !{!22, !22, i64 0}
!22 = !{!"reserve_id_t", !17, i64 0}
!23 = !{!"char", !"char*"}
