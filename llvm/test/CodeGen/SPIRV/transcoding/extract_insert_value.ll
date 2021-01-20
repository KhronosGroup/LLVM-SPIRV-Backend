; ModuleID = ''
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; Check 'LLVM ==> SPIR-V' conversion of extractvalue/insertvalue.

%struct.arr = type { [7 x float] }
%struct.st = type { %struct.inner }
%struct.inner = type { float }

; CHECK-SPIRV-LABEL:  OpFunction
; CHECK-SPIRV-NEXT:   %[[object:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV:        %{{[0-9]+}} = OpInBoundsPtrAccessChain %{{[0-9]+}} %[[object]] %{{[0-9]+}} %{{[0-9]+}}
; CHECK-SPIRV:        %[[extracted_array:[0-9]+]] = OpLoad %{{[0-9]+}} %{{[0-9]+}} Aligned 4
; CHECK-SPIRV:        %[[elem_4:[0-9]+]] = OpCompositeExtract %{{[0-9]+}} %[[extracted_array]] 4
; CHECK-SPIRV:        %[[elem_2:[0-9]+]] = OpCompositeExtract %{{[0-9]+}} %[[extracted_array]] 2
; CHECK-SPIRV:        %[[add:[0-9]+]] = OpFAdd %{{[0-9]+}} %[[elem_4]] %[[elem_2]]
; CHECK-SPIRV:        %[[inserted_array:[0-9]+]] = OpCompositeInsert %{{[0-9]+}} %[[add]] %[[extracted_array]] 5
; CHECK-SPIRV:        OpStore %{{[0-9]+}} %[[inserted_array]]
; CHECK-SPIRV-LABEL:  OpFunctionEnd

; Function Attrs: nounwind
define spir_func void @array_test(%struct.arr addrspace(1)* %object) #0 {
entry:
  %0 = getelementptr inbounds %struct.arr, %struct.arr addrspace(1)* %object, i32 0, i32 0
  %1 = load [7 x float], [7 x float] addrspace(1)* %0, align 4
  %2 = extractvalue [7 x float] %1, 4
  %3 = extractvalue [7 x float] %1, 2
  %4 = fadd float %2, %3
  %5 = insertvalue [7 x float] %1, float %4, 5
  store [7 x float] %5, [7 x float] addrspace(1)* %0
  ret void
}

; CHECK-SPIRV-LABEL:  OpFunction
; CHECK-SPIRV-NEXT:   %[[object:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV:        %{{[0-9]+}} = OpInBoundsPtrAccessChain %{{[0-9]+}} %[[object]] %{{[0-9]+}} %{{[0-9]+}}
; CHECK-SPIRV:        %[[extracted_struct:[0-9]+]] = OpLoad %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}} 4
; CHECK-SPIRV:        %[[elem:[0-9]+]] = OpCompositeExtract %{{[0-9]+}} %[[extracted_struct]] 0
; CHECK-SPIRV:        %[[add:[0-9]+]] = OpFAdd %{{[0-9]+}} %[[elem]] %{{[0-9]+}}
; CHECK-SPIRV:        %[[inserted_struct:[0-9]+]] = OpCompositeInsert %{{[0-9]+}} %[[add]] %[[extracted_struct]] 0
; CHECK-SPIRV:        OpStore %{{[0-9]+}} %[[inserted_struct]]
; CHECK-SPIRV-LABEL:  OpFunctionEnd

; Function Attrs: nounwind
define spir_func void @struct_test(%struct.st addrspace(1)* %object) #0 {
entry:
  %0 = getelementptr inbounds %struct.st, %struct.st addrspace(1)* %object, i32 0, i32 0
  %1 = load %struct.inner, %struct.inner addrspace(1)* %0, align 4
  %2 = extractvalue %struct.inner %1, 0
  %3 = fadd float %2, 1.000000e+00
  %4 = insertvalue %struct.inner %1, float %3, 0
  store %struct.inner %4, %struct.inner addrspace(1)* %0
  ret void
}

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!1}
!opencl.used.optional.core.features = !{!1}
!opencl.compiler.options = !{!1}

!0 = !{i32 1, i32 2}
!1 = !{}
