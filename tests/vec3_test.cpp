// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>
#include <limits>

#include "mdc/core/vec3.hpp"

using mdc::core::Vec3;

namespace {

std::uint64_t bits(double value) {
    std::uint64_t raw = 0;
    std::memcpy(&raw, &value, sizeof raw);
    return raw;
}

} // namespace

TEST_CASE("operações são componente a componente") {
    const Vec3 a{1.0, -2.0, 0.5};
    const Vec3 b{0.25, 4.0, -0.5};

    const Vec3 sum = a.add(b);
    CHECK(sum.x == 1.25);
    CHECK(sum.y == 2.0);
    CHECK(sum.z == 0.0);

    const Vec3 diff = a.subtract(b);
    CHECK(diff.x == 0.75);
    CHECK(diff.y == -6.0);
    CHECK(diff.z == 1.0);

    const Vec3 scaled = a.scale(2.0);
    CHECK(scaled.x == 2.0);
    CHECK(scaled.y == -4.0);
    CHECK(scaled.z == 1.0);
}

TEST_CASE("dot e cross seguem a ordem dos produtos do original") {
    const Vec3 a{2.0, 3.0, 4.0};
    const Vec3 b{5.0, 6.0, 7.0};

    CHECK(a.dot(b) == 2.0 * 5.0 + 3.0 * 6.0 + 4.0 * 7.0);

    const Vec3 c = a.cross(b);
    CHECK(c.x == 3.0 * 7.0 - 4.0 * 6.0);
    CHECK(c.y == 4.0 * 5.0 - 2.0 * 7.0);
    CHECK(c.z == 2.0 * 6.0 - 3.0 * 5.0);

    // O produto vetorial é ortogonal às duas entradas.
    CHECK(a.dot(c) == doctest::Approx(0.0).scale(1e-9));
}

TEST_CASE("lerp é from + alpha*(to-from), não a interpolação simétrica") {
    const Vec3 from{1.0, 2.0, 3.0};
    const Vec3 to{5.0, 4.0, -1.0};
    const double alpha = 0.25;

    const Vec3 mid = from.lerp_to(to, alpha);
    CHECK(mid.x == from.x + alpha * (to.x - from.x));
    CHECK(mid.y == from.y + alpha * (to.y - from.y));
    CHECK(mid.z == from.z + alpha * (to.z - from.z));

    // Par real onde as duas formas divergem em 1 ULP: 0.1 -> 0.3 com alpha=0.3.
    const Vec3 left{0.1, 0.1, 0.1};
    const Vec3 right{0.3, 0.3, 0.3};
    const Vec3 t = left.lerp_to(right, 0.3);
    const double symmetric = left.x * (1.0 - 0.3) + right.x * 0.3;

    CHECK(bits(t.x) == bits(0.16));
    CHECK(bits(t.x) != bits(symmetric));
}

TEST_CASE("normalize devolve vetor nulo abaixo do limiar") {
    const Vec3 tiny{9.0e-6, 0.0, 0.0};
    CHECK(tiny.normalize().equals(Vec3::zero()));

    CHECK(Vec3{}.normalize().equals(Vec3::zero()));

    // Acima do limiar, normaliza de verdade.
    CHECK(Vec3{1.0e-4, 0.0, 0.0}.normalize().x == 1.0);
}

TEST_CASE("o limiar de normalize é float(1e-5), não 1e-5 em double") {
    // Fronteira medida pelo harness `vec3` (bissecção no probe Java): o último
    // vetor zerado e o primeiro normalizado são vizinhos deste valor.
    const double threshold = static_cast<double>(1.0E-5F);
    CHECK(bits(threshold) == 0x3EE4F8B580000000ULL);
    CHECK(bits(1.0e-5) == 0x3EE4F8B588E368F1ULL); // 27 bits de mantissa a mais

    CHECK(Vec3{threshold, 0.0, 0.0}.normalize().x == 1.0);
    CHECK(Vec3{9.99999974737875e-06, 0.0, 0.0}.normalize().x == 0.0);
}

TEST_CASE("length e length_sqr") {
    const Vec3 v{3.0, 0.0, 4.0};
    CHECK(v.length_sqr() == 25.0);
    CHECK(v.length() == 5.0);

    const Vec3 zero{};
    CHECK(zero.length() == 0.0);

    const Vec3 self{3.0, -4.0, 12.0};
    CHECK(self.distance_to_sqr(self) == 0.0);

    const Vec3 origin{};
    CHECK(origin.distance_to_sqr(self) == self.length_sqr());
}

TEST_CASE("zero com sinal não é perdido na soma") {
    const Vec3 neg_zero{-0.0, 0.0, -0.0};

    // -0.0 + -0.0 = -0.0; -0.0 + 0.0 = +0.0. `==` diria que são iguais.
    CHECK(bits(neg_zero.add(neg_zero).x) == bits(-0.0));
    CHECK(bits(neg_zero.add(Vec3{}).x) == bits(0.0));
    CHECK(bits(neg_zero.x) == bits(-0.0));
    CHECK(bits(Vec3{}.x) == bits(0.0));
}
