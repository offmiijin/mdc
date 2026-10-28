// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/core/vec3.hpp"

#include <cmath>

namespace mdc::core {

namespace {

/// Comprimento abaixo do qual `normalize()` considera o vetor nulo.
///
/// **Medido pelo harness `vec3`** até o último bit, não estimado: a bissecção
/// (probe Java) mostra que o último vetor zerado e o primeiro normalizado são
/// vizinhos em torno de `0x3EE4F8B580000000` — que é exatamente `(double)1e-5f`,
/// e **não** `1e-5` em double (`0x3EE4F8B588E368F1`). O original compara com um
/// literal float promovido; 27 bits de mantissa a menos mudam 147 dos 2.020 casos.
constexpr double kNormalizeEpsilon = static_cast<double>(1.0E-5F);

} // namespace

double Vec3::length() const noexcept {
    return std::sqrt(length_sqr());
}

Vec3 Vec3::normalize() const noexcept {
    const double len = length();
    if (len < kNormalizeEpsilon) {
        return zero();
    }
    return {x / len, y / len, z / len};
}

} // namespace mdc::core
