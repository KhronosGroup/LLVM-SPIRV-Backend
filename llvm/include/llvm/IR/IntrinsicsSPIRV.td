//===- IntrinsicsSPIRV.td - Defines SPIRV intrinsics -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines all of the SPIRV-specific intrinsics.
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "spv" in {
  def int_spv_assign_type : Intrinsic<[], [llvm_any_ty, llvm_metadata_ty]>;
  //def int_spv_assign_type_to_const : Intrinsic<[], [llvm_any_ty, llvm_metadata_ty]>;

  def int_spv_gep : Intrinsic<[llvm_anyptr_ty], [llvm_i1_ty, llvm_any_ty, llvm_vararg_ty]>;
  def int_spv_load : Intrinsic<[llvm_i32_ty], [llvm_anyptr_ty, llvm_i16_ty]>;
  def int_spv_store : Intrinsic<[], [llvm_i32_ty, llvm_anyptr_ty, llvm_i16_ty]>;
  def int_spv_extractv : Intrinsic<[llvm_any_ty], [llvm_i32_ty, llvm_vararg_ty]>;
  def int_spv_insertv : Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_vararg_ty]>;
}