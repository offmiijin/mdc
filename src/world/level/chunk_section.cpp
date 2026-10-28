// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/world/level/chunk_section.hpp"

namespace mdc::world::level {

ChunkSection::ChunkSection()
    : states_(BlockRegistry::get().air_state(),
              Strategy{BlockRegistry::get().state_count()}) {}

std::uint32_t ChunkSection::get(int x, int y, int z) const {
    return states_.get(x, y, z);
}

void ChunkSection::set(int x, int y, int z, std::uint32_t state) {
    const std::uint32_t previous = states_.get(x, y, z);
    if (previous == state) {
        return;
    }
    const std::uint32_t air = BlockRegistry::get().air_state();
    if (previous != air) {
        --non_air_count_;
    }
    if (state != air) {
        ++non_air_count_;
    }
    states_.set(x, y, z, state);
}

} // namespace mdc::world::level
