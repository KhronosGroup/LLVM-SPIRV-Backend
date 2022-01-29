; RUN: llc -O0 %s -o - | FileCheck %s

target triple = "spirv64-unknown-unknown"

; CHECK: OpEntryPoint Kernel %[[f1:[0-9]+]] "writer" %[[var:[0-9]+]]
; CHECK: OpEntryPoint Kernel %[[f2:[0-9]+]] "reader" %[[var]]
; CHECK: OpName %[[var]] "var"
@var = addrspace(1) global i8 0, align 1
@g_var = addrspace(1) global i8 1, align 1
@a_var = addrspace(1) global [2 x i8] c"\01\01", align 1
@p_var = addrspace(1) global i8 addrspace(1)* getelementptr inbounds ([2 x i8], [2 x i8] addrspace(1)* @a_var, i32 0, i64 1), align 8

; Function Attrs: nounwind
define spir_func zeroext i8 @from_buf(i8 zeroext %a) #0 {
entry:
  %tobool = icmp ne i8 %a, 0
  %i1promo = zext i1 %tobool to i8
  ret i8 %i1promo
}

; Function Attrs: nounwind
define spir_func zeroext i8 @to_buf(i8 zeroext %a) #0 {
entry:
  %i1trunc = trunc i8 %a to i1
  %frombool = select i1 %i1trunc, i8 1, i8 0
  %0 = and i8 %frombool, 1
  %tobool = icmp ne i8 %0, 0
  %conv = select i1 %tobool, i8 1, i8 0
  ret i8 %conv
}

; Function Attrs: nounwind
define spir_kernel void @writer(i8 addrspace(1)* %src, i32 %idx) #0 {
entry:
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 0
  %0 = load i8, i8 addrspace(1)* %arrayidx, align 1
  %call = call spir_func zeroext i8 @from_buf(i8 zeroext %0) #0
  %i1trunc = trunc i8 %call to i1
  %frombool = select i1 %i1trunc, i8 1, i8 0
  store i8 %frombool, i8 addrspace(1)* @var, align 1
  %arrayidx1 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 1
  %1 = load i8, i8 addrspace(1)* %arrayidx1, align 1
  %call2 = call spir_func zeroext i8 @from_buf(i8 zeroext %1) #0
  %i1trunc1 = trunc i8 %call2 to i1
  %frombool3 = select i1 %i1trunc1, i8 1, i8 0
  store i8 %frombool3, i8 addrspace(1)* @g_var, align 1
  %arrayidx4 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 2
  %2 = load i8, i8 addrspace(1)* %arrayidx4, align 1
  %call5 = call spir_func zeroext i8 @from_buf(i8 zeroext %2) #0
  %i1trunc2 = trunc i8 %call5 to i1
  %frombool6 = select i1 %i1trunc2, i8 1, i8 0
  %3 = getelementptr inbounds [2 x i8], [2 x i8] addrspace(1)* @a_var, i64 0, i64 0
  store i8 %frombool6, i8 addrspace(1)* %3, align 1
  %arrayidx7 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 3
  %4 = load i8, i8 addrspace(1)* %arrayidx7, align 1
  %call8 = call spir_func zeroext i8 @from_buf(i8 zeroext %4) #0
  %i1trunc3 = trunc i8 %call8 to i1
  %frombool9 = select i1 %i1trunc3, i8 1, i8 0
  %5 = getelementptr inbounds [2 x i8], [2 x i8] addrspace(1)* @a_var, i64 0, i64 1
  store i8 %frombool9, i8 addrspace(1)* %5, align 1
  %idx.ext = zext i32 %idx to i64
  %add.ptr = getelementptr inbounds i8, i8 addrspace(1)* %3, i64 %idx.ext
  store i8 addrspace(1)* %add.ptr, i8 addrspace(1)* addrspace(1)* @p_var, align 8
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @reader(i8 addrspace(1)* %dest, i8 zeroext %ptr_write_val) #0 {
entry:
  %call = call spir_func zeroext i8 @from_buf(i8 zeroext %ptr_write_val) #0
  %i1trunc = trunc i8 %call to i1
  %0 = load i8 addrspace(1)*, i8 addrspace(1)* addrspace(1)* @p_var, align 8
  %frombool = select i1 %i1trunc, i8 1, i8 0
  store volatile i8 %frombool, i8 addrspace(1)* %0, align 1
  %1 = load i8, i8 addrspace(1)* @var, align 1
  %2 = and i8 %1, 1
  %tobool = icmp ne i8 %2, 0
  %i1promo = zext i1 %tobool to i8
  %call1 = call spir_func zeroext i8 @to_buf(i8 zeroext %i1promo) #0
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %dest, i64 0
  store i8 %call1, i8 addrspace(1)* %arrayidx, align 1
  %3 = load i8, i8 addrspace(1)* @g_var, align 1
  %4 = and i8 %3, 1
  %tobool2 = icmp ne i8 %4, 0
  %i1promo1 = zext i1 %tobool2 to i8
  %call3 = call spir_func zeroext i8 @to_buf(i8 zeroext %i1promo1) #0
  %arrayidx4 = getelementptr inbounds i8, i8 addrspace(1)* %dest, i64 1
  store i8 %call3, i8 addrspace(1)* %arrayidx4, align 1
  %5 = getelementptr inbounds [2 x i8], [2 x i8] addrspace(1)* @a_var, i64 0, i64 0
  %6 = load i8, i8 addrspace(1)* %5, align 1
  %7 = and i8 %6, 1
  %tobool5 = icmp ne i8 %7, 0
  %i1promo2 = zext i1 %tobool5 to i8
  %call6 = call spir_func zeroext i8 @to_buf(i8 zeroext %i1promo2) #0
  %arrayidx7 = getelementptr inbounds i8, i8 addrspace(1)* %dest, i64 2
  store i8 %call6, i8 addrspace(1)* %arrayidx7, align 1
  %8 = getelementptr inbounds [2 x i8], [2 x i8] addrspace(1)* @a_var, i64 0, i64 1
  %9 = load i8, i8 addrspace(1)* %8, align 1
  %10 = and i8 %9, 1
  %tobool8 = icmp ne i8 %10, 0
  %i1promo3 = zext i1 %tobool8 to i8
  %call9 = call spir_func zeroext i8 @to_buf(i8 zeroext %i1promo3) #0
  %arrayidx10 = getelementptr inbounds i8, i8 addrspace(1)* %dest, i64 3
  store i8 %call9, i8 addrspace(1)* %arrayidx10, align 1
  ret void
}

attributes #0 = { nounwind }

!opencl.kernels = !{!0, !7}
!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!172}
!opencl.ocl.version = !{!172}
!opencl.used.extensions = !{!173}
!opencl.used.optional.core.features = !{!173}
!opencl.compiler.options = !{!174}

!0 = !{void (i8 addrspace(1)*, i32)* @writer, !1, !2, !3, !4, !5, !6}
!1 = !{!"kernel_arg_addr_space", i32 1, i32 0}
!2 = !{!"kernel_arg_access_qual", !"none", !"none"}
!3 = !{!"kernel_arg_type", !"uchar*", !"uint"}
!4 = !{!"kernel_arg_type_qual", !"", !""}
!5 = !{!"kernel_arg_base_type", !"char*", !"int"}
!6 = !{!"kernel_arg_name", !"src", !"idx"}
!7 = !{void (i8 addrspace(1)*, i8)* @reader, !1, !2, !8, !4, !9, !10}
!8 = !{!"kernel_arg_type", !"uchar*", !"uchar"}
!9 = !{!"kernel_arg_base_type", !"char*", !"uchar"}
!10 = !{!"kernel_arg_name", !"dest", !"ptr_write_val"}
!172 = !{i32 2, i32 0}
!173 = !{}
!174 = !{!"-cl-std=CL2.0"}
