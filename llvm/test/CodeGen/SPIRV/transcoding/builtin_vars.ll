; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpDecorate %[[Id:[0-9]+]] BuiltIn GlobalLinearId
; CHECK-SPIRV: %[[Id:[0-9]+]] = OpVariable %{{[0-9]+}}

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

@__spirv_BuiltInGlobalLinearId = external addrspace(1) global i32

; Function Attrs: nounwind readnone
define spir_kernel void @f(i32 addrspace(1)* nocapture %order) #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
entry:
  %0 = load i32, i32 addrspace(4)* addrspacecast (i32 addrspace(1)* @__spirv_BuiltInGlobalLinearId to i32 addrspace(4)*), align 4
  ; Need to store the result somewhere, otherwise the access to GlobalLinearId
  ; may be removed.
  store i32 %0, i32 addrspace(1)* %order, align 4
  ret void
}

attributes #0 = { alwaysinline nounwind readonly "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!0 = !{}
