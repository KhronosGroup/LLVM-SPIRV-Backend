; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV-DAG: OpCapability ImageBasic
; CHECK-SPIRV-DAG: OpCapability ImageReadWrite
; CHECK-SPIRV-DAG: OpCapability LiteralSampler
; FIXME: "2 0 0 0 0 0 0 2"
; CHECK-SPIRV-DAG: %[[TyImageID:[0-9]+]] = OpTypeImage 2 0 0 0 0 0 0 2 
; CHECK-SPIRV-DAG: %[[TySampledImageID:[0-9]+]] = OpTypeSampledImage %[[TyImageID]]

; CHECK-SPIRV-DAG: %[[ResID:[0-9]+]] = OpSampledImage %[[TySampledImageID]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageSampleExplicitLod %{{[0-9]+}} %[[ResID]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%opencl.image1d_rw_t = type opaque

; Function Attrs: nounwind
define spir_func void @sampFun(%opencl.image1d_rw_t addrspace(1)* %image) #0 {
entry:
  %image.addr = alloca %opencl.image1d_rw_t addrspace(1)*, align 4
  store %opencl.image1d_rw_t addrspace(1)* %image, %opencl.image1d_rw_t addrspace(1)** %image.addr, align 4
  %0 = load %opencl.image1d_rw_t addrspace(1)*, %opencl.image1d_rw_t addrspace(1)** %image.addr, align 4
  %call = call spir_func <4 x float> @_Z11read_imagef14ocl_image1d_rw11ocl_sampleri(%opencl.image1d_rw_t addrspace(1)* %0, i32 8, i32 2) #2
  ret void
}

; Function Attrs: nounwind readnone
declare spir_func <4 x float> @_Z11read_imagef14ocl_image1d_rw11ocl_sampleri(%opencl.image1d_rw_t addrspace(1)*, i32, i32) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}

!0 = !{i32 1, i32 2}
!1 = !{i32 2, i32 0}
!2 = !{}
