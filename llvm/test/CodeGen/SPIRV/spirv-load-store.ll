; Translate SPIR-V friendly OpLoad and OpStore calls
; RUN: llc -O0 %s -o - | FileCheck %s

; CHECK: OpStore %[[#PTR:]]
; CHECK: %[[#]] = OpLoad %[[#]] %[[#PTR]]

; ModuleID = 'before.bc'
source_filename = "test.cpp"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

define weak_odr dso_local spir_kernel void @foo(i32 addrspace(1)* %var) {
entry:
  tail call spir_func void @_Z13__spirv_StorePiiii(i32 addrspace(1)* %var, i32 42, i32 3, i32 4)
  %0 = tail call spir_func double @_Z12__spirv_LoadPi(i32 addrspace(1)* %var)
  ret void
}

declare dso_local spir_func double @_Z12__spirv_LoadPi(i32 addrspace(1)*) local_unnamed_addr

declare dso_local spir_func void @_Z13__spirv_StorePiiii(i32 addrspace(1)*, i32, i32, i32) local_unnamed_addr

!llvm.module.flags = !{!0, !1}
!opencl.spir.version = !{!2}
!spirv.Source = !{!3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"frame-pointer", i32 2}
!2 = !{i32 1, i32 2}
!3 = !{i32 4, i32 100000}
!4 = !{!"clang version 14.0.0 (https://github.com/intel/llvm.git)"}
