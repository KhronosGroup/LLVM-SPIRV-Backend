; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: %[[IntTypeID:[0-9]+]] = OpTypeInt 32 {{[0-9]+}}
; CHECK-SPIRV: %[[Int2TypeID:[0-9]+]] = OpTypeVector %[[IntTypeID]] 2
; ;HECK-SPIRV: %[[CompositeID:[0-9]+]] = OpCompositeInsert %[[Int2TypeID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
; ;HECK-SPIRV: %[[ShuffleID:[0-9]+]] = OpVectorShuffle %[[Int2TypeID]] %[[CompositeID]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
; CHECK-SPIRV: %{{[0-9]+}} = OpExtInst %[[Int2TypeID]] %1 s_min %{{[0-9]+}} %{{[0-9]+}}

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @test() #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
entry:
  %call = tail call spir_func <2 x i32> @_Z3minDv2_ii(<2 x i32> <i32 1, i32 10>, i32 5) #2
  ret void
}

declare spir_func <2 x i32> @_Z3minDv2_ii(<2 x i32>, i32) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!opencl.compiler.options = !{!0}

!0 = !{}
!1 = !{i32 1, i32 2}
!2 = !{i32 2, i32 0}
