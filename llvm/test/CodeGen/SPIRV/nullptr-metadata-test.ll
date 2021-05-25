; This test ensures that the translator does not crash
; RUN: llc -O0 -global-isel %s

; ModuleID = 'test.bc'
target triple = "spirv64-unknown-unknown"

declare dllexport void @test_func(i32) #0

attributes #0 = { "VCSLMSize"="0" }
