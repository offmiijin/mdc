# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Avisos e regras de ponto flutuante. O segundo é requisito de paridade:
# ver docs/HARNESS.md, seção 2 — fast-math e contração de FMA quebram a
# comparação bit a bit com o original.

add_library(mdc_warnings INTERFACE)
if(MSVC)
  target_compile_options(mdc_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(mdc_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wnon-virtual-dtor)
endif()
if(MDC_WERROR)
  if(MSVC)
    target_compile_options(mdc_warnings INTERFACE /WX)
  else()
    target_compile_options(mdc_warnings INTERFACE -Werror)
  endif()
endif()

add_library(mdc_fp_strict INTERFACE)
if(NOT MSVC)
  target_compile_options(mdc_fp_strict INTERFACE
    # proibido: -ffast-math, -Ofast, -mfpmath=387
    -fno-fast-math
    # proibido contrair a*b+c em fma — o Java não contrai
    -ffp-contract=off)
endif()
