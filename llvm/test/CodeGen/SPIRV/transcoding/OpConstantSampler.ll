; Sources:
;
; void kernel foo(__read_only image2d_t src) {
;   sampler_t sampler1 = CLK_NORMALIZED_COORDS_TRUE |
;                        CLK_ADDRESS_REPEAT |
;                        CLK_FILTER_NEAREST;
;   sampler_t sampler2 = 0x00;
;
;   read_imagef(src, sampler1, 0, 0);
;   read_imagef(src, sampler2, 0, 0);
; }

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; FIXME: Check last 3 parameters of OpConstantSampler - how its should be properly converted?
; CHECK-SPIRV: %[[SamplerID0:[0-9]+]] = OpConstantSampler %{{[0-9]+}} Repeat 1 Nearest
; CHECK-SPIRV: %[[SamplerID1:[0-9]+]] = OpConstantSampler %{{[0-9]+}} None 0 Nearest
; CHECK-SPIRV: %[[SamplerID0]] = OpSampledImage {{.*}}
; CHECK-SPIRV: %[[SamplerID1]] = OpSampledImage {{.*}}

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%opencl.image2d_ro_t = type opaque
%opencl.sampler_t = type opaque

; Function Attrs: convergent nounwind
define spir_func <4 x float> @foo(%opencl.image2d_ro_t addrspace(1)* %src) local_unnamed_addr #0 {
entry:
  %0 = tail call %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32 23) #2
  %1 = tail call %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32 0) #2
  %call = tail call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_ff(%opencl.image2d_ro_t addrspace(1)* %src, %opencl.sampler_t addrspace(2)* %0, <2 x float> zeroinitializer, float 0.000000e+00) #3
  %call1 = tail call spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_ff(%opencl.image2d_ro_t addrspace(1)* %src, %opencl.sampler_t addrspace(2)* %1, <2 x float> zeroinitializer, float 0.000000e+00) #3
  %add = fadd <4 x float> %call, %call1
  ret <4 x float> %add
}

declare %opencl.sampler_t addrspace(2)* @__translate_sampler_initializer(i32) local_unnamed_addr

; Function Attrs: convergent nounwind readonly
declare spir_func <4 x float> @_Z11read_imagef14ocl_image2d_ro11ocl_samplerDv2_ff(%opencl.image2d_ro_t addrspace(1)*, %opencl.sampler_t addrspace(2)*, <2 x float>, float) local_unnamed_addr #1

attributes #0 = { convergent nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { convergent nounwind readonly "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }
attributes #3 = { convergent nounwind readonly }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
