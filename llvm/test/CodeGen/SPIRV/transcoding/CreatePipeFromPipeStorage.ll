; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV


; CHECK-SPIRV: OpCapability Pipes
; CHECK-SPIRV: OpCapability PipeStorage

; CHECK-SPIRV: OpName %[[PIPE_STORAGE_ID:[0-9]+]] "mygpipe"
; CHECK-SPIRV: OpName %[[READ_PIPE_WRAPPER_ID:[0-9]+]] "myrpipe"
; CHECK-SPIRV: OpName %[[WRITE_PIPE_WRAPPER_ID:[0-9]+]] "mywpipe"
; CHECK-SPIRV: OpName %[[WRITE_PIPE_WRAPPER_CTOR:[0-9]+]] "_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE1EEC1EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE1EEE"
; CHECK-SPIRV: OpName %[[READ_PIPE_WRAPPER_CTOR:[0-9]+]] "_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE0EEC1EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE0EEE"

; CHECK-SPIRV: %[[INT_T:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV: %[[CONSTANT_ZERO_ID:[0-9]+]] = OpConstant %[[INT_T]] 0

; CHECK-SPIRV: %[[READ_PIPE:[0-9]+]] = OpTypePipe 0
; CHECK-SPIRV: %[[READ_PIPE_WRAPPER:[0-9]+]] = OpTypeStruct %[[READ_PIPE]]
; FIXME: Change "7" to appropriate class storage
; CHECK-SPIRV: %[[READ_PIPE_WRAPPER_PTR:[0-9]+]] = OpTypePointer 7 %[[READ_PIPE_WRAPPER]]
; CHECK-SPIRV: %[[WRITE_PIPE:[0-9]+]] = OpTypePipe 1
; CHECK-SPIRV: %[[WRITE_PIPE_WRAPPER:[0-9]+]] = OpTypeStruct %[[WRITE_PIPE]]

; FIXME: Change "7" to appropriate class storage
; CHECK-SPIRV: %[[WRITE_PIPE_WRAPPER_PTR:[0-9]+]] = OpTypePointer 7 %[[WRITE_PIPE_WRAPPER]]


target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%spirv.ConstantPipeStorage = type { i32, i32, i32 }
%"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>" = type { %spirv.PipeStorage addrspace(1)* }
%spirv.PipeStorage = type opaque
%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>" = type { %spirv.Pipe._0 addrspace(1)* }
%spirv.Pipe._0 = type opaque
%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>" = type { %spirv.Pipe._1 addrspace(1)* }
%spirv.Pipe._1 = type opaque

@_ZN2cl9__details29OpConstantPipeStorage_CreatorILi16ELi16ELi1EE5valueE = linkonce_odr addrspace(1) global %spirv.ConstantPipeStorage { i32 16, i32 16, i32 1 }, align 4
@mygpipe = addrspace(1) global %"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>" { %spirv.PipeStorage addrspace(1)* bitcast (%spirv.ConstantPipeStorage addrspace(1)* @_ZN2cl9__details29OpConstantPipeStorage_CreatorILi16ELi16ELi1EE5valueE to %spirv.PipeStorage addrspace(1)*) }, align 4


; Function Attrs: nounwind
define spir_kernel void @worker() {
entry:
  ; FIXME: fix storage class
  ; CHECK-SPIRV: %[[READ_PIPE_WRAPPER_ID]] = OpVariable %[[READ_PIPE_WRAPPER_PTR]] 7
  ; CHECK-SPIRV: %[[WRITE_PIPE_WRAPPER_ID]] = OpVariable %[[WRITE_PIPE_WRAPPER_PTR]] 7

  %myrpipe = alloca %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>", align 4
  %mywpipe = alloca %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>", align 4


  ; CHECK-SPIRV: %[[SPIRV0:[0-9]+]] = OpPtrCastToGeneric %{{[0-9]+}} %[[PIPE_STORAGE_ID]]
  ; CHECK-SPIRV: %[[SPIRV1:[0-9]+]] = OpPtrAccessChain %{{[0-9]+}} %[[SPIRV0]] %[[CONSTANT_ZERO_ID]] %[[CONSTANT_ZERO_ID]]

  %0 = addrspacecast %"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>" addrspace(1)* @mygpipe to %"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>" addrspace(4)*
  %1 = getelementptr %"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>", %"class.cl::pipe_storage<int __attribute__((ext_vector_type(4))), 1>" addrspace(4)* %0, i32 0, i32 0


  ; FIXME: "2 4"
  ; CHECK-SPIRV: %[[PIPE_STORAGE_ID0:[0-9]+]] = OpLoad %{{[0-9]+}} %[[SPIRV1]] 2 4
  ; CHECK-SPIRV: %[[WRITE_PIPE_ID:[0-9]+]] = OpCreatePipeFromPipeStorage %[[WRITE_PIPE]] %[[PIPE_STORAGE_ID0]]
  ; CHECK-SPIRV: %[[GENERIC_WRITE_PIPE_WRAPPER_ID:[0-9]+]] = OpPtrCastToGeneric %{{[0-9]+}} %[[WRITE_PIPE_WRAPPER_ID]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionCall %{{[0-9]+}} %[[WRITE_PIPE_WRAPPER_CTOR]] %[[GENERIC_WRITE_PIPE_WRAPPER_ID]] %[[WRITE_PIPE_ID]]

  %2 = load %spirv.PipeStorage addrspace(1)*, %spirv.PipeStorage addrspace(1)* addrspace(4)* %1, align 4
  %3 = tail call spir_func %spirv.Pipe._1 addrspace(1)* @_Z39__spirv_CreatePipeFromPipeStorage_writePU3AS1K19__spirv_PipeStorage(%spirv.PipeStorage addrspace(1)* %2)
  %4 = addrspacecast %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>"* %mywpipe to %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>" addrspace(4)*
  call spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE1EEC1EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE1EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>" addrspace(4)* %4, %spirv.Pipe._1 addrspace(1)* %3)


  ; FIXME: "2 4"
  ; CHECK-SPIRV: %[[PIPE_STORAGE_ID1:[0-9]+]] = OpLoad %{{[0-9]+}} %[[SPIRV1]] 2 4
  ; CHECK-SPIRV: %[[READ_PIPE_ID:[0-9]+]] = OpCreatePipeFromPipeStorage %[[READ_PIPE]] %[[PIPE_STORAGE_ID1]]
  ; CHECK-SPIRV: %[[GENERIC_READ_PIPE_WRAPPER_ID:[0-9]+]] = OpPtrCastToGeneric %{{[0-9]+}} %[[READ_PIPE_WRAPPER_ID]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionCall %{{[0-9]+}} %[[READ_PIPE_WRAPPER_CTOR]] %[[GENERIC_READ_PIPE_WRAPPER_ID]] %[[READ_PIPE_ID]]

  %5 = load %spirv.PipeStorage addrspace(1)*, %spirv.PipeStorage addrspace(1)* addrspace(4)* %1, align 4
  %6 = tail call spir_func %spirv.Pipe._0 addrspace(1)* @_Z38__spirv_CreatePipeFromPipeStorage_readPU3AS1K19__spirv_PipeStorage(%spirv.PipeStorage addrspace(1)* %5)
  %7 = addrspacecast %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>"* %myrpipe to %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>" addrspace(4)*
  call spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE0EEC1EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE0EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>" addrspace(4)* %7, %spirv.Pipe._0 addrspace(1)* %6)


  ret void
}

; Function Attrs: nounwind
define linkonce_odr spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE0EEC1EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE0EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>" addrspace(4)* nocapture %this, %spirv.Pipe._0 addrspace(1)* %handle) unnamed_addr align 2 {
entry:
  tail call spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE0EEC2EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE0EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>" addrspace(4)* %this, %spirv.Pipe._0 addrspace(1)* %handle)
  ret void
}

; Function Attrs: nounwind
define linkonce_odr spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE0EEC2EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE0EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>" addrspace(4)* nocapture %this, %spirv.Pipe._0 addrspace(1)* %handle) unnamed_addr align 2 {
entry:
  %_handle = getelementptr inbounds %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>", %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::read>" addrspace(4)* %this, i32 0, i32 0
  store %spirv.Pipe._0 addrspace(1)* %handle, %spirv.Pipe._0 addrspace(1)* addrspace(4)* %_handle, align 4, !tbaa !11
  ret void
}

; Function Attrs: nounwind
declare spir_func %spirv.Pipe._0 addrspace(1)* @_Z38__spirv_CreatePipeFromPipeStorage_readPU3AS1K19__spirv_PipeStorage(%spirv.PipeStorage addrspace(1)*)

; Function Attrs: nounwind
define linkonce_odr spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE1EEC1EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE1EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>" addrspace(4)* nocapture %this, %spirv.Pipe._1 addrspace(1)* %handle) unnamed_addr align 2 {
entry:
  tail call spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE1EEC2EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE1EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>" addrspace(4)* %this, %spirv.Pipe._1 addrspace(1)* %handle)
  ret void
}

; Function Attrs: nounwind
define linkonce_odr spir_func void @_ZNU3AS42cl4pipeIDv4_iLNS_11pipe_accessE1EEC2EPU3AS1NS_7__spirv10OpTypePipeILNS3_15AccessQualifierE1EEE(%"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>" addrspace(4)* nocapture %this, %spirv.Pipe._1 addrspace(1)* %handle) unnamed_addr align 2 {
entry:
  %_handle = getelementptr inbounds %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>", %"class.cl::pipe<int __attribute__((ext_vector_type(4))), cl::pipe_access::write>" addrspace(4)* %this, i32 0, i32 0
  store %spirv.Pipe._1 addrspace(1)* %handle, %spirv.Pipe._1 addrspace(1)* addrspace(4)* %_handle, align 4, !tbaa !13
  ret void
}

; Function Attrs: nounwind
declare spir_func %spirv.Pipe._1 addrspace(1)* @_Z39__spirv_CreatePipeFromPipeStorage_writePU3AS1K19__spirv_PipeStorage(%spirv.PipeStorage addrspace(1)*)

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
!6 = !{!7, !8, i64 0}
!7 = !{!"_ZTSN2cl12pipe_storageIDv4_iLj1EEE", !8, i64 0}
!8 = !{!"any pointer", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!12, !8, i64 0}
!12 = !{!"_ZTSN2cl4pipeIDv4_iLNS_11pipe_accessE0EEE", !8, i64 0}
!13 = !{!14, !8, i64 0}
!14 = !{!"_ZTSN2cl4pipeIDv4_iLNS_11pipe_accessE1EEE", !8, i64 0}
