; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=SPIRV

; ModuleID = 'c:/work/tmp/testLink.c'
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; SPIRV:  OpCapability Linkage
; SPIRV: OpEntryPoint Kernel %[[kern:[0-9]+]] "kern"

@ae = available_externally addrspace(1) global i32 79, align 4
; SPIRV: OpName %[[ae:[0-9]+]] "ae"

@i1 = addrspace(1) global i32 1, align 4
; SPIRV: OpName %[[i1:[0-9]+]] "i1"

@i2 = internal addrspace(1) global i32 2, align 4
; SPIRV: OpName %[[i2:[0-9]+]] "i2"

@i3 = addrspace(1) global i32 3, align 4
; SPIRV: OpName %[[i3:[0-9]+]] "i3"

@i4 = common addrspace(1) global i32 0, align 4
; SPIRV: OpName %[[i4:[0-9]+]] "i4"

@i5 = internal addrspace(1) global i32 0, align 4
; SPIRV: OpName %[[i5:[0-9]+]] "i5"

@color_table = addrspace(2) constant [2 x i32] [i32 0, i32 1], align 4
; SPIRV: OpName %[[color_table:[0-9]+]] "color_table"

@noise_table = external addrspace(2) constant [256 x i32]
; SPIRV: OpName %[[noise_table:[0-9]+]] "noise_table"

@w = addrspace(1) constant i32 0, align 4
; SPIRV: OpName %[[w:[0-9]+]] "w"

@f.color_table = internal addrspace(2) constant [2 x i32] [i32 2, i32 3], align 4
; SPIRV: OpName %[[f_color_table:[0-9]+]] "f.color_table"

@e = external addrspace(1) global i32
; SPIRV: OpName %[[e:[0-9]+]] "e"

@f.t = internal addrspace(1) global i32 5, align 4
; SPIRV: OpName %[[f_t:[0-9]+]] "f.t"

@f.stint = internal addrspace(1) global i32 0, align 4
; SPIRV: OpName %[[f_stint:[0-9]+]] "f.stint"

@f.inside = internal addrspace(1) global i32 0, align 4
; SPIRV: OpName %[[f_inside:[0-9]+]] "f.inside"

@f.b = internal addrspace(2) constant float 1.000000e+00, align 4
; SPIRV: OpName %[[f_b:[0-9]+]] "f.b"

; SPIRV-DAG: OpName %[[foo:[0-9]+]] "foo"
; SPIRV-DAG: OpName %[[f:[0-9]+]] "f"
; SPIRV-DAG: OpName %[[g:[0-9]+]] "g"
; SPIRV-DAG: OpName %[[inline_fun:[0-9]+]] "inline_fun"

; SPIRV-DAG: OpDecorate %[[ae]] LinkageAttributes "ae" Import
; SPIRV-DAG: OpDecorate %[[e]] LinkageAttributes "e" Import
; SPIRV-DAG: OpDecorate %[[f]] LinkageAttributes "f" Export
; SPIRV-DAG: OpDecorate %[[w]] LinkageAttributes "w" Export
; SPIRV-DAG: OpDecorate %[[i1]] LinkageAttributes "i1" Export
; SPIRV-DAG: OpDecorate %[[i3]] LinkageAttributes "i3" Export
; SPIRV-DAG: OpDecorate %[[i4]] LinkageAttributes "i4" Export
; SPIRV-DAG: OpDecorate %[[foo]] LinkageAttributes "foo" Import
; SPIRV-DAG: OpDecorate %[[inline_fun]] LinkageAttributes "inline_fun" Export
; SPIRV-DAG: OpDecorate %[[color_table]] LinkageAttributes "color_table" Export
; SPIRV-DAG: OpDecorate %[[noise_table]] LinkageAttributes "noise_table" Import

; SPIRV: %[[foo]] = OpFunction %{{[0-9]+}}
declare spir_func void @foo() #2

; SPIRV: %[[f]] = OpFunction %{{[0-9]+}}
; Function Attrs: nounwind
define spir_func void @f() #0 {
entry:
  %q = alloca i32, align 4
  %r = alloca i32, align 4
  %0 = load i32, i32 addrspace(1)* @i2, align 4
  store i32 %0, i32* %q, align 4
  %1 = load i32, i32 addrspace(1)* @i3, align 4
  store i32 %1, i32 addrspace(1)* @i5, align 4
  %2 = load i32, i32 addrspace(1)* @e, align 4
  store i32 %2, i32* %r, align 4
  %3 = load i32, i32 addrspace(2)* getelementptr inbounds ([256 x i32], [256 x i32] addrspace(2)* @noise_table, i32 0, i32 0), align 4
  store i32 %3, i32* %r, align 4
  %4 = load i32, i32 addrspace(2)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(2)* @f.color_table, i32 0, i32 0), align 4
  store i32 %4, i32* %r, align 4
  %call = call spir_func i32 @g()
  call spir_func void @inline_fun()
  ret void
}

; SPIRV: %[[g]] = OpFunction %{{[0-9]+}}
; Function Attrs: nounwind
define internal spir_func i32 @g() #0 {
entry:
  call spir_func void @foo()
  ret i32 25
}

; SPIRV: %[[inline_fun]] = OpFunction %{{[0-9]+}}
; "linkonce_odr" is lost in translation !
; Function Attrs: inlinehint nounwind
define linkonce_odr spir_func void @inline_fun() #1 {
entry:
  %t = alloca i32 addrspace(1)*, align 4
  store i32 addrspace(1)* @i1, i32 addrspace(1)** %t, align 4
  ret void
}

; SPIRV: %[[kern]] = OpFunction %{{[0-9]+}}
; Function Attrs: nounwind
define spir_kernel void @kern() #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !1 !kernel_arg_type !1 !kernel_arg_base_type !1 !kernel_arg_type_qual !1 {
entry:
  call spir_func void @f()
  ret void
}

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inlinehint nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!3}
!opencl.used.extensions = !{!1}
!opencl.used.optional.core.features = !{!1}
!opencl.compiler.options = !{!1}

!1 = !{}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
