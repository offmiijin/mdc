// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace mdc::world::level {

/// Empacotamento de bits: `size` valores de `bits` bits em `std::uint64_t`.
///
/// Uma regra não é óbvia e foi medida no harness `palette`: um valor **nunca**
/// atravessa a fronteira de um `long`. Com 5 bits cabem 12 valores por long (60
/// bits usados, 4 desperdiçados) e não 12,8 — é por isso que 4.096 valores de 5
/// bits ocupam 342 longs e não 320.
///
/// `bits == 0` é o storage degenerado do original: todos os valores são zero,
/// `raw()` é vazio e nada é alocado.
class BitStorage {
  public:
    BitStorage(int bits, int size);

    [[nodiscard]] int bits() const noexcept { return bits_; }
    [[nodiscard]] int size() const noexcept { return size_; }

    [[nodiscard]] int get(int index) const noexcept;

    /// Grava `value`, que deve caber em `bits()`; não há validação por valor
    /// porque o container sempre escreve um índice válido.
    void set(int index, int value) noexcept;

    [[nodiscard]] std::span<const std::uint64_t> raw() const noexcept { return raw_; }

  private:
    [[nodiscard]] static int values_per_long(int bits) noexcept { return 64 / bits; }

    int bits_{0};
    int size_{0};
    int values_per_long_{0};
    std::vector<std::uint64_t> raw_{};
};

} // namespace mdc::world::level
