// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <span>

#include "mdc/core/aabb.hpp"
#include "mdc/core/vec3.hpp"

namespace mdc::world::phys {

/// Resultado de resolver um movimento contra sólidos.
struct MoveResult {
    core::Vec3 applied{};                ///< movimento realmente aplicado (o resto foi barrado)
    bool on_ground{false};         ///< parou de descer: havia sólido abaixo
    bool horizontal_collision{false};
};

/// Aplica `movement` à caixa `box`, barrando em cada sólido de `solids`.
///
/// A ordem dos eixos é a do original e foi **medida** pelo harness `collision`:
/// **y primeiro** e depois o eixo de maior `|movimento|` entre x e z (empate vai
/// para x) — a caixa anda entre os eixos, e o movimento de um eixo muda a
/// sobreposição que decide o seguinte. Ordem fixa (y, x, z) diverge em ~0,3% dos
/// casos diagonais.
[[nodiscard]] MoveResult collide_move(const core::Aabb& box, const core::Vec3& movement,
                                      std::span<const core::Aabb> solids) noexcept;

} // namespace mdc::world::phys
