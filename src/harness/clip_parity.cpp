// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de paridade de `Aabb::clip` (raycast caixa-alinhada).
//
// Casos: `minX minY minZ maxX maxY maxZ fromX fromY fromZ toX toY toZ`.
// Saída: `hit` e o ponto de entrada, ou `null`.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "jsonl.hpp"
#include "mdc/core/aabb.hpp"

namespace {

constexpr int kFields = 12;

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    mdc::harness::print_header("clip");

    char line[1024];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        double v[kFields];
        const int count = std::sscanf(line,
                                      "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", &v[0], &v[1],
                                      &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10],
                                      &v[11]);
        if (count != kFields) {
            std::fprintf(stderr, "linha %ld inválida (%d campos)\n", index, count);
            if (input != stdin) {
                std::fclose(input);
            }
            return 2;
        }

        const mdc::core::Aabb box{v[0], v[1], v[2], v[3], v[4], v[5]};
        const mdc::core::Vec3 from{v[6], v[7], v[8]};
        const mdc::core::Vec3 to{v[9], v[10], v[11]};

        const std::optional<mdc::core::Vec3> hit = box.clip(from, to);

        std::printf("{\"case\":%ld,\"hit\":%s,\"point\":", index, hit.has_value() ? "true" : "false");
        if (hit.has_value()) {
            mdc::harness::print_vec(*hit);
        } else {
            std::printf("null");
        }
        std::printf("}\n");
        ++index;
    }

    if (input != stdin) {
        std::fclose(input);
    }
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
