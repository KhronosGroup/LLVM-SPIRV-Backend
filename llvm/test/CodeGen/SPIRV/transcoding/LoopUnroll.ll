; Source:
; /*** for ***/
; void for_count()
; {
;     __attribute__((opencl_unroll_hint(1)))
;     for( int i = 0; i < 1024; ++i) {
;        if(i%2) continue;
;        int x = i;
;     }
; }
;
; /*** while ***/
; void while_count()
; {
;     int i = 1024;
;     __attribute__((opencl_unroll_hint(8)))
;     while(i-->0) {
;       if(i%2) continue;
;       int x = i;
;     }
; }
;
; /*** do ***/
; void do_count()
; {
;     int i = 1024;
;     __attribute__((opencl_unroll_hint))
;     do {
;       if(i%2) continue;
;       int x = i;
;    } while(i--> 0);
; }
;
; for_count_unusual() is a synthetically written function
;
; Command:
; clang -cc1 -triple spir64 -O0 LoopUnroll.cl -emit-llvm -o /test/SPIRV/transcoding/LoopUnroll.ll

; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV: OpFunction
; Function Attrs: noinline nounwind optnone
define spir_func void @for_count() #0 {
entry:
; CHECK-SPIRV: OpLabel
  %i = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
; CHECK-SPIRV: %[[#HEADER:]] = OpLabel
  %0 = load i32, i32* %i, align 4
  %cmp = icmp slt i32 %0, 1024
; Per SPIRV spec p3.23 "DontUnroll" loop control = 0x2
; FIXME: LoopMerge's format
; CHECK-SPIRV: OpLoopMerge %[[#MERGEBLOCK:]] %[[#CONTINUE:]] DontUnroll
; CHECK-SPIRV: OpBranchConditional %[[#]] %[[#]] %[[#MERGEBLOCK]]
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
; CHECK-SPIRV: OpLabel
  %1 = load i32, i32* %i, align 4
  %rem = srem i32 %1, 2
  %tobool = icmp ne i32 %rem, 0
  br i1 %tobool, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
; CHECK-SPIRV: OpLabel
  br label %for.inc

if.end:                                           ; preds = %for.body
; CHECK-SPIRV: OpLabel
  %2 = load i32, i32* %i, align 4
  store i32 %2, i32* %x, align 4
  br label %for.inc

for.inc:                                          ; preds = %if.end, %if.then
; CHECK-SPIRV: %[[#CONTINUE]] = OpLabel
  %3 = load i32, i32* %i, align 4
  %inc = add nsw i32 %3, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond, !llvm.loop !5
; CHECK-SPIRV: OpBranch %[[#HEADER]]

for.end:                                          ; preds = %for.cond
; CHECK-SPIRV: %[[#MERGEBLOCK]] = OpLabel
  ret void
}

; CHECK-SPIRV: OpFunction
; Function Attrs: noinline nounwind optnone
define spir_func void @while_count() #0 {
entry:
; CHECK-SPIRV: OpLabel
  %i = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 1024, i32* %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %if.end, %if.then, %entry
; CHECK-SPIRV: %[[#HEADER:]] = OpLabel
  %0 = load i32, i32* %i, align 4
  %dec = add nsw i32 %0, -1
  store i32 %dec, i32* %i, align 4
  %cmp = icmp sgt i32 %0, 0
; Per SPIRV spec p3.23 "Unroll" loop control = 0x1
; FIXME: LoopMerge's format - what is "8"?
; CHECK-SPIRV: OpLoopMerge %[[#MERGEBLOCK:]] %[[#CONTINUE:]] PartialCount 8
; CHECK-SPIRV: OpBranchConditional %[[#]] %[[#]] %[[#MERGEBLOCK]]
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
; CHECK-SPIRV: OpLabel
  %1 = load i32, i32* %i, align 4
  %rem = srem i32 %1, 2
  %tobool = icmp ne i32 %rem, 0
  br i1 %tobool, label %if.then, label %if.end

if.then:                                          ; preds = %while.body
; CHECK-SPIRV: OpLabel
  br label %while.cond, !llvm.loop !7

; loop-simplify pass will create extra basic block which is the only one in
; loop having a back-edge to the header
; CHECK-SPIRV: %[[#CONTINUE]]
; CHECK-SPIRV: OpBranch %[[#HEADER]]

if.end:                                           ; preds = %while.body
; CHECK-SPIRV: OpLabel
  %2 = load i32, i32* %i, align 4
  store i32 %2, i32* %x, align 4
  br label %while.cond, !llvm.loop !7

while.end:                                        ; preds = %while.cond
; CHECK-SPIRV: %[[#MERGEBLOCK]]
  ret void
}

; CHECK-SPIRV: OpFunction
; Function Attrs: noinline nounwind optnone
define spir_func void @do_count() #0 {
entry:
; CHECK-SPIRV: OpLabel
  %i = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 1024, i32* %i, align 4
  br label %do.body

do.body:                                          ; preds = %do.cond, %entry
; CHECK-SPIRV: %[[#HEADER:]] = OpLabel
  %0 = load i32, i32* %i, align 4
  %rem = srem i32 %0, 2
  %tobool = icmp ne i32 %rem, 0
; Per SPIRV spec p3.23 "Unroll" loop control = 0x1
; FIXME: LoopMerge's format
; CHECK-SPIRV: OpLoopMerge %[[#MERGEBLOCK:]] %[[#CONTINUE:]] Unroll
; CHECK-SPIRV: OpBranchConditional
  br i1 %tobool, label %if.then, label %if.end

if.then:                                          ; preds = %do.body
; CHECK-SPIRV: OpLabel
  br label %do.cond

if.end:                                           ; preds = %do.body
; CHECK-SPIRV: OpLabel
  %1 = load i32, i32* %i, align 4
  store i32 %1, i32* %x, align 4
  br label %do.cond

do.cond:                                          ; preds = %if.end, %if.then
; CHECK-SPIRV: %[[#CONTINUE]] = OpLabel
  %2 = load i32, i32* %i, align 4
  %dec = add nsw i32 %2, -1
  store i32 %dec, i32* %i, align 4
  %cmp = icmp sgt i32 %2, 0
; CHECK-SPIRV: OpBranchConditional %[[#]] %[[#HEADER]] %[[#MERGEBLOCK]]
  br i1 %cmp, label %do.body, label %do.end, !llvm.loop !9

do.end:                                           ; preds = %do.cond
; CHECK-SPIRV: %[[#MERGEBLOCK]] = OpLabel
  ret void
}

; CHECK-SPIRV: OpFunction
; Function Attrs: noinline nounwind optnone
define spir_func void @for_count_unusual() #0 {
entry:
; CHECK-SPIRV: OpLabel
  %i = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 1024, i32* %i, align 4
  br label %for.body

for.body:                                          ; preds = %for.cond, %entry
; CHECK-SPIRV: %[[#HEADER:]] = OpLabel
  %0 = load i32, i32* %i, align 4
  %rem = srem i32 %0, 2
  %tobool = icmp ne i32 %rem, 0
; Per SPIRV spec p3.23 "Unroll" loop control = 0x1
; FIXME: LoopMerge's format
; CHECK-SPIRV: OpLoopMerge %[[#MERGEBLOCK:]] %[[#CONTINUE:]] Unroll
; CHECK-SPIRV: OpBranchConditional
  br i1 %tobool, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
; CHECK-SPIRV: OpLabel
  br label %for.cond

if.end:                                           ; preds = %for.body
; CHECK-SPIRV: OpLabel
  %1 = load i32, i32* %i, align 4
  store i32 %1, i32* %x, align 4
  br label %for.cond

for.cond:                                          ; preds = %if.end, %if.then
; CHECK-SPIRV: %[[#CONTINUE]] = OpLabel
  %2 = load i32, i32* %i, align 4
  %dec = add nsw i32 %2, -1
  store i32 %dec, i32* %i, align 4
  %cmp = icmp sgt i32 %2, 0
; CHECK-SPIRV: OpBranchConditional %[[#]] %[[#MERGEBLOCK]] %[[#HEADER]]
  br i1 %cmp, label %for.end, label %for.body, !llvm.loop !9

for.end:                                           ; preds = %for.cond
; CHECK-SPIRV: %[[#MERGEBLOCK]] = OpLabel
  ret void
}

attributes #0 = { noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!opencl.compiler.options = !{!3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{!"clang version 5.0.1 (cfe/trunk)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.unroll.disable"}
!7 = distinct !{!7, !8}
!8 = !{!"llvm.loop.unroll.count", i32 8}
!9 = distinct !{!9, !10}
!10 = !{!"llvm.loop.unroll.enable"}
