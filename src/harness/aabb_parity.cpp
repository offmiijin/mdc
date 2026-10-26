// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de paridade de AABB.
//
// Lê casos (texto, um por linha) e escreve uma transcrição JSONL com os
// resultados **em bits**: cada double vira 16 dígitos hex de seu padrão IEEE-754.
// Comparação de bits elimina qualquer ambiguidade de formatação entre Java e
// C++ (ver docs/HARNESS.md §3).
//
// A entrada são casos NOSSOS (podem ser versionados). A saída é derivada do
// jogo quando gerada pelo probe Java — por isso nunca é versionada.
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mdc/core/aabb.hpp"

namespace {

void print_bits(double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    std::printf("\"%016" PRIx64 "\"", bits);
}

void print_vec(const mdc::core::Vec3& v) {
    std::printf("[");
    print_bits(v.x);
    std::printf(",");
    print_bits(v.y);
    std::printf(",");
    print_bits(v.z);
    std::printf("]");
}

void print_box(const mdc::core::Aabb& b) {
    std::printf("[");
    print_bits(b.min_x());
    std::printf(",");
    print_bits(b.min_y());
    std::printf(",");
    print_bits(b.min_z());
    std::printf(",");
    print_bits(b.max_x());
    std::printf(",");
    print_bits(b.max_y());
    std::printf(",");
    print_bits(b.max_z());
    std::printf("]");
}

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    // Cabeçalho de proveniência: idêntico nos dois lados, para o diff valer na
    // transcrição inteira (a versão do jogo é registrada pelo runner).
    std::printf("{\"schema\":1,\"harness\":\"aabb\"}\n");

    char line[1024];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        double v[13];
        const int count = std::sscanf(line, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                                      &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8],
                                      &v[9], &v[10], &v[11], &v[12]);
        if (count != 13) {
            std::fprintf(stderr, "linha %ld inválida (%d campos)\n", index, count);
            if (input != stdin) {
                std::fclose(input);
            }
            return 2;
        }

        const mdc::core::Aabb a{v[0], v[1], v[2], v[3], v[4], v[5]};
        const mdc::core::Aabb b{v[6], v[7], v[8], v[9], v[10], v[11]};
        const mdc::core::Vec3 delta{v[12], v[12] * 0.5, -v[12]};
        const mdc::core::Vec3 inflate{v[12], v[12] * 0.25, v[12] * 2.0};

        std::printf("{\"case\":%ld,\"intersects\":%s,\"center_a\":", index,
                    a.intersects(b) ? "true" : "false");
        print_vec(a.center());
        std::printf(",\"move_a\":");
        print_box(a.move(delta));
        std::printf(",\"inflate_a\":");
        print_box(a.inflate(inflate.x, inflate.y, inflate.z));
        std::printf("}\n");
        ++index;
    }

    if (input != stdin) {
        std::fclose(input);
    }
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
