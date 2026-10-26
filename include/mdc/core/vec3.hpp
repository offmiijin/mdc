// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

namespace mdc::core {

/// Vetor de três doubles. Espelha o uso do jogo (que é double em toda a física).
struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

} // namespace mdc::core
