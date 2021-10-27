; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV-NOT: llvm.memmove

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spirv64-unknown-unknown"

declare void @llvm.memmove.p1i8.p1i8.i64(i8 addrspace(1)* nocapture, i8 addrspace(1)* nocapture readonly, i64, i1)

define spir_func void @memmove_caller(i8 addrspace(1)* %dst, i8 addrspace(1)* %src, i64 %n) {
entry:
  call void @llvm.memmove.p1i8.p1i8.i64(i8 addrspace(1)* %dst, i8 addrspace(1)* %src, i64 %n, i1 false)
  call void @llvm.memmove.p1i8.p1i8.i64(i8 addrspace(1)* %dst, i8 addrspace(1)* %src, i64 %n, i1 false)
  call void @llvm.memmove.p1i8.p1i8.i64(i8 addrspace(1)* %dst, i8 addrspace(1)* %src, i64 %n, i1 false)
  ret void
}
