# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Binários de harness: sem SDL, sem GPU. Compilam em qualquer máquina e são o
# lado C++ das comparações descritas em docs/HARNESS.md.

add_executable(mdc-parity-aabb aabb_parity.cpp)

target_link_libraries(mdc-parity-aabb PRIVATE
  mdc_core
  mdc_warnings
  mdc_fp_strict)

set_target_properties(mdc-parity-aabb PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
