# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Binários de harness: sem SDL, sem GPU. Compilam em qualquer máquina e são o
# lado C++ das comparações descritas em docs/HARNESS.md.

# Um alvo por harness, sempre `mdc-parity-<harness>` a partir de
# `<harness>_parity.cpp` — o mesmo nome que `tools/parity/run_parity.sh` procura.
foreach(harness aabb vec3 clip collision blockpos)
  add_executable(mdc-parity-${harness} ${harness}_parity.cpp)

  target_link_libraries(mdc-parity-${harness} PRIVATE
    mdc_core
    mdc_phys
    mdc_warnings
    mdc_fp_strict)

  set_target_properties(mdc-parity-${harness} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()

# Harnesses do M2 em diante não são de física: têm alvo próprio para deixar
# explícito de qual módulo cada um depende.
foreach(harness palette blockstate atlas tint)
  add_executable(mdc-parity-${harness} ${harness}_parity.cpp)
  target_link_libraries(mdc-parity-${harness} PRIVATE
    mdc_level
    mdc_render
    mdc_res
    mdc_warnings
    mdc_fp_strict)
  set_target_properties(mdc-parity-${harness} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()
