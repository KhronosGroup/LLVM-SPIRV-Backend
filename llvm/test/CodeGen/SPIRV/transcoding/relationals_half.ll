; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: %[[BoolTypeID:[0-9]+]] = OpTypeBool
; CHECK-SPIRV: %[[BoolVectorTypeID:[0-9]+]] = OpTypeVector %[[BoolTypeID]] 2

; CHECK-SPIRV: OpIsFinite %[[BoolTypeID]]
; CHECK-SPIRV: OpIsNan %[[BoolTypeID]]
; CHECK-SPIRV: OpIsInf %[[BoolTypeID]]
; CHECK-SPIRV: OpIsNormal %[[BoolTypeID]]
; CHECK-SPIRV: OpSignBitSet %[[BoolTypeID]]
; CHECK-SPIRV: OpLessOrGreater %[[BoolTypeID]]
; CHECK-SPIRV: OpOrdered %[[BoolTypeID]]
; CHECK-SPIRV: OpUnordered %[[BoolTypeID]]

; CHECK-SPIRV: OpIsFinite %[[BoolVectorTypeID]]
; CHECK-SPIRV: OpIsNan %[[BoolVectorTypeID]]
; CHECK-SPIRV: OpIsInf %[[BoolVectorTypeID]]
; CHECK-SPIRV: OpIsNormal %[[BoolVectorTypeID]]
; CHECK-SPIRV: OpLessOrGreater %[[BoolVectorTypeID]]
; CHECK-SPIRV: OpOrdered %[[BoolVectorTypeID]]
; CHECK-SPIRV: OpUnordered %[[BoolVectorTypeID]]

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @test_scalar(i32 addrspace(1)* nocapture %out, half %f) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !3 !kernel_arg_base_type !4 !kernel_arg_type_qual !5 {
entry:
  %call = tail call spir_func i32 @_Z8isfiniteDh(half %f) #2
  %call1 = tail call spir_func i32 @_Z5isnanDh(half %f) #2
  %add = add nsw i32 %call1, %call
  %call2 = tail call spir_func i32 @_Z5isinfDh(half %f) #2
  %add3 = add nsw i32 %add, %call2
  %call4 = tail call spir_func i32 @_Z8isnormalDh(half %f) #2
  %add5 = add nsw i32 %add3, %call4
  %call6 = tail call spir_func i32 @_Z7signbitDh(half %f) #2
  %add7 = add nsw i32 %add5, %call6
  %call8 = tail call spir_func i32 @_Z13islessgreaterDhDh(half %f, half %f) #2
  %add9 = add nsw i32 %add7, %call8
  %call10 = tail call spir_func i32 @_Z9isorderedDhDh(half %f, half %f) #2
  %add11 = add nsw i32 %add9, %call10
  %call12 = tail call spir_func i32 @_Z11isunorderedDhDh(half %f, half %f) #2
  %add13 = add nsw i32 %add11, %call12
  store i32 %add13, i32 addrspace(1)* %out, align 4
  ret void
}

declare spir_func i32 @_Z8isfiniteDh(half) #1

declare spir_func i32 @_Z5isnanDh(half) #1

declare spir_func i32 @_Z5isinfDh(half) #1

declare spir_func i32 @_Z8isnormalDh(half) #1

declare spir_func i32 @_Z7signbitDh(half) #1

declare spir_func i32 @_Z13islessgreaterDhDh(half, half) #1

declare spir_func i32 @_Z9isorderedDhDh(half, half) #1

declare spir_func i32 @_Z11isunorderedDhDh(half, half) #1

; Function Attrs: nounwind
define spir_kernel void @test_vector(<2 x i16> addrspace(1)* nocapture %out, <2 x half> %f) #0 !kernel_arg_addr_space !1 !kernel_arg_access_qual !2 !kernel_arg_type !7 !kernel_arg_base_type !8 !kernel_arg_type_qual !5 {
entry:
  %call = tail call spir_func <2 x i16> @_Z8isfiniteDv2_Dh(<2 x half> %f) #2
  %call1 = tail call spir_func <2 x i16> @_Z5isnanDv2_Dh(<2 x half> %f) #2
  %add = add <2 x i16> %call, %call1
  %call2 = tail call spir_func <2 x i16> @_Z5isinfDv2_Dh(<2 x half> %f) #2
  %add3 = add <2 x i16> %add, %call2
  %call4 = tail call spir_func <2 x i16> @_Z8isnormalDv2_Dh(<2 x half> %f) #2
  %add5 = add <2 x i16> %add3, %call4
  %call6 = tail call spir_func <2 x i16> @_Z13islessgreaterDv2_DhS_(<2 x half> %f, <2 x half> %f) #2
  %add7 = add <2 x i16> %add5, %call6
  %call8 = tail call spir_func <2 x i16> @_Z9isorderedDv2_DhS_(<2 x half> %f, <2 x half> %f) #2
  %add9 = add <2 x i16> %add7, %call8
  %call10 = tail call spir_func <2 x i16> @_Z11isunorderedDv2_DhS_(<2 x half> %f, <2 x half> %f) #2
  %add11 = add <2 x i16> %add9, %call10
  store <2 x i16> %add11, <2 x i16> addrspace(1)* %out, align 8
  ret void
}

declare spir_func <2 x i16> @_Z8isfiniteDv2_Dh(<2 x half>) #1

declare spir_func <2 x i16> @_Z5isnanDv2_Dh(<2 x half>) #1

declare spir_func <2 x i16> @_Z5isinfDv2_Dh(<2 x half>) #1

declare spir_func <2 x i16> @_Z8isnormalDv2_Dh(<2 x half>) #1

declare spir_func <2 x i16> @_Z13islessgreaterDv2_DhS_(<2 x half>, <2 x half>) #1

declare spir_func <2 x i16> @_Z9isorderedDv2_DhS_(<2 x half>, <2 x half>) #1

declare spir_func <2 x i16> @_Z11isunorderedDv2_DhS_(<2 x half>, <2 x half>) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!9}
!opencl.ocl.version = !{!10}
!opencl.used.extensions = !{!11}
!opencl.used.optional.core.features = !{!11}
!opencl.compiler.options = !{!11}

!1 = !{i32 1, i32 0}
!2 = !{!"none", !"none"}
!3 = !{!"int*", !"half"}
!4 = !{!"int*", !"half"}
!5 = !{!"", !""}
!7 = !{!"short2*", !"half2"}
!8 = !{!"short2*", !"half2"}
!9 = !{i32 1, i32 2}
!10 = !{i32 2, i32 0}
!11 = !{}
