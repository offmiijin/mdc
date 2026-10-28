// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/world/level/paletted_container.hpp"

#include <cstddef>

namespace mdc::world::level {

namespace {

/// Marca "ainda não apareceu" na varredura que monta a paleta.
constexpr std::uint32_t kUnmapped = 0xFFFFFFFFU;

/// `ceil(log2(n))`, com `n >= 1`; é o que o original usa para a paleta global
/// (4.096 valores → 12 bits).
[[nodiscard]] int ceil_log2(std::uint32_t n) noexcept {
    int bits = 0;
    std::uint32_t value = n - 1;
    while (value != 0) {
        value >>= 1;
        ++bits;
    }
    return bits;
}

} // namespace

PalettedContainer::PalettedContainer(std::uint32_t default_value, Strategy strategy)
    : strategy_(strategy),
      values_(static_cast<std::size_t>(kSize), default_value),
      present_(strategy.entry_count, false),
      distinct_(1),
      bits_per_entry_(0) {
    present_[default_value] = true;
}

void PalettedContainer::grow_to(int distinct) noexcept {
    // Degraus medidos: 0 → 4 → 5..8 (linear até 256) → global (12 para 4.096).
    const int linear_bits = distinct <= 1 ? 0 : ceil_log2(static_cast<std::uint32_t>(distinct));
    int bits = linear_bits;
    if (bits < 4) {
        bits = 4;
    }
    if (linear_bits > 8) {
        bits = ceil_log2(strategy_.entry_count);
    }
    if (bits > bits_per_entry_) {
        bits_per_entry_ = bits;
    }
}

std::uint32_t PalettedContainer::get(int x, int y, int z) const noexcept {
    return values_[static_cast<std::size_t>(index(x, y, z))];
}

void PalettedContainer::set(int x, int y, int z, std::uint32_t value) noexcept {
    std::uint32_t& cell = values_[static_cast<std::size_t>(index(x, y, z))];
    if (cell == value) {
        return;
    }
    cell = value;
    if (!present_[value]) {
        present_[value] = true;
        ++distinct_;
        grow_to(distinct_);
    }
}

PalettedContainer::Packed PalettedContainer::pack() const {
    Packed result;
    std::vector<std::uint32_t> position_of(strategy_.entry_count, kUnmapped);

    // Ordem da paleta = primeira aparição varrendo o storage (medido).
    for (int i = 0; i < kSize; ++i) {
        const std::uint32_t value = values_[static_cast<std::size_t>(i)];
        if (position_of[value] == kUnmapped) {
            position_of[value] = static_cast<std::uint32_t>(result.palette.size());
            result.palette.push_back(value);
        }
    }

    const std::size_t count = result.palette.size();
    if (count <= 1) {
        // Valor único: nada de storage, nem bits.
        result.bits = 0;
        return result;
    }

    result.bits = ceil_log2(static_cast<std::uint32_t>(count));
    if (result.bits < 4) {
        result.bits = 4;
    }

    BitStorage storage(result.bits, kSize);
    for (int i = 0; i < kSize; ++i) {
        storage.set(i, static_cast<int>(position_of[values_[static_cast<std::size_t>(i)]]));
    }
    result.storage.assign(storage.raw().begin(), storage.raw().end());
    return result;
}

} // namespace mdc::world::level
