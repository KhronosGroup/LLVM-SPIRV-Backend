; RUN: llc -O0 %s -o - | FileCheck %s

target triple = "spirv32-unknown-unknown"

; CHECK: OpUnreachable
define void @test_unreachable() {
  unreachable
}
