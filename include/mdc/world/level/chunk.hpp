// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <vector>

#include "mdc/world/level/chunk_section.hpp"

namespace mdc::world::level {

/// Chunk mínimo do M2: uma coluna de seções de 16³ e a aritmética de coordenada
/// de mundo para (seção, célula).
///
/// Sem altura variável, sem heightmap, sem tick list: nada disso é lido pelo
/// mesher, e o M2 declara o que fica de fora (ver docs/PARITY.md).
class Chunk {
  public:
    static constexpr int kEdge = 16;
    static constexpr int kMinSectionY = -4; /// primeira seção: y = -64
    static constexpr int kSectionCount = 24; /// -64 .. 319

    Chunk();

    [[nodiscard]] std::uint32_t get(int x, int y, int z) const;
    void set(int x, int y, int z, std::uint32_t state);

    [[nodiscard]] const ChunkSection& section(int index) const;
    [[nodiscard]] ChunkSection& section(int index);

    [[nodiscard]] static int section_index(int y);

  private:
    std::vector<ChunkSection> sections_;
};

} // namespace mdc::world::level
