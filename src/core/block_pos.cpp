// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/core/block_pos.hpp"

#include <cmath>
#include <limits>

namespace mdc::core {

int BlockPos::floor_to_int(double value) noexcept {
    const double floored = std::floor(value);
    // NaN não é comparável: `!(x < hi)` cobre o caso sem `std::isnan`.
    if (!(floored < static_cast<double>(std::numeric_limits<int>::max()))) {
        return std::numeric_limits<int>::max();
    }
    if (!(floored > static_cast<double>(std::numeric_limits<int>::min()))) {
        return std::numeric_limits<int>::min();
    }
    return static_cast<int>(floored);
}

BlockPos BlockPos::containing(const Vec3& point) noexcept {
    return {floor_to_int(point.x), floor_to_int(point.y), floor_to_int(point.z)};
}

} // namespace mdc::core
