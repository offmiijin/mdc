// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mdc::res {

/// Imagem RGBA de 8 bits por canal, na ordem de leitura (linha a linha).
struct Image {
    int width{0};
    int height{0};
    std::vector<std::uint8_t> rgba;

    [[nodiscard]] bool empty() const noexcept { return width <= 0 || height <= 0; }
};

/// Decodifica PNG sem interlace: cinza, RGB, **indexado** (1/2/4/8 bits, com
/// PLTE e tRNS), cinza+alfa e RGBA.
///
/// O indexado não é luxo: no pack do 26.3 quase toda textura de bloco é
/// indexada (688 em 8 bits, 349 em 4, 64 em 2) e a de pedra é cinza. Interlace
/// devolve erro em vez de adivinhar: paridade não combina com "quase
/// decodificado".
[[nodiscard]] std::optional<Image> decode_png(std::string_view bytes, std::string* error = nullptr);

} // namespace mdc::res
