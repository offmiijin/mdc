// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/core/aabb.hpp"

#include <limits>

namespace mdc::core {

bool Aabb::intersects(const Aabb& other) const noexcept {
    // Comparações estritas: tocar a face não conta como interseção.
    return min_x_ < other.max_x_ && max_x_ > other.min_x_ && min_y_ < other.max_y_ &&
           max_y_ > other.min_y_ && min_z_ < other.max_z_ && max_z_ > other.min_z_;
}

Vec3 Aabb::center() const noexcept {
    // O original usa Mth.lerp(0.5, min, max) = min + 0.5 * (max - min).
    // Não é equivalente a (min + max) / 2 em bits: os dois divergem em ~10% dos
    // casos (1–2 ULP). Ver docs/HARNESS.md §2.
    return Vec3{min_x_ + 0.5 * (max_x_ - min_x_), min_y_ + 0.5 * (max_y_ - min_y_),
                min_z_ + 0.5 * (max_z_ - min_z_)};
}

Aabb Aabb::move(const Vec3& delta) const noexcept {
    return Aabb{min_x_ + delta.x, min_y_ + delta.y, min_z_ + delta.z,
                max_x_ + delta.x, max_y_ + delta.y, max_z_ + delta.z};
}

Aabb Aabb::inflate(double x_add, double y_add, double z_add) const noexcept {
    return Aabb{min_x_ - x_add, min_y_ - y_add, min_z_ - z_add,
                max_x_ + x_add, max_y_ + y_add, max_z_ + z_add};
}

Aabb Aabb::expand_towards(const Vec3& delta) const noexcept {
    // Cada limite só anda para o lado do sinal: delta positivo expande o máximo,
    // negativo o mínimo. O outro lado fica onde está.
    return Aabb{delta.x < 0.0 ? min_x_ + delta.x : min_x_, delta.y < 0.0 ? min_y_ + delta.y : min_y_,
                delta.z < 0.0 ? min_z_ + delta.z : min_z_, delta.x > 0.0 ? max_x_ + delta.x : max_x_,
                delta.y > 0.0 ? max_y_ + delta.y : max_y_, delta.z > 0.0 ? max_z_ + delta.z : max_z_};
}

bool Aabb::contains(const Vec3& point) const noexcept {
    // Inclusivo nas seis faces. Ponto exatamente na face conta como dentro — é o
    // que faz `clip` recusar um raio que parte da casca (ver harness `clip`).
    return point.x >= min_x_ && point.x <= max_x_ && point.y >= min_y_ && point.y <= max_y_ &&
           point.z >= min_z_ && point.z <= max_z_;
}

bool Aabb::has_nan() const noexcept {
    // Comparação com NaN é sempre falsa: `x != x` detecta o bit sem depender de
    // std::isnan e sem puxar <cmath> para o núcleo aritmético.
    return min_x_ != min_x_ || min_y_ != min_y_ || min_z_ != min_z_ || max_x_ != max_x_ ||
           max_y_ != max_y_ || max_z_ != max_z_;
}

namespace {

/// Intervalo de entrada/saída de um eixo. Devolve `false` quando o segmento é
/// paralelo ao eixo e está fora da laje — caso em que não há impacto.
bool axis_interval(double from, double delta, double lo, double hi, double& entry,
                   double& exit) noexcept {
    if (delta == 0.0) {
        if (from < lo || from > hi) {
            return false;
        }
        entry = -std::numeric_limits<double>::infinity();
        exit = std::numeric_limits<double>::infinity();
        return true;
    }
    const double first = (lo - from) / delta;
    const double second = (hi - from) / delta;
    entry = first < second ? first : second;
    exit = first < second ? second : first;
    return true;
}

} // namespace

std::optional<Vec3> Aabb::clip(const Vec3& from, const Vec3& to) const noexcept {
    // Ponto de partida na casca ou dentro não tem face de entrada: o original
    // devolve vazio (medido — inclusive para `-0.0` na face mínima).
    if (contains(from)) {
        return std::nullopt;
    }

    const Vec3 delta = to.subtract(from);

    double entry = -std::numeric_limits<double>::infinity();
    double exit = std::numeric_limits<double>::infinity();
    double axis_entry[3] = {0.0, 0.0, 0.0};

    const double froms[3] = {from.x, from.y, from.z};
    const double deltas[3] = {delta.x, delta.y, delta.z};
    const double lows[3] = {min_x_, min_y_, min_z_};
    const double highs[3] = {max_x_, max_y_, max_z_};

    for (int axis = 0; axis < 3; ++axis) {
        double axis_exit = 0.0;
        if (!axis_interval(froms[axis], deltas[axis], lows[axis], highs[axis], axis_entry[axis],
                           axis_exit)) {
            return std::nullopt;
        }
        if (axis_entry[axis] > entry) {
            entry = axis_entry[axis];
        }
        if (axis_exit < exit) {
            exit = axis_exit;
        }
    }

    // Entrada e saída podem coincidir: caixa de espessura zero é tocada num
    // único `t`, e o original aceita (medido nos casos de caixa degenerada).
    // Impacto exatamente em `to` (t == 1.0), não.
    if (entry > exit || entry < 0.0 || !(entry < 1.0)) {
        return std::nullopt;
    }

    // Ponto **sem grampear**: o original devolve o `lerp` puro, e ele pode cair
    // 1 ULP fora da caixa (caso 1420 do harness). Grampear "conserta" e diverge.
    return from.add(delta.scale(entry));
}

} // namespace mdc::core
