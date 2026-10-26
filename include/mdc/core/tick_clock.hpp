// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>

namespace mdc::core {

/// Relógio de tick fixo a 20 Hz, com acumulador inteiro em nanossegundos.
///
/// Deliberadamente **não** consulta nenhum relógio: quem chama entrega o tempo
/// decorrido. Isso o torna determinístico e testável sem dormir nem esperar —
/// requisito do harness (ver docs/HARNESS.md, seção 1).
class TickClock {
  public:
    static constexpr std::int64_t kTicksPerSecond = 20;
    static constexpr std::int64_t kTickDurationNs = 1'000'000'000 / kTicksPerSecond; // 50 ms
    /// Teto de ticks por chamada: corta a "espiral da morte" quando uma pausa
    /// longa faz o acumulador explodir. O excedente é contado, não silenciado.
    static constexpr std::int64_t kMaxTicksPerCall = kTicksPerSecond; // 1 s de simulação

    /// Entrega o tempo decorrido; devolve quantos ticks simular agora.
    /// Valores <= 0 são ignorados (proteção contra delta negativo de relógio).
    [[nodiscard]] int advance(std::int64_t elapsed_ns) noexcept;

    [[nodiscard]] std::int64_t ticks() const noexcept { return ticks_; }
    [[nodiscard]] std::int64_t dropped_ticks() const noexcept { return dropped_; }
    [[nodiscard]] std::int64_t accumulator_ns() const noexcept { return accumulator_ns_; }

  private:
    std::int64_t accumulator_ns_{0};
    std::int64_t ticks_{0};
    std::int64_t dropped_{0};
};

} // namespace mdc::core
