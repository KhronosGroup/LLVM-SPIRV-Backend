; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpName %[[MYPIPE_ID:[0-9]+]] "mygpipe"

; CHECK-SPIRV: %[[PIPE_STORAGE_ID:[0-9]+]] = OpTypePipeStorage
; CHECK-SPIRV-NEXT: %[[PIPE_STORAGE_ID_2:[0-9]+]] = OpTypePipeStorage
; CHECK-SPIRV: %[[CL_PIPE_STORAGE_ID:[0-9]+]] = OpTypeStruct %[[PIPE_STORAGE_ID_2]]
; CHECK-SPIRV: %[[CL_PIPE_STORAGE_PTR_ID:[0-9]+]] = OpTypePointer CrossWorkgroup %[[CL_PIPE_STORAGE_ID]]

; FIXME: How to convert "16 16 1"?
; CHECK-SPIRV: %[[CPS_ID:[0-9]+]] = OpConstantPipeStorage %[[PIPE_STORAGE_ID]] 16 16 1
; CHECK-SPIRV: %[[COMPOSITE_ID:[0-9]+]] = OpConstantComposite %[[CL_PIPE_STORAGE_ID]] %[[CPS_ID]]
; CHECK-SPIRV: %[[MYPIPE_ID]] = OpVariable %[[CL_PIPE_STORAGE_PTR_ID]] CrossWorkgroup %[[COMPOSITE_ID]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%spirv.ConstantPipeStorage = type { i32, i32, i32 }
%"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>" = type { %spirv.PipeStorage addrspace(1)* }
%spirv.PipeStorage = type opaque

@_ZN2cl9__details29OpConstantPipeStorage_CreatorILi16ELi16ELi1EE5valueE = linkonce_odr addrspace(1) global %spirv.ConstantPipeStorage { i32 16, i32 16, i32 1 }, align 4
@mygpipe = addrspace(1) global %"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>" { %spirv.PipeStorage addrspace(1)* bitcast (%spirv.ConstantPipeStorage addrspace(1)* @_ZN2cl9__details29OpConstantPipeStorage_CreatorILi16ELi16ELi1EE5valueE to %spirv.PipeStorage addrspace(1)*) }, align 4

; Function Attrs: nounwind
define spir_kernel void @worker() {
entry:
  ret void
}

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}
!llvm.ident = !{!3}
!spirv.Source = !{!4}
!spirv.String = !{}

!0 = !{i32 1, i32 2}
!1 = !{i32 2, i32 2}
!2 = !{}
!3 = !{!"clang version 3.6.1 "}
!4 = !{i32 4, i32 202000}
