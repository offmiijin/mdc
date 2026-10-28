// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/res/json.hpp"

#include <cctype>
#include <cstdlib>

namespace mdc::res {

const JsonValue* JsonValue::find(std::string_view key) const noexcept {
    if (type_ != Type::kObject) {
        return nullptr;
    }
    for (const auto& [name, value] : items_) {
        if (name == key) {
            return &value;
        }
    }
    return nullptr;
}

std::string_view JsonValue::string_or(std::string_view key, std::string_view fallback) const {
    const JsonValue* value = find(key);
    return value != nullptr && value->is_string() ? std::string_view(value->string_) : fallback;
}

double JsonValue::number_or(std::string_view key, double fallback) const {
    const JsonValue* value = find(key);
    return value != nullptr && value->is_number() ? value->number_ : fallback;
}

bool JsonValue::bool_or(std::string_view key, bool fallback) const {
    const JsonValue* value = find(key);
    return value != nullptr && value->is_bool() ? value->bool_ : fallback;
}

/// Analisador descendente recursivo. Sem exceção: erro volta por `error`.
class JsonParser {
  public:
    JsonParser(std::string_view text, std::string* error) : text_(text), error_(error) {}

    [[nodiscard]] bool parse_document(JsonValue& out) {
        skip_space();
        if (!parse_value(out, 0)) {
            return false;
        }
        skip_space();
        if (pos_ != text_.size()) {
            return fail("sobrou conteúdo depois do valor");
        }
        return true;
    }

  private:
    static constexpr int kMaxDepth = 64;

    [[nodiscard]] bool fail(const char* message) {
        if (error_ != nullptr) {
            *error_ = std::string(message) + " na posição " + std::to_string(pos_);
        }
        return false;
    }

    void skip_space() {
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                break;
            }
            ++pos_;
        }
    }

    [[nodiscard]] bool literal(std::string_view word) {
        if (text_.compare(pos_, word.size(), word) != 0) {
            return fail("literal inválido");
        }
        pos_ += word.size();
        return true;
    }

    [[nodiscard]] bool parse_value(JsonValue& out, int depth) {
        if (depth > kMaxDepth) {
            return fail("aninhamento demais");
        }
        skip_space();
        if (pos_ >= text_.size()) {
            return fail("fim inesperado");
        }
        switch (text_[pos_]) {
            case '{':
                return parse_object(out, depth);
            case '[':
                return parse_array(out, depth);
            case '"':
                out.type_ = JsonValue::Type::kString;
                return parse_string(out.string_);
            case 't':
                out.type_ = JsonValue::Type::kBool;
                out.bool_ = true;
                return literal("true");
            case 'f':
                out.type_ = JsonValue::Type::kBool;
                out.bool_ = false;
                return literal("false");
            case 'n':
                out.type_ = JsonValue::Type::kNull;
                return literal("null");
            default:
                return parse_number(out);
        }
    }

    [[nodiscard]] bool parse_number(JsonValue& out) {
        const std::size_t start = pos_;
        if (pos_ < text_.size() && (text_[pos_] == '-' || text_[pos_] == '+')) {
            ++pos_;
        }
        bool digits = false;
        while (pos_ < text_.size() &&
               (std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0 || text_[pos_] == '.' ||
                text_[pos_] == 'e' || text_[pos_] == 'E' || text_[pos_] == '-' || text_[pos_] == '+')) {
            digits = digits || std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0;
            ++pos_;
        }
        if (!digits) {
            return fail("número inválido");
        }
        // `strtod` com locale C: o parser não depende do locale do processo.
        out.type_ = JsonValue::Type::kNumber;
        out.number_ = std::strtod(std::string(text_.substr(start, pos_ - start)).c_str(), nullptr);
        return true;
    }

    [[nodiscard]] bool parse_string(std::string& out) {
        if (text_[pos_] != '"') {
            return fail("esperava aspas");
        }
        ++pos_;
        out.clear();
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (pos_ >= text_.size()) {
                return fail("escape truncado");
            }
            const char escape = text_[pos_++];
            switch (escape) {
                case '"':
                    out.push_back('"');
                    break;
                case '\\':
                    out.push_back('\\');
                    break;
                case '/':
                    out.push_back('/');
                    break;
                case 'b':
                    out.push_back('\b');
                    break;
                case 'f':
                    out.push_back('\f');
                    break;
                case 'n':
                    out.push_back('\n');
                    break;
                case 'r':
                    out.push_back('\r');
                    break;
                case 't':
                    out.push_back('\t');
                    break;
                case 'u': {
                    if (pos_ + 4 > text_.size()) {
                        return fail("escape unicode truncado");
                    }
                    unsigned code = 0;
                    for (int i = 0; i < 4; ++i) {
                        const char digit = text_[pos_++];
                        code <<= 4U;
                        if (digit >= '0' && digit <= '9') {
                            code |= static_cast<unsigned>(digit - '0');
                        } else if (digit >= 'a' && digit <= 'f') {
                            code |= static_cast<unsigned>(digit - 'a' + 10);
                        } else if (digit >= 'A' && digit <= 'F') {
                            code |= static_cast<unsigned>(digit - 'A' + 10);
                        } else {
                            return fail("hex inválido");
                        }
                    }
                    append_utf8(out, code);
                    break;
                }
                default:
                    return fail("escape desconhecido");
            }
        }
        return fail("string sem fechamento");
    }

    /// UTF-8 de um code point; fora do BMP (par substituto) vira o próprio
    /// code point, que é o suficiente para nomes do pack.
    static void append_utf8(std::string& out, unsigned code) {
        if (code < 0x80U) {
            out.push_back(static_cast<char>(code));
        } else if (code < 0x800U) {
            out.push_back(static_cast<char>(0xC0U | (code >> 6U)));
            out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        } else {
            out.push_back(static_cast<char>(0xE0U | (code >> 12U)));
            out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
            out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        }
    }

    [[nodiscard]] bool parse_object(JsonValue& out, int depth) {
        out.type_ = JsonValue::Type::kObject;
        ++pos_; // '{'
        skip_space();
        if (pos_ < text_.size() && text_[pos_] == '}') {
            ++pos_;
            return true;
        }
        while (true) {
            skip_space();
            if (pos_ >= text_.size()) {
                return fail("objeto sem fechamento");
            }
            std::string key;
            if (!parse_string(key)) {
                return false;
            }
            skip_space();
            if (pos_ >= text_.size() || text_[pos_] != ':') {
                return fail("esperava ':'");
            }
            ++pos_;
            JsonValue value;
            if (!parse_value(value, depth + 1)) {
                return false;
            }
            out.items_.emplace_back(std::move(key), std::move(value));
            skip_space();
            if (pos_ >= text_.size()) {
                return fail("objeto sem fechamento");
            }
            if (text_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (text_[pos_] == '}') {
                ++pos_;
                return true;
            }
            return fail("esperava ',' ou '}'");
        }
    }

    [[nodiscard]] bool parse_array(JsonValue& out, int depth) {
        out.type_ = JsonValue::Type::kArray;
        ++pos_; // '['
        skip_space();
        if (pos_ < text_.size() && text_[pos_] == ']') {
            ++pos_;
            return true;
        }
        while (true) {
            JsonValue value;
            if (!parse_value(value, depth + 1)) {
                return false;
            }
            out.items_.emplace_back(std::string{}, std::move(value));
            skip_space();
            if (pos_ >= text_.size()) {
                return fail("array sem fechamento");
            }
            if (text_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (text_[pos_] == ']') {
                ++pos_;
                return true;
            }
            return fail("esperava ',' ou ']'");
        }
    }

    std::string_view text_;
    std::string* error_;
    std::size_t pos_{0};
};

std::optional<JsonValue> JsonValue::parse(std::string_view text, std::string* error) {
    JsonValue value;
    JsonParser parser(text, error);
    if (!parser.parse_document(value)) {
        return std::nullopt;
    }
    return value;
}

} // namespace mdc::res
