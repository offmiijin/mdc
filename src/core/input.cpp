// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/core/input.hpp"

#include <cmath>

namespace mdc::core {

void InputBuffer::add_mouse_delta(double dx, double dy) noexcept {
    if (!std::isfinite(dx) || !std::isfinite(dy)) {
        return;
    }
    mouse_dx_ += dx;
    mouse_dy_ += dy;
}

InputFrame InputBuffer::consume() noexcept {
    const InputFrame frame{keys_, mouse_dx_, mouse_dy_};
    // Só o mouse é zerado: o teclado é nível e vale até o cliente reescrevê-lo.
    mouse_dx_ = 0.0;
    mouse_dy_ = 0.0;
    return frame;
}

} // namespace mdc::core
