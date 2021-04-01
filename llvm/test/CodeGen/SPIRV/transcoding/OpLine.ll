;__kernel void foo(){}
;int bar(int a) {
;    a += 3;
;    int b = 10;
;    while (b) {
;        if (a > 4)
;            b = a - 1;
;        else  {
;            b = a + 1;
;            a = a *3;
;        }
;    }
;    return b;
;}
; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; CHECK-SPIRV: %[[str:[0-9]+]] = OpString "/tmp.cl"

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV: OpLine %[[str]] 1 0
; CHECK-SPIRV-NEXT: OpFunction
; Function Attrs: nounwind
define spir_kernel void @foo() #0 !kernel_arg_addr_space !2 !kernel_arg_access_qual !2 !kernel_arg_type !2 !kernel_arg_base_type !2 !kernel_arg_type_qual !2 {
entry:
ret void, !dbg !23
}
; CHECK-SPIRV: OpFunctionEnd

; CHECK-SPIRV: OpLine %[[str]] 2 0
; CHECK-SPIRV-NEXT: OpFunction
; Function Attrs: nounwind
define spir_func i32 @bar(i32 %a) #0 {
; CHECK-SPIRV: %[[entry:[0-9]+]] = OpLabel
entry:
  call void @llvm.dbg.value(metadata i32 %a, i64 0, metadata !24, metadata !25), !dbg !26
; CHECK-SPIRV: OpLine %[[str]] 3 0
; CHECK-SPIRV-NEXT: OpIAdd
  %add = add nsw i32 %a, 3, !dbg !27
  call void @llvm.dbg.value(metadata i32 %add, i64 0, metadata !24, metadata !25), !dbg !26
  call void @llvm.dbg.value(metadata i32 10, i64 0, metadata !28, metadata !25), !dbg !29
; CHECK-SPIRV: OpLine %[[str]] 5 0
; CHECK-SPIRV-NEXT: OpBranch %[[while_cond:[0-9]+]]
  br label %while.cond, !dbg !30

; CHECK-SPIRV: %[[while_cond]] = OpLabel
while.cond:                                       ; preds = %if.end, %entry
  %b.0 = phi i32 [ 10, %entry ], [ %b.1, %if.end ]
  %a.addr.0 = phi i32 [ %add, %entry ], [ %a.addr.1, %if.end ]
; CHECK-SPIRV: OpLine %[[str]] 5 0
; CHECK-SPIRV-NEXT: OpINotEqual
  %tobool = icmp ne i32 %b.0, 0, !dbg !31
; CHECK-SPIRV-NEXT: OpBranchConditional %{{[0-9]+}} %[[while_body:[0-9]+]] %[[while_end:[0-9]+]]
  br i1 %tobool, label %while.body, label %while.end, !dbg !31

; CHECK-SPIRV: %[[while_body]] = OpLabel
while.body:                                       ; preds = %while.cond
; CHECK-SPIRV: OpLine %[[str]] 6 0
; CHECK-SPIRV-NEXT: OpSGreaterThan
  %cmp = icmp sgt i32 %a.addr.0, 4, !dbg !34
; CHECK-SPIRV-NEXT: OpBranchConditional %{{[0-9]+}} %[[if_then:[0-9]+]] %[[if_else:[0-9]+]]
  br i1 %cmp, label %if.then, label %if.else, !dbg !37

; CHECK-SPIRV: %[[if_then]] = OpLabel
if.then:                                          ; preds = %while.body
; CHECK-SPIRV: OpLine %[[str]] 7 0
; CHECK-SPIRV-NEXT: OpISub
  %sub = sub nsw i32 %a.addr.0, 1, !dbg !38
  call void @llvm.dbg.value(metadata i32 %sub, i64 0, metadata !28, metadata !25), !dbg !29
; CHECK-SPIRV-NEXT: OpBranch %[[if_end:[0-9]+]]
  br label %if.end, !dbg !38

; CHECK-SPIRV: %[[if_else]] = OpLabel
if.else:                                          ; preds = %while.body
; CHECK-SPIRV: OpLine %[[str]] 9 0
; CHECK-SPIRV-NEXT: OpIAdd
  %add1 = add nsw i32 %a.addr.0, 1, !dbg !39
  call void @llvm.dbg.value(metadata i32 %add1, i64 0, metadata !28, metadata !25), !dbg !29
; CHECK-SPIRV: OpLine %[[str]] 10 0
; CHECK-SPIRV-NEXT: OpIMul
  %mul = mul nsw i32 %a.addr.0, 3, !dbg !41
  call void @llvm.dbg.value(metadata i32 %mul, i64 0, metadata !24, metadata !25), !dbg !26
; CHECK-SPIRV-NEXT: OpBranch %[[if_end]]
  br label %if.end

; CHECK-SPIRV: %[[if_end]] = OpLabel
if.end:                                           ; preds = %if.else, %if.then
  %b.1 = phi i32 [ %sub, %if.then ], [ %add1, %if.else ]
  %a.addr.1 = phi i32 [ %a.addr.0, %if.then ], [ %mul, %if.else ]
; CHECK-SPIRV: OpLine %[[str]] 5 0
; CHECK-SPIRV-NEXT: OpBranch %[[while_cond]]
  br label %while.cond, !dbg !30

; CHECK-SPIRV: %[[while_end]] = OpLabel
while.end:                                        ; preds = %while.cond
; CHECK-SPIRV: OpLine %[[str]] 13 0
; CHECK-SPIRV-NEXT: OpReturnValue
  ret i32 %b.0, !dbg !42
}
; CHECK-SPIRV: OpFunctionEnd

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata, metadata) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!19, !20}
!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!21}
!opencl.ocl.version = !{!21}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}
!llvm.ident = !{!22}

!0 = !{!"0x11\0012\00clang version 3.6.1 (https://github.com/KhronosGroup/SPIR.git 5df927fb80a9b807bf12eb82ae686c3978b4ccc7) (https://github.com/KhronosGroup/SPIRV-LLVM.git dcefbf9bcd9eb48c41aa16935e5a4e17d8160c8a)\000\00\000\00\001", !1, !2, !2, !3, !2, !2} ; [ DW_TAG_compile_unit ] [/tmp//<stdin>] [DW_LANG_C99]
!1 = !{!"/<stdin>", !"/spirv_limits_characters_in_a_literal_name"}
!2 = !{}
!3 = !{!4, !9}
!4 = !{!"0x2e\00foo\00foo\00\001\000\001\000\000\000\000\001", !5, !6, !7, null, void ()* @foo, null, null, !2} ; [ DW_TAG_subprogram ] [line 1] [def] [foo]
!5 = !{!"/tmp.cl", !"/spirv_limits_characters_in_a_literal_name"}
!6 = !{!"0x29", !5}                               ; [ DW_TAG_file_type ] [/tmp//tmp.cl]
!7 = !{!"0x15\00\000\000\000\000\000\000", null, null, null, !8, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!8 = !{null}
!9 = !{!"0x2e\00bar\00bar\00\002\000\001\000\000\00256\000\002", !5, !6, !10, null, i32 (i32)* @bar, null, null, !2} ; [ DW_TAG_subprogram ] [line 2] [def] [bar]
!10 = !{!"0x15\00\000\000\000\000\000\000", null, null, null, !11, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!11 = !{!12, !12}
!12 = !{!"0x24\00int\000\0032\0032\000\000\005", null, null} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!19 = !{i32 2, !"Dwarf Version", i32 4}
!20 = !{i32 2, !"Debug Info Version", i32 2}
!21 = !{i32 1, i32 2}
!22 = !{!"clang version 3.6.1 (https://github.com/KhronosGroup/SPIR.git 5df927fb80a9b807bf12eb82ae686c3978b4ccc7) (https://github.com/KhronosGroup/SPIRV-LLVM.git dcefbf9bcd9eb48c41aa16935e5a4e17d8160c8a)"}
!23 = !{}
!24 = !{!"0x101\00a\0016777218\000", !9, !6, !12} ; [ DW_TAG_arg_variable ] [a] [line 2]
!25 = !{!"0x102"}                                 ; [ DW_TAG_expression ]
!26 = !{}
!27 = !{}
!28 = !{!"0x100\00b\004\000", !9, !6, !12}        ; [ DW_TAG_auto_variable ] [b] [line 4]
!29 = !{}
!30 = !{}
!31 = !{}
!32 = !{!"0xb\002", !5, !33}                      ; [ DW_TAG_lexical_block ] [/tmp//tmp.cl]
!33 = !{!"0xb\001", !5, !9}                       ; [ DW_TAG_lexical_block ] [/tmp//tmp.cl]
!34 = !{}
!35 = !{!"0xb\006\000\001", !5, !36}              ; [ DW_TAG_lexical_block ] [/tmp//tmp.cl]
!36 = !{!"0xb\005\000\000", !5, !9}               ; [ DW_TAG_lexical_block ] [/tmp//tmp.cl]
!37 = !{}
!38 = !{}
!39 = !{}
!40 = !{!"0xb\008\000\002", !5, !35}              ; [ DW_TAG_lexical_block ] [/tmp//tmp.cl]
!41 = !{}
!42 = !{}
