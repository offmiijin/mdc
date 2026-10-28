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

/// Decodifica PNG sem interlace, 8 bits por canal, tipos 2 (RGB) e 6 (RGBA).
///
/// É o mínimo que o pack usa para colormap e textura de bloco. Interlace e
/// profundidades menores devolvem erro em vez de adivinhar: paridade não
/// combina com "quase decodificado".
[[nodiscard]] std::optional<Image> decode_png(std::string_view bytes, std::string* error = nullptr);

} // namespace mdc::res
