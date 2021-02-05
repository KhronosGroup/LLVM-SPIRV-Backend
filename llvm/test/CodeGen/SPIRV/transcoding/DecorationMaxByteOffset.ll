; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpName %[[PTR_ID:[0-9]+]] "ptr"
; CHECK-SPIRV: OpName %[[PTR2_ID:[0-9]+]] "ptr2"
; FIXME: "MaxByteOffset 12", "123"
; CHECK-SPIRV: OpDecorate %[[PTR_ID]] MaxByteOffset 12
; CHECK-SPIRV: OpDecorate %[[PTR2_ID]] MaxByteOffset 123
; CHECK-SPIRV: %[[CHAR_T:[0-9]+]] = OpTypeInt 8 0
; CHECK-SPIRV: %[[CHAR_PTR_T:[0-9]+]] = OpTypePointer Workgroup %[[CHAR_T]]
; CHECK-SPIRV: %[[PTR_ID]] = OpFunctionParameter %[[CHAR_PTR_T]]
; CHECK-SPIRV: %[[PTR2_ID]] = OpFunctionParameter %[[CHAR_PTR_T]]

; CHECK-SPIRV_1_0-NOT: OpDecorate %{{[0-9]+}} MaxByteOffset

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @worker(i8 addrspace(3)* dereferenceable(12) %ptr) #0 {
entry:
  %ptr.addr = alloca i8 addrspace(3)*, align 4
  store i8 addrspace(3)* %ptr, i8 addrspace(3)** %ptr.addr, align 4
  ret void
}

; Function Attrs: nounwind
define spir_func void @not_a_kernel(i8 addrspace(3)* dereferenceable(123) %ptr2) #0 {
entry:
  ret void
}

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

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
