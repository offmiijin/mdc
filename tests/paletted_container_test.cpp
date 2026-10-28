// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Testes de unidade do container paletizado. A paridade bit a bit é do harness
// `palette`; aqui ficam as regras medidas (crescimento dos bits, ordem da
// paleta na serialização) em forma verificável sem Java.
#include <doctest/doctest.h>

#include <set>

#include "mdc/world/level/paletted_container.hpp"

using mdc::world::level::PalettedContainer;
using mdc::world::level::Strategy;

namespace {

/// Escreve `count` valores distintos nas primeiras `count` células.
void fill_distinct(PalettedContainer& container, int count) {
    for (int i = 0; i < count; ++i) {
        container.set(i & 15, (i >> 4) & 15, (i >> 8) & 15, static_cast<std::uint32_t>(i));
    }
}

} // namespace

TEST_CASE("container novo é valor único: zero bit, paleta de um, sem storage") {
    const PalettedContainer container(0U, Strategy{});
    CHECK(container.bits_per_entry() == 0);
    const auto packed = container.pack();
    CHECK(packed.palette == std::vector<std::uint32_t>{0U});
    CHECK(packed.bits == 0);
    CHECK(packed.storage.empty());
}

TEST_CASE("bits crescem em degraus e param em 12 na paleta global") {
    struct Step {
        int distinct;
        int bits;
    };
    const Step steps[] = {{1, 0}, {2, 4}, {16, 4}, {17, 5}, {33, 6}, {256, 8}, {257, 12}, {4096, 12}};
    for (const Step& step : steps) {
        PalettedContainer container(0U, Strategy{});
        fill_distinct(container, step.distinct);
        CHECK_MESSAGE(container.bits_per_entry() == step.bits, "degrau " << step.distinct);
    }
}

TEST_CASE("a paleta da serialização segue a primeira aparição no storage") {
    // A ordem de escrita não importa: quem manda é a varredura do storage, e o
    // default entra por último (é o valor que preenche o resto da seção).
    PalettedContainer container(0U, Strategy{});
    container.set(5, 0, 0, 9U);
    container.set(0, 0, 0, 4U);
    CHECK(container.pack().palette == std::vector<std::uint32_t>{4U, 0U, 9U});

    PalettedContainer reversed(0U, Strategy{});
    reversed.set(0, 0, 0, 4U);
    reversed.set(5, 0, 0, 9U);
    CHECK(reversed.pack().palette == std::vector<std::uint32_t>{4U, 0U, 9U});
}

TEST_CASE("get devolve o valor escrito e o default nas células intocadas") {
    PalettedContainer container(3U, Strategy{});
    container.set(1, 2, 3, 77U);
    CHECK(container.get(1, 2, 3) == 77U);
    CHECK(container.get(0, 0, 0) == 3U);
    CHECK(container.get(15, 15, 15) == 3U);
}

TEST_CASE("o índice linear usa y como eixo mais significativo") {
    CHECK(PalettedContainer::index(0, 0, 0) == 0);
    CHECK(PalettedContainer::index(1, 0, 0) == 1);
    CHECK(PalettedContainer::index(0, 0, 1) == 16);
    CHECK(PalettedContainer::index(0, 1, 0) == 256);
    CHECK(PalettedContainer::index(15, 15, 15) == 4095);
}

TEST_CASE("storage serializado guarda índice de paleta, não o valor") {
    PalettedContainer container(0U, Strategy{});
    container.set(0, 0, 0, 1000U);
    container.set(1, 0, 0, 2000U);
    const auto packed = container.pack();
    REQUIRE(packed.palette.size() == 3U);
    // Primeiras duas células são os dois valores escritos, na ordem da varredura.
    CHECK(packed.palette[0] == 1000U);
    CHECK(packed.palette[1] == 2000U);
    CHECK((packed.storage[0] & 0xFU) == 0U);
    CHECK(((packed.storage[0] >> 4) & 0xFU) == 1U);
}

TEST_CASE("a paleta guarda cada valor uma vez só") {
    PalettedContainer container(0U, Strategy{});
    for (int i = 0; i < 100; ++i) {
        container.set(i & 15, 0, 0, static_cast<std::uint32_t>(i % 5));
    }
    const auto packed = container.pack();
    const std::set<std::uint32_t> unique(packed.palette.begin(), packed.palette.end());
    CHECK(unique.size() == packed.palette.size());
}
