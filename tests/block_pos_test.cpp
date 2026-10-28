// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include <doctest/doctest.h>

#include <limits>

#include "mdc/core/block_pos.hpp"

using mdc::core::BlockPos;
using mdc::core::Vec3;

TEST_CASE("containing usa floor, não truncamento") {
    // Truncar daria 0; o chão é -1.
    CHECK(BlockPos::containing(Vec3{-0.5, -0.5, -0.5}).equals(BlockPos{-1, -1, -1}));
    CHECK(BlockPos::containing(Vec3{0.999, 0.999, 0.999}).equals(BlockPos{0, 0, 0}));
    CHECK(BlockPos::containing(Vec3{1.0, 2.0, 3.0}).equals(BlockPos{1, 2, 3}));
    CHECK(BlockPos::containing(Vec3{-1.0, -2.0, -3.0}).equals(BlockPos{-1, -2, -3}));
    CHECK(BlockPos::containing(Vec3{-0.0, 0.0, -0.0}).equals(BlockPos{0, 0, 0}));
}

TEST_CASE("o cast do Java satura em vez de estourar") {
    constexpr int kMax = std::numeric_limits<int>::max();
    constexpr int kMin = std::numeric_limits<int>::min();

    CHECK(BlockPos::floor_to_int(1.0e300) == kMax);
    CHECK(BlockPos::floor_to_int(-1.0e300) == kMin);
    CHECK(BlockPos::floor_to_int(static_cast<double>(kMax) + 1024.0) == kMax);
    CHECK(BlockPos::floor_to_int(static_cast<double>(kMin) - 1024.0) == kMin);
}

TEST_CASE("vizinhos e empacotamento em long") {
    const BlockPos origin{};
    CHECK(origin.above().equals(BlockPos{0, 1, 0}));
    CHECK(origin.below().equals(BlockPos{0, -1, 0}));
    CHECK(BlockPos{1, 2, 3}.offset(-1, -2, -3).equals(origin));

    CHECK(origin.as_long() == 0);
    CHECK(BlockPos{1, 0, 0}.as_long() != BlockPos{0, 0, 1}.as_long());
}
