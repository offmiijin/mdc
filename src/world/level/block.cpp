// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/world/level/block.hpp"

#include <array>
#include <cstddef>

namespace mdc::world::level {

namespace {

// Domínios na ordem de declaração do jogo (medido no harness `blockstate`).
constexpr std::string_view kBool[] = {"true", "false"};
constexpr std::string_view kAxis[] = {"x", "y", "z"};
constexpr std::string_view kFacing[] = {"north", "south", "west", "east"};
constexpr std::string_view kHalf[] = {"top", "bottom"};
constexpr std::string_view kStairsShape[] = {"straight", "inner_left", "inner_right", "outer_left",
                                             "outer_right"};
constexpr std::string_view kSlabType[] = {"top", "bottom", "double"};
constexpr std::string_view kDistance[] = {"1", "2", "3", "4", "5", "6", "7"};
constexpr std::string_view kMoisture[] = {"0", "1", "2", "3", "4", "5", "6", "7"};
constexpr std::string_view kLevel[] = {"0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",
                                       "8",  "9",  "10", "11", "12", "13", "14", "15"};
constexpr std::string_view kSnowy[] = {"true", "false"};

// Flags por bloco, do estado default. As variações por estado vêm das regras
// medidas em `state_flags()`.
constexpr StateFlags kStoneLike{};
constexpr StateFlags kAir{RenderShape::kInvisible, false, true, false, 0, 0};
constexpr StateFlags kGlass{RenderShape::kModel, false, true, false, 0, 0};
constexpr StateFlags kLeaves{RenderShape::kModel, false, false, false, 0, 1};
constexpr StateFlags kIce{RenderShape::kModel, false, false, false, 0, 1};
// Degrau/laje/farmland: parte sólida, parte vazada — o estado decide (medido).
constexpr StateFlags kPartial{RenderShape::kModel, false, true, true, 0, 0};
constexpr StateFlags kFluid{RenderShape::kInvisible, false, false, false, 0, 1};
constexpr StateFlags kLava{RenderShape::kInvisible, false, false, false, 15, 1};
constexpr StateFlags kLightSource{RenderShape::kModel, true, false, false, 15, 15};

constexpr PropertyDef kPropAxis[] = {{"axis", kAxis}};
constexpr PropertyDef kPropSnowy[] = {{"snowy", kSnowy}};
constexpr PropertyDef kPropLeaves[] = {{"distance", kDistance},
                                      {"persistent", kBool},
                                      {"waterlogged", kBool}};
constexpr PropertyDef kPropStairs[] = {{"facing", kFacing},
                                      {"half", kHalf},
                                      {"shape", kStairsShape},
                                      {"waterlogged", kBool}};
constexpr PropertyDef kPropSlab[] = {{"type", kSlabType}, {"waterlogged", kBool}};
constexpr PropertyDef kPropFarmland[] = {{"moisture", kMoisture}};
constexpr PropertyDef kPropFluid[] = {{"level", kLevel}};

constexpr std::uint8_t kDefaultsAxis[] = {1};
constexpr std::uint8_t kDefaultsSnowy[] = {1};
constexpr std::uint8_t kDefaultsLeaves[] = {6, 1, 1};
constexpr std::uint8_t kDefaultsStairs[] = {0, 1, 0, 1};
constexpr std::uint8_t kDefaultsSlab[] = {1, 1};
constexpr std::uint8_t kDefaultsFarmland[] = {0};
constexpr std::uint8_t kDefaultsFluid[] = {0};

// Tabela curada: o terreno do M2 cabe nisto, e cada linha foi conferida contra
// o jogo pelo harness `blockstate` (propriedades, defaults e flags).
constexpr BlockDef kBlocks[] = {
    {"minecraft:air", {}, {}, kAir},
    {"minecraft:stone", {}, {}, kStoneLike},
    {"minecraft:dirt", {}, {}, kStoneLike},
    {"minecraft:grass_block", kPropSnowy, kDefaultsSnowy, kStoneLike},
    {"minecraft:sand", {}, {}, kStoneLike},
    {"minecraft:gravel", {}, {}, kStoneLike},
    {"minecraft:cobblestone", {}, {}, kStoneLike},
    {"minecraft:bedrock", {}, {}, kStoneLike},
    {"minecraft:coal_ore", {}, {}, kStoneLike},
    {"minecraft:iron_ore", {}, {}, kStoneLike},
    {"minecraft:oak_log", kPropAxis, kDefaultsAxis, kStoneLike},
    {"minecraft:oak_planks", {}, {}, kStoneLike},
    {"minecraft:oak_leaves", kPropLeaves, kDefaultsLeaves, kLeaves},
    {"minecraft:oak_stairs", kPropStairs, kDefaultsStairs, kPartial},
    {"minecraft:stone_slab", kPropSlab, kDefaultsSlab, kPartial},
    {"minecraft:glass", {}, {}, kGlass},
    {"minecraft:sandstone", {}, {}, kStoneLike},
    {"minecraft:snow_block", {}, {}, kStoneLike},
    {"minecraft:ice", {}, {}, kIce},
    {"minecraft:farmland", kPropFarmland, kDefaultsFarmland, kPartial},
    {"minecraft:water", kPropFluid, kDefaultsFluid, kFluid},
    {"minecraft:lava", kPropFluid, kDefaultsFluid, kLava},
    {"minecraft:glowstone", {}, {}, kLightSource},
    {"minecraft:sea_lantern", {}, {}, kLightSource},
};

constexpr std::uint32_t kBlockCount = sizeof(kBlocks) / sizeof(kBlocks[0]);

/// Valor da propriedade por nome, dentro do bloco; `kMissing` se não existe.
constexpr std::uint8_t kMissing = 0xFF;

[[nodiscard]] std::uint8_t find_value(const PropertyDef& property, std::string_view value) {
    for (std::size_t i = 0; i < property.values.size(); ++i) {
        if (property.values[i] == value) {
            return static_cast<std::uint8_t>(i);
        }
    }
    return kMissing;
}

/// Índice (0) da propriedade `name` no bloco, se existir.
[[nodiscard]] std::uint8_t find_property(const BlockDef& def, std::string_view name) {
    for (std::size_t i = 0; i < def.properties.size(); ++i) {
        if (def.properties[i].name == name) {
            return static_cast<std::uint8_t>(i);
        }
    }
    return kMissing;
}

/// Regras por estado medidas no harness `blockstate`, não deduzidas:
///
/// - `waterlogged=true` baixa a luz do bloco: `light_dampening` vira 1 e o
///   céu deixa de passar (degrau 0→1, `skylight_down` true→false);
/// - `stone_slab[type=double]` oclui como pedra cheia: sólido para o render,
///   `light_dampening` 15, `skylight_down` falso e a forma não serve para luz.
[[nodiscard]] StateFlags state_flags(const BlockDef& def, std::span<const std::uint8_t> values) {
    StateFlags flags = def.base_flags;

    const std::uint8_t waterlogged = find_property(def, "waterlogged");
    // No jogo o booleano é declarado `true, false`, então 0 é `true`.
    if (waterlogged != kMissing && values[waterlogged] == 0) {
        if (flags.light_dampening < 1) {
            flags.light_dampening = 1;
        }
        flags.skylight_down = false;
    }

    const std::uint8_t type = find_property(def, "type");
    if (type != kMissing && def.properties[type].values[values[type]] == "double") {
        flags.solid_render = true;
        flags.light_dampening = 15;
        flags.skylight_down = false;
        flags.shape_for_light = false;
    }
    return flags;
}

} // namespace

BlockRegistry::BlockRegistry() {
    ranges_.reserve(kBlockCount);
    for (std::uint32_t b = 0; b < kBlockCount; ++b) {
        const BlockDef& def = kBlocks[b];
        std::uint32_t count = 1;
        for (const PropertyDef& property : def.properties) {
            count *= static_cast<std::uint32_t>(property.values.size());
        }
        ranges_.push_back(BlockRange{state_count_, count});
        state_count_ += count;

        // Enumera os estados na ordem do odômetro: a última propriedade varia
        // mais rápido, como na composição em base mista.
        for (std::uint32_t local = 0; local < count; ++local) {
            std::uint32_t remainder = local;
            std::uint8_t values[4] = {0, 0, 0, 0};
            for (std::size_t p = def.properties.size(); p > 0; --p) {
                const std::size_t index = p - 1;
                const auto radix = static_cast<std::uint32_t>(def.properties[index].values.size());
                values[index] = static_cast<std::uint8_t>(remainder % radix);
                remainder /= radix;
            }
            for (std::size_t p = 0; p < 4; ++p) {
                state_values_.push_back(values[p]);
            }
        }

        if (def.id == "minecraft:air") {
            air_block_ = b;
        }
    }
    air_state_ = default_state(air_block_);
}

const BlockRegistry& BlockRegistry::get() {
    static const BlockRegistry registry;
    return registry;
}

std::uint32_t BlockRegistry::block_count() const noexcept {
    return kBlockCount;
}

std::uint32_t BlockRegistry::state_count() const noexcept {
    return state_count_;
}

std::optional<std::uint32_t> BlockRegistry::find_block(std::string_view id) const {
    for (std::uint32_t b = 0; b < kBlockCount; ++b) {
        if (kBlocks[b].id == id) {
            return b;
        }
    }
    return std::nullopt;
}

const BlockDef& BlockRegistry::block_def(std::uint32_t block) const {
    return kBlocks[block];
}

std::uint32_t BlockRegistry::default_state(std::uint32_t block) const {
    return state_of(block, kBlocks[block].defaults);
}

std::uint32_t BlockRegistry::state_of(std::uint32_t block,
                                      std::span<const std::uint8_t> values) const {
    const BlockDef& def = kBlocks[block];
    std::uint32_t local = 0;
    for (std::size_t p = 0; p < def.properties.size(); ++p) {
        const auto radix = static_cast<std::uint32_t>(def.properties[p].values.size());
        local = local * radix + values[p];
    }
    return ranges_[block].first_state + local;
}

std::uint32_t BlockRegistry::block_of(std::uint32_t state) const {
    for (std::uint32_t b = 0; b < kBlockCount; ++b) {
        if (state < ranges_[b].first_state + ranges_[b].state_count) {
            return b;
        }
    }
    return kBlockCount - 1;
}

std::span<const std::uint8_t> BlockRegistry::values_of(std::uint32_t state) const {
    return std::span<const std::uint8_t>(state_values_.data() + static_cast<std::size_t>(state) * 4U,
                                        4U);
}

std::optional<std::uint8_t> BlockRegistry::property_value(std::uint32_t block, std::string_view name,
                                                          std::string_view value) const {
    const BlockDef& def = kBlocks[block];
    const std::uint8_t property = find_property(def, name);
    if (property == kMissing) {
        return std::nullopt;
    }
    const std::uint8_t index = find_value(def.properties[property], value);
    if (index == kMissing) {
        return std::nullopt;
    }
    return index;
}

StateFlags BlockRegistry::flags(std::uint32_t state) const {
    const std::uint32_t block = block_of(state);
    return state_flags(kBlocks[block], values_of(state));
}

} // namespace mdc::world::level
