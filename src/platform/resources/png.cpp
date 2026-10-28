// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/res/png.hpp"

#include <array>
#include <cstring>

#include <zlib.h>

namespace mdc::res {

namespace {

constexpr std::uint8_t kColorTypeGray = 0;
constexpr std::uint8_t kColorTypeRgb = 2;
constexpr std::uint8_t kColorTypeIndexed = 3;
constexpr std::uint8_t kColorTypeGrayAlpha = 4;
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

struct Header {
    int width{0};
    int height{0};
    int depth{8};
    std::uint8_t color_type{kColorTypeRgba};
    int channels{4};
};

[[nodiscard]] int channels_of(std::uint8_t color_type) {
    switch (color_type) {
        case kColorTypeGray:
            return 1;
        case kColorTypeRgb:
            return 3;
        case kColorTypeIndexed:
            return 1;
        case kColorTypeGrayAlpha:
            return 2;
        default:
            return 4;
    }
}

/// Amostra `index` da linha já desfiltrada, para profundidade < 8 bits.
[[nodiscard]] std::uint8_t sample(const std::uint8_t* row, int index, int depth) {
    if (depth == 8) {
        return row[index];
    }
    const int per_byte = 8 / depth;
    const int byte = index / per_byte;
    const int shift = 8 - depth * (index % per_byte + 1);
    const int mask = (1 << depth) - 1;
    return static_cast<std::uint8_t>((row[byte] >> shift) & mask);
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

    Header header;
    std::string idat;
    std::vector<std::uint8_t> palette;  ///< PLTE: 3 bytes por cor
    std::vector<std::uint8_t> palette_alpha; ///< tRNS: 1 byte por cor
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
            header.width = static_cast<int>(read_be32(bytes, data));
            header.height = static_cast<int>(read_be32(bytes, data + 4));
            header.depth = static_cast<std::uint8_t>(bytes[data + 8]);
            header.color_type = static_cast<std::uint8_t>(bytes[data + 9]);
            const auto interlace = static_cast<std::uint8_t>(bytes[data + 12]);
            header.channels = channels_of(header.color_type);
            const bool depth_ok = header.depth == 8 || header.depth == 4 || header.depth == 2 ||
                                  header.depth == 1;
            if (interlace != 0 || !depth_ok) {
                if (error != nullptr) {
                    *error = "PNG com interlace ou profundidade não suportada";
                }
                return std::nullopt;
            }
            // A especificação só permite profundidade < 8 em cinza e indexado.
            if (header.depth != 8 && header.color_type != kColorTypeIndexed &&
                header.color_type != kColorTypeGray) {
                if (error != nullptr) {
                    *error = "profundidade < 8 fora de cinza/indexado";
                }
                return std::nullopt;
            }
            if (header.depth == 16) {
                if (error != nullptr) {
                    *error = "PNG de 16 bits não suportado";
                }
                return std::nullopt;
            }
        } else if (type == "PLTE") {
            palette.assign(bytes.begin() + static_cast<std::ptrdiff_t>(data),
                           bytes.begin() + static_cast<std::ptrdiff_t>(data + length));
        } else if (type == "tRNS") {
            palette_alpha.assign(bytes.begin() + static_cast<std::ptrdiff_t>(data),
                                 bytes.begin() + static_cast<std::ptrdiff_t>(data + length));
        } else if (type == "IDAT") {
            idat.append(bytes.substr(data, length));
        } else if (type == "IEND") {
            break;
        }
        pos = data + length + 4; // + CRC
    }

    if (header.color_type == kColorTypeIndexed && palette.empty()) {
        if (error != nullptr) {
            *error = "PNG indexado sem PLTE";
        }
        return std::nullopt;
    }

    const int bits_per_pixel = header.channels * header.depth;
    const std::size_t row_bytes =
        (static_cast<std::size_t>(header.width) * static_cast<std::size_t>(bits_per_pixel) + 7U) / 8U;
    const std::size_t stride = row_bytes + 1; // + byte de filtro
    const std::size_t raw_size = stride * static_cast<std::size_t>(header.height);
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

    Image image;
    image.width = header.width;
    image.height = header.height;
    image.rgba.assign(static_cast<std::size_t>(header.width) * header.height * 4U, 0xFF);

    // Os filtros operam sobre bytes: `bpp` é no mínimo 1 mesmo em 1/2/4 bits.
    const auto bpp = static_cast<std::size_t>(bits_per_pixel >= 8 ? bits_per_pixel / 8 : 1);
    std::vector<std::uint8_t> previous(row_bytes, 0);
    std::vector<std::uint8_t> current(row_bytes, 0);
    for (int y = 0; y < header.height; ++y) {
        const auto* row = reinterpret_cast<const std::uint8_t*>(
            raw.data() + static_cast<std::size_t>(y) * stride);
        const std::uint8_t filter = row[0];
        for (std::size_t i = 0; i < row_bytes; ++i) {
            const int left = i >= bpp ? current[i - bpp] : 0;
            const int up = previous[i];
            const int up_left = i >= bpp ? previous[i - bpp] : 0;
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

        for (int x = 0; x < header.width; ++x) {
            const std::size_t dst = (static_cast<std::size_t>(y) * header.width + x) * 4;
            switch (header.color_type) {
                case kColorTypeIndexed: {
                    const std::uint8_t index =
                        sample(current.data(), x, header.depth);
                    const std::size_t base = static_cast<std::size_t>(index) * 3U;
                    if (base + 2U >= palette.size()) {
                        if (error != nullptr) {
                            *error = "índice de paleta fora do PLTE";
                        }
                        return std::nullopt;
                    }
                    image.rgba[dst] = palette[base];
                    image.rgba[dst + 1] = palette[base + 1];
                    image.rgba[dst + 2] = palette[base + 2];
                    image.rgba[dst + 3] = index < palette_alpha.size() ? palette_alpha[index] : 0xFF;
                    break;
                }
                case kColorTypeGray: {
                    const std::uint8_t raw_sample = sample(current.data(), x, header.depth);
                    // Escala de 1/2/4 bits para a faixa 0..255.
                    const int maximum = (1 << header.depth) - 1;
                    const auto gray = static_cast<std::uint8_t>(
                        maximum == 255 ? raw_sample : (raw_sample * 255 + maximum / 2) / maximum);
                    image.rgba[dst] = gray;
                    image.rgba[dst + 1] = gray;
                    image.rgba[dst + 2] = gray;
                    break;
                }
                case kColorTypeGrayAlpha: {
                    image.rgba[dst] = current[static_cast<std::size_t>(x) * 2U];
                    image.rgba[dst + 1] = image.rgba[dst];
                    image.rgba[dst + 2] = image.rgba[dst];
                    image.rgba[dst + 3] = current[static_cast<std::size_t>(x) * 2U + 1U];
                    break;
                }
                case kColorTypeRgb: {
                    const std::size_t src = static_cast<std::size_t>(x) * 3U;
                    image.rgba[dst] = current[src];
                    image.rgba[dst + 1] = current[src + 1];
                    image.rgba[dst + 2] = current[src + 2];
                    break;
                }
                default: {
                    const std::size_t src = static_cast<std::size_t>(x) * 4U;
                    image.rgba[dst] = current[src];
                    image.rgba[dst + 1] = current[src + 1];
                    image.rgba[dst + 2] = current[src + 2];
                    image.rgba[dst + 3] = current[src + 3];
                    break;
                }
            }
        }
        previous = current;
    }
    return image;
}

} // namespace mdc::res
