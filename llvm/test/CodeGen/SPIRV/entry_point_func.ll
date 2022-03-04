;; Test to check that an LLVM spir_kernel gets translated into an
;; Entrypoint wrapper and Function with LinkageAttributes
; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

define spir_kernel void @testfunction() {
   ret void
}

; Check there is an entrypoint and a function produced.
; CHECK-SPIRV: OpEntryPoint Kernel %[[EP:[0-9]+]] "testfunction"
; CHECK-SPIRV: OpName %[[FUNC:[0-9]+]] "testfunction"
; CHECK-SPIRV: OpDecorate %[[FUNC]] LinkageAttributes "testfunction" Export
; CHECK-SPIRV: %[[FUNC]] = OpFunction %2 None %3
; CHECK-SPIRV: %[[EP]] = OpFunction %2 None %3
; CHECK-SPIRV: %8 = OpFunctionCall %2 %[[FUNC]]
