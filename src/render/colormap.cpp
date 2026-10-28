// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/render/colormap.hpp"

#include <cstdio>

#include "mdc/res/png.hpp"

namespace mdc::render {

std::optional<Colormap> Colormap::load(const res::ResourcePack& pack, std::string_view kind,
                                       std::string* error) {
    std::string path = "textures/colormap/";
    path += kind;
    path += ".png";

    const std::optional<std::string> bytes = pack.read_text("minecraft", path, error);
    if (!bytes.has_value()) {
        return std::nullopt;
    }
    const std::optional<res::Image> image = res::decode_png(*bytes, error);
    if (!image.has_value()) {
        return std::nullopt;
    }
    if (image->width != kSize || image->height != kSize) {
        if (error != nullptr) {
            *error = "colormap fora do tamanho esperado (256x256): " + path;
        }
        return std::nullopt;
    }

    Colormap colormap;
    colormap.pixels_.resize(static_cast<std::size_t>(kSize) * kSize);
    for (std::size_t i = 0; i < colormap.pixels_.size(); ++i) {
        const std::uint8_t r = image->rgba[i * 4];
        const std::uint8_t g = image->rgba[i * 4 + 1];
        const std::uint8_t b = image->rgba[i * 4 + 2];
        colormap.pixels_[i] = 0xFF000000U | (static_cast<std::uint32_t>(r) << 16U) |
                             (static_cast<std::uint32_t>(g) << 8U) | static_cast<std::uint32_t>(b);
    }
    return colormap;
}

std::uint32_t Colormap::get(double temperature, double downfall) const {
    // `(int)` trunca em direção a zero nas duas linguagens; a ordem das
    // operações é a do original e importa para o bit do resultado.
    const int x = static_cast<int>((1.0 - temperature) * 255.0);
    const int y = static_cast<int>((1.0 - temperature * downfall) * 255.0);
    // Fora de [0, 255] o original estoura exceção (não existe cor): nenhum caso
    // do harness cai aqui. A guarda existe porque esta API é pública.
    if (x < 0 || x > kSize - 1 || y < 0 || y > kSize - 1) {
        return kOutOfRange;
    }
    return pixels_[static_cast<std::size_t>(y) * kSize + static_cast<std::size_t>(x)];
}

} // namespace mdc::render
