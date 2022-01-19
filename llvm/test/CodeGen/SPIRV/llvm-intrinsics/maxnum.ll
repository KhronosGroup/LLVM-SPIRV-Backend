; RUN: llc -O0 %s -o - | FileCheck %s

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

define spir_func float @Test(float %x, float %y) {
entry:
  %0 = call float @llvm.maxnum.f32(float %x, float %y)
  ret float %0
}

; CHECK: OpFunction
; CHECK: %[[x:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK: %[[y:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK: %[[res:[0-9]+]] = OpExtInst %{{[0-9]+}} %{{[0-9]+}} fmax %[[x]] %[[y]]
; CHECK: OpReturnValue %[[res]]

declare float @llvm.maxnum.f32(float, float)
