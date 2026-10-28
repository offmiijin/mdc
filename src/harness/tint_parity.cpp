// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Lado C++ do harness de tint (degrau M2): cor de bioma a partir de
// (temperatura, precipitação). A tabela vem do pack do usuário (`MDC_MC_JAR`).
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>

#include "jsonl.hpp"
#include "mdc/render/colormap.hpp"

namespace {

using mdc::render::Colormap;

[[nodiscard]] std::optional<Colormap> load(const char* kind, std::string& error) {
    const std::optional<mdc::res::ResourcePack> pack = mdc::res::ResourcePack::from_environment(&error);
    if (!pack.has_value()) {
        return std::nullopt;
    }
    return Colormap::load(*pack, kind, &error);
}

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "-";
    std::FILE* input = std::strcmp(path, "-") == 0 ? stdin : std::fopen(path, "r");
    if (input == nullptr) {
        std::fprintf(stderr, "não abriu %s\n", path);
        return 2;
    }

    std::string error;
    std::optional<Colormap> grass = load("grass", error);
    if (!grass.has_value()) {
        std::fprintf(stderr, "erro: %s\n", error.c_str());
        std::fclose(input);
        return 2;
    }
    std::optional<Colormap> foliage = load("foliage", error);
    std::optional<Colormap> dry_foliage = load("dry_foliage", error);
    if (!foliage.has_value() || !dry_foliage.has_value()) {
        std::fprintf(stderr, "erro: %s\n", error.c_str());
        std::fclose(input);
        return 2;
    }

    mdc::harness::print_header("tint");

    char line[512];
    long index = 0;
    while (std::fgets(line, sizeof line, input) != nullptr) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        char kind[64];
        double temperature = 0.0;
        double downfall = 0.0;
        if (std::sscanf(line, "%63s %lf %lf", kind, &temperature, &downfall) != 3) {
            std::fprintf(stderr, "linha inválida: %s", line);
            std::fclose(input);
            return 2;
        }
        const Colormap* colormap = &*grass;
        if (std::strcmp(kind, "foliage") == 0) {
            colormap = &*foliage;
        } else if (std::strcmp(kind, "dry_foliage") == 0) {
            colormap = &*dry_foliage;
        }

        std::printf("{\"case\":%ld,\"kind\":\"%s\",\"temperature\":", index, kind);
        mdc::harness::print_bits(temperature);
        std::printf(",\"downfall\":");
        mdc::harness::print_bits(downfall);
        // O probe imprime o `int` do jogo: mesmo sinal, mesma base decimal.
        const auto color = static_cast<std::int32_t>(colormap->get(temperature, downfall));
        std::printf(",\"color\":%" PRId32 "}\n", color);
        ++index;
    }

    std::fclose(input);
    std::printf("{\"cases\":%ld}\n", index);
    return 0;
}
