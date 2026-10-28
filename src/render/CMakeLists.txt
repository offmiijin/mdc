# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Camada de render do lado CPU: inventário de atlas, modelo assado e (M2.5)
# mesher. Sem SDL e sem Vulkan — é o que o harness compara (degrau M2).

add_library(mdc_render STATIC
  colormap.cpp
  sprite_inventory.cpp)

target_include_directories(mdc_render PUBLIC
  "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(mdc_render
  PRIVATE mdc_warnings
  PUBLIC  mdc_res
          mdc_core
          mdc_fp_strict)
