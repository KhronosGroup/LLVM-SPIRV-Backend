; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; FIXME: fix "70" (?)
; CHECK-SPIRV: %{{[0-9]*}} = OpSpecConstantOp %{{[0-9]*}} 70
; CHECK-SPIRV: %{{[0-9]*}} = OpSpecConstantOp %{{[0-9]*}} 70

@a_var = addrspace(1) global [2 x i8] c"\96\96", align 1
@k_var = addrspace(1) global [2 x i8 addrspace(1)*] [i8 addrspace(1)* getelementptr inbounds ([2 x i8], [2 x i8] addrspace(1)* @a_var, i32 0, i64 1), i8 addrspace(1)* getelementptr inbounds ([2 x i8], [2 x i8] addrspace(1)* @a_var, i32 0, i32 0)], align 4

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!1}
!opencl.used.optional.core.features = !{!1}
!opencl.compiler.options = !{!1}

!0 = !{i32 1, i32 2}
!1 = !{}
