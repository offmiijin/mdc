#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""PRNG determinístico dos geradores de casos de harness.

xorshift64* implementado aqui, e não via `random` do Python, por dois motivos:

- a mesma seed tem de produzir o mesmo arquivo em qualquer versão do Python;
- o caso versionado é entrada do harness, e entrada que muda sozinha quebra a
  comparação com a fixture.

Ver docs/HARNESS.md §3.1.
"""

from __future__ import annotations

import struct

MASK = 0xFFFFFFFFFFFFFFFF


class XorShift64Star:
    def __init__(self, seed: int) -> None:
        self.state = seed & MASK or 0x9E3779B97F4A7C15

    def next(self) -> int:
        x = self.state
        x ^= (x >> 12) & MASK
        x ^= (x << 25) & MASK
        x ^= (x >> 27) & MASK
        self.state = x & MASK
        return (self.state * 0x2545F4914F6CDD1D) & MASK

    def coord(self, lo: float, hi: float) -> float:
        """Valor em [lo, hi], múltiplo de 1/8 — exatamente representável em double."""
        span = int((hi - lo) * 8)
        return lo + (self.next() % (span + 1)) / 8.0

    def bits(self, exponent_lo: int, exponent_hi: int) -> int:
        """Padrão de 64 bits de um double finito com expoente na faixa dada."""
        sign = self.next() & 1
        exponent = exponent_lo + self.next() % (exponent_hi - exponent_lo + 1)
        mantissa = self.next() & 0xFFFFFFFFFFFFF
        return (sign << 63) | (exponent << 52) | mantissa

    def double(self, exponent_lo: int, exponent_hi: int) -> float:
        return struct.unpack("<d", struct.pack("<Q", self.bits(exponent_lo, exponent_hi)))[0]


def double_from_bits(bits: int) -> float:
    return struct.unpack("<d", struct.pack("<Q", bits))[0]
