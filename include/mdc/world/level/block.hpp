// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace mdc::world::level {

/// Como o bloco entra no render. `kInvisible` é o caso dos fluidos e do ar:
/// eles existem como estado, mas não geram geometria de modelo.
enum class RenderShape : std::uint8_t { kInvisible, kModel };

/// Consultas por estado que o mesher e a luz vão usar. São **por estado**, não
/// por bloco: `stone_slab[type=double]` oclui como pedra, e
/// `oak_stairs[waterlogged=true]` altera a luz (medido no harness `blockstate`).
struct StateFlags {
    RenderShape render_shape{RenderShape::kModel};
    bool solid_render{true};
    bool skylight_down{false};
    bool shape_for_light{false};
    std::uint8_t light_emission{0};
    std::uint8_t light_dampening{15};
};

/// Uma propriedade de estado: nome e domínio na ordem de declaração do jogo.
struct PropertyDef {
    std::string_view name;
    std::span<const std::string_view> values;
};

struct BlockDef {
    std::string_view id;
    std::span<const PropertyDef> properties;
    /// Índices do estado default, uma entrada por propriedade.
    std::span<const std::uint8_t> defaults;
    StateFlags base_flags{};
};

/// Registro de blocos: tabela curada (os blocos que o terreno do M2 encontra) e
/// enumeração dos estados.
///
/// O espaço de estado é **nosso**: cada bloco ocupa um intervalo contíguo e,
/// dentro dele, os valores são indexados em base mista na ordem das
/// propriedades. Isso dá ao M2 um token inteiro estável para a paleta, sem
/// depender da ordem do registro do jogo (que é dado do original).
class BlockRegistry {
  public:
    static const BlockRegistry& get();

    [[nodiscard]] std::uint32_t block_count() const noexcept;
    [[nodiscard]] std::uint32_t state_count() const noexcept;

    [[nodiscard]] std::optional<std::uint32_t> find_block(std::string_view id) const;
    [[nodiscard]] const BlockDef& block_def(std::uint32_t block) const;

    [[nodiscard]] std::uint32_t default_state(std::uint32_t block) const;
    /// Compõe o índice de estado a partir dos valores por propriedade.
    [[nodiscard]] std::uint32_t state_of(std::uint32_t block,
                                        std::span<const std::uint8_t> values) const;

    [[nodiscard]] std::uint32_t block_of(std::uint32_t state) const;
    [[nodiscard]] std::span<const std::uint8_t> values_of(std::uint32_t state) const;

    /// Índice do valor `value` na propriedade `name`, se ambos existirem.
    [[nodiscard]] std::optional<std::uint8_t> property_value(std::uint32_t block,
                                                            std::string_view name,
                                                            std::string_view value) const;

    [[nodiscard]] StateFlags flags(std::uint32_t state) const;

    [[nodiscard]] std::uint32_t air_state() const { return air_state_; }

  private:
    BlockRegistry();

    struct BlockRange {
        std::uint32_t first_state{0};
        std::uint32_t state_count{1};
    };

    std::vector<BlockRange> ranges_{};
    std::vector<std::uint8_t> state_values_{};
    std::uint32_t state_count_{0};
    std::uint32_t air_block_{0};
    std::uint32_t air_state_{0};
};

} // namespace mdc::world::level
