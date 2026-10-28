// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>

#include "mdc/core/aabb.hpp"

using mdc::core::Aabb;
using mdc::core::Vec3;

namespace {

std::uint64_t bits(double value) {
    std::uint64_t raw = 0;
    std::memcpy(&raw, &value, sizeof raw);
    return raw;
}

const Aabb kUnit{0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

} // namespace

TEST_CASE("clip: de fora para fora devolve o ponto de entrada") {
    const auto hit = kUnit.clip(Vec3{-1.0, 0.5, 0.5}, Vec3{2.0, 0.5, 0.5});
    REQUIRE(hit.has_value());
    CHECK(hit->x == 0.0);
    CHECK(hit->y == 0.5);
    CHECK(hit->z == 0.5);

    // Sentido invertido entra pela face oposta.
    const auto back = kUnit.clip(Vec3{2.0, 0.5, 0.5}, Vec3{-1.0, 0.5, 0.5});
    REQUIRE(back.has_value());
    CHECK(back->x == 1.0);

    const auto up = kUnit.clip(Vec3{0.5, -3.0, 0.5}, Vec3{0.5, 3.0, 0.5});
    REQUIRE(up.has_value());
    CHECK(up->y == 0.0);
}

TEST_CASE("clip: partir de dentro ou da casca não tem face de entrada") {
    CHECK(!kUnit.clip(Vec3{0.5, 0.5, 0.5}, Vec3{2.0, 0.5, 0.5}).has_value());
    CHECK(!kUnit.clip(Vec3{0.0, 0.5, 0.5}, Vec3{2.0, 0.5, 0.5}).has_value());
    CHECK(!kUnit.clip(Vec3{-0.0, 0.5, 0.5}, Vec3{2.0, 0.5, 0.5}).has_value());
    CHECK(!kUnit.clip(Vec3{1.0, 1.0, 1.0}, Vec3{2.0, 2.0, 2.0}).has_value());
}

TEST_CASE("clip: impacto exatamente em to não conta") {
    CHECK(kUnit.clip(Vec3{-1.0, 0.5, 0.5}, Vec3{0.5, 0.5, 0.5}).has_value());
    CHECK(!kUnit.clip(Vec3{-1.0, 0.5, 0.5}, Vec3{0.0, 0.5, 0.5}).has_value());
}

TEST_CASE("clip: eixo paralelo fora da laje não cruza") {
    CHECK(!kUnit.clip(Vec3{-1.0, 2.0, 0.5}, Vec3{2.0, 2.0, 0.5}).has_value());
    CHECK(!kUnit.clip(Vec3{-1.0, 0.5, -3.0}, Vec3{-1.0, 0.5, 3.0}).has_value());

    // Rasante sobre a face conta: a coordenada fica no limite.
    const auto graze = kUnit.clip(Vec3{-1.0, 1.0, 0.5}, Vec3{2.0, 1.0, 0.5});
    REQUIRE(graze.has_value());
    CHECK(graze->x == 0.0);
    CHECK(graze->y == 1.0);
}

TEST_CASE("clip: caixa de espessura zero é tocada num único t") {
    const Aabb flat_x{0.0, 0.0, 0.0, 0.0, 1.0, 1.0};
    const auto hit = flat_x.clip(Vec3{-1.0, 0.5, 0.5}, Vec3{2.0, 0.5, 0.5});
    REQUIRE(hit.has_value());
    CHECK(hit->x == 0.0);

    const Aabb point{2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
    CHECK(point.clip(Vec3{0.0, 2.0, 2.0}, Vec3{4.0, 2.0, 2.0}).has_value());
    CHECK(point.clip(Vec3{0.0, 0.0, 0.0}, Vec3{4.0, 4.0, 4.0}).has_value());
}

TEST_CASE("clip: empate de eixos devolve o canto") {
    const auto hit = kUnit.clip(Vec3{-1.0, -1.0, 0.5}, Vec3{0.5, 0.5, 0.5});
    REQUIRE(hit.has_value());
    CHECK(hit->x == 0.0);
    CHECK(hit->y == 0.0);
    CHECK(hit->z == 0.5);
}

TEST_CASE("clip: o ponto é lerp puro, não grampeado à caixa") {
    // Caso do harness em que o lerp cai 1 ULP acima de max_z = 3.375, e o
    // original devolve esse valor mesmo assim. Grampear "conserta" e diverge.
    const Aabb box{0.25, -2.625, 0.75, 1.625, 1.375, 3.375};
    const auto hit = box.clip(Vec3{7.375, 1.375, 7.25}, Vec3{-0.125, 0.5, 2.375});
    REQUIRE(hit.has_value());
    CHECK(bits(hit->z) == 0x400B000000000001ULL);
    CHECK(hit->z > box.max_z());
}
