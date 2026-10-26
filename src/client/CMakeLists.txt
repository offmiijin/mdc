# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Binário do cliente. Em M0 faz janela + laço de tick; o render (Vulkan/RHI)
# entra no M2 (docs/PARITY.md).

mdc_add_sdl3()

add_executable(mdc-client main.cpp)

target_link_libraries(mdc-client PRIVATE
  mdc_core
  mdc_sdl3
  mdc_warnings
  mdc_fp_strict)

set_target_properties(mdc-client PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
