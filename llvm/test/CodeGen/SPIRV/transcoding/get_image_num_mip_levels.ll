; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; Generated from the following OpenCL C code:
; #pragma OPENCL EXTENSION cl_khr_mipmap_image : enable
; void test(image1d_t img1, 
;           image2d_t img2,
;           image3d_t img3,
;           image1d_array_t img4,
;           image2d_array_t img5,
;           image2d_depth_t img6,
;           image2d_array_depth_t img7)
; {
;     get_image_num_mip_levels(img1);
;     get_image_num_mip_levels(img2);
;     get_image_num_mip_levels(img3);
;     get_image_num_mip_levels(img4);
;     get_image_num_mip_levels(img5);
;     get_image_num_mip_levels(img6);
;     get_image_num_mip_levels(img7);
; }

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

%opencl.image1d_ro_t = type opaque
%opencl.image2d_ro_t = type opaque
%opencl.image3d_ro_t = type opaque
%opencl.image1d_array_ro_t = type opaque
%opencl.image2d_array_ro_t = type opaque
%opencl.image2d_depth_ro_t = type opaque
%opencl.image2d_array_depth_ro_t = type opaque

; CHECK-SPIRV: %[[INT:[0-9]+]] = OpTypeInt 32
; CHECK-SPIRV: %[[VOID:[0-9]+]] = OpTypeVoid
; CHECK-SPIRV: %[[IMAGE1D_T:[0-9]+]] = OpTypeImage %[[VOID]] 1D 0 0 0 0 Unknown ReadOnly
; CHECK-SPIRV: %[[IMAGE2D_T:[0-9]+]] = OpTypeImage %[[VOID]] 2D 0 0 0 0 Unknown ReadOnly
; CHECK-SPIRV: %[[IMAGE3D_T:[0-9]+]] = OpTypeImage %[[VOID]] 3D 0 0 0 0 Unknown ReadOnly
; CHECK-SPIRV: %[[IMAGE1D_ARRAY_T:[0-9]+]] = OpTypeImage %[[VOID]] 1D 0 1 0 0 Unknown ReadOnly
; CHECK-SPIRV: %[[IMAGE2D_ARRAY_T:[0-9]+]] = OpTypeImage %[[VOID]] 2D 0 1 0 0 Unknown ReadOnly
; CHECK-SPIRV: %[[IMAGE2D_DEPTH_T:[0-9]+]] = OpTypeImage %[[VOID]] 2D 1 0 0 0 Unknown ReadOnly
; CHECK-SPIRV: %[[IMAGE2D_ARRAY_DEPTH_T:[0-9]+]] = OpTypeImage %[[VOID]] 2D 1 1 0 0 Unknown ReadOnly

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_func void @testimage1d(%opencl.image1d_ro_t addrspace(1)* %img1, %opencl.image2d_ro_t addrspace(1)* %img2, %opencl.image3d_ro_t addrspace(1)* %img3, %opencl.image1d_array_ro_t addrspace(1)* %img4, %opencl.image2d_array_ro_t addrspace(1)* %img5, %opencl.image2d_depth_ro_t addrspace(1)* %img6, %opencl.image2d_array_depth_ro_t addrspace(1)* %img7) #0 {
entry:
  %img1.addr = alloca %opencl.image1d_ro_t addrspace(1)*, align 4
  %img2.addr = alloca %opencl.image2d_ro_t addrspace(1)*, align 4
  %img3.addr = alloca %opencl.image3d_ro_t addrspace(1)*, align 4
  %img4.addr = alloca %opencl.image1d_array_ro_t addrspace(1)*, align 4
  %img5.addr = alloca %opencl.image2d_array_ro_t addrspace(1)*, align 4
  %img6.addr = alloca %opencl.image2d_depth_ro_t addrspace(1)*, align 4
  %img7.addr = alloca %opencl.image2d_array_depth_ro_t addrspace(1)*, align 4
  store %opencl.image1d_ro_t addrspace(1)* %img1, %opencl.image1d_ro_t addrspace(1)** %img1.addr, align 4
  store %opencl.image2d_ro_t addrspace(1)* %img2, %opencl.image2d_ro_t addrspace(1)** %img2.addr, align 4
  store %opencl.image3d_ro_t addrspace(1)* %img3, %opencl.image3d_ro_t addrspace(1)** %img3.addr, align 4
  store %opencl.image1d_array_ro_t addrspace(1)* %img4, %opencl.image1d_array_ro_t addrspace(1)** %img4.addr, align 4
  store %opencl.image2d_array_ro_t addrspace(1)* %img5, %opencl.image2d_array_ro_t addrspace(1)** %img5.addr, align 4
  store %opencl.image2d_depth_ro_t addrspace(1)* %img6, %opencl.image2d_depth_ro_t addrspace(1)** %img6.addr, align 4
  store %opencl.image2d_array_depth_ro_t addrspace(1)* %img7, %opencl.image2d_array_depth_ro_t addrspace(1)** %img7.addr, align 4
  %0 = load %opencl.image1d_ro_t addrspace(1)*, %opencl.image1d_ro_t addrspace(1)** %img1.addr, align 4
  %call = call spir_func i32 @_Z24get_image_num_mip_levels14ocl_image1d_ro(%opencl.image1d_ro_t addrspace(1)* %0) #2
  %1 = load %opencl.image2d_ro_t addrspace(1)*, %opencl.image2d_ro_t addrspace(1)** %img2.addr, align 4
  %call1 = call spir_func i32 @_Z24get_image_num_mip_levels14ocl_image2d_ro(%opencl.image2d_ro_t addrspace(1)* %1) #2
  %2 = load %opencl.image3d_ro_t addrspace(1)*, %opencl.image3d_ro_t addrspace(1)** %img3.addr, align 4
  %call2 = call spir_func i32 @_Z24get_image_num_mip_levels14ocl_image3d_ro(%opencl.image3d_ro_t addrspace(1)* %2) #2
  %3 = load %opencl.image1d_array_ro_t addrspace(1)*, %opencl.image1d_array_ro_t addrspace(1)** %img4.addr, align 4
  %call3 = call spir_func i32 @_Z24get_image_num_mip_levels20ocl_image1d_array_ro(%opencl.image1d_array_ro_t addrspace(1)* %3) #2
  %4 = load %opencl.image2d_array_ro_t addrspace(1)*, %opencl.image2d_array_ro_t addrspace(1)** %img5.addr, align 4
  %call4 = call spir_func i32 @_Z24get_image_num_mip_levels20ocl_image2d_array_ro(%opencl.image2d_array_ro_t addrspace(1)* %4) #2
  %5 = load %opencl.image2d_depth_ro_t addrspace(1)*, %opencl.image2d_depth_ro_t addrspace(1)** %img6.addr, align 4
  %call5 = call spir_func i32 @_Z24get_image_num_mip_levels20ocl_image2d_depth_ro(%opencl.image2d_depth_ro_t addrspace(1)* %5) #2
  %6 = load %opencl.image2d_array_depth_ro_t addrspace(1)*, %opencl.image2d_array_depth_ro_t addrspace(1)** %img7.addr, align 4
  %call6 = call spir_func i32 @_Z24get_image_num_mip_levels26ocl_image2d_array_depth_ro(%opencl.image2d_array_depth_ro_t addrspace(1)* %6) #2
  ret void
}

; CHECK-SPIRV: %[[IMAGE1D:[0-9]+]] = OpLoad %[[IMAGE1D_T]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQueryLevels %[[INT]] %[[IMAGE1D]]
; CHECK-SPIRV: %[[IMAGE2D:[0-9]+]] = OpLoad %[[IMAGE2D_T]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQueryLevels %[[INT]] %[[IMAGE2D]]
; CHECK-SPIRV: %[[IMAGE3D:[0-9]+]] = OpLoad %[[IMAGE3D_T]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQueryLevels %[[INT]] %[[IMAGE3D]]
; CHECK-SPIRV: %[[IMAGE1D_ARRAY:[0-9]+]] = OpLoad %[[IMAGE1D_ARRAY_T]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQueryLevels %[[INT]] %[[IMAGE1D_ARRAY]]
; CHECK-SPIRV: %[[IMAGE2D_ARRAY:[0-9]+]] = OpLoad %[[IMAGE2D_ARRAY_T]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQueryLevels %[[INT]] %[[IMAGE2D_ARRAY]]
; CHECK-SPIRV: %[[IMAGE2D_DEPTH:[0-9]+]] = OpLoad %[[IMAGE2D_DEPTH_T]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQueryLevels %[[INT]] %[[IMAGE2D_DEPTH]]
; CHECK-SPIRV: %[[IMAGE2D_ARRAY_DEPTH:[0-9]+]] = OpLoad %[[IMAGE2D_ARRAY_DEPTH_T]]
; CHECK-SPIRV: %{{[0-9]+}} = OpImageQueryLevels %[[INT]] %[[IMAGE2D_ARRAY_DEPTH]]

; Function Attrs: convergent
declare spir_func i32 @_Z24get_image_num_mip_levels14ocl_image1d_ro(%opencl.image1d_ro_t addrspace(1)*) #1

; Function Attrs: convergent
declare spir_func i32 @_Z24get_image_num_mip_levels14ocl_image2d_ro(%opencl.image2d_ro_t addrspace(1)*) #1

; Function Attrs: convergent
declare spir_func i32 @_Z24get_image_num_mip_levels14ocl_image3d_ro(%opencl.image3d_ro_t addrspace(1)*) #1

; Function Attrs: convergent
declare spir_func i32 @_Z24get_image_num_mip_levels20ocl_image1d_array_ro(%opencl.image1d_array_ro_t addrspace(1)*) #1

; Function Attrs: convergent
declare spir_func i32 @_Z24get_image_num_mip_levels20ocl_image2d_array_ro(%opencl.image2d_array_ro_t addrspace(1)*) #1

; Function Attrs: convergent
declare spir_func i32 @_Z24get_image_num_mip_levels20ocl_image2d_depth_ro(%opencl.image2d_depth_ro_t addrspace(1)*) #1

; Function Attrs: convergent
declare spir_func i32 @_Z24get_image_num_mip_levels26ocl_image2d_array_depth_ro(%opencl.image2d_array_depth_ro_t addrspace(1)*) #1

attributes #0 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
