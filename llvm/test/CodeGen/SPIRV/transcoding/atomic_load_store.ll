; ModuleID = ''
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; Check 'LLVM ==> SPIR-V' conversion of atomic_load and atomic_store.


; Function Attrs: nounwind

; CHECK-SPIRV-LABEL:  OpFunction
; CHECK-SPIRV-NEXT:   %[[object:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV:        %[[ret:[0-9]+]] = OpAtomicLoad %{{[0-9]+}} %[[object]] %{{[0-9]+}} %{{[0-9]+}}
; CHECK-SPIRV:        ReturnValue %[[ret]]
; CHECK-SPIRV-LABEL:  OpFunctionEnd

; Function Attrs: nounwind
define spir_func i32 @test_load(i32 addrspace(4)* %object) #0 {
entry:
  %0 = call spir_func i32 @_Z11atomic_loadPVU3AS4U7_Atomici(i32 addrspace(4)* %object) #2
  ret i32 %0
}

; CHECK-SPIRV-LABEL:  OpFunction
; CHECK-SPIRV-NEXT:   %[[object:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV-NEXT:   OpFunctionParameter
; CHECK-SPIRV-NEXT:   %[[desired:[0-9]+]] = OpFunctionParameter %{{[0-9]+}}
; CHECK-SPIRV:        %{{[0-9]+}} = OpAtomicStore %[[object]] %{{[0-9]+}} %[[desired]]
; CHECK-SPIRV-LABEL:  OpFunctionEnd

; Function Attrs: nounwind
define spir_func void @test_store(i32 addrspace(4)* %object, i32 addrspace(4)* %expected, i32 %desired) #0 {
entry:
  call spir_func void @_Z12atomic_storePVU3AS4U7_Atomicii(i32 addrspace(4)* %object, i32 %desired) #2
  ret void
}

declare spir_func i32 @_Z11atomic_loadPVU3AS4U7_Atomici(i32 addrspace(4)*) #1

declare spir_func void @_Z12atomic_storePVU3AS4U7_Atomicii(i32 addrspace(4)*, i32) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}

!0 = !{i32 1, i32 2}
!1 = !{i32 2, i32 0}
!2 = !{}
