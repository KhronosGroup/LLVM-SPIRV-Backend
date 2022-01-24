; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV-OFF
 

; CHECK-SPIRV-OFF-NOT: OpCapability FPFastMathModeINTEL
; CHECK-SPIRV-OFF: OpName %[[mu:[0-9]+]] "mul"
; CHECK-SPIRV-OFF: OpName %[[su:[0-9]+]] "sub"
; CHECK-SPIRV-OFF-NOT: OpDecorate %[[mu]] FPFastMathMode AllowContractFastINTEL
; CHECK-SPIRV-OFF-NOT: OpDecorate %[[su]] FPFastMathMode AllowReassocINTEL

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv32-unknown-unknown"

; Function Attrs: convergent noinline norecurse nounwind optnone
define spir_kernel void @test(float %a, float %b) #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %a.addr = alloca float, align 4
  %b.addr = alloca float, align 4
  store float %a, float* %a.addr, align 4
  store float %b, float* %b.addr, align 4
  %0 = load float, float* %a.addr, align 4
  %1 = load float, float* %a.addr, align 4
  %mul = fmul contract float %0, %1
  store float %mul, float* %b.addr, align 4
  %2 = load float, float* %b.addr, align 4
  %3 = load float, float* %b.addr, align 4
  %sub = fsub reassoc float %2, %3
  store float %sub, float* %b.addr, align 4
  ret void
}

attributes #0 = { convergent noinline norecurse nounwind optnone "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 12.0.0 (https://github.com/intel/llvm.git 5cf8088c994778561c8584d5433d7d32618725b2)"}
!3 = !{i32 0, i32 0}
!4 = !{!"none", !"none"}
!5 = !{!"float", !"float"}
!6 = !{!"", !""}
