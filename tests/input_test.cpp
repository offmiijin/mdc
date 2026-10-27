// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include "mdc/core/input.hpp"

using mdc::core::InputAction;
using mdc::core::InputBuffer;
using mdc::core::InputFrame;
using mdc::core::mask_of;

TEST_CASE("teclado é nível: set_keys substitui, não acumula") {
    InputBuffer input;

    input.set_keys(mask_of(InputAction::kForward));
    CHECK(input.consume().held(InputAction::kForward));

    // Soltar a tecla = reescrever a máscara sem aquele bit.
    input.set_keys(0);
    const InputFrame released = input.consume();
    CHECK(!released.held(InputAction::kForward));
    CHECK(released.keys == 0); // e continua valendo no tick seguinte
    CHECK(!input.consume().held(InputAction::kForward));
}

TEST_CASE("mouse acumula entre eventos e é consumido uma única vez") {
    InputBuffer input;

    // Dois eventos no mesmo intervalo de tick (o caso comum a 1000 Hz de poll).
    input.add_mouse_delta(3.0, -1.5);
    input.add_mouse_delta(-0.5, 0.5);

    const InputFrame first = input.consume();
    CHECK(first.mouse_dx == 2.5);
    CHECK(first.mouse_dy == -1.0);

    // O tick seguinte não recebe de novo o que já foi aplicado.
    const InputFrame second = input.consume();
    CHECK(second.mouse_dx == 0.0);
    CHECK(second.mouse_dy == 0.0);
}

TEST_CASE("delta não finito é descartado em par") {
    InputBuffer input;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    input.add_mouse_delta(nan, 1.0); // o par inteiro é suspeito: cai fora
    input.add_mouse_delta(1.0, inf);
    CHECK(input.consume().mouse_dx == 0.0);

    input.add_mouse_delta(2.0, 3.0); // depois disso o buffer volta a funcionar
    const InputFrame frame = input.consume();
    CHECK(frame.mouse_dx == 2.0);
    CHECK(frame.mouse_dy == 3.0);
}

TEST_CASE("ações são bits independentes") {
    InputBuffer input;
    // Fora do CHECK: o doctest não decompõe `a == b | c` e o aviso vira erro.
    const auto forward_and_left = static_cast<std::uint16_t>(mask_of(InputAction::kForward) |
                                                             mask_of(InputAction::kLeft));
    input.set_keys(forward_and_left);

    const InputFrame frame = input.consume();
    CHECK(frame.held(InputAction::kForward));
    CHECK(frame.held(InputAction::kLeft));
    CHECK(!frame.held(InputAction::kBack));
    CHECK(!frame.held(InputAction::kSprint));
    CHECK(frame.keys == forward_and_left);
}

TEST_CASE("cada delta chega a exatamente um tick, em ordem") {
    InputBuffer input;
    double total = 0.0;

    for (int tick = 0; tick < 20; ++tick) {
        input.set_keys(mask_of(InputAction::kForward));
        input.add_mouse_delta(static_cast<double>(tick), 0.0);
        input.add_mouse_delta(0.5, 0.0);
        total += input.consume().mouse_dx;
    }

    // Soma esperada = soma dos deltas injetados, sem perda nem duplicação.
    const double expected = (19.0 * 20.0 / 2.0) + 20 * 0.5;
    CHECK(total == expected);
}
