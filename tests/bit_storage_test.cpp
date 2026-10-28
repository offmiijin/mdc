// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Testes de unidade do empacotamento de bits. A paridade com o original é do
// harness `palette`; aqui ficam as propriedades que devem valer por si.
#include <doctest/doctest.h>

#include "mdc/world/level/bit_storage.hpp"

using mdc::world::level::BitStorage;

TEST_CASE("valor nunca atravessa a fronteira de um long") {
    // 5 bits cabem 12 vezes em 60 bits (4 desperdiçados). Se a implementação
    // dividisse o valor entre dois longs, 4.096 células ocupariam 320 longs.
    const BitStorage storage(5, 4096);
    CHECK(storage.bits() == 5);
    CHECK(storage.size() == 4096);
    CHECK(storage.raw().size() == 342);
}

TEST_CASE("ida e volta preserva o valor em todas as posições do primeiro long") {
    BitStorage storage(5, 64);
    for (int i = 0; i < 12; ++i) {
        storage.set(i, (i * 7) & 31);
    }
    for (int i = 0; i < 12; ++i) {
        CHECK(storage.get(i) == ((i * 7) & 31));
    }
}

TEST_CASE("bits zero é o storage degenerado: tudo zero, nada alocado") {
    const BitStorage storage(0, 4096);
    CHECK(storage.bits() == 0);
    CHECK(storage.raw().empty());
    CHECK(storage.get(0) == 0);
    CHECK(storage.get(4095) == 0);
}

TEST_CASE("bits 32 cabe exatamente duas células por long") {
    BitStorage storage(32, 4);
    CHECK(storage.raw().size() == 2);
    storage.set(0, 7);
    storage.set(1, 9);
    CHECK(storage.raw()[0] == ((std::uint64_t{9} << 32) | 7U));
}
