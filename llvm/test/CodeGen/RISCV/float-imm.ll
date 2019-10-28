; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=riscv32 -mattr=+f -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32IF %s
; RUN: llc -mtriple=riscv64 -mattr=+f -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64IF %s

; TODO: constant pool shouldn't be necessary for RV64IF.
define float @float_imm() nounwind {
; RV32IF-LABEL: float_imm:
; RV32IF:       # %bb.0:
; RV32IF-NEXT:    lui a0, 263313
; RV32IF-NEXT:    addi a0, a0, -37
; RV32IF-NEXT:    ret
;
; RV64IF-LABEL: float_imm:
; RV64IF:       # %bb.0:
; RV64IF-NEXT:    lui a0, %hi(.LCPI0_0)
; RV64IF-NEXT:    addi a0, a0, %lo(.LCPI0_0)
; RV64IF-NEXT:    flw ft0, 0(a0)
; RV64IF-NEXT:    fmv.x.w a0, ft0
; RV64IF-NEXT:    ret
  ret float 3.14159274101257324218750
}

define float @float_imm_op(float %a) nounwind {
; TODO: addi should be folded in to the flw
; RV32IF-LABEL: float_imm_op:
; RV32IF:       # %bb.0:
; RV32IF-NEXT:    lui a1, %hi(.LCPI1_0)
; RV32IF-NEXT:    addi a1, a1, %lo(.LCPI1_0)
; RV32IF-NEXT:    flw ft0, 0(a1)
; RV32IF-NEXT:    fmv.w.x ft1, a0
; RV32IF-NEXT:    fadd.s ft0, ft1, ft0
; RV32IF-NEXT:    fmv.x.w a0, ft0
; RV32IF-NEXT:    ret
;
; RV64IF-LABEL: float_imm_op:
; RV64IF:       # %bb.0:
; RV64IF-NEXT:    lui a1, %hi(.LCPI1_0)
; RV64IF-NEXT:    addi a1, a1, %lo(.LCPI1_0)
; RV64IF-NEXT:    flw ft0, 0(a1)
; RV64IF-NEXT:    fmv.w.x ft1, a0
; RV64IF-NEXT:    fadd.s ft0, ft1, ft0
; RV64IF-NEXT:    fmv.x.w a0, ft0
; RV64IF-NEXT:    ret
  %1 = fadd float %a, 1.0
  ret float %1
}
