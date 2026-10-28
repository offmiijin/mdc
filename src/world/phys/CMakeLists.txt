# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Física de mundo: geometria aplicada (colisão). Depende de `mdc_core` e de mais
# nada — nada de SDL, nada de GPU, nada de I/O. Ver docs/PARITY.md (M1).

add_library(mdc_phys STATIC
  collision.cpp)

target_include_directories(mdc_phys PUBLIC
  "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(mdc_phys
  PRIVATE mdc_warnings
  PUBLIC  mdc_core
          mdc_fp_strict)
