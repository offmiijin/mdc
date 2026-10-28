// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

namespace mdc::core {

/// Vetor de três doubles. Espelha o uso do jogo (que é double em toda a física).
///
/// Imutável por convenção: todo método devolve um vetor novo. A ordem das
/// operações segue a do original, porque `(a+b)+c ≠ a+(b+c)` em bits e o
/// harness compara bits, não `==` (docs/HARNESS.md §2).
struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    [[nodiscard]] static constexpr Vec3 zero() noexcept { return {}; }

    [[nodiscard]] constexpr Vec3 add(const Vec3& other) const noexcept {
        return {x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] constexpr Vec3 subtract(const Vec3& other) const noexcept {
        return {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] constexpr Vec3 scale(double factor) const noexcept {
        return {x * factor, y * factor, z * factor};
    }

    [[nodiscard]] constexpr Vec3 multiply(const Vec3& other) const noexcept {
        return {x * other.x, y * other.y, z * other.z};
    }

    [[nodiscard]] constexpr double dot(const Vec3& other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] constexpr Vec3 cross(const Vec3& other) const noexcept {
        return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
    }

    [[nodiscard]] constexpr double length_sqr() const noexcept { return x * x + y * y + z * z; }

    /// `Math.sqrt` do Java é *correctly rounded* por exigência da linguagem, e o
    /// `sqrt` do C também é (IEEE-754): os bits são idênticos sem fdlibm.
    /// (fdlibm é necessário para `sin/cos/atan2/pow/exp/log`, não para raiz.)
    [[nodiscard]] double length() const noexcept;

    /// Vetor unitário. Abaixo de 1e-4 de comprimento o original devolve o vetor
    /// nulo em vez de amplificar ruído para 1,0 — o limiar é verificado pelo
    /// harness `vec3`, não estimado.
    [[nodiscard]] Vec3 normalize() const noexcept;

    [[nodiscard]] constexpr double distance_to_sqr(const Vec3& other) const noexcept {
        return subtract(other).length_sqr();
    }

    /// `Mth.lerp`: `from + alpha * (to - from)` componente a componente. Não é
    /// `from*(1-alpha) + to*alpha` — as duas formas divergem em ULP.
    [[nodiscard]] constexpr Vec3 lerp_to(const Vec3& to, double alpha) const noexcept {
        return {x + alpha * (to.x - x), y + alpha * (to.y - y), z + alpha * (to.z - z)};
    }

    [[nodiscard]] constexpr bool equals(const Vec3& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
};

} // namespace mdc::core
