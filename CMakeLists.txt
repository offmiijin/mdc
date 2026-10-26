# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
cmake_minimum_required(VERSION 3.24)

project(mdc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MDC_BUILD_CLIENT "Compila o cliente SDL3 (precisa de SDL3)" ON)
option(MDC_BUILD_TESTS "Compila os testes unitários (doctest)" ON)
option(MDC_BUILD_PARITY "Compila os binários do harness de paridade" ON)
option(MDC_USE_SYSTEM_SDL3 "Usa SDL3 do sistema quando encontrado; senão baixa via FetchContent" ON)
option(MDC_WERROR "Trata avisos como erro" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Tipo de build" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerFlags)
include(Dependencies)

add_subdirectory(src/core)

if(MDC_BUILD_CLIENT)
  add_subdirectory(src/client)
else()
  message(STATUS "MDC_BUILD_CLIENT=OFF — cliente e SDL3 não serão compilados")
endif()

if(MDC_BUILD_PARITY)
  add_subdirectory(src/harness)
endif()

if(MDC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

message(STATUS "mdc ${PROJECT_VERSION} · build=${CMAKE_BUILD_TYPE} · cliente=${MDC_BUILD_CLIENT} · testes=${MDC_BUILD_TESTS} · paridade=${MDC_BUILD_PARITY}")
