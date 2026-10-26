// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Ponto único de implementação do doctest. Os arquivos de teste apenas incluem
// <doctest/doctest.h>; se dois deles definirem DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN,
// o linker acusa múltipla definição.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
