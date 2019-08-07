; RUN: llc -O0 -global-isel %s -o - | FileCheck %s

target triple = "spirv32-unknown-unknown"

; FIXME: Is it mandatory to declare foo?
; CHECK-NOT: OpFunction
declare void @foo()

