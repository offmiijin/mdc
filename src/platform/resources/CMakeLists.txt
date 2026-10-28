# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Leitura do pack de recursos do usuário: ZIP (zlib), JSON e caminhos do pack.
# Sem SDL e sem GPU; não entra em `mdc_core` (ADR 0009).

add_library(mdc_res STATIC
  json.cpp
  png.cpp
  resource_pack.cpp
  zip.cpp)

target_include_directories(mdc_res PUBLIC
  "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(mdc_res
  PRIVATE mdc_warnings
  PUBLIC  ZLIB::ZLIB
          mdc_fp_strict)
