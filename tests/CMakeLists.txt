# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Testes unitários (doctest). Os testes de PARIDADE são outra coisa: vivem no
# harness e comparam com o jogo original (docs/HARNESS.md).

mdc_add_doctest()

add_executable(mdc_tests
  aabb_test.cpp
  input_test.cpp
  test_main.cpp
  tick_clock_test.cpp)

target_link_libraries(mdc_tests PRIVATE
  mdc_core
  doctest::doctest
  mdc_fp_strict
  mdc_warnings)

set_target_properties(mdc_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_test(NAME unit COMMAND mdc_tests)
