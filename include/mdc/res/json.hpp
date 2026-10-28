// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mdc::res {

/// Valor JSON mínimo: o suficiente para blockstate, modelo, atlas e
/// `pack.mcmeta`.
///
/// **Ordem dos membros é preservada** (vetor, não mapa): o pack é lido em ordem
/// de arquivo e há regra que depende disso (herança de `textures`, por exemplo).
/// Exceções ficam fora: a carga devolve `std::nullopt` e uma mensagem.
class JsonValue {
  public:
    enum class Type : std::uint8_t { kNull, kBool, kNumber, kString, kArray, kObject };

    JsonValue() = default;

    [[nodiscard]] Type type() const noexcept { return type_; }
    [[nodiscard]] bool is_null() const noexcept { return type_ == Type::kNull; }
    [[nodiscard]] bool is_object() const noexcept { return type_ == Type::kObject; }
    [[nodiscard]] bool is_array() const noexcept { return type_ == Type::kArray; }
    [[nodiscard]] bool is_string() const noexcept { return type_ == Type::kString; }
    [[nodiscard]] bool is_number() const noexcept { return type_ == Type::kNumber; }
    [[nodiscard]] bool is_bool() const noexcept { return type_ == Type::kBool; }

    [[nodiscard]] bool as_bool() const noexcept { return bool_; }
    [[nodiscard]] double as_number() const noexcept { return number_; }
    [[nodiscard]] int as_int() const noexcept { return static_cast<int>(number_); }
    [[nodiscard]] const std::string& as_string() const noexcept { return string_; }

    /// Membro de objeto; `nullptr` quando ausente (ou quando não é objeto).
    [[nodiscard]] const JsonValue* find(std::string_view key) const noexcept;
    [[nodiscard]] bool contains(std::string_view key) const noexcept { return find(key) != nullptr; }

    /// Itens de array, ou membros de objeto em ordem de arquivo.
    [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }
    [[nodiscard]] const JsonValue& at(std::size_t index) const noexcept { return items_[index].second; }
    [[nodiscard]] const std::string& key_at(std::size_t index) const noexcept {
        return items_[index].first;
    }

    /// Atalhos com default, para o código de carga ficar legível.
    [[nodiscard]] std::string_view string_or(std::string_view key, std::string_view fallback) const;
    [[nodiscard]] double number_or(std::string_view key, double fallback) const;
    [[nodiscard]] bool bool_or(std::string_view key, bool fallback) const;

    /// Analisa `text`. Em erro devolve `std::nullopt` e preenche `error`.
    [[nodiscard]] static std::optional<JsonValue> parse(std::string_view text,
                                                        std::string* error = nullptr);

  private:
    friend class JsonParser;

    Type type_{Type::kNull};
    bool bool_{false};
    double number_{0.0};
    std::string string_{};
    std::vector<std::pair<std::string, JsonValue>> items_{};
};

} // namespace mdc::res
