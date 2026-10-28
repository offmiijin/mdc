// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de atlas (degrau M2): inventário de sprites.
//
// Entrada: `minecraft:blocks` (uma linha por atlas). O pack vem do jar do
// usuário por `MDC_MC_JAR`; nada é versionado nem baixado.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "jsonl.hpp"
#include "mdc/render/sprite_inventory.hpp"

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    std::string error;
    const std::optional<mdc::res::ResourcePack> pack = mdc::res::ResourcePack::from_environment(&error);
    if (!pack.has_value()) {
        std::fprintf(stderr, "erro: %s\n", error.c_str());
        std::fclose(input);
        return 2;
    }

    mdc::harness::print_header("atlas");

    char line[512];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        char* newline = std::strchr(line, '\n');
        if (newline != nullptr) {
            *newline = '\0';
        }
        char* cursor = line;
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        if (*cursor == '\0') {
            continue;
        }

        error.clear();
        const std::vector<mdc::render::SpriteEntry> sprites =
            mdc::render::sprite_inventory(*pack, cursor, &error);
        std::printf("{\"case\":%ld,\"atlas\":\"%s\",\"count\":%zu,\"sprites\":[", index, cursor,
                    sprites.size());
        for (std::size_t i = 0; i < sprites.size(); ++i) {
            if (i > 0) {
                std::printf(",");
            }
            std::printf("[\"%s\",%d,%d]", sprites[i].id.c_str(), sprites[i].width, sprites[i].height);
        }
        std::printf("]}\n");
        ++index;
    }

    std::fclose(input);
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
