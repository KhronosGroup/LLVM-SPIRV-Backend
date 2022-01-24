; RUN: llc -O0 %s -o - | FileCheck %s --check-prefixes=CHECK-SPIRV,CHECK-SPIRV1_4

; There are no blocks in SPIR-V. Therefore they are translated into regular
; functions. An LLVM module which uses blocks, also contains some auxiliary
; block-specific instructions, which are redundant in SPIR-V and should be
; removed

; kernel void block_kernel(__global int* res) {
;   typedef int (^block_t)(int);
;   constant block_t b1 = ^(int i) { return i + 1; };
;   *res = b1(5);
; }

; ModuleID = 'global_block.cl'
source_filename = "global_block.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV1_4: OpEntryPoint Kernel %[[#]] "block_kernel" [[#InterfaceId:]]
; CHECK-SPIRV1_4: OpName %[[#InterfaceId]] "__block_literal_global"
; CHECK-SPIRV: OpName %[[block_invoke:[0-9]+]] "_block_invoke"
; CHECK-SPIRV: %[[int:[0-9]+]] = OpTypeInt 32
; CHECK-SPIRV: %[[int8:[0-9]+]] = OpTypeInt 8
; CHECK-SPIRV: %[[five:[0-9]+]] = OpConstant %[[int]] 5
; CHECK-SPIRV: %[[int8Ptr:[0-9]+]] = OpTypePointer Generic %[[int8]]
; CHECK-SPIRV: %[[block_invoke_type:[0-9]+]] = OpTypeFunction %[[int]] %[[int8Ptr]] %[[int]]

; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionCall %[[int]] %[[block_invoke]] %{{[0-9]+}} %[[five]]

; CHECK-SPIRV: %[[block_invoke]] = OpFunction %[[int]] DontInline %[[block_invoke_type]]
; CHECK-SPIRV-NEXT: %{{[0-9]+}} = OpFunctionParameter %[[int8Ptr]]
; CHECK-SPIRV-NEXT: %{{[0-9]+}} = OpFunctionParameter %[[int]]

%struct.__opencl_block_literal_generic = type { i32, i32, i8 addrspace(4)* }

@block_kernel.b1 = internal addrspace(2) constant %struct.__opencl_block_literal_generic addrspace(4)* addrspacecast (%struct.__opencl_block_literal_generic addrspace(1)* bitcast ({ i32, i32, i8 addrspace(4)* } addrspace(1)* @__block_literal_global to %struct.__opencl_block_literal_generic addrspace(1)*) to %struct.__opencl_block_literal_generic addrspace(4)*), align 4
@__block_literal_global = internal addrspace(1) constant { i32, i32, i8 addrspace(4)* } { i32 12, i32 4, i8 addrspace(4)* addrspacecast (i8* bitcast (i32 (i8 addrspace(4)*, i32)* @_block_invoke to i8*) to i8 addrspace(4)*) }, align 4 #0

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @block_kernel(i32 addrspace(1)* noundef %res) #1 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %res.addr = alloca i32 addrspace(1)*, align 4
  store i32 addrspace(1)* %res, i32 addrspace(1)** %res.addr, align 4
  %call = call spir_func i32 @_block_invoke(i8 addrspace(4)* noundef addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32, i8 addrspace(4)* } addrspace(1)* @__block_literal_global to i8 addrspace(1)*) to i8 addrspace(4)*), i32 noundef 5) #3
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %res.addr, align 4
  store i32 %call, i32 addrspace(1)* %0, align 4
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func i32 @_block_invoke(i8 addrspace(4)* noundef %.block_descriptor, i32 noundef %i) #2 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %i.addr = alloca i32, align 4
  %block.addr = alloca <{ i32, i32, i8 addrspace(4)* }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32, i8 addrspace(4)* }> addrspace(4)*
  store i32 %i, i32* %i.addr, align 4
  store <{ i32, i32, i8 addrspace(4)* }> addrspace(4)* %block, <{ i32, i32, i8 addrspace(4)* }> addrspace(4)** %block.addr, align 4
  %0 = load i32, i32* %i.addr, align 4
  %add = add nsw i32 %0, 1
  ret i32 %add
}

attributes #0 = { "objc_arc_inert" }
attributes #1 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #2 = { convergent noinline nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { convergent }

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
