// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/world/level/bit_storage.hpp"

namespace mdc::world::level {

BitStorage::BitStorage(int bits, int size) : bits_(bits), size_(size) {
    if (bits_ <= 0) {
        // Storage degenerado: nenhum long, todo `get` devolve zero.
        bits_ = 0;
        return;
    }
    values_per_long_ = values_per_long(bits_);
    // Sem divisão de valor entre longs: arredonda para cima o número de longs.
    const int longs = (size_ + values_per_long_ - 1) / values_per_long_;
    raw_.assign(static_cast<std::size_t>(longs), 0U);
}

int BitStorage::get(int index) const noexcept {
    if (bits_ == 0) {
        return 0;
    }
    const int long_index = index / values_per_long_;
    const int offset = (index % values_per_long_) * bits_;
    const std::uint64_t mask = (std::uint64_t{1} << bits_) - 1U;
    return static_cast<int>((raw_[static_cast<std::size_t>(long_index)] >> offset) & mask);
}

void BitStorage::set(int index, int value) noexcept {
    if (bits_ == 0) {
        return;
    }
    const int long_index = index / values_per_long_;
    const int offset = (index % values_per_long_) * bits_;
    const std::uint64_t mask = ((std::uint64_t{1} << bits_) - 1U);
    std::uint64_t& target = raw_[static_cast<std::size_t>(long_index)];
    target = (target & ~(mask << offset)) | ((static_cast<std::uint64_t>(value) & mask) << offset);
}

} // namespace mdc::world::level
