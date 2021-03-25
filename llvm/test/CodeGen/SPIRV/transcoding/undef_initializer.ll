; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%"struct.my_struct" = type { i8 }

@__const.struct = private unnamed_addr addrspace(2) constant %"struct.my_struct" undef, align 1

; Function Attrs: nounwind readnone
define spir_kernel void @k() #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !0 !kernel_arg_type !0 !kernel_arg_base_type !0 !kernel_arg_type_qual !0 {
entry:
  ret void
}

; CHECK-SPIRV: %[[UNDEF_ID:[0-9]+]] = OpUndef %{{[0-9]+}}
; CHECK-SPIRV: %[[CC_ID:[0-9]+]] = OpConstantComposite %{{[0-9]+}} %[[UNDEF_ID]]
; CHECK-SPIRV: %{{[0-9]+}} = OpVariable %{{[0-9]+}} %{{[0-9]+}} %[[CC_ID]]

!llvm.module.flags = !{!1}
!opencl.ocl.version = !{!2}
!opencl.spir.version = !{!3}

!0 = !{}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 1, i32 0}
!3 = !{i32 1, i32 2}
