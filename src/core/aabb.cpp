// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/core/aabb.hpp"

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

} // namespace mdc::core
