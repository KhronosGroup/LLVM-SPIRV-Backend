; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'struct.c'
source_filename = "struct.c"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

%struct.ST = type { i32, i32, i32 }

; CHECK-SPIRV: OpName %[[struct:[0-9]+]] "struct.ST"
; CHECK-SPIRV: %[[int:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV: %[[struct]] = OpTypeStruct %[[int]] %[[int]] %[[int]]
; CHECK-SPIRV: %[[structP:[0-9]+]] = OpTypePointer Function %[[struct]]
; CHECK-SPIRV: %[[intP:[0-9]+]] = OpTypePointer Function %[[int]]
; CHECK-SPIRV: %[[zero:[0-9]+]] = OpConstant %[[int]] 0
; CHECK-SPIRV: %[[one:[0-9]+]] = OpConstant %[[int]] 1
; CHECK-SPIRV: %[[two:[0-9]+]] = OpConstant %[[int]] 2
; CHECK-SPIRV: %[[three:[0-9]+]] = OpConstant %[[int]] 3

; Function Attrs: noinline nounwind optnone
define dso_local spir_func i32 @func() #0 {
entry:
; CHECK-SPIRV: %[[st:[0-9]+]] = OpVariable %[[structP]]
  %st = alloca %struct.ST, align 4
; CHECK-SPIRV: %[[a:[0-9]+]] = OpInBoundsAccessChain %[[intP]] %[[st]] %[[zero]]
; CHECK-SPIRV: OpStore %[[a]] %[[one]]
  %a = getelementptr inbounds %struct.ST, %struct.ST* %st, i32 0, i32 0
  store i32 1, i32* %a, align 4
; CHECK-SPIRV: %[[b:[0-9]+]] = OpInBoundsAccessChain %[[intP]] %[[st]] %[[one]]
; CHECK-SPIRV: OpStore %[[b]] %[[two]]
  %b = getelementptr inbounds %struct.ST, %struct.ST* %st, i32 0, i32 1
  store i32 2, i32* %b, align 4
; CHECK-SPIRV: %[[c:[0-9]+]] = OpInBoundsAccessChain %[[intP]] %[[st]] %[[two]]
; CHECK-SPIRV: OpStore %[[c]] %[[three]]
  %c = getelementptr inbounds %struct.ST, %struct.ST* %st, i32 0, i32 2
  store i32 3, i32* %c, align 4
; CHECK-SPIRV: %[[a1:[0-9]+]] = OpInBoundsAccessChain %[[intP]] %[[st]] %[[zero]]
; CHECK-SPIRV: %{{[0-9]+}} = OpLoad %[[int]] %[[a1]]
  %a1 = getelementptr inbounds %struct.ST, %struct.ST* %st, i32 0, i32 0
  %0 = load i32, i32* %a1, align 4
; CHECK-SPIRV: %[[b1:[0-9]+]] = OpInBoundsAccessChain %[[intP]] %[[st]] %[[one]]
; CHECK-SPIRV: %{{[0-9]+}} = OpLoad %[[int]] %[[b1]]
  %b2 = getelementptr inbounds %struct.ST, %struct.ST* %st, i32 0, i32 1
  %1 = load i32, i32* %b2, align 4
  %add = add nsw i32 %0, %1
; CHECK-SPIRV: %[[c1:[0-9]+]] = OpInBoundsAccessChain %[[intP]] %[[st]] %[[two]]
; CHECK-SPIRV: %{{[0-9]+}} = OpLoad %[[int]] %[[c1]]
  %c3 = getelementptr inbounds %struct.ST, %struct.ST* %st, i32 0, i32 2
  %2 = load i32, i32* %c3, align 4
  %add4 = add nsw i32 %add, %2
  ret i32 %add4
}

attributes #0 = { noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 12.0.0"}
