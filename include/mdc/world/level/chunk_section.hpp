// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>

#include "mdc/world/level/block.hpp"
#include "mdc/world/level/paletted_container.hpp"

namespace mdc::world::level {

/// Seção de 16³ blocos: o container paletizado com o registro de estados como
/// espaço de valores.
///
/// É a unidade de comparação do M2 (`chunk-mesh` compila uma seção de cada vez)
/// e o que a paleta do M2.0 existe para servir.
class ChunkSection {
  public:
    ChunkSection();

    [[nodiscard]] std::uint32_t get(int x, int y, int z) const;
    void set(int x, int y, int z, std::uint32_t state);

    /// Contagem de estados não-ar (o `hasOnlyAir` do original).
    [[nodiscard]] bool has_only_air() const noexcept { return non_air_count_ == 0; }

    [[nodiscard]] int bits_per_entry() const noexcept { return states_.bits_per_entry(); }

  private:
    PalettedContainer states_;
    int non_air_count_{0};
};

} // namespace mdc::world::level
