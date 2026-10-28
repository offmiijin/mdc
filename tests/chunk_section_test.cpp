// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include <doctest/doctest.h>

#include "mdc/world/level/chunk.hpp"

using mdc::world::level::BlockRegistry;
using mdc::world::level::Chunk;
using mdc::world::level::ChunkSection;

TEST_CASE("seção nova é só ar") {
    const ChunkSection section;
    CHECK(section.has_only_air());
    CHECK(section.bits_per_entry() == 0);
    CHECK(section.get(0, 0, 0) == BlockRegistry::get().air_state());
}

TEST_CASE("seção conta estados não-ar, inclusive ao voltar para ar") {
    const BlockRegistry& registry = BlockRegistry::get();
    const auto stone = registry.default_state(*registry.find_block("minecraft:stone"));

    ChunkSection section;
    section.set(1, 2, 3, stone);
    CHECK_FALSE(section.has_only_air());
    CHECK(section.get(1, 2, 3) == stone);
    CHECK(section.bits_per_entry() == 4);

    section.set(1, 2, 3, registry.air_state());
    CHECK(section.has_only_air());
    // A paleta não encolhe, como no original.
    CHECK(section.bits_per_entry() == 4);
}

TEST_CASE("chunk mapeia coordenada de mundo para seção, inclusive y negativo") {
    const BlockRegistry& registry = BlockRegistry::get();
    const auto stone = registry.default_state(*registry.find_block("minecraft:stone"));

    CHECK(Chunk::section_index(0) == 4);
    CHECK(Chunk::section_index(15) == 4);
    CHECK(Chunk::section_index(16) == 5);
    CHECK(Chunk::section_index(-1) == 3);
    CHECK(Chunk::section_index(-64) == 0);

    Chunk chunk;
    chunk.set(3, -64, 9, stone);
    CHECK(chunk.get(3, -64, 9) == stone);
    CHECK(chunk.section(0).get(3, 0, 9) == stone);
    // Fora da faixa: ar, sem estourar o vetor.
    CHECK(chunk.get(3, -65, 9) == registry.air_state());
    CHECK(chunk.get(3, 320, 9) == registry.air_state());
}
