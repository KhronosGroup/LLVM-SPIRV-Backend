; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

declare dso_local spir_func <4 x i8> @_Z13__spirv_IsNanIDv4_aDv4_fET_T0_(<4 x float>)
declare dso_local spir_func <4 x i8> @_Z13__spirv_IsInfIDv4_aDv4_fET_T0_(<4 x float>)
declare dso_local spir_func <4 x i8> @_Z16__spirv_IsFiniteIDv4_aDv4_fET_T0_(<4 x float>)
declare dso_local spir_func <4 x i8> @_Z16__spirv_IsNormalIDv4_aDv4_fET_T0_(<4 x float>)
declare dso_local spir_func <4 x i8> @_Z18__spirv_SignBitSetIDv4_aDv4_fET_T0_(<4 x float>)

; CHECK-SPIRV: %[[TBool:[0-9]+]] = OpTypeBool
; CHECK-SPIRV: %[[TBoolVec:[0-9]+]] = OpTypeVector %[[TBool]]

; Function Attrs: nounwind readnone
define spir_kernel void @k() #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
entry:
  %arg1 = alloca <4 x float>, align 16
  %ret = alloca <4 x i8>, align 4
  %0 = load <4 x float>, <4 x float>* %arg1, align 16
  %call1 = call spir_func <4 x i8> @_Z13__spirv_IsNanIDv4_aDv4_fET_T0_(<4 x float> %0)
; CHECK-SPIRV: %[[IsNanRes:[0-9]+]] = OpIsNan %[[TBoolVec]]
; CHECK-SPIRV: %[[SelectRes:[0-9]+]] = OpSelect %{{[0-9]+}} %[[IsNanRes]]
; CHECK-SPIRV: OpStore %{{[0-9]+}} %[[SelectRes]]
  store <4 x i8> %call1, <4 x i8>* %ret, align 4
  %call2 = call spir_func <4 x i8> @_Z13__spirv_IsInfIDv4_aDv4_fET_T0_(<4 x float> %0)
; CHECK-SPIRV: %[[IsInfRes:[0-9]+]] = OpIsInf %[[TBoolVec]]
; CHECK-SPIRV: %[[Select1Res:[0-9]+]] = OpSelect %{{[0-9]+}} %[[IsInfRes]]
; CHECK-SPIRV: OpStore %{{[0-9]+}} %[[Select1Res]]
  store <4 x i8> %call2, <4 x i8>* %ret, align 4
  %call3 = call spir_func <4 x i8> @_Z16__spirv_IsFiniteIDv4_aDv4_fET_T0_(<4 x float> %0)
; CHECK-SPIRV: %[[IsFiniteRes:[0-9]+]] = OpIsFinite %[[TBoolVec]]
; CHECK-SPIRV: %[[Select2Res:[0-9]+]] = OpSelect %{{[0-9]+}} %[[IsFiniteRes]]
; CHECK-SPIRV: OpStore %{{[0-9]+}} %[[Select2Res]]
  store <4 x i8> %call3, <4 x i8>* %ret, align 4
  %call4 = call spir_func <4 x i8> @_Z16__spirv_IsNormalIDv4_aDv4_fET_T0_(<4 x float> %0)
; CHECK-SPIRV: %[[IsNormalRes:[0-9]+]] = OpIsNormal %[[TBoolVec]]
; CHECK-SPIRV: %[[Select3Res:[0-9]+]] = OpSelect %{{[0-9]+}} %[[IsNormalRes]]
; CHECK-SPIRV: OpStore %{{[0-9]+}} %[[Select3Res]]
  store <4 x i8> %call4, <4 x i8>* %ret, align 4
  %call5 = call spir_func <4 x i8> @_Z18__spirv_SignBitSetIDv4_aDv4_fET_T0_(<4 x float> %0)
; CHECK-SPIRV: %[[SignBitSetRes:[0-9]+]] = OpSignBitSet %[[TBoolVec]]
; CHECK-SPIRV: %[[Select4Res:[0-9]+]] = OpSelect %{{[0-9]+}} %[[SignBitSetRes]]
; CHECK-SPIRV: OpStore %{{[0-9]+}} %[[Select4Res]]
  store <4 x i8> %call5, <4 x i8>* %ret, align 4
  ret void
}

!llvm.module.flags = !{!1}
!opencl.spir.version = !{!2}

!0 = !{}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 1, i32 2}
