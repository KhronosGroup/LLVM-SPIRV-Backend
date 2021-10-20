; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV-DAG: OpDecorate %[[Id:[0-9]+]] BuiltIn GlobalInvocationId
; CHECK-SPIRV-DAG: OpDecorate %[[Id:[0-9]+]] BuiltIn GlobalLinearId
; CHECK-SPIRV: %[[Id:[0-9]+]] = OpVariable %{{[0-9]+}}
; CHECK-SPIRV: %[[Id:[0-9]+]] = OpVariable %{{[0-9]+}}

; Function Attrs: nounwind readnone
define spir_kernel void @f() #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
entry:
  %0 = call spir_func i32 @_Z29__spirv_BuiltInGlobalLinearIdv()
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
  ret void
}

declare spir_func i32 @_Z29__spirv_BuiltInGlobalLinearIdv()
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

attributes #0 = { alwaysinline nounwind readonly "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!0 = !{}
