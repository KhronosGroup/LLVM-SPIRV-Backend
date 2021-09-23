; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

@.str.1 = private unnamed_addr addrspace(1) constant [1 x i8] zeroinitializer, align 1

define linkonce_odr hidden spir_func void @foo() {
entry:
; CHECK-SPIRV: %[[Cast:[0-9]+]] = OpPtrCastToGeneric %{{[0-9]+}}
; CHECK-SPIRV: %[[Gep:[0-9]+]] = OpInBoundsPtrAccessChain %{{[0-9]+}} %[[Cast]]
; CHECK-SPIRV: %[[Gep1:[0-9]+]] = OpInBoundsPtrAccessChain %{{[0-9]+}} %[[Cast]]
; CHECK-SPIRV: %[[PtrToU1:[0-9]+]] = OpConvertPtrToU %{{[0-9]+}} %[[Gep1]]
; CHECK-SPIRV: %[[PtrToU2:[0-9]+]] = OpConvertPtrToU %{{[0-9]+}}
; CHECK-SPIRV: %[[IEq:[0-9]+]] = OpIEqual %{{[0-9]+}} %[[PtrToU1]] %[[PtrToU2]]
; CHECK-SPIRV: %[[UToPtr:[0-9]+]] = OpConvertUToPtr %{{[0-9]+}}
; CHECK-SPIRV: %[[Sel:[0-9]+]] = OpSelect %{{[0-9]+}} %[[IEq]] %[[UToPtr]] %[[Gep1]]
; CHECK-SPIRV: %{{[0-9]+}} = OpFunctionCall %{{[0-9]+}} %{{[0-9]+}} %[[Gep]] %[[Sel]]
  call spir_func void @bar(i8 addrspace(4)* getelementptr inbounds ([1 x i8], [1 x i8] addrspace(4)* addrspacecast ([1 x i8] addrspace(1)* @.str.1 to [1 x i8] addrspace(4)*), i64 0, i64 0), i8 addrspace(4)* select (i1 icmp eq (i8 addrspace(4)* getelementptr inbounds ([1 x i8], [1 x i8] addrspace(4)* addrspacecast ([1 x i8] addrspace(1)* @.str.1 to [1 x i8] addrspace(4)*), i64 0, i64 0), i8 addrspace(4)* null), i8 addrspace(4)* inttoptr (i64 -1 to i8 addrspace(4)*), i8 addrspace(4)* getelementptr inbounds ([1 x i8], [1 x i8] addrspace(4)* addrspacecast ([1 x i8] addrspace(1)* @.str.1 to [1 x i8] addrspace(4)*), i64 0, i64 0)))
  ret void
}

define linkonce_odr hidden spir_func void @bar(i8 addrspace(4)* %__beg, i8 addrspace(4)* %__end) {
entry:
  ret void
}

