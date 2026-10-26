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

} // namespace

TEST_CASE("intersects é estrito: tocar a face não conta") {
    const Aabb unit{0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

    CHECK(unit.intersects(Aabb{0.5, 0.5, 0.5, 1.5, 1.5, 1.5}));
    CHECK(!unit.intersects(Aabb{1.0, 0.0, 0.0, 2.0, 1.0, 1.0}));   // encosta em X
    CHECK(!unit.intersects(Aabb{0.0, 1.0, 0.0, 1.0, 2.0, 1.0}));   // encosta em Y
    CHECK(!unit.intersects(Aabb{0.0, 0.0, -1.0, 1.0, 1.0, 0.0}));  // encosta em Z
    CHECK(!unit.intersects(Aabb{2.0, 2.0, 2.0, 3.0, 3.0, 3.0}));   // longe
    CHECK(unit.intersects(unit));                                  // idênticas
}

TEST_CASE("center usa a forma do lerp, não a média") {
    // Par real onde (min+max)/2 e min+0.5*(max-min) divergem em bits.
    const double lo = -21938.145353255928;
    const double hi = 20846.024216233964;
    const Aabb box{lo, lo, lo, hi, hi, hi};
    const Vec3 c = box.center();

    CHECK(bits(c.x) == bits(lo + 0.5 * (hi - lo)));
    CHECK(bits(c.x) != bits((lo + hi) / 2.0)); // 1–2 ULP de diferença
}

TEST_CASE("center de caixa unitária") {
    const Vec3 c = Aabb{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}.center();
    CHECK(c.x == 0.5);
    CHECK(c.y == 0.5);
    CHECK(c.z == 0.5);
}

TEST_CASE("move soma o mesmo delta nos dois lados") {
    const Aabb moved = Aabb{0.0, 0.0, 0.0, 1.0, 2.0, 3.0}.move(Vec3{1.0, -1.0, 0.5});
    CHECK(moved.min_x() == 1.0);
    CHECK(moved.min_y() == -1.0);
    CHECK(moved.min_z() == 0.5);
    CHECK(moved.max_x() == 2.0);
    CHECK(moved.max_y() == 1.0);
    CHECK(moved.max_z() == 3.5);
}

TEST_CASE("inflate expande cada eixo pelo seu valor") {
    const Aabb inflated = Aabb{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}.inflate(0.5, 1.0, 2.0);
    CHECK(inflated.min_x() == -0.5);
    CHECK(inflated.min_y() == -1.0);
    CHECK(inflated.min_z() == -2.0);
    CHECK(inflated.max_x() == 1.5);
    CHECK(inflated.max_y() == 2.0);
    CHECK(inflated.max_z() == 3.0);
}

TEST_CASE("o construtor normaliza: caixa invertida não existe") {
    // O original aplica Math.min/Math.max a cada par no construtor. Descoberto
    // pelo harness: 1.865 de 2.007 casos divergiam sem isso.
    const Aabb inverted{5.0, 5.0, 5.0, 1.0, 1.0, 1.0};
    CHECK(inverted.min_x() == 1.0);
    CHECK(inverted.min_y() == 1.0);
    CHECK(inverted.min_z() == 1.0);
    CHECK(inverted.max_x() == 5.0);
    CHECK(inverted.max_y() == 5.0);
    CHECK(inverted.max_z() == 5.0);

    const Aabb degenerate{2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
    CHECK(degenerate.min_x() == 2.0);
    CHECK(degenerate.max_x() == 2.0);
    // Volume zero: não intercepta nada, nem a si mesma (comparações estritas).
    CHECK(!degenerate.intersects(degenerate));
}

TEST_CASE("java_min/java_max preservam o sinal do zero") {
    // std::min devolveria o primeiro argumento e erraria o bit de sinal.
    CHECK(bits(mdc::core::java_min(0.0, -0.0)) == bits(-0.0));
    CHECK(bits(mdc::core::java_min(-0.0, 0.0)) == bits(-0.0));
    CHECK(bits(mdc::core::java_max(0.0, -0.0)) == bits(0.0));
    CHECK(bits(mdc::core::java_max(-0.0, 0.0)) == bits(0.0));
    CHECK(bits(mdc::core::java_max(-0.0, -0.0)) == bits(-0.0));

    Aabb mixed{-0.0, -0.0, -0.0, 1.0, 1.0, 1.0};
    CHECK(bits(mixed.min_x()) == bits(-0.0));

    // Regressão: iguais e negativos (não-zero) NÃO são zero. Foi o bug que o
    // caso 42 do harness pegou — caixa degenerada em X virava min=0.
    CHECK(mdc::core::java_min(-510.625, -510.625) == -510.625);
    CHECK(mdc::core::java_max(-510.625, -510.625) == -510.625);
    CHECK(bits(mdc::core::java_min(-1.0, -1.0)) == bits(-1.0));
    const Aabb flat{-510.625, 0.0, 0.0, -510.625, 1.0, 1.0};
    CHECK(flat.min_x() == -510.625);
    CHECK(flat.center().x == -510.625);
}
