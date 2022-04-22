; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK

; CHECK: %[[Int8Ty:[0-9]+]] = OpTypeInt 8 0
; CHECK: %[[PtrTy:[0-9]+]] = OpTypePointer Function %[[Int8Ty]]
; CHECK: %[[Int64Ty:[0-9]+]] = OpTypeInt 64 0
; CHECK: %[[FTy:[0-9]+]] = OpTypeFunction %[[Int64Ty]] %[[PtrTy]]
; CHECK: %[[Int32Ty:[0-9]+]] = OpTypeInt 32 0
; CHECK: %[[Const:[0-9]+]] = OpConstant %[[Int32Ty]] 0 
; CHECK: OpFunction %[[Int64Ty]] None %[[FTy]]
; CHECK: %[[Parm:[0-9]+]] = OpFunctionParameter %[[PtrTy]]
; CHECK: OpStore %[[Parm]] %[[Const]] Aligned 4
; CHECK: %[[Res:[0-9]+]] = OpLoad %[[Int64Ty]] %[[Parm]] Aligned 4
; CHECK: OpReturnValue %[[Res]]

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

define i64 @test(ptr %p) {
  store i32 0, ptr %p
  %v = load i64, ptr %p
  ret i64 %v
}
