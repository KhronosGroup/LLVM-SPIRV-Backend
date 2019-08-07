//===-- SPIRVExtInsts.h - SPIR-V Extended Instructions ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Define the different SPIR-V extended instruction sets plus helper functions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVEXTINSTS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVEXTINSTS_H

#include "llvm/ADT/Optional.h"
#include <cstdint>
#include <string>

enum class ExtInstSet : std::uint32_t {
  OpenCL_std,
  GLSL_std_450,
  SPV_AMD_shader_trinary_minmax
};

const char *getExtInstSetName(ExtInstSet e);
ExtInstSet getExtInstSetFromString(const std::string &nameStr);

const char *getExtInstName(ExtInstSet set, std::uint32_t instNum);

#define MAKE_EXT_INST_ENUM(EnumName, Varname, Val) Varname = Val,

#define MAKE_EXT_INST_NAME_CASE(EnumName, Varname, Val)                        \
  case EnumName::Varname:                                                      \
    return #Varname;

#define MAKE_EXT_INST_NAME_TO_ID(EnumName, Varname, Val)                       \
  {#Varname, EnumName::Varname},

#define DEF_EXT_INST_ENUM(EnumName, DefEnumCommand)                            \
  namespace EnumName {                                                         \
  enum EnumName { DefEnumCommand(EnumName, MAKE_EXT_INST_ENUM) };              \
  }

#define DEF_EXT_INST_NAME_FUNC_HEADER(EnumName)                                \
  const char *get##EnumName##Name(std::uint32_t e);

#define DEF_NAME_TO_EXT_INST_FUNC_HEADER(EnumName)                             \
  llvm::Optional<EnumName::EnumName> get##EnumName##FromName(                  \
      const std::string &name);

#define DEF_EXT_INST_NAME_FUNC_BODY(EnumName, DefEnumCommand)                  \
  const char *get##EnumName##Name(std::uint32_t e) {                           \
    switch (e) { DefEnumCommand(EnumName, MAKE_EXT_INST_NAME_CASE) }           \
    return "UNKNOWN_EXT_INST";                                                 \
  }

#define DEF_NAME_TO_EXT_INST_FUNC_BODY(EnumName, DefEnumCommand)               \
  llvm::Optional<EnumName::EnumName> get##EnumName##FromName(                  \
      const std::string &name) {                                               \
    static const std::map<std::string, EnumName::EnumName> nameToExtInstMap =  \
        {DefEnumCommand(EnumName, MAKE_EXT_INST_NAME_TO_ID)};                  \
    auto found = nameToExtInstMap.find(name);                                  \
    if (found != nameToExtInstMap.end()) {                                     \
      return {found->second};                                                  \
    } else {                                                                   \
      return {};                                                               \
    }                                                                          \
  }

#define GEN_EXT_INST_HEADER(EnumName)                                          \
  DEF_EXT_INST_ENUM(EnumName, DEF_##EnumName)                                  \
  DEF_EXT_INST_NAME_FUNC_HEADER(EnumName)                                      \
  DEF_NAME_TO_EXT_INST_FUNC_HEADER(EnumName)

#define GEN_EXT_INST_IMPL(EnumName)                                            \
  DEF_EXT_INST_NAME_FUNC_BODY(EnumName, DEF_##EnumName)                        \
  DEF_NAME_TO_EXT_INST_FUNC_BODY(EnumName, DEF_##EnumName)

#define DEF_GLSL_std_450(N, X)                                                 \
  X(N, Round, 1)                                                               \
  X(N, RoundEven, 2)                                                           \
  X(N, Trunc, 3)                                                               \
  X(N, FAbs, 4)                                                                \
  X(N, SAbs, 5)                                                                \
  X(N, FSign, 6)                                                               \
  X(N, SSign, 7)                                                               \
  X(N, Floor, 8)                                                               \
  X(N, Ceil, 9)                                                                \
  X(N, Fract, 10)                                                              \
  X(N, Radians, 11)                                                            \
  X(N, Degrees, 12)                                                            \
  X(N, Sin, 13)                                                                \
  X(N, Cos, 14)                                                                \
  X(N, Tan, 15)                                                                \
  X(N, Asin, 16)                                                               \
  X(N, Acos, 17)                                                               \
  X(N, Atan, 18)                                                               \
  X(N, Sinh, 19)                                                               \
  X(N, Cosh, 20)                                                               \
  X(N, Tanh, 21)                                                               \
  X(N, Asinh, 22)                                                              \
  X(N, Acosh, 23)                                                              \
  X(N, Atanh, 24)                                                              \
  X(N, Atan2, 25)                                                              \
  X(N, Pow, 26)                                                                \
  X(N, Exp, 27)                                                                \
  X(N, Log, 28)                                                                \
  X(N, Exp2, 29)                                                               \
  X(N, Log2, 30)                                                               \
  X(N, Sqrt, 31)                                                               \
  X(N, InverseSqrt, 32)                                                        \
  X(N, Determinant, 33)                                                        \
  X(N, MatrixInverse, 34)                                                      \
  X(N, Modf, 35)                                                               \
  X(N, ModfStruct, 36)                                                         \
  X(N, FMin, 37)                                                               \
  X(N, UMin, 38)                                                               \
  X(N, SMin, 39)                                                               \
  X(N, FMax, 40)                                                               \
  X(N, UMax, 41)                                                               \
  X(N, SMax, 42)                                                               \
  X(N, FClamp, 43)                                                             \
  X(N, UClamp, 44)                                                             \
  X(N, SClamp, 45)                                                             \
  X(N, FMix, 46)                                                               \
  X(N, Step, 48)                                                               \
  X(N, SmoothStep, 49)                                                         \
  X(N, Fma, 50)                                                                \
  X(N, Frexp, 51)                                                              \
  X(N, FrexpStruct, 52)                                                        \
  X(N, Ldexp, 53)                                                              \
  X(N, PackSnorm4x8, 54)                                                       \
  X(N, PackUnorm4x8, 55)                                                       \
  X(N, PackSnorm2x16, 56)                                                      \
  X(N, PackUnorm2x16, 57)                                                      \
  X(N, PackHalf2x16, 58)                                                       \
  X(N, PackDouble2x32, 59)                                                     \
  X(N, UnpackSnorm2x16, 60)                                                    \
  X(N, UnpackUnorm2x16, 61)                                                    \
  X(N, UnpackHalf2x16, 62)                                                     \
  X(N, UnpackSnorm4x8, 63)                                                     \
  X(N, UnpackUnorm4x8, 64)                                                     \
  X(N, UnpackDouble2x32, 65)                                                   \
  X(N, Length, 66)                                                             \
  X(N, Distance, 67)                                                           \
  X(N, Cross, 68)                                                              \
  X(N, Normalize, 69)                                                          \
  X(N, FaceForward, 70)                                                        \
  X(N, Reflect, 71)                                                            \
  X(N, Refract, 72)                                                            \
  X(N, FindILsb, 73)                                                           \
  X(N, FindSMsb, 74)                                                           \
  X(N, FindUMsb, 75)                                                           \
  X(N, InterpolateAtCentroid, 76)                                              \
  X(N, InterpolateAtSample, 77)                                                \
  X(N, InterpolateAtOffset, 78)                                                \
  X(N, NMin, 79)                                                               \
  X(N, NMax, 80)                                                               \
  X(N, NClamp, 81)
GEN_EXT_INST_HEADER(GLSL_std_450)

#define DEF_OpenCL_std(N, X)                                                   \
  X(N, acos, 0) /* 2.1 Math extended instructions */                           \
  X(N, acosh, 1)                                                               \
  X(N, acospi, 2)                                                              \
  X(N, asin, 3)                                                                \
  X(N, asinh, 4)                                                               \
  X(N, asinpi, 5)                                                              \
  X(N, atan, 6)                                                                \
  X(N, atan2, 7)                                                               \
  X(N, atanh, 8)                                                               \
  X(N, atanpi, 9)                                                              \
  X(N, atan2pi, 10)                                                            \
  X(N, cbrt, 11)                                                               \
  X(N, ceil, 12)                                                               \
  X(N, copysign, 13)                                                           \
  X(N, cos, 14)                                                                \
  X(N, cosh, 15)                                                               \
  X(N, cospi, 16)                                                              \
  X(N, erfc, 17)                                                               \
  X(N, erf, 18)                                                                \
  X(N, exp, 19)                                                                \
  X(N, exp2, 20)                                                               \
  X(N, exp10, 21)                                                              \
  X(N, expm1, 22)                                                              \
  X(N, fabs, 23)                                                               \
  X(N, fdim, 24)                                                               \
  X(N, floor, 25)                                                              \
  X(N, fma, 26)                                                                \
  X(N, fmax, 27)                                                               \
  X(N, fmin, 28)                                                               \
  X(N, fmod, 29)                                                               \
  X(N, fract, 30)                                                              \
  X(N, frexp, 31)                                                              \
  X(N, hypot, 32)                                                              \
  X(N, ilogb, 33)                                                              \
  X(N, ldexp, 34)                                                              \
  X(N, lgamma, 35)                                                             \
  X(N, lgamma_r, 36)                                                           \
  X(N, log, 37)                                                                \
  X(N, log2, 38)                                                               \
  X(N, log10, 39)                                                              \
  X(N, log1p, 40)                                                              \
  X(N, log1b, 41)                                                              \
  X(N, mad, 42)                                                                \
  X(N, maxmag, 43)                                                             \
  X(N, minmag, 44)                                                             \
  X(N, modf, 45)                                                               \
  X(N, nan, 46)                                                                \
  X(N, nextafter, 47)                                                          \
  X(N, pow, 48)                                                                \
  X(N, pown, 49)                                                               \
  X(N, powr, 50)                                                               \
  X(N, remainder, 51)                                                          \
  X(N, remquo, 52)                                                             \
  X(N, rint, 53)                                                               \
  X(N, rootn, 54)                                                              \
  X(N, round, 55)                                                              \
  X(N, rsqrt, 56)                                                              \
  X(N, sin, 57)                                                                \
  X(N, sincos, 58)                                                             \
  X(N, sinh, 59)                                                               \
  X(N, sinpi, 60)                                                              \
  X(N, sqrt, 61)                                                               \
  X(N, tan, 62)                                                                \
  X(N, tanh, 63)                                                               \
  X(N, tanpi, 64)                                                              \
  X(N, tgamma, 65)                                                             \
  X(N, trunc, 66)                                                              \
  X(N, half_cos, 67)                                                           \
  X(N, half_divide, 68)                                                        \
  X(N, half_exp, 69)                                                           \
  X(N, half_exp2, 70)                                                          \
  X(N, half_exp10, 71)                                                         \
  X(N, half_log, 72)                                                           \
  X(N, half_log2, 73)                                                          \
  X(N, half_log10, 74)                                                         \
  X(N, half_powr, 75)                                                          \
  X(N, half_recip, 76)                                                         \
  X(N, half_rsqrt, 77)                                                         \
  X(N, half_sin, 78)                                                           \
  X(N, half_sqrt, 79)                                                          \
  X(N, half_tan, 80)                                                           \
  X(N, native_cos, 81)                                                         \
  X(N, native_divide, 82)                                                      \
  X(N, native_exp, 83)                                                         \
  X(N, native_exp2, 84)                                                        \
  X(N, native_exp10, 85)                                                       \
  X(N, native_log, 86)                                                         \
  X(N, native_log2, 87)                                                        \
  X(N, native_log10, 88)                                                       \
  X(N, native_powr, 89)                                                        \
  X(N, native_recip, 90)                                                       \
  X(N, native_rsqrt, 91)                                                       \
  X(N, native_sin, 92)                                                         \
  X(N, native_sqrt, 93)                                                        \
  X(N, native_tan, 94)                                                         \
  X(N, s_abs, 141) /* 2.2 Integer Instructions */                              \
  X(N, s_abs_diff, 142)                                                        \
  X(N, s_add_sat, 143)                                                         \
  X(N, u_add_sat, 144)                                                         \
  X(N, s_hadd, 145)                                                            \
  X(N, u_hadd, 146)                                                            \
  X(N, s_rhadd, 147)                                                           \
  X(N, u_rhadd, 148)                                                           \
  X(N, s_clamp, 149)                                                           \
  X(N, u_clamp, 150)                                                           \
  X(N, clz, 151)                                                               \
  X(N, ctz, 152)                                                               \
  X(N, s_mad_hi, 153)                                                          \
  X(N, u_mad_sat, 154)                                                         \
  X(N, s_mad_sat, 155)                                                         \
  X(N, s_max, 156)                                                             \
  X(N, u_max, 157)                                                             \
  X(N, s_min, 158)                                                             \
  X(N, u_min, 159)                                                             \
  X(N, s_mul_hi, 160)                                                          \
  X(N, rotate, 161)                                                            \
  X(N, s_sub_sat, 162)                                                         \
  X(N, u_sub_sat, 163)                                                         \
  X(N, u_upsample, 164)                                                        \
  X(N, s_upsample, 165)                                                        \
  X(N, popcount, 166)                                                          \
  X(N, s_mad24, 167)                                                           \
  X(N, u_mad24, 168)                                                           \
  X(N, s_mul24, 169)                                                           \
  X(N, u_mul24, 170)                                                           \
  X(N, u_abs, 201)                                                             \
  X(N, u_abs_diff, 202)                                                        \
  X(N, u_mul_hi, 203)                                                          \
  X(N, u_mad_hi, 204)                                                          \
  X(N, fclamp, 95) /* 2.3 Common Instructions */                               \
  X(N, degrees, 96)                                                            \
  X(N, fmax_common, 97)                                                        \
  X(N, fmin_common, 98)                                                        \
  X(N, mix, 99)                                                                \
  X(N, radians, 100)                                                           \
  X(N, step, 101)                                                              \
  X(N, smoothstep, 102)                                                        \
  X(N, sign, 103)                                                              \
  X(N, cross, 104) /* 2.4 Geometric Instructions */                            \
  X(N, distance, 105)                                                          \
  X(N, length, 106)                                                            \
  X(N, normalize, 107)                                                         \
  X(N, fast_distance, 108)                                                     \
  X(N, fast_length, 109)                                                       \
  X(N, fast_normalize, 110)                                                    \
  X(N, bitselect, 186) /* 2.5 Relational Instructions */                       \
  X(N, select, 187)                                                            \
  X(N, vloadn, 171) /* 2.6 Vector Data Load and Store Instructions */          \
  X(N, vstoren, 172)                                                           \
  X(N, vload_half, 173)                                                        \
  X(N, vload_halfn, 174)                                                       \
  X(N, vstore_half, 175)                                                       \
  X(N, vstore_half_r, 176)                                                     \
  X(N, vstore_halfn, 177)                                                      \
  X(N, vstore_halfn_r, 178)                                                    \
  X(N, vloada_halfn, 179)                                                      \
  X(N, vstorea_halfn, 180)                                                     \
  X(N, vstorea_halfn_r, 181)                                                   \
  X(N, shuffle, 182) /* 2.7 Miscellaneous Vector Instructions */               \
  X(N, shuffle2, 183)                                                          \
  X(N, printf, 184) /* 2.8 Misc Instructions */                                \
  X(N, prefetch, 185)
GEN_EXT_INST_HEADER(OpenCL_std)

#define DEF_SPV_AMD_shader_trinary_minmax(N, X)                                \
  X(N, FMin3AMD, 1)                                                            \
  X(N, UMin3AMD, 2)                                                            \
  X(N, SMin3AMD, 3)                                                            \
  X(N, FMax3AMD, 4)                                                            \
  X(N, UMax3AMD, 5)                                                            \
  X(N, SMax3AMD, 6)                                                            \
  X(N, FMid3AMD, 7)                                                            \
  X(N, UMid3AMD, 8)                                                            \
  X(N, SMid3AMD, 9)
GEN_EXT_INST_HEADER(SPV_AMD_shader_trinary_minmax)

#endif
