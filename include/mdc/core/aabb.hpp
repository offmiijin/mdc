// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <optional>

#include <bit>
#include <cstdint>

#include "mdc/core/vec3.hpp"

namespace mdc::core {

/// `Math.min` do Java para doubles, com a mesma semântica de zero:
/// entre `+0.0` e `-0.0` o resultado é **`-0.0`**. `std::min` do C++ devolve o
/// primeiro argumento e erra o bit de sinal — diferença invisível em `==` e
/// fatal numa comparação por bits.
[[nodiscard]] constexpr bool is_negative(double value) noexcept {
    return (std::bit_cast<std::uint64_t>(value) >> 63U) != 0U;
}

[[nodiscard]] constexpr double java_min(double a, double b) noexcept {
    if (a < b) {
        return a;
    }
    if (b < a) {
        return b;
    }
    // Iguais. Só o zero tem duas representações: -0.0 vence.
    if (a == 0.0 && b == 0.0) {
        return (is_negative(a) || is_negative(b)) ? -0.0 : 0.0;
    }
    return a;
}

[[nodiscard]] constexpr double java_max(double a, double b) noexcept {
    if (a > b) {
        return a;
    }
    if (b > a) {
        return b;
    }
    if (a == 0.0 && b == 0.0) {
        return (is_negative(a) && is_negative(b)) ? -0.0 : 0.0;
    }
    return a;
}

/// Caixa alinhada aos eixos (AABB), com doubles — o tipo que o jogo usa para
/// colisão, raycast e limites de entidade.
///
/// O construtor **normaliza**: `min` é sempre o menor dos dois valores. Caixa
/// invertida não existe no jogo, porque `AABB(double…)` aplica `Math.min`/
/// `Math.max` a cada par. (Descoberto pelo harness: 1.865 de 2.007 casos
/// divergiam por causa disso.)
///
/// A paridade de `intersects`, `center`, `move` e `inflate` é verificada por
/// `tools/parity/run_aabb.sh`; as expressões seguem a mesma ordem de operações
/// do original de propósito (docs/HARNESS.md §2).
class Aabb {
  public:
    constexpr Aabb() = default;
    constexpr Aabb(double min_x, double min_y, double min_z, double max_x, double max_y,
                   double max_z) noexcept
        : min_x_(java_min(min_x, max_x)),
          min_y_(java_min(min_y, max_y)),
          min_z_(java_min(min_z, max_z)),
          max_x_(java_max(min_x, max_x)),
          max_y_(java_max(min_y, max_y)),
          max_z_(java_max(min_z, max_z)) {}

    [[nodiscard]] constexpr double min_x() const noexcept { return min_x_; }
    [[nodiscard]] constexpr double min_y() const noexcept { return min_y_; }
    [[nodiscard]] constexpr double min_z() const noexcept { return min_z_; }
    [[nodiscard]] constexpr double max_x() const noexcept { return max_x_; }
    [[nodiscard]] constexpr double max_y() const noexcept { return max_y_; }
    [[nodiscard]] constexpr double max_z() const noexcept { return max_z_; }

    /// Interseção estrita: caixas que apenas se tocam **não** se interceptam.
    [[nodiscard]] bool intersects(const Aabb& other) const noexcept;

    [[nodiscard]] Vec3 center() const noexcept;

    [[nodiscard]] Aabb move(const Vec3& delta) const noexcept;

    [[nodiscard]] Aabb inflate(double x_add, double y_add, double z_add) const noexcept;

    /// Expande cada limite na direção do delta (só para o lado do sinal).
    [[nodiscard]] Aabb expand_towards(const Vec3& delta) const noexcept;

    /// Pertencimento **inclusivo**: ponto na face conta como dentro — ao
    /// contrário de `intersects`, que é estrito.
    [[nodiscard]] bool contains(const Vec3& point) const noexcept;

    [[nodiscard]] bool has_nan() const noexcept;

    /// Primeiro ponto onde o segmento `from`→`to` toca a caixa.
    ///
    /// `nullopt` quando não há impacto de entrada: segmento que não cruza,
    /// `from` dentro ou sobre a face (não existe face de entrada) e impacto que
    /// cai exatamente em `to` — o comportamento foi medido no harness `clip`,
    /// não deduzido.
    [[nodiscard]] std::optional<Vec3> clip(const Vec3& from, const Vec3& to) const noexcept;

  private:
    double min_x_{0.0};
    double min_y_{0.0};
    double min_z_{0.0};
    double max_x_{0.0};
    double max_y_{0.0};
    double max_z_{0.0};
};

} // namespace mdc::core
