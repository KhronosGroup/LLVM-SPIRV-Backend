; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpConstantTrue
; CHECK-SPIRV: OpConstantFalse

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; Function Attrs: nounwind
define spir_func zeroext i1 @f() #0 {
entry:
  ret i1 true
}

; Function Attrs: nounwind
define spir_func zeroext i1 @f2() #0 {
entry:
  ret i1 false
}

; Function Attrs: nounwind
define spir_kernel void @test(i32 addrspace(1)* %i) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %i.addr = alloca i32 addrspace(1)*, align 4
  store i32 addrspace(1)* %i, i32 addrspace(1)** %i.addr, align 4
  %call = call spir_func zeroext i1 @f()
  %conv = zext i1 %call to i32
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %i.addr, align 4
  store i32 %conv, i32 addrspace(1)* %0, align 4
  ret void
}

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!8}
!opencl.compiler.options = !{!8}

!1 = !{i32 1}
!2 = !{!"none"}
!3 = !{!"int*"}
!4 = !{!"int*"}
!5 = !{!""}
!6 = !{i32 1, i32 2}
!7 = !{i32 2, i32 0}
!8 = !{}
