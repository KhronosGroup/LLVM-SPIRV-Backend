; ModuleID = ''
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; Check 'LLVM ==> SPIR-V' conversion of extractvalue/insertvalue.

%struct.arr = type { [7 x float] }
%struct.st = type { %struct.inner }
%struct.inner = type { float }

; CHECK-SPIRV: %[[float_ty:[0-9]+]] = OpTypeFloat 32
; FIXME: properly order array type decl & num elems constant
; CHECK-SPIRV: %[[array_ty:[0-9]+]] = OpTypeArray %[[float_ty]]
; CHECK-SPIRV: %[[struct_ty:[0-9]+]] = OpTypeStruct %[[array_ty]]
; CHECK-SPIRV-DAG: %[[array_ptr_ty:[0-9]+]] = OpTypePointer CrossWorkgroup %[[array_ty]]
; CHECK-SPIRV-DAG: %[[struct_ptr_ty:[0-9]+]] = OpTypePointer CrossWorkgroup %[[struct_ty]]

; CHECK-SPIRV: %[[struct1_in_ty:[0-9]+]] = OpTypeStruct %[[float_ty]]
; CHECK-SPIRV: %[[struct1_ty:[0-9]+]] = OpTypeStruct %[[struct1_in_ty]]
; CHECK-SPIRV-DAG: %[[struct1_in_ptr_ty:[0-9]+]] = OpTypePointer CrossWorkgroup %[[struct1_in_ty]]
; CHECK-SPIRV-DAG: %[[struct1_ptr_ty:[0-9]+]] = OpTypePointer CrossWorkgroup %[[struct1_ty]]

; CHECK-SPIRV-LABEL:  OpFunction
; CHECK-SPIRV-NEXT:   %[[object:[0-9]+]] = OpFunctionParameter %[[struct_ptr_ty]]
; CHECK-SPIRV:        %[[store_ptr:[0-9]+]] = OpInBoundsPtrAccessChain %[[array_ptr_ty]] %[[object]] %{{[0-9]+}} %{{[0-9]+}}
; CHECK-SPIRV:        %[[extracted_array:[0-9]+]] = OpLoad %[[array_ty]] %[[store_ptr]] Aligned 4
; CHECK-SPIRV:        %[[elem_4:[0-9]+]] = OpCompositeExtract %[[float_ty]] %[[extracted_array]] 4
; CHECK-SPIRV:        %[[elem_2:[0-9]+]] = OpCompositeExtract %[[float_ty]] %[[extracted_array]] 2
; CHECK-SPIRV:        %[[add:[0-9]+]] = OpFAdd %[[float_ty]] %[[elem_4]] %[[elem_2]]
; CHECK-SPIRV:        %[[inserted_array:[0-9]+]] = OpCompositeInsert %[[array_ty]] %[[add]] %[[extracted_array]] 5
; CHECK-SPIRV:        OpStore %[[store_ptr]] %[[inserted_array]]
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
; CHECK-SPIRV-NEXT:   %[[object:[0-9]+]] = OpFunctionParameter %[[struct1_ptr_ty]]
; CHECK-SPIRV:        %[[store1_ptr:[0-9]+]] = OpInBoundsPtrAccessChain %[[struct1_in_ptr_ty]] %[[object]] %{{[0-9]+}} %{{[0-9]+}}
; CHECK-SPIRV:        %[[extracted_struct:[0-9]+]] = OpLoad %[[struct1_in_ty]] %[[store1_ptr]] Aligned 4
; CHECK-SPIRV:        %[[elem:[0-9]+]] = OpCompositeExtract %[[float_ty]] %[[extracted_struct]] 0
; CHECK-SPIRV:        %[[add:[0-9]+]] = OpFAdd %[[float_ty]] %[[elem]] %{{[0-9]+}}
; CHECK-SPIRV:        %[[inserted_struct:[0-9]+]] = OpCompositeInsert %[[struct1_in_ty]] %[[add]] %[[extracted_struct]] 0
; CHECK-SPIRV:        OpStore %[[store1_ptr]] %[[inserted_struct]]
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
