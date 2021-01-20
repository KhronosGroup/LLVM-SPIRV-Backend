; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; FIXME: CHECK-SPIRV-DAG for OpTypeInt, OpTypeVoid and OpTypeImage?
; CHECK-SPIRV: %[[IntTyID:[0-9]+]] = OpTypeInt
; CHECK-SPIRV: %[[VoidTyID:[0-9]+]] = OpTypeVoid
; FIXME: fix "1 0 1 0 0 0 0" -> "2D 0 0 0 0 Unknown ReadOnly" ?
; CHECK-SPIRV: %[[ImageTyID:[0-9]+]] = OpTypeImage %[[VoidTyID]] 1 0 1 0 0 0 0
; CHECK-SPIRV: %[[VectorTyID:[0-9]+]] = OpTypeVector %[[IntTyID]] %{{[0-9]+}}
; CHECK-SPIRV: %[[ImageArgID:[0-9]+]] = OpFunctionParameter %[[ImageTyID]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQuerySizeLod %[[VectorTyID]] %[[ImageArgID]]

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

%opencl.image2d_array_ro_t = type opaque

; Function Attrs: nounwind
define spir_kernel void @sample_kernel(%opencl.image2d_array_ro_t addrspace(1)* %input) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %call = call spir_func i32 @_Z15get_image_width20ocl_image2d_array_ro(%opencl.image2d_array_ro_t addrspace(1)* %input) #2
  ret void
}

; Function Attrs: nounwind readnone
declare spir_func i32 @_Z15get_image_width20ocl_image2d_array_ro(%opencl.image2d_array_ro_t addrspace(1)*) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!6}
!opencl.used.extensions = !{!7}
!opencl.used.optional.core.features = !{!7}
!opencl.compiler.options = !{!7}
!llvm.ident = !{!8}

!1 = !{i32 1}
!2 = !{!"read_only"}
!3 = !{!"__read_only image2d_array_t"}
!4 = !{!"__read_only image2d_array_t"}
!5 = !{!""}
!6 = !{i32 1, i32 2}
!7 = !{}
!8 = !{!"clang version 3.6.1 "}
