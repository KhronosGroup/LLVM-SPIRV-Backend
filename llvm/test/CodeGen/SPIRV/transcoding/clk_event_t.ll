; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'clk_event_t.cl'
source_filename = "clk_event_t.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpTypeDeviceEvent
; CHECK-SPIRV: OpFunction
; CHECK-SPIRV: OpCreateUserEvent
; CHECK-SPIRV: OpIsValidEvent
; CHECK-SPIRV: OpRetainEvent
; CHECK-SPIRV: OpSetUserEventStatus
; CHECK-SPIRV: OpCaptureEventProfilingInfo
; CHECK-SPIRV: OpReleaseEvent
; CHECK-SPIRV: OpFunctionEnd

; kernel void clk_event_t_test(global int *res, global void *prof) {
;   clk_event_t e1 = create_user_event();
;   *res = is_valid_event(e1);
;   retain_event(e1);
;   set_user_event_status(e1, -42);
;   capture_event_profiling_info(e1, CLK_PROFILING_COMMAND_EXEC_TIME, prof);
;   release_event(e1);
; }

%opencl.clk_event_t = type opaque

; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @clk_event_t_test(i32 addrspace(1)* nocapture noundef writeonly %res, i8 addrspace(1)* noundef %prof) local_unnamed_addr #0 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %call = call spir_func %opencl.clk_event_t* @_Z17create_user_eventv() #2
  %call1 = call spir_func zeroext i1 @_Z14is_valid_event12ocl_clkevent(%opencl.clk_event_t* %call) #2
  %conv = zext i1 %call1 to i32
  store i32 %conv, i32 addrspace(1)* %res, align 4, !tbaa !7
  call spir_func void @_Z12retain_event12ocl_clkevent(%opencl.clk_event_t* %call) #2
  call spir_func void @_Z21set_user_event_status12ocl_clkeventi(%opencl.clk_event_t* %call, i32 noundef -42) #2
  call spir_func void @_Z28capture_event_profiling_info12ocl_clkeventiPU3AS1v(%opencl.clk_event_t* %call, i32 noundef 1, i8 addrspace(1)* noundef %prof) #2
  call spir_func void @_Z13release_event12ocl_clkevent(%opencl.clk_event_t* %call) #2
  ret void
}

; Function Attrs: convergent
declare spir_func %opencl.clk_event_t* @_Z17create_user_eventv() local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func zeroext i1 @_Z14is_valid_event12ocl_clkevent(%opencl.clk_event_t*) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z12retain_event12ocl_clkevent(%opencl.clk_event_t*) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z21set_user_event_status12ocl_clkeventi(%opencl.clk_event_t*, i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z28capture_event_profiling_info12ocl_clkeventiPU3AS1v(%opencl.clk_event_t*, i32 noundef, i8 addrspace(1)* noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func void @_Z13release_event12ocl_clkevent(%opencl.clk_event_t*) local_unnamed_addr #1

attributes #0 = { convergent norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 1}
!4 = !{!"none", !"none"}
!5 = !{!"int*", !"void*"}
!6 = !{!"", !""}
!7 = !{!8, !8, i64 0}
!8 = !{!"int", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
