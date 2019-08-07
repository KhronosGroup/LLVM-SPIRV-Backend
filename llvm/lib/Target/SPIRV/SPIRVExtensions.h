//===-- SPIRVExtensions.h - SPIR-V Extensions -------------------*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_SPIRV_SPIRVEXTENSIONS_H
#define LLVM_LIB_TARGET_SPIRV_SPIRVEXTENSIONS_H

#include <stdint.h>
#include <string>

#define MAKE_EXTENSION_ENUM(EnumName, Varname, Val) Varname = Val,

#define MAKE_EXTENSION_NAME_CASE(EnumName, Varname, Val)                       \
  case EnumName::Varname:                                                      \
    return #Varname;

#define DEF_EXTENSION_ENUM(EnumName, DefEnumCommand)                           \
  namespace EnumName {                                                         \
  enum EnumName { DefEnumCommand(EnumName, MAKE_EXTENSION_ENUM) };             \
  }

#define DEF_EXTENSION_NAME_FUNC_HEADER(EnumName)                               \
  const char *get##EnumName##Name(EnumName::EnumName e);

#define DEF_EXTENSION_NAME_FUNC_BODY(EnumName, DefEnumCommand)                 \
  const char *get##EnumName##Name(EnumName::EnumName e) {                      \
    switch (e) { DefEnumCommand(EnumName, MAKE_EXTENSION_NAME_CASE) }          \
    return "UNKNOWN_EXTENSION";                                                \
  }

#define GEN_EXTENSION_HEADER(EnumName)                                         \
  DEF_EXTENSION_ENUM(EnumName, DEF_##EnumName)                                 \
  DEF_EXTENSION_NAME_FUNC_HEADER(EnumName)

#define GEN_EXTENSION_IMPL(EnumName)                                           \
  DEF_EXTENSION_NAME_FUNC_BODY(EnumName, DEF_##EnumName)

#define DEF_Extension(N, X)                                                    \
  X(N, SPV_AMD_shader_explicit_vertex_parameter, 1)                            \
  X(N, SPV_AMD_shader_trinary_minmax, 2)                                       \
  X(N, SPV_AMD_gcn_shader, 3)                                                  \
  X(N, SPV_KHR_shader_ballot, 4)                                               \
  X(N, SPV_AMD_shader_ballot, 5)                                               \
  X(N, SPV_AMD_gpu_shader_half_float, 6)                                       \
  X(N, SPV_KHR_shader_draw_parameters, 7)                                      \
  X(N, SPV_KHR_subgroup_vote, 8)                                               \
  X(N, SPV_KHR_16bit_storeage, 9)                                              \
  X(N, SPV_KHR_device_group, 10)                                               \
  X(N, SPV_KHR_multiview, 11)                                                  \
  X(N, SPV_NVX_multiview_per_view_attributes, 12)                              \
  X(N, SPV_NV_viewport_array2, 13)                                             \
  X(N, SPV_NV_stereo_view_rendering, 14)                                       \
  X(N, SPV_NV_sample_mask_override_coverage, 15)                               \
  X(N, SPV_NV_geometry_shader_passthrough, 16)                                 \
  X(N, SPV_AMD_texture_gather_bias_lod, 17)                                    \
  X(N, SPV_KHR_storage_buffer_storage_class, 18)                               \
  X(N, SPV_KHR_variable_pointers, 19)                                          \
  X(N, SPV_AMD_gpu_shader_int16, 20)                                           \
  X(N, SPV_KHR_post_depth_coverage, 21)                                        \
  X(N, SPV_KHR_shader_atomic_counter_ops, 22)                                  \
  X(N, SPV_EXT_shader_stencil_export, 23)                                      \
  X(N, SPV_EXT_shader_viewport_index_layer, 24)                                \
  X(N, SPV_AMD_shader_image_load_store_lod, 25)                                \
  X(N, SPV_AMD_shader_fragment_mask, 26)                                       \
  X(N, SPV_EXT_fragment_fully_covered, 27)                                     \
  X(N, SPV_AMD_gpu_shader_half_float_fetch, 28)                                \
  X(N, SPV_GOOGLE_decorate_string, 29)                                         \
  X(N, SPV_GOOGLE_hlsl_functionality1, 30)                                     \
  X(N, SPV_NV_shader_subgroup_partitioned, 31)                                 \
  X(N, SPV_EXT_descriptor_indexing, 32)                                        \
  X(N, SPV_KHR_8bit_storage, 33)                                               \
  X(N, SPV_KHR_vulkan_memory_model, 34)                                        \
  X(N, SPV_NV_ray_tracing, 35)                                                 \
  X(N, SPV_NV_compute_shader_derivatives, 36)                                  \
  X(N, SPV_NV_fragment_shader_barycentric, 37)                                 \
  X(N, SPV_NV_mesh_shader, 38)                                                 \
  X(N, SPV_NV_shader_image_footprint, 39)                                      \
  X(N, SPV_NV_shading_rate, 40)                                                \
  X(N, SPV_INTEL_subgroups, 41)                                                \
  X(N, SPV_INTEL_media_block_io, 42)                                           \
  X(N, SPV_EXT_fragment_invocation_density, 44)                                \
  X(N, SPV_KHR_no_integer_wrap_decoration, 45)                                 \
  X(N, SPV_KHR_float_controls, 46)                                             \
  X(N, SPV_EXT_physical_storage_buffer, 47)                                    \
  X(N, SPV_INTEL_fpga_memory_attributes, 48)                                   \
  X(N, SPV_NV_cooperative_matrix, 49)                                          \
  X(N, SPV_INTEL_shader_integer_functions2, 50)                                \
  X(N, SPV_INTEL_fpga_loop_controls, 51)                                       \
  X(N, SPV_EXT_fragment_shader_interlock, 52)                                  \
  X(N, SPV_NV_shader_sm_builtins, 53)                                          \
  X(N, SPV_KHR_shader_clock, 54)                                               \
  X(N, SPV_INTEL_unstructured_loop_controls, 55)                               \
  X(N, SPV_EXT_demote_to_helper_invocation, 56)                                \
  X(N, SPV_INTEL_fpga_reg, 57)
GEN_EXTENSION_HEADER(Extension)

#endif
