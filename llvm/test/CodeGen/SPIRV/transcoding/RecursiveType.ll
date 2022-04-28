; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%struct.A = type { i32, %struct.C }
%struct.C = type { i32, %struct.B }
%struct.B = type { i32, %struct.A addrspace(4)* }
%struct.Node = type { %struct.Node addrspace(1)*, i32 }

; CHECK-SPIRV-DAG: OpTypeForwardPointer %[[NodeFwdPtr:[0-9]+]] CrossWorkgroup
; CHECK-SPIRV-DAG: OpTypeForwardPointer %[[AFwdPtr:[0-9]+]] Generic
; CHECK-SPIRV-DAG: %[[IntID:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV: %[[BID:[0-9]+]] = OpTypeStruct %{{[0-9]+}} %[[AFwdPtr]]
; CHECK-SPIRV: %[[CID:[0-9]+]] = OpTypeStruct %{{[0-9]+}} %[[BID]]
; CHECK-SPIRV: %[[AID:[0-9]+]] = OpTypeStruct %{{[0-9]+}} %[[CID]]
; CHECK-SPIRV-DAG: %[[AFwdPtr]] = OpTypePointer Generic %[[AID:[0-9]+]]
; CHECK-SPIRV-DAG: %[[NodeID:[0-9]+]] = OpTypeStruct %[[NodeFwdPtr]]
; CHECK-SPIRV-DAG: %[[NodeFwdPtr]] = OpTypePointer CrossWorkgroup %[[NodeID]]

; Function Attrs: nounwind
define spir_kernel void @test(%struct.A addrspace(1)* %result, %struct.Node addrspace(1)* %node) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
  ret void
}

attributes #0 = { nounwind "less-precise-fpmad"="true" "no-frame-pointer-elim"="false" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="true" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!7}
!opencl.ocl.version = !{!8}
!opencl.used.extensions = !{!9}
!opencl.used.optional.core.features = !{!9}
!opencl.compiler.options = !{!9}
!llvm.ident = !{!10}

!1 = !{i32 1, i32 1}
!2 = !{!"none", !"none"}
!3 = !{!"struct A*", !"struct Node*"}
!4 = !{!"struct A*", !"struct Node*"}
!5 = !{!"", !""}
!6 = !{!"result", !"node"}
!7 = !{i32 1, i32 2}
!8 = !{i32 2, i32 0}
!9 = !{}
!10 = !{!"clang version 3.6.1 "}
