// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/world/phys/collision.hpp"

#include <cmath>

namespace mdc::world::phys {

namespace {

using core::Aabb;

/// Quanto do movimento de entrada cabe no eixo antes de encostar no sólido.
///
/// Só considera o sólido se as **outras duas** lajes da caixa se sobrepõem às do
/// sólido — é essa checagem que impede o bloqueio "de lado" (encostar a lateral
/// não freia a queda). As comparações incluem a face (`<=`/`>=`), ao contrário
/// de `intersects`, que é estrito: a caixa que repousa exatamente sobre o sólido
/// continua barrada.
double clip_y(const Aabb& box, const Aabb& solid, double movement) noexcept {
    if (box.max_x() <= solid.min_x() || box.min_x() >= solid.max_x()) {
        return movement;
    }
    if (box.max_z() <= solid.min_z() || box.min_z() >= solid.max_z()) {
        return movement;
    }
    if (movement > 0.0 && box.max_y() <= solid.min_y()) {
        const double free = solid.min_y() - box.max_y();
        if (free < movement) {
            return free;
        }
    }
    if (movement < 0.0 && box.min_y() >= solid.max_y()) {
        const double free = solid.max_y() - box.min_y();
        if (free > movement) {
            return free;
        }
    }
    return movement;
}

double clip_x(const Aabb& box, const Aabb& solid, double movement) noexcept {
    if (box.max_y() <= solid.min_y() || box.min_y() >= solid.max_y()) {
        return movement;
    }
    if (box.max_z() <= solid.min_z() || box.min_z() >= solid.max_z()) {
        return movement;
    }
    if (movement > 0.0 && box.max_x() <= solid.min_x()) {
        const double free = solid.min_x() - box.max_x();
        if (free < movement) {
            return free;
        }
    }
    if (movement < 0.0 && box.min_x() >= solid.max_x()) {
        const double free = solid.max_x() - box.min_x();
        if (free > movement) {
            return free;
        }
    }
    return movement;
}

double clip_z(const Aabb& box, const Aabb& solid, double movement) noexcept {
    if (box.max_x() <= solid.min_x() || box.min_x() >= solid.max_x()) {
        return movement;
    }
    if (box.max_y() <= solid.min_y() || box.min_y() >= solid.max_y()) {
        return movement;
    }
    if (movement > 0.0 && box.max_z() <= solid.min_z()) {
        const double free = solid.min_z() - box.max_z();
        if (free < movement) {
            return free;
        }
    }
    if (movement < 0.0 && box.min_z() >= solid.max_z()) {
        const double free = solid.max_z() - box.min_z();
        if (free > movement) {
            return free;
        }
    }
    return movement;
}

} // namespace

MoveResult collide_move(const Aabb& box, const core::Vec3& movement,
                        std::span<const Aabb> solids) noexcept {
    MoveResult result;
    if (solids.empty() || movement.equals(core::Vec3::zero())) {
        result.applied = movement;
        return result;
    }

    // Sólido de espessura zero não colide: o shape correspondente é vazio no
    // jogo. Sem isso, uma "laje" de altura 0 barra a queda de quem já está
    // apoiado nela (caso 15 do harness).
    const auto degenerate = [](const Aabb& solid) noexcept {
        return solid.min_x() == solid.max_x() || solid.min_y() == solid.max_y() ||
               solid.min_z() == solid.max_z();
    };

    // Ordem do original (medida pelo harness `collision`, não deduzida):
    // **y primeiro**, e depois o eixo de maior |movimento| entre x e z — empate
    // vai para x. A caixa anda entre os eixos: o movimento de um eixo muda a
    // sobreposição que decide o seguinte, e é por isso que a ordem importa.
    // Uma ordem fixa (y,x,z) diverge em ~0,3% dos casos diagonais.
    Aabb current = box;

    double dy = movement.y;
    for (const Aabb& solid : solids) {
        if (!degenerate(solid)) {
            dy = clip_y(current, solid, dy);
        }
    }
    current = current.move(core::Vec3{0.0, dy, 0.0});

    double dx = movement.x;
    double dz = movement.z;
    const bool x_first = !(std::abs(movement.x) < std::abs(movement.z));
    if (x_first) {
        for (const Aabb& solid : solids) {
            if (!degenerate(solid)) {
                dx = clip_x(current, solid, dx);
            }
        }
        current = current.move(core::Vec3{dx, 0.0, 0.0});
        for (const Aabb& solid : solids) {
            if (!degenerate(solid)) {
                dz = clip_z(current, solid, dz);
            }
        }
    } else {
        for (const Aabb& solid : solids) {
            if (!degenerate(solid)) {
                dz = clip_z(current, solid, dz);
            }
        }
        current = current.move(core::Vec3{0.0, 0.0, dz});
        for (const Aabb& solid : solids) {
            if (!degenerate(solid)) {
                dx = clip_x(current, solid, dx);
            }
        }
    }

    result.applied = core::Vec3{dx, dy, dz};
    // Flag derivada do movimento barrado (o original a marca na entidade, que
    // não existe num harness de geometria): desceu menos do que pediu.
    result.on_ground = dy != movement.y && movement.y < 0.0;
    result.horizontal_collision = (dx != movement.x || dz != movement.z);
    return result;
}

} // namespace mdc::world::phys
