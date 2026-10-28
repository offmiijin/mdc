// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>

#include "mdc/core/vec3.hpp"

namespace mdc::core {

/// Coordenada de bloco: três inteiros, imutável.
///
/// O `java_floor` que converte `double` em `int` tem semântica própria: satura
/// em `INT_MIN`/`INT_MAX` e transforma NaN em 0 — o cast do Java, não o do C++
/// (que é indefinido fora da faixa).
struct BlockPos {
    int x{0};
    int y{0};
    int z{0};

    /// `Mth.floor` do Java: `Math.floor` e depois cast com saturação.
    [[nodiscard]] static int floor_to_int(double value) noexcept;

    /// `BlockPos.containing(Vec3)`: o bloco que contém o ponto.
    [[nodiscard]] static BlockPos containing(const Vec3& point) noexcept;

    [[nodiscard]] constexpr BlockPos offset(int dx, int dy, int dz) const noexcept {
        return {x + dx, y + dy, z + dz};
    }

    [[nodiscard]] constexpr BlockPos above() const noexcept { return offset(0, 1, 0); }
    [[nodiscard]] constexpr BlockPos below() const noexcept { return offset(0, -1, 0); }

    [[nodiscard]] constexpr bool equals(const BlockPos& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }

    /// Empacotamento em `long` usado como chave de mapa no jogo.
    [[nodiscard]] constexpr std::int64_t as_long() const noexcept {
        return (static_cast<std::int64_t>(x) & 0x3FFFFFF) << 38 |
               (static_cast<std::int64_t>(z) & 0x3FFFFFF) << 12 |
               (static_cast<std::int64_t>(y) & 0xFFF);
    }
};

} // namespace mdc::core
