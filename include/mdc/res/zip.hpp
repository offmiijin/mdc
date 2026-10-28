// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mdc::res {

/// Leitor de ZIP por diretório central — o formato do `client.jar`, de onde
/// saem blockstates, modelos, texturas e o `pack.mcmeta`.
///
/// Só leitura, sem escrever nada: o pack do usuário é aberto no lugar e nunca
/// copiado para o repositório (docs/LEGAL.md).
class ZipArchive {
  public:
    ZipArchive() = default;

    [[nodiscard]] static std::optional<ZipArchive> open(const std::string& path,
                                                        std::string* error = nullptr);

    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> names() const;

    /// Conteúdo descomprimido da entrada; `std::nullopt` se ausente ou corrompida.
    [[nodiscard]] std::optional<std::string> read(std::string_view name,
                                                  std::string* error = nullptr) const;

    [[nodiscard]] std::size_t entry_count() const noexcept { return entries_.size(); }

  private:
    struct Entry {
        std::string name;
        std::uint16_t method{0};
        std::uint32_t compressed_size{0};
        std::uint32_t uncompressed_size{0};
        std::uint32_t local_header_offset{0};
    };

    std::string path_{};
    std::vector<Entry> entries_{};
};

} // namespace mdc::res
