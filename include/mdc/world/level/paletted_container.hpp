// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <vector>

#include "mdc/world/level/bit_storage.hpp"

namespace mdc::world::level {

/// Como o container paletizado se comporta em relação ao espaço de valores.
///
/// No original isso é um objeto genérico; aqui só existe o que o M2 usa: o
/// número de valores possíveis (o registro de estados de bloco tem 4.096).
struct Strategy {
    std::uint32_t entry_count{4096};
};

/// Container de 16³ valores com paleta, no modelo do original.
///
/// Duas regras foram **medidas** pelo harness `palette`, não deduzidas:
///
/// 1. `bits_per_entry()` cresce em degraus e **nunca encolhe**: valor único → 0
///    bit; até 256 valores → `max(4, ceil(log2(n)))`; acima disso → paleta
///    global, com `ceil(log2(entry_count))` bits.
/// 2. `pack()` **não** devolve a ordem de inserção: a lista da paleta é a ordem
///    em que cada valor aparece pela primeira vez **varrendo o storage** (o
///    último valor a aparecer é o default, se ele ainda estiver em uso). Por
///    isso dois casos com a mesma escrita em ordens diferentes produzem a mesma
///    transcrição.
///
/// O estado interno guarda os valores direto (16 KiB); a paleta e o
/// empacotamento são derivados no `pack()`, que é o único lugar onde o formato
/// serializado é observável.
class PalettedContainer {
  public:
    static constexpr int kEdge = 16;
    static constexpr int kSize = kEdge * kEdge * kEdge;

    /// Resultado de `pack()`: a paleta em ordem de primeira aparição e os
    /// valores como índice de paleta, empacotados.
    struct Packed {
        std::vector<std::uint32_t> palette;
        std::vector<std::uint64_t> storage;
        int bits{0};
    };

    PalettedContainer(std::uint32_t default_value, Strategy strategy);

    [[nodiscard]] std::uint32_t get(int x, int y, int z) const noexcept;
    void set(int x, int y, int z, std::uint32_t value) noexcept;

    /// Bits por entrada do container (0 = valor único).
    [[nodiscard]] int bits_per_entry() const noexcept { return bits_per_entry_; }

    [[nodiscard]] Packed pack() const;

    /// Índice linear de `(x, y, z)`: `y` é o eixo mais significativo.
    [[nodiscard]] static int index(int x, int y, int z) noexcept {
        return (y << 8) | (z << 4) | x;
    }

  private:
    void grow_to(int distinct) noexcept;

    Strategy strategy_{};
    std::vector<std::uint32_t> values_{};
    std::vector<bool> present_{};
    int distinct_{0};
    int bits_per_entry_{0};
};

} // namespace mdc::world::level
