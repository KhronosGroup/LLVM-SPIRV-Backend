; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV: OpDecorate %[[ALIGNMENT:[0-9]+]] Alignment 16
; CHECk-SPIRV: %[[ALIGNMENT]] = OpFunctionParameter %{{[0-9]+}}

%struct._ZTS6Struct.Struct = type { %struct._ZTS11floatStruct.floatStruct, %struct._ZTS11floatStruct.floatStruct }
%struct._ZTS11floatStruct.floatStruct = type { float, float, float, float }

; Function Attrs: noinline nounwind
define spir_func void @_ZN3FooC2Ev(%struct._ZTS6Struct.Struct addrspace(4)* align 16 %0) #0 {
  ret void
}

attributes #0 = { noinline nounwind }

!llvm.module.flags = !{!0, !1}
!opencl.spir.version = !{!2}
!spirv.Source = !{!3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!opencl.compiler.options = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"frame-pointer", i32 2}
!2 = !{i32 1, i32 2}
!3 = !{i32 4, i32 100000}
!4 = !{}
