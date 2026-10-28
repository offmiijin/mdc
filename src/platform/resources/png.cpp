// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/res/png.hpp"

#include <array>
#include <cstring>

#include <zlib.h>

namespace mdc::res {

namespace {

constexpr std::uint8_t kBitDepth8 = 8;
constexpr std::uint8_t kColorTypeRgb = 2;
constexpr std::uint8_t kColorTypeRgba = 6;

[[nodiscard]] std::uint32_t read_be32(std::string_view bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset])) << 24U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 16U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 8U) |
           static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3]));
}

[[nodiscard]] std::uint8_t paeth(int a, int b, int c) {
    const int p = a + b - c;
    const int pa = p > a ? p - a : a - p;
    const int pb = p > b ? p - b : b - p;
    const int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) {
        return static_cast<std::uint8_t>(a);
    }
    return static_cast<std::uint8_t>(pb <= pc ? b : c);
}

} // namespace

std::optional<Image> decode_png(std::string_view bytes, std::string* error) {
    constexpr std::array<std::uint8_t, 8> kSignature = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (bytes.size() < 8 || std::memcmp(bytes.data(), kSignature.data(), kSignature.size()) != 0) {
        if (error != nullptr) {
            *error = "assinatura de PNG inválida";
        }
        return std::nullopt;
    }

    Image image;
    std::uint8_t color_type = 0;
    std::string idat;
    std::size_t pos = 8;
    while (pos + 8 <= bytes.size()) {
        const std::uint32_t length = read_be32(bytes, pos);
        const std::string_view type = bytes.substr(pos + 4, 4);
        const std::size_t data = pos + 8;
        if (data + length > bytes.size()) {
            if (error != nullptr) {
                *error = "chunk truncado";
            }
            return std::nullopt;
        }
        if (type == "IHDR") {
            image.width = static_cast<int>(read_be32(bytes, data));
            image.height = static_cast<int>(read_be32(bytes, data + 4));
            const auto depth = static_cast<std::uint8_t>(bytes[data + 8]);
            color_type = static_cast<std::uint8_t>(bytes[data + 9]);
            const auto interlace = static_cast<std::uint8_t>(bytes[data + 12]);
            if (depth != kBitDepth8 || interlace != 0 ||
                (color_type != kColorTypeRgb && color_type != kColorTypeRgba)) {
                if (error != nullptr) {
                    *error = "PNG fora do subconjunto suportado (8 bits, sem interlace, RGB/RGBA)";
                }
                return std::nullopt;
            }
        } else if (type == "IDAT") {
            idat.append(bytes.substr(data, length));
        } else if (type == "IEND") {
            break;
        }
        pos = data + length + 4; // + CRC
    }

    const int channels = color_type == kColorTypeRgba ? 4 : 3;
    const std::size_t stride = static_cast<std::size_t>(image.width) * channels + 1;
    const std::size_t raw_size = stride * static_cast<std::size_t>(image.height);
    std::string raw(raw_size, '\0');

    z_stream stream{};
    if (inflateInit(&stream) != Z_OK) {
        if (error != nullptr) {
            *error = "inflateInit falhou";
        }
        return std::nullopt;
    }
    stream.next_in = reinterpret_cast<Bytef*>(idat.data()); // NOLINT: zlib não escreve na entrada
    stream.avail_in = static_cast<uInt>(idat.size());
    stream.next_out = reinterpret_cast<Bytef*>(raw.data());
    stream.avail_out = static_cast<uInt>(raw.size());
    const int status = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    if (status != Z_STREAM_END || stream.total_out != raw.size()) {
        if (error != nullptr) {
            *error = "inflate dos dados de imagem falhou";
        }
        return std::nullopt;
    }

    image.rgba.assign(static_cast<std::size_t>(image.width) * image.height * 4, 0xFF);
    std::vector<std::uint8_t> previous(static_cast<std::size_t>(image.width) * channels, 0);
    std::vector<std::uint8_t> current(previous.size(), 0);
    for (int y = 0; y < image.height; ++y) {
        const auto* row = reinterpret_cast<const std::uint8_t*>(
            raw.data() + static_cast<std::size_t>(y) * stride);
        const std::uint8_t filter = row[0];
        for (std::size_t i = 0; i < current.size(); ++i) {
            const int left = i >= static_cast<std::size_t>(channels) ? current[i - channels] : 0;
            const int up = previous[i];
            const int up_left = i >= static_cast<std::size_t>(channels) ? previous[i - channels] : 0;
            const int value = row[i + 1];
            int restored = value;
            switch (filter) {
                case 0:
                    break;
                case 1:
                    restored = value + left;
                    break;
                case 2:
                    restored = value + up;
                    break;
                case 3:
                    restored = value + (left + up) / 2;
                    break;
                case 4:
                    restored = value + paeth(left, up, up_left);
                    break;
                default:
                    if (error != nullptr) {
                        *error = "filtro de PNG desconhecido";
                    }
                    return std::nullopt;
            }
            current[i] = static_cast<std::uint8_t>(restored & 0xFF);
        }
        for (int x = 0; x < image.width; ++x) {
            const std::size_t src = static_cast<std::size_t>(x) * channels;
            const std::size_t dst = (static_cast<std::size_t>(y) * image.width + x) * 4;
            image.rgba[dst] = current[src];
            image.rgba[dst + 1] = current[src + 1];
            image.rgba[dst + 2] = current[src + 2];
            if (channels == 4) {
                image.rgba[dst + 3] = current[src + 3];
            }
        }
        previous = current;
    }
    return image;
}

} // namespace mdc::res
