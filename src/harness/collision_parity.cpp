// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de paridade da resolução de colisão.
//
// Caso por linha: `n  minX..maxZ (6)  moveX moveY moveZ (3)  n × 6 doubles`.
// Saída: movimento aplicado (bits).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "jsonl.hpp"
#include "mdc/world/phys/collision.hpp"

namespace {

constexpr int kMaxSolids = 64;

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    mdc::harness::print_header("collision");

    char line[8192];
    long index = 0;
    std::vector<mdc::core::Aabb> solids;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        double v[1 + 6 + 3 + 6 * kMaxSolids];
        int count = 0;
        char* cursor = line;
        while (count < static_cast<int>(sizeof v / sizeof v[0])) {
            char* end = nullptr;
            const double value = std::strtod(cursor, &end);
            if (end == cursor) {
                break;
            }
            v[count++] = value;
            cursor = end;
        }

        const int solid_count = static_cast<int>(v[0]);
        if (solid_count < 0 || solid_count > kMaxSolids ||
            count != 1 + 6 + 3 + 6 * solid_count) {
            std::fprintf(stderr, "linha %ld inválida (%d sólidos, %d campos)\n", index, solid_count,
                         count);
            if (input != stdin) {
                std::fclose(input);
            }
            return 2;
        }

        const mdc::core::Aabb box{v[1], v[2], v[3], v[4], v[5], v[6]};
        const mdc::core::Vec3 movement{v[7], v[8], v[9]};

        solids.clear();
        for (int i = 0; i < solid_count; ++i) {
            const int at = 10 + 6 * i;
            solids.push_back(mdc::core::Aabb{v[at], v[at + 1], v[at + 2], v[at + 3], v[at + 4],
                                             v[at + 5]});
        }

        const mdc::world::phys::MoveResult moved =
            mdc::world::phys::collide_move(box, movement, solids);
        std::printf("{\"case\":%ld,\"applied\":", index);
        mdc::harness::print_vec(moved.applied);
        std::printf("}\n");
        ++index;
    }

    if (input != stdin) {
        std::fclose(input);
    }
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
