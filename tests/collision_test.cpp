// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include <doctest/doctest.h>

#include <array>

#include "mdc/world/phys/collision.hpp"

using mdc::core::Aabb;
using mdc::core::Vec3;
using mdc::world::phys::collide_move;

namespace {

const Aabb kPlayer{0.0, 0.0, 0.0, 0.6, 1.8, 0.6};
const Aabb kFloor{-8.0, -1.0, -8.0, 8.0, 0.0, 8.0};

} // namespace

TEST_CASE("colisão: cai até o chão e para") {
    const std::array<Aabb, 1> solids{kFloor};
    const auto result = collide_move(kPlayer, Vec3{0.0, -0.5, 0.0}, solids);
    CHECK(result.applied.y == 0.0);
    CHECK(result.on_ground);
    CHECK(!result.horizontal_collision);
}

TEST_CASE("colisão: subir e andar não são barrados pelo chão") {
    const std::array<Aabb, 1> solids{kFloor};

    const auto up = collide_move(kPlayer, Vec3{0.0, 0.5, 0.0}, solids);
    CHECK(up.applied.y == 0.5);
    CHECK(!up.on_ground);

    const auto slide = collide_move(kPlayer, Vec3{0.5, -0.5, 0.0}, solids);
    CHECK(slide.applied.x == 0.5);
    CHECK(slide.applied.y == 0.0);
    CHECK(slide.on_ground);
}

TEST_CASE("colisão: parede barra x e deixa deslizar em z") {
    const Aabb wall{2.0, 0.0, -4.0, 3.0, 4.0, 4.0};
    const std::array<Aabb, 2> solids{wall, kFloor};

    const auto result = collide_move(kPlayer, Vec3{3.0, -1.0, 0.0}, solids);
    CHECK(result.applied.x == 1.4); // 2.0 - 0.6
    CHECK(result.applied.y == 0.0);
    CHECK(result.horizontal_collision);
    CHECK(result.on_ground);
}

TEST_CASE("colisão: sem sólidos o movimento passa inteiro") {
    const auto free = collide_move(kPlayer, Vec3{0.5, -0.5, 0.25}, {});
    CHECK(free.applied.x == 0.5);
    CHECK(free.applied.y == -0.5);
    CHECK(free.applied.z == 0.25);
    CHECK(!free.on_ground);
    CHECK(!free.horizontal_collision);
}

TEST_CASE("colisão: sólido de espessura zero não colide") {
    // Laje de altura 0: no jogo o shape correspondente é vazio.
    const std::array<Aabb, 1> flat{Aabb{-8.0, 0.0, -8.0, 8.0, 0.0, 8.0}};
    const auto result = collide_move(kPlayer, Vec3{0.0, -0.5, 0.0}, flat);
    CHECK(result.applied.y == -0.5);
    CHECK(!result.on_ground);
}

TEST_CASE("colisão: a ordem dos eixos é y, depois o maior movimento") {
    // Dois bloqueadores: +x em 0.5 e +z em 0.5. Quem anda primeiro decide.
    const Aabb x_blocker{1.5, 0.0, -1.0, 2.5, 2.0, 0.5};
    const Aabb z_blocker{-1.0, 0.0, 1.5, 0.5, 2.0, 2.5};
    const Aabb box{0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    const std::array<Aabb, 2> solids{x_blocker, z_blocker};

    // |dz| > |dx|: z anda primeiro e para em 0.5; x escapa inteiro.
    const auto z_bigger = collide_move(box, Vec3{1.0, 0.0, 2.0}, solids);
    CHECK(z_bigger.applied.x == 1.0);
    CHECK(z_bigger.applied.z == 0.5);

    // |dx| > |dz|: x anda primeiro e para em 0.5; z escapa inteiro.
    const auto x_bigger = collide_move(box, Vec3{2.0, 0.0, 1.0}, solids);
    CHECK(x_bigger.applied.x == 0.5);
    CHECK(x_bigger.applied.z == 1.0);

    // Empate vai para x.
    const auto tie = collide_move(box, Vec3{1.0, 0.0, 1.0}, solids);
    CHECK(tie.applied.x == 0.5);
    CHECK(tie.applied.z == 1.0);
}
