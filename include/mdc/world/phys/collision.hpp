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
/// A ordem dos eixos é a do original (y, depois x, depois z) e a caixa é movida
/// entre os eixos — quem decide a ordem é o harness `collision`, não a intuição.
[[nodiscard]] MoveResult collide_move(const core::Aabb& box, const core::Vec3& movement,
                                      std::span<const core::Aabb> solids) noexcept;

} // namespace mdc::world::phys
