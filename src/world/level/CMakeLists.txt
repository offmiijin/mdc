# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Armazenamento de mundo no nível de chunk: empacotamento de bits e paleta.
# Sem SDL, sem GPU, sem I/O — compila em CI e no harness (degrau M2).

add_library(mdc_level STATIC
  bit_storage.cpp
  paletted_container.cpp)

target_include_directories(mdc_level PUBLIC
  "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(mdc_level
  PRIVATE mdc_warnings
  PUBLIC  mdc_core
          mdc_fp_strict)
