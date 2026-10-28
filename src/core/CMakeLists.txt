# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Núcleo do jogo: sem dependências externas, sem SDL, sem GPU.
# Tudo aqui tem de ser compilável e testável em qualquer máquina, inclusive CI
# sem driver gráfico — é o que permite rodar o harness (docs/HARNESS.md).

add_library(mdc_core STATIC
  aabb.cpp
  input.cpp
  tick_clock.cpp
  vec3.cpp)

target_include_directories(mdc_core PUBLIC
  "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(mdc_core
  PRIVATE mdc_warnings
  PUBLIC  mdc_fp_strict)
