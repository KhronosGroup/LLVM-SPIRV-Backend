; RUN: llc -O0 %s -o - | FileCheck %s

target triple = "spirv64-unknown-unknown"

; CHECK-DAG: OpCapability Addresses
; CHECK-DAG: OpCapability Linkage
; CHECK-DAG: OpCapability Kernel
; CHECK: %1 = OpExtInstImport "OpenCL.std"
; CHECK: OpMemoryModel Physical64 OpenCL
; CHECK: OpSource Unknown 0
