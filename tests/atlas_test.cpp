// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Teste do inventário de atlas. Depende do jar do usuário (`MDC_MC_JAR`);
// sem ele, é pulado — o jar não é versionado nem vem no CI.
#include <doctest/doctest.h>

#include <algorithm>
#include <string>

#include "mdc/render/sprite_inventory.hpp"

using mdc::render::SpriteEntry;

namespace {

[[nodiscard]] const SpriteEntry* find(const std::vector<SpriteEntry>& sprites,
                                     const std::string& id) {
    const auto it = std::find_if(sprites.begin(), sprites.end(),
                                 [&id](const SpriteEntry& entry) { return entry.id == id; });
    return it == sprites.end() ? nullptr : &*it;
}

} // namespace

TEST_CASE("inventário do atlas de blocos (condicional ao jar)") {
    std::string error;
    const auto pack = mdc::res::ResourcePack::from_environment(&error);
    if (!pack.has_value()) {
        MESSAGE("MDC_MC_JAR ausente: teste do atlas pulado (" << error << ")");
        return;
    }

    const std::vector<SpriteEntry> sprites = mdc::render::sprite_inventory(*pack, "minecraft:blocks", &error);
    REQUIRE_MESSAGE(!sprites.empty(), error);

    // Bloco comum: frame igual ao PNG.
    const SpriteEntry* stone = find(sprites, "minecraft:block/stone");
    REQUIRE(stone != nullptr);
    CHECK(stone->width == 16);
    CHECK(stone->height == 16);

    // Animado sem `width`/`height` no .mcmeta: frame quadrado do lado da
    // largura, não a altura da tira de quadros.
    const SpriteEntry* furnace = find(sprites, "minecraft:block/blast_furnace_front_on");
    REQUIRE(furnace != nullptr);
    CHECK(furnace->width == 16);
    CHECK(furnace->height == 16);

    // Animado com `height` declarado: vale o declarado.
    const SpriteEntry* conduit = find(sprites, "minecraft:entity/conduit/wind");
    REQUIRE(conduit != nullptr);
    CHECK(conduit->width == 64);
    CHECK(conduit->height == 32);

    // O fallback do atlas faz parte do inventário.
    CHECK(find(sprites, "minecraft:missingno") != nullptr);

    // Ordem determinística e sem repetição.
    CHECK(std::is_sorted(sprites.begin(), sprites.end(),
                         [](const SpriteEntry& a, const SpriteEntry& b) { return a.id < b.id; }));
    for (std::size_t i = 1; i < sprites.size(); ++i) {
        CHECK(sprites[i - 1].id != sprites[i].id);
    }
}
