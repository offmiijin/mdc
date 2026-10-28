// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mdc/res/resource_pack.hpp"

namespace mdc::render {

/// Tabela de cor de bioma (`textures/colormap/*.png`) e a consulta do jogo.
///
/// A fórmula foi **medida** no harness `tint`, ponto a ponto contra a tabela:
///
/// ```
/// x = (int)((1 - temperatura) * 255)
/// y = (int)((1 - temperatura * precipitação) * 255)
/// ```
///
/// Repare no `temperatura * precipitação` do eixo y: a precipitação sozinha não
/// explica a linha escolhida (medido — 34 pontos por tipo, 0 divergências). Fora
/// de `[0, 255]` a consulta devolve magenta, como o original.
class Colormap {
  public:
    static constexpr int kSize = 256;
    /// Cor devolvida quando o índice cai fora da tabela.
    static constexpr std::uint32_t kOutOfRange = 0xFFFF00FFU;

    [[nodiscard]] static std::optional<Colormap> load(const res::ResourcePack& pack,
                                                      std::string_view kind,
                                                      std::string* error = nullptr);

    /// ARGB (alfa opaco, como o jogo devolve).
    [[nodiscard]] std::uint32_t get(double temperature, double downfall) const;

    [[nodiscard]] bool empty() const noexcept { return pixels_.empty(); }

  private:
    std::vector<std::uint32_t> pixels_{};
};

} // namespace mdc::render
