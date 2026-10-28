// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de BlockPos: ponto (3 doubles) -> bloco, vizinhos e long.
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "jsonl.hpp"
#include "mdc/core/block_pos.hpp"

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    mdc::harness::print_header("blockpos");

    char line[1024];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        double v[3];
        if (std::sscanf(line, "%lf %lf %lf", &v[0], &v[1], &v[2]) != 3) {
            std::fprintf(stderr, "linha %ld inválida\n", index);
            if (input != stdin) {
                std::fclose(input);
            }
            return 2;
        }

        const mdc::core::BlockPos pos = mdc::core::BlockPos::containing({v[0], v[1], v[2]});

        std::printf("{\"case\":%ld,\"pos\":[%d,%d,%d],\"above\":%" PRId64 ",\"below\":%" PRId64
                    ",\"as_long\":%" PRId64 "}\n",
                    index, pos.x, pos.y, pos.z, pos.above().as_long(), pos.below().as_long(),
                    pos.as_long());
        ++index;
    }

    if (input != stdin) {
        std::fclose(input);
    }
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
