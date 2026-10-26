// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/core/tick_clock.hpp"

namespace mdc::core {

int TickClock::advance(std::int64_t elapsed_ns) noexcept {
    if (elapsed_ns <= 0) {
        return 0;
    }
    accumulator_ns_ += elapsed_ns;

    std::int64_t due = accumulator_ns_ / kTickDurationNs;
    accumulator_ns_ -= due * kTickDurationNs;

    if (due > kMaxTicksPerCall) {
        dropped_ += due - kMaxTicksPerCall;
        due = kMaxTicksPerCall;
    }

    ticks_ += due;
    return static_cast<int>(due);
}

} // namespace mdc::core
