; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

; CHECK: OpCapability
; CHECK: OpExtInstImport
; CHECK: OpMemoryModel
; CHECK-NEXT: OpEntryPoint
; CHECK: OpSource

; CHECK-NOT: OpCapability
; CHECK-NOT: OpExtInstImport
; CHECK-NOT: OpMemoryModel
; CHECK-NOT: OpEntryPoint
; CHECK-NOT: OpSource
; CHECK-NOT: OpDecorate
; CHECK-NOT: OpType
; CHECK-NOT: OpVariable
; CHECK-NOT: OpFunction

; CHECK: OpName

; CHECK-NOT: OpCapability
; CHECK-NOT: OpExtInstImport
; CHECK-NOT: OpMemoryModel
; CHECK-NOT: OpEntryPoint
; CHECK-NOT: OpSource
; CHECK-NOT: OpType
; CHECK-NOT: OpVariable
; CHECK-NOT: OpFunction

; CHECK: OpDecorate

; CHECK-NOT: OpCapability
; CHECK-NOT: OpExtInstImport
; CHECK-NOT: OpMemoryModel
; CHECK-NOT: OpEntryPoint
; CHECK-NOT: OpSource
; CHECK-NOT: OpName
; CHECK-NOT: OpVariable
; CHECK-NOT: OpFunction

; CHECK: %[[AFwdPtr:[0-9]+]] = OpTypeForwardPointer
; CHECK: %[[TypeInt:[0-9]+]] = OpTypeInt
; CHECK: %[[Two:[0-9]+]] = OpConstant %[[TypeInt]] 2
; CHECK: %[[TPointer:[0-9]+]] = OpTypePointer
; CHECK: %[[SConstOpType:[0-9]+]] = OpTypePointer
; CHECK: %[[TypeFloat:[0-9]+]] = OpTypeFloat
; CHECK: %[[TypeArray:[0-9]+]] = OpTypeArray %[[TypeFloat]] %[[Two]]
; CHECK: %[[TypeVectorInt3:[0-9]+]] = OpTypeVector %[[TypeInt]] 3
; CHECK: %[[BID:[0-9]+]] = OpTypeStruct %[[AFwdPtr]]
; CHECK: %[[CID:[0-9]+]] = OpTypeStruct %[[BID]]
; CHECK: %[[AID:[0-9]+]] = OpTypeStruct %[[CID]]
; CHECK: %[[AFwdPtr]] = OpTypePointer CrossWorkgroup %[[AID]]
; CHECK: %[[Void:[0-9]+]] = OpTypeVoid
; CHECK: %[[Int3Ptr:[0-9]+]] = OpTypePointer UniformConstant %[[TypeVectorInt3]]
; CHECK: %[[TypeBar1:[0-9]+]] = OpTypeFunction %[[Void]] %[[Int3Ptr]]
; CHECK: %[[Var:[0-9]+]] = OpVariable %[[TPointer]]
; CHECK: %[[SConstOp:[0-9]+]] = OpSpecConstantOp %[[SConstOpType]] 70 %[[Var]]
; CHECK: %{{[0-9]+}} = OpVariable %{{[0-9]+}} 5 %[[SConstOp]]
; FIXME: 70? 5? %?

; CHECK-NOT: OpCapability
; CHECK-NOT: OpExtInstImport
; CHECK-NOT: OpMemoryModel
; CHECK-NOT: OpEntryPoint
; CHECK-NOT: OpSource
; CHECK-NOT: OpName
; CHECK-NOT: OpDecorate

; CHECK: OpFunction
; CHECK: OpFunctionParameter
; CHECK-NOT: OpReturn
; CHECK: OpFunctionEnd

; CHECK-NOT: OpCapability
; CHECK-NOT: OpExtInstImport
; CHECK-NOT: OpMemoryModel
; CHECK-NOT: OpEntryPoint
; CHECK-NOT: OpSource
; CHECK-NOT: OpName
; CHECK-NOT: OpType
; CHECK-NOT: OpDecorate
; CHECK-NOT: OpVariable

; CHECK: OpFunction
; CHECK: OpFunctionParameter
; CHECK: OpFunctionParameter
; CHECK-NOT: OpReturn
; CHECK: OpFunctionEnd

; CHECK-NOT: OpCapability
; CHECK-NOT: OpExtInstImport
; CHECK-NOT: OpMemoryModel
; CHECK-NOT: OpEntryPoint
; CHECK-NOT: OpSource
; CHECK-NOT: OpName
; CHECK-NOT: OpType
; CHECK-NOT: OpDecorate
; CHECK-NOT: OpVariable

; CHECK: OpFunction
; CHECK: OpFunctionParameter
; CHECK: OpLabel
; CHECK: OpFunctionCall
; CHECK: OpFunctionCall
; CHECK: OpReturn
; CHECK: OpFunctionEnd

; CHECK-NOT: OpCapability
; CHECK-NOT: OpExtInstImport
; CHECK-NOT: OpMemoryModel
; CHECK-NOT: OpEntryPoint
; CHECK-NOT: OpSource
; CHECK-NOT: OpName
; CHECK-NOT: OpType
; CHECK-NOT: OpDecorate
; CHECK-NOT: OpVariable

; ModuleID = 'layout.bc'
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

@v = addrspace(1) global [2 x i32] [i32 1, i32 2], align 4
@s = addrspace(1) global i32 addrspace(1)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(1)* @v, i32 0, i32 0), align 4

%struct.A = type { i32, %struct.C }
%struct.C = type { i32, %struct.B }
%struct.B = type { i32, %struct.A addrspace(4)* }

@f = addrspace(2) constant [2 x float] zeroinitializer, align 4
@b = external addrspace(2) constant <3 x i32>
@a = common addrspace(1) global %struct.A zeroinitializer, align 4

; Function Attrs: nounwind
define spir_kernel void @foo(<3 x i32> addrspace(1)* %a) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  call spir_func void @bar1(<3 x i32> addrspace(1)* %a)
  %loadVec4 = load <4 x i32> , <4 x i32> addrspace(2)* bitcast (<3 x i32> addrspace(2)* @b to <4 x i32> addrspace(2)*)
  %extractVec = shufflevector <4 x i32> %loadVec4, <4 x i32> undef, <3 x i32> <i32 0, i32 1, i32 2>
  call spir_func void @bar2(<3 x i32> addrspace(1)* %a, <3 x i32> %extractVec)
  ret void
}

declare spir_func void @bar1(<3 x i32> addrspace(1)*) #1

declare spir_func void @bar2(<3 x i32> addrspace(1)*, <3 x i32>) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!8}
!opencl.compiler.options = !{!8}

!0 = !{void (<3 x i32> addrspace(1)*)* @foo, !1, !2, !3, !4, !5}
!1 = !{i32 1}
!2 = !{!"none"}
!3 = !{!"int3*"}
!4 = !{!"int3*"}
!5 = !{!""}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 0}
!8 = !{}

