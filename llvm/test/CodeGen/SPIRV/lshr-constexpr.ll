; RUN: llc -O0 -global-isel %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spirv64-unknown-unknown"

; CHECK-SPIRV: %[[type_int32:[0-9]+]] = OpTypeInt 32 0
; CHECK-SPIRV: %[[type_int64:[0-9]+]] = OpTypeInt 64 0
; CHECK-SPIRV: %[[const1:[0-9]+]] = OpConstant %[[type_int32]] 1
; CHECK-SPIRV: %[[const32:[0-9]+]] = OpConstant %[[type_int64]] 32 0
; CHECK-SPIRV: %[[type_vec:[0-9]+]] = OpTypeVector %[[type_int32]] 2
; CHECK-SPIRV: %[[vec_const:[0-9]+]] = OpConstantComposite %[[type_vec]] %[[const1]] %[[const1]]

; CHECK-SPIRV: %[[bitcast_res:[0-9]+]] = OpBitcast %[[type_int64]] %[[vec_const]]
; CHECK-SPIRV: %[[shift_res:[0-9]+]] = OpShiftRightLogical %[[type_int64]] %[[bitcast_res]] %[[const32]]
; FIXME: Have no information about OpDebugValue and it's syntax
; CHECK-SPIRV: OpDebugValue %{{[0-9]+}} %[[shift_res]]

; Function Attrs: nounwind ssp uwtable
define void @foo() #0 !dbg !4 {
entry:
  tail call void @llvm.dbg.value(metadata i64 lshr (i64 bitcast (<2 x i32> <i32 1, i32 1> to i64), i64 32), metadata !12, metadata !17) #3, !dbg !18
  ret void, !dbg !20
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

attributes #0 = { nounwind ssp uwtable  }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!13, !14, !15}
!llvm.ident = !{!16}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 3.7.0 (trunk 235110) (llvm/trunk 235108)", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !2, globals: !2, imports: !2)
!1 = !DIFile(filename: "t.c", directory: "/path/to/dir")
!2 = !{}
!4 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 3, type: !5, isLocal: false, isDefinition: true, scopeLine: 3, flags: DIFlagPrototyped, isOptimized: true, unit: !0, retainedNodes: !2)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!7 = distinct !DISubprogram(name: "bar", scope: !1, file: !1, line: 2, type: !8, isLocal: true, isDefinition: true, scopeLine: 2, flags: DIFlagPrototyped, isOptimized: true, unit: !0, retainedNodes: !11)
!8 = !DISubroutineType(types: !9)
!9 = !{null, !10}
!10 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!11 = !{!12}
!12 = !DILocalVariable(name: "a", arg: 1, scope: !7, file: !1, line: 2, type: !10)
!13 = !{i32 2, !"Dwarf Version", i32 2}
!14 = !{i32 2, !"Debug Info Version", i32 3}
!15 = !{i32 1, !"PIC Level", i32 2}
!16 = !{!"clang version 3.7.0 (trunk 235110) (llvm/trunk 235108)"}
!17 = !DIExpression()
!18 = !DILocation(line: 2, column: 52, scope: !7, inlinedAt: !19)
!19 = distinct !DILocation(line: 4, column: 3, scope: !4)
!20 = !DILocation(line: 6, column: 1, scope: !4)
