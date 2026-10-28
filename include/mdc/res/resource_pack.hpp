// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "mdc/res/json.hpp"
#include "mdc/res/zip.hpp"

namespace mdc::res {

/// Pack de recursos lido do jar do **usuário** (nunca versionado).
///
/// O caminho vem do ambiente (`MDC_MC_JAR`), como o harness faz com o jar do
/// oracle: o programa não empacota nem baixa asset do jogo.
class ResourcePack {
  public:
    [[nodiscard]] static std::optional<ResourcePack> open_jar(const std::string& jar_path,
                                                              std::string* error = nullptr);

    /// Jar indicado por `MDC_MC_JAR`; `std::nullopt` se a variável não existir.
    [[nodiscard]] static std::optional<ResourcePack> from_environment(std::string* error = nullptr);

    [[nodiscard]] std::optional<std::string> read_text(std::string_view namespace_id,
                                                       std::string_view path,
                                                       std::string* error = nullptr) const;

    [[nodiscard]] std::optional<JsonValue> read_json(std::string_view namespace_id,
                                                     std::string_view path,
                                                     std::string* error = nullptr) const;

    /// `assets/<namespace>/blockstates/<name>.json`, com `<name>` aceitando o
    /// prefixo `minecraft:` (ou namespace explícito).
    [[nodiscard]] std::optional<JsonValue> blockstate(std::string_view id,
                                                      std::string* error = nullptr) const;

    /// `assets/<namespace>/models/<path>.json` para um id de modelo
    /// (`minecraft:block/stone` ou `block/stone`).
    [[nodiscard]] std::optional<JsonValue> model(std::string_view id,
                                                 std::string* error = nullptr) const;

    [[nodiscard]] const ZipArchive& archive() const noexcept { return archive_; }

  private:
    ZipArchive archive_{};
};

/// Divide `minecraft:block/stone` em namespace e caminho, com default
/// `minecraft` quando não há `:`.
struct ResourceId {
    std::string namespace_id{"minecraft"};
    std::string path;
};

[[nodiscard]] ResourceId split_id(std::string_view id);

} // namespace mdc::res
