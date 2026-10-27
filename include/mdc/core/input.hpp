// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>

namespace mdc::core {

/// Ação que o laço de tick consome. O mapeamento tecla → ação é do cliente
/// (que é quem conhece SDL); o núcleo só sabe que existe um conjunto de bits.
enum class InputAction : std::uint8_t {
    kForward,
    kBack,
    kLeft,
    kRight,
    kJump,
    kSneak,
    kSprint,
    kCount,
};

/// Bit correspondente à ação, para compor a máscara de teclas.
[[nodiscard]] constexpr std::uint16_t mask_of(InputAction action) noexcept {
    return static_cast<std::uint16_t>(1U << static_cast<unsigned>(action));
}

/// Entrada de **um** tick: o que estava pressionado e o mouse que chegou desde
/// o tick anterior.
struct InputFrame {
    std::uint16_t keys{0};
    double mouse_dx{0.0};
    double mouse_dy{0.0};

    [[nodiscard]] constexpr bool held(InputAction action) const noexcept {
        return (keys & mask_of(action)) != 0U;
    }
};

/// Costura entre o laço de eventos (SDL, no cliente) e o laço de tick (núcleo,
/// determinístico). Duas semânticas diferentes convivem aqui de propósito:
///
/// - **teclado é nível**: `set_keys()` substitui o conjunto. O cliente reescreve
///   a máscara a cada quadro, então o tick sempre lê o estado corrente — soltar
///   a tecla não depende de nenhum evento ser entregue.
/// - **mouse é borda**: o movimento chega em eventos, muitos por tick, e cada
///   delta vale uma vez. Acumular e zerar no `consume()` garante que nenhum
///   delta se perde nem é aplicado duas vezes — é o "sem tick perdido ou
///   duplicado" do critério do M0 (docs/PARITY.md).
class InputBuffer {
  public:
    void set_keys(std::uint16_t keys) noexcept { keys_ = keys; }

    /// Soma ao acumulado do próximo tick. Deltas não finitos (NaN/Inf de driver
    /// quebrado) são descartados em par: um NaN aqui envenenaria a posição para
    /// sempre, e é invisível até virar teleporte.
    void add_mouse_delta(double dx, double dy) noexcept;

    /// Devolve a entrada deste tick e zera só os deltas do mouse. O teclado
    /// continua valendo para o tick seguinte (é nível, não borda).
    [[nodiscard]] InputFrame consume() noexcept;

    [[nodiscard]] std::uint16_t keys() const noexcept { return keys_; }

  private:
    std::uint16_t keys_{0};
    double mouse_dx_{0.0};
    double mouse_dy_{0.0};
};

} // namespace mdc::core
