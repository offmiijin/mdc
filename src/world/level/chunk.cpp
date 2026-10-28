// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/world/level/chunk.hpp"

#include <cstdlib>

namespace mdc::world::level {

Chunk::Chunk() : sections_(static_cast<std::size_t>(kSectionCount)) {}

int Chunk::section_index(int y) {
    // Deslocamento aritmético: -1 >> 4 é -1 (seção de baixo), não 0. É o mesmo
    // cuidado que `BlockPos::floor_to_int` toma para o lado negativo.
    return (y >> 4) - kMinSectionY;
}

std::uint32_t Chunk::get(int x, int y, int z) const {
    const int index = section_index(y);
    if (index < 0 || index >= kSectionCount) {
        return BlockRegistry::get().air_state();
    }
    return sections_[static_cast<std::size_t>(index)].get(x & 15, y & 15, z & 15);
}

void Chunk::set(int x, int y, int z, std::uint32_t state) {
    const int index = section_index(y);
    if (index < 0 || index >= kSectionCount) {
        return;
    }
    sections_[static_cast<std::size_t>(index)].set(x & 15, y & 15, z & 15, state);
}

const ChunkSection& Chunk::section(int index) const {
    return sections_[static_cast<std::size_t>(index)];
}

ChunkSection& Chunk::section(int index) {
    return sections_[static_cast<std::size_t>(index)];
}

} // namespace mdc::world::level
