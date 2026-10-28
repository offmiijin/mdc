# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Dependências por FetchContent (ADR 0004), com preferência por SDL3 do sistema.
# Nenhuma dependência é vendorizada no repositório: o que está versionado aqui é
# só o pino de versão. Licenças em LICENSES/README.md.

include(FetchContent)

# zlib do sistema: o leitor de ZIP do pack (ADR 0009 — sem vendorizar miniz).
find_package(ZLIB REQUIRED)

set(MDC_SDL3_TAG "release-3.4.18" CACHE STRING "Tag do SDL3 usada quando não há versão de sistema")
set(MDC_DOCTEST_TAG "v2.4.12" CACHE STRING "Tag do doctest")

# Alvo de interface `mdc_sdl3`: esconde se veio do sistema ou do FetchContent.
function(mdc_add_sdl3)
  if(TARGET mdc_sdl3)
    return()
  endif()

  set(_found FALSE)
  if(MDC_USE_SYSTEM_SDL3)
    find_package(SDL3 CONFIG QUIET)
    if(SDL3_FOUND)
      message(STATUS "SDL3: sistema ${SDL3_VERSION}")
      add_library(mdc_sdl3 INTERFACE)
      target_link_libraries(mdc_sdl3 INTERFACE SDL3::SDL3)
      set(_found TRUE)
    endif()
  endif()

  if(NOT _found)
    message(STATUS "SDL3: FetchContent ${MDC_SDL3_TAG}")
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG ${MDC_SDL3_TAG}
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(SDL3)
    add_library(mdc_sdl3 INTERFACE)
    if(TARGET SDL3::SDL3-static)
      target_link_libraries(mdc_sdl3 INTERFACE SDL3::SDL3-static)
    else()
      target_link_libraries(mdc_sdl3 INTERFACE SDL3::SDL3)
    endif()
  endif()
endfunction()

function(mdc_add_doctest)
  if(TARGET doctest::doctest)
    return()
  endif()
  message(STATUS "doctest: FetchContent ${MDC_DOCTEST_TAG}")
  FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG ${MDC_DOCTEST_TAG}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(doctest)
endfunction()
