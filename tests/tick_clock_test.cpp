// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include <doctest/doctest.h>

#include <cstdint>
#include <utility>

#include "mdc/core/tick_clock.hpp"

using mdc::core::TickClock;

namespace {
constexpr std::int64_t kMs = 1'000'000;
}

TEST_CASE("tick tem 50 ms exatos") {
    static_assert(TickClock::kTickDurationNs == 50 * kMs);
    CHECK(TickClock::kTicksPerSecond == 20);
}

TEST_CASE("acumulador não dispara antes de um tick completo") {
    TickClock clock;
    CHECK(clock.advance(0) == 0);
    CHECK(clock.advance(49 * kMs) == 0);   // 49 ms
    CHECK(clock.ticks() == 0);
    CHECK(clock.advance(1 * kMs) == 1);    // fecha 50 ms
    CHECK(clock.ticks() == 1);
    CHECK(clock.accumulator_ns() == 0);
}

TEST_CASE("um segundo produz exatamente 20 ticks") {
    TickClock clock;
    CHECK(clock.advance(1'000 * kMs) == 20);
    CHECK(clock.ticks() == 20);
    CHECK(clock.dropped_ticks() == 0);
}

TEST_CASE("deltas irregulares não acumulam drift") {
    TickClock clock;
    // 60 frames de 16,666666 ms: 999.999.960 ns, ou seja 19 ticks e sobra.
    std::int64_t simulated = 0;
    int ticks = 0;
    for (int frame = 0; frame < 60; ++frame) {
        const std::int64_t delta = 16'666'666; // 16,666666 ms
        simulated += delta;
        ticks += clock.advance(delta);
    }
    CHECK(ticks == 19);
    CHECK(clock.ticks() == 19);
    CHECK(clock.dropped_ticks() == 0);
    // O resto é exatamente o que sobrou — nada é perdido nem inventado.
    CHECK(clock.accumulator_ns() == simulated - 19 * TickClock::kTickDurationNs);
    CHECK(clock.accumulator_ns() < TickClock::kTickDurationNs);
}

TEST_CASE("criterio de aceite do M0: 12.000 ticks em 10 minutos") {
    // Com deltas exatos de 50 ms, 600 s são 12.000 ticks, sem drift e sem descarte.
    TickClock clock;
    int total = 0;
    for (int tick = 0; tick < 12'000; ++tick) {
        total += clock.advance(TickClock::kTickDurationNs);
    }
    CHECK(total == 12'000);
    CHECK(clock.ticks() == 12'000);
    CHECK(clock.dropped_ticks() == 0);
    CHECK(clock.accumulator_ns() == 0);

    // Com frames de 60 fps (16.666.666 ns) o mesmo tempo rende 11.999 ticks:
    // a diferença é truncamento honesto, não drift acumulado.
    TickClock at60fps;
    std::int64_t simulated = 0;
    int produced = 0;
    for (int frame = 0; frame < 36'000; ++frame) {
        produced += at60fps.advance(16'666'666);
        simulated += 16'666'666;
    }
    CHECK(produced == 11'999);
    CHECK(at60fps.ticks() == 11'999);
    CHECK(at60fps.dropped_ticks() == 0);
    CHECK(at60fps.accumulator_ns() == simulated - 11'999 * TickClock::kTickDurationNs);
}

TEST_CASE("delta grande é cortado e contabilizado, não engolido") {
    TickClock clock;
    CHECK(clock.advance(1'000 * kMs) == 20); // 20 ticks devidos, teto é 20
    CHECK(clock.dropped_ticks() == 0);

    TickClock slow;
    CHECK(slow.advance(5'000 * kMs) == TickClock::kMaxTicksPerCall); // 100 devidos
    CHECK(slow.dropped_ticks() == 100 - TickClock::kMaxTicksPerCall);
    CHECK(slow.ticks() == TickClock::kMaxTicksPerCall);
}

TEST_CASE("delta zero ou negativo é ignorado") {
    TickClock clock;
    CHECK(clock.advance(0) == 0);
    CHECK(clock.advance(-1) == 0);
    CHECK(clock.advance(-1'000 * kMs) == 0);
    CHECK(clock.ticks() == 0);
    CHECK(clock.accumulator_ns() == 0);
}

TEST_CASE("determinismo: mesma sequência de deltas, mesmo resultado") {
    const std::int64_t deltas[] = {16'666'666, 16'666'666, 33'333'333, 0, 16'666'666, 50'000'000};
    auto run = [&] {
        TickClock clock;
        std::int64_t ticks = 0;
        for (std::int64_t delta : deltas) {
            ticks += clock.advance(delta);
        }
        return std::pair{ticks, clock.accumulator_ns()};
    };
    CHECK(run() == run());
}
