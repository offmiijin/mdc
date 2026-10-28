// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de estado de bloco (degrau M2).
//
// Entrada: `<id> [prop=valor ...]`, uma por linha. Saída: o contrato de estado
// (propriedades na ordem de declaração, domínio de cada uma, índices do default
// e do estado pedido) e as consultas por estado que o mesher e a luz usam.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <vector>

#include "jsonl.hpp"
#include "mdc/world/level/block.hpp"

namespace {

using mdc::world::level::BlockRegistry;
using mdc::world::level::RenderShape;

const char* render_shape_name(RenderShape shape) {
    return shape == RenderShape::kInvisible ? "INVISIBLE" : "MODEL";
}

void print_bool(bool value) { std::printf("%s", value ? "true" : "false"); }

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    const BlockRegistry& registry = BlockRegistry::get();
    mdc::harness::print_header("blockstate");

    char line[4096];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // Quebra em campos.
        std::vector<char*> fields;
        char* cursor = line;
        while (true) {
            cursor += std::strspn(cursor, " \t\r\n");
            if (*cursor == '\0') {
                break;
            }
            const std::size_t length = std::strcspn(cursor, " \t\r\n");
            if (cursor[length] == '\0') {
                fields.push_back(cursor);
                break;
            }
            cursor[length] = '\0';
            fields.push_back(cursor);
            cursor += length + 1;
        }
        if (fields.empty()) {
            continue;
        }

        const char* id = fields[0];
        const std::optional<std::uint32_t> block = registry.find_block(id);
        if (!block.has_value()) {
            std::fprintf(stderr, "bloco fora da tabela: %s\n", id);
            if (input != stdin) {
                std::fclose(input);
            }
            return 2;
        }
        const auto& def = registry.block_def(*block);

        std::vector<std::uint8_t> values(def.properties.size(), 0);
        for (std::size_t p = 0; p < def.properties.size(); ++p) {
            values[p] = def.defaults[p];
        }
        for (int i = 1; i < static_cast<int>(fields.size()); ++i) {
            char* eq = std::strchr(fields[i], '=');
            if (eq == nullptr) {
                std::fprintf(stderr, "campo inválido: %s\n", fields[i]);
                if (input != stdin) {
                    std::fclose(input);
                }
                return 2;
            }
            *eq = '\0';
            const char* name = fields[i];
            const char* value = eq + 1;
            std::size_t slot = 0;
            while (slot < def.properties.size() && def.properties[slot].name != name) {
                ++slot;
            }
            if (slot == def.properties.size()) {
                std::fprintf(stderr, "propriedade fora da tabela: %s.%s\n", id, name);
                if (input != stdin) {
                    std::fclose(input);
                }
                return 2;
            }
            const std::optional<std::uint8_t> value_index =
                registry.property_value(*block, name, value);
            if (!value_index.has_value()) {
                std::fprintf(stderr, "valor fora do domínio: %s.%s=%s\n", id, name, value);
                if (input != stdin) {
                    std::fclose(input);
                }
                return 2;
            }
            values[slot] = *value_index;
        }

        const std::uint32_t state = registry.state_of(*block, values);
        const mdc::world::level::StateFlags flags = registry.flags(state);

        std::printf("{\"case\":%ld,\"id\":\"%s\",\"props\":[", index, id);
        for (std::size_t p = 0; p < def.properties.size(); ++p) {
            if (p > 0) {
                std::printf(",");
            }
            std::printf("[\"%.*s\",[", static_cast<int>(def.properties[p].name.size()),
                        def.properties[p].name.data());
            for (std::size_t v = 0; v < def.properties[p].values.size(); ++v) {
                if (v > 0) {
                    std::printf(",");
                }
                std::printf("\"%.*s\"", static_cast<int>(def.properties[p].values[v].size()),
                            def.properties[p].values[v].data());
            }
            std::printf("]]");
        }

        std::printf("],\"default\":[");
        for (std::size_t p = 0; p < def.properties.size(); ++p) {
            if (p > 0) {
                std::printf(",");
            }
            std::printf("%u", static_cast<unsigned>(def.defaults[p]));
        }
        std::printf("],\"state\":[");
        for (std::size_t p = 0; p < values.size(); ++p) {
            if (p > 0) {
                std::printf(",");
            }
            std::printf("%u", static_cast<unsigned>(values[p]));
        }

        std::printf("],\"render_shape\":\"%s\",\"solid_render\":", render_shape_name(flags.render_shape));
        print_bool(flags.solid_render);
        std::printf(",\"light_emission\":%u,\"light_dampening\":%u,\"skylight_down\":",
                    static_cast<unsigned>(flags.light_emission),
                    static_cast<unsigned>(flags.light_dampening));
        print_bool(flags.skylight_down);
        std::printf(",\"shape_for_light\":");
        print_bool(flags.shape_for_light);
        std::printf("}\n");

        ++index;
    }

    if (input != stdin) {
        std::fclose(input);
    }
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
