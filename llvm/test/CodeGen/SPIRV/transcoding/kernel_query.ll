; Source
; typedef struct {int a;} ndrange_t;
;
; kernel void device_side_enqueue() {
;   ndrange_t ndrange;
;
;   get_kernel_work_group_size(^(){});
;   get_kernel_preferred_work_group_size_multiple(^(){});
;
; #pragma OPENCL EXTENSION cl_khr_subgroups : enable
;   get_kernel_max_sub_group_size_for_ndrange(ndrange, ^(){});
;   get_kernel_sub_group_count_for_ndrange(ndrange, ^(){});
; }
;
; Compilation command:
; clang -cc1 -triple spir-unknown-unknown -O0 -cl-std=CL2.0 -emit-llvm kernel_query.cl

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

source_filename = "kernel_query.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%struct.ndrange_t = type { i32 }

; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer1:[0-9]+]] "__device_side_enqueue_block_invoke_kernel"
; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer2:[0-9]+]] "__device_side_enqueue_block_invoke_2_kernel"
; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer3:[0-9]+]] "__device_side_enqueue_block_invoke_3_kernel"
; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer4:[0-9]+]] "__device_side_enqueue_block_invoke_4_kernel"
; CHECK-SPIRV: OpName %[[BlockGlb1:[0-9]+]] "__block_literal_global"
; CHECK-SPIRV: OpName %[[BlockGlb2:[0-9]+]] "__block_literal_global.1"
; CHECK-SPIRV: OpName %[[BlockGlb3:[0-9]+]] "__block_literal_global.2"
; CHECK-SPIRV: OpName %[[BlockGlb4:[0-9]+]] "__block_literal_global.3"

%1 = type <{ i32, i32 }>

@__block_literal_global = internal addrspace(1) constant { i32, i32 } { i32 8, i32 4 }, align 4
@__block_literal_global.1 = internal addrspace(1) constant { i32, i32 } { i32 8, i32 4 }, align 4
@__block_literal_global.2 = internal addrspace(1) constant { i32, i32 } { i32 8, i32 4 }, align 4
@__block_literal_global.3 = internal addrspace(1) constant { i32, i32 } { i32 8, i32 4 }, align 4

; CHECK-SPIRV: %[[Int32Ty:[0-9]+]] = OpTypeInt 32
; CHECK-SPIRV: %[[Int8Ty:[0-9]+]] = OpTypeInt 8
; CHECK-SPIRV: %[[ConstInt8:[0-9]+]] = OpConstant %[[Int32Ty]] 8
; CHECK-SPIRV: %[[VoidTy:[0-9]+]] = OpTypeVoid
; CHECK-SPIRV: %[[NDRangeTy:[0-9]+]] = OpTypeStruct %[[Int32Ty]] {{$}}
; CHECK-SPIRV: %[[NDRangePtrTy:[0-9]+]] = OpTypePointer Function %[[NDRangeTy]]
; CHECK-SPIRV: %[[Int8PtrGenTy:[0-9]+]] = OpTypePointer Generic %[[Int8Ty]]
; CHECK-SPIRV: %[[BlockKerTy:[0-9]+]] = OpTypeFunction %[[VoidTy]] %[[Int8PtrGenTy]]

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @device_side_enqueue() #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !2 !kernel_arg_type !2 !kernel_arg_base_type !2 !kernel_arg_type_qual !2 {
entry:

; CHECK-SPIRV: %[[NDRange:[0-9]+]] = OpVariable %[[NDRangePtrTy]]

  %ndrange = alloca %struct.ndrange_t, align 4

; CHECK-SPIRV: %[[BlockLit1Tmp:[0-9]+]] = OpBitcast %{{[0-9]+}} %[[BlockGlb1]]
; CHECK-SPIRV: %[[BlockLit1:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]] %[[BlockLit1Tmp]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGetKernelWorkGroupSize %[[Int32Ty]] %[[BlockKer1]] %[[BlockLit1]] %[[ConstInt8]] %[[ConstInt8]]

  %0 = call i32 @__get_kernel_work_group_size_impl(i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32 } addrspace(1)* @__block_literal_global to i8 addrspace(1)*) to i8 addrspace(4)*))

; CHECK-SPIRV: %[[BlockLit2Tmp:[0-9]+]] = OpBitcast %{{[0-9]+}} %[[BlockGlb2]]
; CHECK-SPIRV: %[[BlockLit2:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]] %[[BlockLit2Tmp]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGetKernelPreferredWorkGroupSizeMultiple %[[Int32Ty]] %[[BlockKer2]] %[[BlockLit2]] %[[ConstInt8]] %[[ConstInt8]]

  %1 = call i32 @__get_kernel_preferred_work_group_size_multiple_impl(i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_2_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32 } addrspace(1)* @__block_literal_global.1 to i8 addrspace(1)*) to i8 addrspace(4)*))

; CHECK-SPIRV: %[[BlockLit3Tmp:[0-9]+]] = OpBitcast %{{[0-9]+}} %[[BlockGlb3]]
; CHECK-SPIRV: %[[BlockLit3:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]] %[[BlockLit3Tmp]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGetKernelNDrangeMaxSubGroupSize %[[Int32Ty]] %[[NDRange]] %[[BlockKer3]] %[[BlockLit3]] %[[ConstInt8]] %[[ConstInt8]]

  %2 = call i32 @__get_kernel_max_sub_group_size_for_ndrange_impl(%struct.ndrange_t* %ndrange, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_3_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32 } addrspace(1)* @__block_literal_global.2 to i8 addrspace(1)*) to i8 addrspace(4)*))

; CHECK-SPIRV: %[[BlockLit4Tmp:[0-9]+]] = OpBitcast %{{[0-9]+}} %[[BlockGlb4]]
; CHECK-SPIRV: %[[BlockLit4:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]] %[[BlockLit4Tmp]]
; CHECK-SPIRV: %{{[0-9]+}} = OpGetKernelNDrangeSubGroupCount %[[Int32Ty]] %[[NDRange]] %[[BlockKer4]] %[[BlockLit4]] %[[ConstInt8]] %[[ConstInt8]]

  %3 = call i32 @__get_kernel_sub_group_count_for_ndrange_impl(%struct.ndrange_t* %ndrange, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_4_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32 } addrspace(1)* @__block_literal_global.3 to i8 addrspace(1)*) to i8 addrspace(4)*))
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke(i8 addrspace(4)* %.block_descriptor) #1 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %block.addr = alloca <{ i32, i32 }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32 }> addrspace(4)*
  store <{ i32, i32 }> addrspace(4)* %block, <{ i32, i32 }> addrspace(4)** %block.addr, align 4
  ret void
}

; Function Attrs: nounwind
define internal spir_kernel void @__device_side_enqueue_block_invoke_kernel(i8 addrspace(4)*) #2 {
entry:
  call void @__device_side_enqueue_block_invoke(i8 addrspace(4)* %0)
  ret void
}

declare i32 @__get_kernel_work_group_size_impl(i8 addrspace(4)*, i8 addrspace(4)*)

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke_2(i8 addrspace(4)* %.block_descriptor) #1 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %block.addr = alloca <{ i32, i32 }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32 }> addrspace(4)*
  store <{ i32, i32 }> addrspace(4)* %block, <{ i32, i32 }> addrspace(4)** %block.addr, align 4
  ret void
}

; Function Attrs: nounwind
define internal spir_kernel void @__device_side_enqueue_block_invoke_2_kernel(i8 addrspace(4)*) #2 {
entry:
  call void @__device_side_enqueue_block_invoke_2(i8 addrspace(4)* %0)
  ret void
}

declare i32 @__get_kernel_preferred_work_group_size_multiple_impl(i8 addrspace(4)*, i8 addrspace(4)*)

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke_3(i8 addrspace(4)* %.block_descriptor) #1 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %block.addr = alloca <{ i32, i32 }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32 }> addrspace(4)*
  store <{ i32, i32 }> addrspace(4)* %block, <{ i32, i32 }> addrspace(4)** %block.addr, align 4
  ret void
}

; Function Attrs: nounwind
define internal spir_kernel void @__device_side_enqueue_block_invoke_3_kernel(i8 addrspace(4)*) #2 {
entry:
  call void @__device_side_enqueue_block_invoke_3(i8 addrspace(4)* %0)
  ret void
}

declare i32 @__get_kernel_max_sub_group_size_for_ndrange_impl(%struct.ndrange_t*, i8 addrspace(4)*, i8 addrspace(4)*)

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke_4(i8 addrspace(4)* %.block_descriptor) #1 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %block.addr = alloca <{ i32, i32 }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32 }> addrspace(4)*
  store <{ i32, i32 }> addrspace(4)* %block, <{ i32, i32 }> addrspace(4)** %block.addr, align 4
  ret void
}

; Function Attrs: nounwind
define internal spir_kernel void @__device_side_enqueue_block_invoke_4_kernel(i8 addrspace(4)*) #2 {
entry:
  call void @__device_side_enqueue_block_invoke_4(i8 addrspace(4)* %0)
  ret void
}

declare i32 @__get_kernel_sub_group_count_for_ndrange_impl(%struct.ndrange_t*, i8 addrspace(4)*, i8 addrspace(4)*)

; CHECK-SPIRV-DAG: %[[BlockKer1]] = OpFunction %[[VoidTy]] None %[[BlockKerTy]]
; CHECK-SPIRV-DAG: %[[BlockKer2]] = OpFunction %[[VoidTy]] None %[[BlockKerTy]]
; CHECK-SPIRV-DAG: %[[BlockKer3]] = OpFunction %[[VoidTy]] None %[[BlockKerTy]]
; CHECK-SPIRV-DAG: %[[BlockKer4]] = OpFunction %[[VoidTy]] None %[[BlockKerTy]]

attributes #0 = { convergent noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { convergent noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }
attributes #3 = { argmemonly nounwind }

!llvm.module.flags = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{}
!3 = !{!"clang version 7.0.0"}
