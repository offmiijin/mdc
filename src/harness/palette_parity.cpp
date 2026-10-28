// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de paleta (degrau M2): empacotamento de bits, paleta e
// container paletizado.
//
// Entrada: casos nossos (versionados). Saída: transcrição JSONL em bits — os
// longs vão em 16 dígitos hex, os valores de célula e os resultados de `get`
// como inteiros decimais (são índices, não floats).
//
// Ver docs/HARNESS.md §3 para o formato da transcrição.
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "jsonl.hpp"
#include "mdc/world/level/bit_storage.hpp"
#include "mdc/world/level/paletted_container.hpp"

namespace {

using mdc::harness::print_hex_u64s;
using mdc::world::level::BitStorage;
using mdc::world::level::PalettedContainer;

constexpr int kSampleCount = 8;

/// Mesma fórmula do probe: espalha as leituras sem aleatoriedade.
[[nodiscard]] int sample_index(int i, int size) {
    return static_cast<int>((static_cast<long long>(i) * 977LL + 13LL) % size);
}

/// Campos hex separados por espaço: `<índice>=<valor>`.
void storage_case(int index, char** fields, int count) {
    const int bits = std::atoi(fields[0]);
    const int size = std::atoi(fields[1]);
    BitStorage storage(bits, size);

    for (int i = 2; i < count; ++i) {
        const char* eq = std::strchr(fields[i], '=');
        if (eq == nullptr) {
            std::fprintf(stderr, "campo inválido: %s\n", fields[i]);
            std::exit(2);
        }
        const int at = std::atoi(fields[i]);
        const int value = std::atoi(eq + 1);
        storage.set(at, value);
    }

    std::printf("{\"case\":%d,\"kind\":\"storage\",\"bits\":%d,\"size\":%d,\"raw\":",
                index, storage.bits(), storage.size());
    print_hex_u64s(storage.raw());
    std::printf(",\"gets\":[");
    for (int i = 0; i < kSampleCount; ++i) {
        if (i > 0) {
            std::printf(",");
        }
        std::printf("%d", storage.get(sample_index(i, size)));
    }
    std::printf("]}\n");
}

/// Células no formato `x,y,z=valor`.
void container_case(int index, char** fields, int count) {
    PalettedContainer container(0U, mdc::world::level::Strategy{});

    for (int i = 0; i < count; ++i) {
        const char* eq = std::strchr(fields[i], '=');
        if (eq == nullptr) {
            std::fprintf(stderr, "campo inválido: %s\n", fields[i]);
            std::exit(2);
        }
        int x = 0;
        int y = 0;
        int z = 0;
        if (std::sscanf(fields[i], "%d,%d,%d=", &x, &y, &z) != 3) {
            std::fprintf(stderr, "coordenada inválida: %s\n", fields[i]);
            std::exit(2);
        }
        container.set(x, y, z, static_cast<std::uint32_t>(std::atoi(eq + 1)));
    }

    const PalettedContainer::Packed packed = container.pack();
    std::printf("{\"case\":%d,\"kind\":\"container\",\"bits\":%d,\"packed_bits\":%d,\"palette\":[",
                index, container.bits_per_entry(), packed.bits);
    for (std::size_t i = 0; i < packed.palette.size(); ++i) {
        if (i > 0) {
            std::printf(",");
        }
        std::printf("%" PRIu32, packed.palette[i]);
    }
    std::printf("],\"storage\":");
    print_hex_u64s(packed.storage);
    std::printf(",\"gets\":[");
    for (int i = 0; i < kSampleCount; ++i) {
        const int at = sample_index(i, PalettedContainer::kSize);
        if (i > 0) {
            std::printf(",");
        }
        std::printf("%" PRIu32, container.get(at & 15, (at >> 8) & 15, (at >> 4) & 15));
    }
    std::printf("]}\n");
}

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    mdc::harness::print_header("palette");

    char line[1 << 20];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // Quebra por espaços sem `strtok`: o número de campos varia muito
        // (uma seção pode trazer milhares de escritas) e o tokenizador do C
        // esconde estado global.
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
            // Termina o campo e salta o separador: sem o `+ 1` o laço pararia
            // no primeiro token (o terminador que acabamos de escrever).
            cursor[length] = '\0';
            fields.push_back(cursor);
            cursor += length + 1;
        }
        if (fields.empty()) {
            continue;
        }

        const char* kind = fields[0];
        const int count = static_cast<int>(fields.size()) - 1;
        if (std::strcmp(kind, "storage") == 0) {
            storage_case(static_cast<int>(index), fields.data() + 1, count);
        } else if (std::strcmp(kind, "container") == 0) {
            container_case(static_cast<int>(index), fields.data() + 1, count);
        } else {
            std::fprintf(stderr, "caso desconhecido: %s\n", kind);
            if (input != stdin) {
                std::fclose(input);
            }
            return 2;
        }
        ++index;
    }

    if (input != stdin) {
        std::fclose(input);
    }
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
