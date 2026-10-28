// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Testes de unidade do registro de blocos e das flags por estado. A paridade com
// o jogo é do harness `blockstate`; aqui ficam as regras medidas em forma
// verificável sem Java.
#include <doctest/doctest.h>

#include "mdc/world/level/block.hpp"

using mdc::world::level::BlockRegistry;
using mdc::world::level::RenderShape;

TEST_CASE("o registro enumera estados de forma estável") {
    const BlockRegistry& registry = BlockRegistry::get();
    CHECK(registry.block_count() == 24);
    // 24 blocos: 16 de estado único + 2 (grass_block) + 3 (log) + 28 (leaves)
    // + 80 (stairs) + 6 (slab) + 8 (farmland) + 16 (water) + 16 (lava).
    CHECK(registry.state_count() == 16 + 2 + 3 + 28 + 80 + 6 + 8 + 16 + 16);
    CHECK(registry.state_count() == 175);
}

TEST_CASE("o estado default é o da tabela, não o primeiro estado do bloco") {
    const BlockRegistry& registry = BlockRegistry::get();
    const auto stairs = registry.find_block("minecraft:oak_stairs");
    REQUIRE(stairs.has_value());
    // Default medido: facing=north(0), half=bottom(1), shape=straight(0),
    // waterlogged=false(1).
    const std::uint8_t defaults[] = {0, 1, 0, 1};
    CHECK(registry.default_state(*stairs) == registry.state_of(*stairs, defaults));
}

TEST_CASE("propriedade e valor fora do bloco são recusados") {
    const BlockRegistry& registry = BlockRegistry::get();
    const auto stone = registry.find_block("minecraft:stone");
    const auto stairs = registry.find_block("minecraft:oak_stairs");
    REQUIRE(stone.has_value());
    REQUIRE(stairs.has_value());

    CHECK_FALSE(registry.property_value(*stone, "facing", "east").has_value());
    CHECK_FALSE(registry.property_value(*stairs, "facing", "up").has_value());
    CHECK(registry.property_value(*stairs, "facing", "east") == 3);
    // O booleano é declarado `true, false`: `true` é o índice 0.
    CHECK(registry.property_value(*stairs, "waterlogged", "true") == 0);
    CHECK_FALSE(registry.find_block("minecraft:not_a_block").has_value());
}

TEST_CASE("waterlogged baixa a luz do bloco") {
    const BlockRegistry& registry = BlockRegistry::get();
    const auto stairs = registry.find_block("minecraft:oak_stairs");
    REQUIRE(stairs.has_value());

    const std::uint8_t dry[] = {0, 1, 0, 1};
    const std::uint8_t wet[] = {0, 1, 0, 0};
    const auto dry_flags = registry.flags(registry.state_of(*stairs, dry));
    const auto wet_flags = registry.flags(registry.state_of(*stairs, wet));

    CHECK(dry_flags.light_dampening == 0);
    CHECK(dry_flags.skylight_down);
    CHECK(wet_flags.light_dampening == 1);
    CHECK_FALSE(wet_flags.skylight_down);
}

TEST_CASE("laje dupla oclui como bloco cheio") {
    const BlockRegistry& registry = BlockRegistry::get();
    const auto slab = registry.find_block("minecraft:stone_slab");
    REQUIRE(slab.has_value());

    const std::uint8_t bottom[] = {1, 1};
    const std::uint8_t double_slab[] = {2, 1};
    const auto bottom_flags = registry.flags(registry.state_of(*slab, bottom));
    const auto double_flags = registry.flags(registry.state_of(*slab, double_slab));

    CHECK_FALSE(bottom_flags.solid_render);
    CHECK(bottom_flags.light_dampening == 0);
    CHECK(double_flags.solid_render);
    CHECK(double_flags.light_dampening == 15);
    CHECK_FALSE(double_flags.skylight_down);
    CHECK_FALSE(double_flags.shape_for_light);
}

TEST_CASE("fluido é invisível para o render, mas não para a luz") {
    const BlockRegistry& registry = BlockRegistry::get();
    const auto water = registry.find_block("minecraft:water");
    const auto lava = registry.find_block("minecraft:lava");
    REQUIRE(water.has_value());
    REQUIRE(lava.has_value());

    const auto water_flags = registry.flags(registry.default_state(*water));
    const auto lava_flags = registry.flags(registry.default_state(*lava));
    CHECK(water_flags.render_shape == RenderShape::kInvisible);
    CHECK(lava_flags.render_shape == RenderShape::kInvisible);
    CHECK(lava_flags.light_emission == 15);
}

TEST_CASE("os estados de um bloco são contíguos e cada um pertence a ele") {
    const BlockRegistry& registry = BlockRegistry::get();
    for (std::uint32_t block = 0; block < registry.block_count(); ++block) {
        const std::uint32_t state = registry.default_state(block);
        CHECK(registry.block_of(state) == block);
    }
}
