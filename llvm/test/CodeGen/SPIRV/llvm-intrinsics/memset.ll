; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: OpDecorate %[[#NonConstMemset:]] LinkageAttributes "spirv.llvm_memset_p3i8_i32"
; CHECK-SPIRV: %[[Int8:[0-9]+]] = OpTypeInt 8 0
; CHECK-SPIRV: %[[Lenmemset21:[0-9]+]] = OpConstant %{{[0-9]+}} 4
; CHECK-SPIRV: %[[Lenmemset0:[0-9]+]] = OpConstant %{{[0-9]+}} 12
; CHECK-SPIRV: %[[Const21:[0-9]+]] = OpConstant %{{[0-9]+}} 21
; CHECK-SPIRV: %[[Int8x4:[0-9]+]] = OpTypeArray %[[Int8]] %[[Lenmemset21]]
; CHECK-SPIRV: %[[Int8Ptr:[0-9]+]] = OpTypePointer Generic %[[Int8]]
; CHECK-SPIRV: %[[Int8x12:[0-9]+]] = OpTypeArray %[[Int8]] %[[Lenmemset0]]
; CHECK-SPIRV: %[[Int8PtrConst:[0-9]+]] = OpTypePointer UniformConstant %[[Int8]]

; CHECK-SPIRV: %[[Init:[0-9]+]] = OpConstantNull %[[Int8x12]]
; CHECK-SPIRV: %[[Val:[0-9]+]] = OpVariable %{{[0-9]+}} UniformConstant %[[Init]]
; CHECK-SPIRV: %[[InitComp:[0-9]+]] = OpConstantComposite %[[Int8x4]] %[[Const21]] %[[Const21]] %[[Const21]] %[[Const21]]
; CHECK-SPIRV: %[[ValComp:[0-9]+]] = OpVariable %{{[0-9]+}} UniformConstant %[[InitComp]]
; CHECK-SPIRV: %[[#False:]] = OpConstantFalse %[[#]]

; CHECK-SPIRV: %[[Target:[0-9]+]] = OpBitcast %[[Int8Ptr]] %{{[0-9]+}}
; CHECK-SPIRV: %[[Source:[0-9]+]] = OpBitcast %[[Int8PtrConst]] %[[Val]]
; CHECK-SPIRV: OpCopyMemorySized %[[Target]] %[[Source]] %[[Lenmemset0]] Aligned Nontemporal

; CHECK-SPIRV: %[[SourceComp:[0-9]+]] = OpBitcast %[[Int8PtrConst]] %[[ValComp]]
; CHECK-SPIRV: OpCopyMemorySized %{{[0-9]+}} %[[SourceComp]] %[[Lenmemset21]] Aligned Nontemporal

; CHECK-SPIRV: %[[#]] = OpFunctionCall %[[#]] %[[#NonConstMemset]] %[[#]] %[[#]] %[[#]] %[[#False]]

; CHECK-SPIRV: %[[#NonConstMemset]] = OpFunction %[[#]]
; CHECK-SPIRV: %[[#Dest:]] = OpFunctionParameter %[[#]]
; CHECK-SPIRV: %[[#Value:]] = OpFunctionParameter %[[#]]
; CHECK-SPIRV: %[[#Len:]] = OpFunctionParameter %[[#]]
; CHECK-SPIRV: %[[#Volatile:]] = OpFunctionParameter %[[#]]

; CHECK-SPIRV: %[[#Entry:]] = OpLabel
; CHECK-SPIRV: %[[#IsZeroLen:]] = OpIEqual %[[#]] %[[#Zero:]] %[[#Len]]
; CHECK-SPIRV: OpBranchConditional %[[#IsZeroLen]] %[[#End:]] %[[#WhileBody:]]

; CHECK-SPIRV: %[[#WhileBody]] = OpLabel
; CHECK-SPIRV: %[[#Offset:]] = OpPhi %[[#]] %[[#Zero]] %[[#Entry]] %[[#OffsetInc:]] %[[#WhileBody]]
; CHECK-SPIRV: %[[#Ptr:]] = OpInBoundsPtrAccessChain %[[#]] %[[#Dest]] %[[#Offset]]
; CHECK-SPIRV: OpStore %[[#Ptr]] %[[#Value]] Aligned 1
; CHECK-SPIRV: %[[#OffsetInc]] = OpIAdd %[[#]] %[[#Offset]] %[[#One:]]
; CHECK-SPIRV: %[[#NotEnd:]] = OpULessThan %[[#]] %[[#OffsetInc]] %[[#Len]]
; CHECK-SPIRV: OpBranchConditional %[[#NotEnd]] %[[#WhileBody]] %[[#End]]

; CHECK-SPIRV: %[[#End]] = OpLabel
; CHECK-SPIRV: OpReturn

; CHECK-SPIRV: OpFunctionEnd

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv32-unknown-unknown"

%struct.S1 = type { i32, i32, i32 }

; Function Attrs: nounwind
define spir_func void @_Z5foo11v(%struct.S1 addrspace(4)* noalias nocapture sret(%struct.S1 addrspace(4)*) %agg.result, i32 %s1, i64 %s2, i8 %v) #0 {
  %x = alloca [4 x i8]
  %x.bc = bitcast [4 x i8]* %x to i8*
  %1 = bitcast %struct.S1 addrspace(4)* %agg.result to i8 addrspace(4)*
  tail call void @llvm.memset.p4i8.i32(i8 addrspace(4)* align 4 %1, i8 0, i32 12, i1 false)
  tail call void @llvm.memset.p0i8.i32(i8* align 4 %x.bc, i8 21, i32 4, i1 false)

  ; non-const value
  tail call void @llvm.memset.p0i8.i32(i8* align 4 %x.bc, i8 %v, i32 3, i1 false)

  ; non-const value and size
  tail call void @llvm.memset.p0i8.i32(i8*  align 4 %x.bc, i8 %v, i32 %s1, i1 false)

  ; Address spaces, non-const value and size
  %a = addrspacecast i8 addrspace(4)* %1 to i8 addrspace(3)*
  tail call void @llvm.memset.p3i8.i32(i8 addrspace(3)* align 4 %a, i8 %v, i32 %s1, i1 false)
  %b = addrspacecast i8 addrspace(4)* %1 to i8 addrspace(1)*
  tail call void @llvm.memset.p1i8.i64(i8 addrspace(1)* align 4 %b, i8 %v, i64 %s2, i1 false)

  ; Volatile
  tail call void @llvm.memset.p1i8.i64(i8 addrspace(1)* align 4 %b, i8 %v, i64 %s2, i1 true)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.memset.p4i8.i32(i8 addrspace(4)* nocapture, i8, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i32(i8* nocapture, i8, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.memset.p3i8.i32(i8 addrspace(3)*, i8, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.memset.p1i8.i64(i8 addrspace(1)*, i8, i64, i1) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }

!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}
!llvm.ident = !{!3}
!spirv.Source = !{!4}
!spirv.String = !{!5}

!0 = !{i32 1, i32 2}
!1 = !{i32 2, i32 2}
!2 = !{}
!3 = !{!"clang version 3.6.1 "}
!4 = !{i32 4, i32 202000, !5}
!5 = !{!"llvm.memset.cl"}
