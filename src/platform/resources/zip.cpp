// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/res/zip.hpp"

#include <cstdio>
#include <cstring>
#include <memory>

#include <zlib.h>

namespace mdc::res {

namespace {

constexpr std::uint32_t kEndSignature = 0x06054b50U;
constexpr std::uint32_t kCentralSignature = 0x02014b50U;
constexpr std::uint32_t kLocalSignature = 0x04034b50U;
constexpr std::uint16_t kMethodStored = 0;
constexpr std::uint16_t kMethodDeflate = 8;
/// O EOCD fica no fim do arquivo, depois de um comentário de até 64 KiB.
constexpr std::size_t kEndSearchWindow = 0x10000U + 22U;

[[nodiscard]] std::uint16_t read_u16(const std::string& data, std::size_t offset) {
    return static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset]) |
                                      (static_cast<unsigned char>(data[offset + 1]) << 8U));
}

[[nodiscard]] std::uint32_t read_u32(const std::string& data, std::size_t offset) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset]) |
                                      (static_cast<unsigned char>(data[offset + 1]) << 8U) |
                                      (static_cast<unsigned char>(data[offset + 2]) << 16U) |
                                      (static_cast<unsigned char>(data[offset + 3]) << 24U));
}

/// Inflate cru (sem cabeçalho zlib/gzip): o deflate do ZIP é um stream cru.
[[nodiscard]] bool inflate_raw(const char* input, std::size_t input_size, std::size_t output_size,
                               std::string& output) {
    output.assign(output_size, '\0');
    z_stream stream{};
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
        return false;
    }
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input)); // NOLINT: zlib não escreve
    stream.avail_in = static_cast<uInt>(input_size);
    stream.next_out = reinterpret_cast<Bytef*>(output.data());
    stream.avail_out = static_cast<uInt>(output_size);

    const int status = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    // `Z_STREAM_END` é o caso bom; `Z_BUF_ERROR` aparece em entradas vazias.
    return (status == Z_STREAM_END || (status == Z_BUF_ERROR && output_size == 0)) &&
           stream.total_out == output_size;
}

} // namespace

std::optional<ZipArchive> ZipArchive::open(const std::string& path, std::string* error) {
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        if (error != nullptr) {
            *error = "não abriu " + path;
        }
        return std::nullopt;
    }
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    if (size <= 0) {
        std::fclose(file);
        if (error != nullptr) {
            *error = "arquivo vazio: " + path;
        }
        return std::nullopt;
    }
    std::string data(static_cast<std::size_t>(size), '\0');
    std::fseek(file, 0, SEEK_SET);
    const std::size_t read = std::fread(data.data(), 1, data.size(), file);
    std::fclose(file);
    if (read != data.size()) {
        if (error != nullptr) {
            *error = "leitura incompleta: " + path;
        }
        return std::nullopt;
    }

    // Procura o EOCD de trás para frente: o comentário do ZIP pode existir.
    const std::size_t search_start =
        data.size() > kEndSearchWindow ? data.size() - kEndSearchWindow : 0;
    std::size_t end = std::string::npos;
    for (std::size_t i = data.size() >= 4 ? data.size() - 4 : 0;; --i) {
        if (read_u32(data, i) == kEndSignature) {
            end = i;
            break;
        }
        if (i == search_start) {
            break;
        }
    }
    if (end == std::string::npos) {
        if (error != nullptr) {
            *error = "não é um ZIP (sem EOCD): " + path;
        }
        return std::nullopt;
    }

    const std::uint16_t entry_count = read_u16(data, end + 10);
    const std::uint32_t central_offset = read_u32(data, end + 16);

    ZipArchive archive;
    archive.path_ = path;
    archive.entries_.reserve(entry_count);

    std::size_t cursor = central_offset;
    for (std::uint16_t i = 0; i < entry_count; ++i) {
        if (cursor + 46 > data.size() || read_u32(data, cursor) != kCentralSignature) {
            if (error != nullptr) {
                *error = "diretório central corrompido";
            }
            return std::nullopt;
        }
        const std::uint16_t name_length = read_u16(data, cursor + 28);
        const std::uint16_t extra_length = read_u16(data, cursor + 30);
        const std::uint16_t comment_length = read_u16(data, cursor + 32);

        Entry entry;
        entry.method = read_u16(data, cursor + 10);
        entry.compressed_size = read_u32(data, cursor + 20);
        entry.uncompressed_size = read_u32(data, cursor + 24);
        entry.local_header_offset = read_u32(data, cursor + 42);
        entry.name = data.substr(cursor + 46, name_length);

        archive.entries_.push_back(std::move(entry));
        cursor += 46U + name_length + extra_length + comment_length;
    }
    return archive;
}

bool ZipArchive::contains(std::string_view name) const {
    for (const Entry& entry : entries_) {
        if (entry.name == name) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> ZipArchive::names() const {
    std::vector<std::string> result;
    result.reserve(entries_.size());
    for (const Entry& entry : entries_) {
        result.push_back(entry.name);
    }
    return result;
}

std::optional<std::string> ZipArchive::read(std::string_view name, std::string* error) const {
    const Entry* found = nullptr;
    for (const Entry& entry : entries_) {
        if (entry.name == name) {
            found = &entry;
            break;
        }
    }
    if (found == nullptr) {
        if (error != nullptr) {
            *error = "entrada ausente: " + std::string(name);
        }
        return std::nullopt;
    }

    std::FILE* file = std::fopen(path_.c_str(), "rb");
    if (file == nullptr) {
        if (error != nullptr) {
            *error = "não reabriu " + path_;
        }
        return std::nullopt;
    }
    std::fseek(file, static_cast<long>(found->local_header_offset), SEEK_SET);
    unsigned char header[30];
    if (std::fread(header, 1, sizeof header, file) != sizeof header) {
        std::fclose(file);
        if (error != nullptr) {
            *error = "cabeçalho local truncado";
        }
        return std::nullopt;
    }
    const std::uint32_t signature = static_cast<std::uint32_t>(header[0]) |
                                    (static_cast<std::uint32_t>(header[1]) << 8U) |
                                    (static_cast<std::uint32_t>(header[2]) << 16U) |
                                    (static_cast<std::uint32_t>(header[3]) << 24U);
    if (signature != kLocalSignature) {
        std::fclose(file);
        if (error != nullptr) {
            *error = "cabeçalho local inválido";
        }
        return std::nullopt;
    }
    const std::uint16_t name_length =
        static_cast<std::uint16_t>(header[26] | (header[27] << 8U));
    const std::uint16_t extra_length =
        static_cast<std::uint16_t>(header[28] | (header[29] << 8U));
    std::fseek(file, static_cast<long>(name_length) + extra_length, SEEK_CUR);

    std::string raw(found->compressed_size, '\0');
    const std::size_t read = std::fread(raw.data(), 1, raw.size(), file);
    std::fclose(file);
    if (read != raw.size()) {
        if (error != nullptr) {
            *error = "dados truncados: " + std::string(name);
        }
        return std::nullopt;
    }

    if (found->method == kMethodStored) {
        raw.resize(found->uncompressed_size);
        return raw;
    }
    if (found->method != kMethodDeflate) {
        if (error != nullptr) {
            *error = "método de compressão não suportado: " + std::to_string(found->method);
        }
        return std::nullopt;
    }

    std::string output;
    if (!inflate_raw(raw.data(), raw.size(), found->uncompressed_size, output)) {
        if (error != nullptr) {
            *error = "inflate falhou: " + std::string(name);
        }
        return std::nullopt;
    }
    return output;
}

} // namespace mdc::res
