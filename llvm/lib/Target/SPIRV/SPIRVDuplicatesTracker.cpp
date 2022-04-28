//===-- SPIRVDuplicatesTracker.cpp - SPIR-V Duplicates Tracker --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//


#include "SPIRVDuplicatesTracker.h"

using namespace llvm;

template <typename T>
const SPIRVDuplicatesTracker<T> *
SPIRVGeneralDuplicatesTracker::get(T *Arg) {
  llvm_unreachable("Querying duplicates of unsupported type");
  return nullptr;
}

template <>
const SPIRVTypeTracker *SPIRVGeneralDuplicatesTracker::get(Type *Arg) {
  return &TT;
}

template <>
const SPIRVConstantTracker *SPIRVGeneralDuplicatesTracker::get(Constant *Arg) {
  return &CT;
}

template <>
const SPIRVGlobalVariableTracker *
SPIRVGeneralDuplicatesTracker::get(GlobalVariable *Arg) {
  return &GT;
}

template <>
const SPIRVFuncDeclsTracker *SPIRVGeneralDuplicatesTracker::get(Function *Arg) {
  return &FT;
}

template <>
const SPIRVSpecialDeclsTracker *SPIRVGeneralDuplicatesTracker::get(SpecialTypeDescriptor *Arg) {
  return &ST;
}
  