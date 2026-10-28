// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de paridade de `Vec3`.
//
// Casos de entrada: `ax ay az bx by bz alpha` por linha. Saída: uma transcrição
// JSONL em bits com cada operação, na mesma ordem de campos do probe Java.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "jsonl.hpp"
#include "mdc/core/vec3.hpp"

namespace {

constexpr int kFields = 7;

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    mdc::harness::print_header("vec3");

    char line[1024];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        double v[kFields];
        const int count = std::sscanf(line, "%lf %lf %lf %lf %lf %lf %lf", &v[0], &v[1], &v[2], &v[3],
                                      &v[4], &v[5], &v[6]);
        if (count != kFields) {
            std::fprintf(stderr, "linha %ld inválida (%d campos)\n", index, count);
            if (input != stdin) {
                std::fclose(input);
            }
            return 2;
        }

        const mdc::core::Vec3 a{v[0], v[1], v[2]};
        const mdc::core::Vec3 b{v[3], v[4], v[5]};
        const double alpha = v[6];

        std::printf("{\"case\":%ld,\"add\":", index);
        mdc::harness::print_vec(a.add(b));
        std::printf(",\"subtract\":");
        mdc::harness::print_vec(a.subtract(b));
        std::printf(",\"dot\":");
        mdc::harness::print_bits(a.dot(b));
        std::printf(",\"cross\":");
        mdc::harness::print_vec(a.cross(b));
        std::printf(",\"length_a\":");
        mdc::harness::print_bits(a.length());
        std::printf(",\"length_sqr_a\":");
        mdc::harness::print_bits(a.length_sqr());
        std::printf(",\"normalize_a\":");
        mdc::harness::print_vec(a.normalize());
        std::printf(",\"distance_sqr\":");
        mdc::harness::print_bits(a.distance_to_sqr(b));
        std::printf(",\"lerp_ab\":");
        mdc::harness::print_vec(a.lerp_to(b, alpha));
        std::printf(",\"scale_a\":");
        mdc::harness::print_vec(a.scale(alpha));
        std::printf("}\n");
        ++index;
    }

    if (input != stdin) {
        std::fclose(input);
    }
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
