#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos de teste para o harness de `BlockPos`.

Cada linha é um ponto: `x y z` (doubles). O que interessa é a conversão para
coordenada de bloco — o `cast` do Java satura e transforma NaN em 0, e é aí que
implementações "óbvias" divergem.

Uso:
    python3 tools/parity/gen_blockpos_cases.py --out tests/parity/blockpos-0001.txt
"""

from __future__ import annotations

import argparse
import os

from prng import XorShift64Star

INT_MAX = 2147483647
INT_MIN = -2147483648


def edge_cases() -> list[list[float]]:
    return [
        [0.0, 0.0, 0.0],
        [-0.0, -0.0, -0.0],
        [0.5, 0.5, 0.5],
        [-0.5, -0.5, -0.5],
        [0.9999999999999999, 0.9999999999999999, 0.9999999999999999],
        [-0.9999999999999999, -0.9999999999999999, -0.9999999999999999],
        [1.0, -1.0, 1.0],
        [-1.0, 1.0, -1.0],
        # Fronteiras do int: saturação, não wrap-around.
        [float(INT_MAX), float(INT_MAX) + 1.0, float(INT_MAX) * 4.0],
        [float(INT_MIN), float(INT_MIN) - 1.0, float(INT_MIN) * 4.0],
        [1.0e300, -1.0e300, 1.0e18],
        # Coordenadas típicas de mundo (limite de construção do 26.3).
        [-30000000.7, 320.0, 29999999.9],
        [1.5e-9, -1.5e-9, 5e-324],
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/blockpos-0001.txt")
    parser.add_argument("--count", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=20261030)
    args = parser.parse_args()

    rng = XorShift64Star(args.seed)
    lines = [
        "# casos do harness de blockpos — entrada nossa, pode ser versionada",
        f"# gerado por tools/parity/gen_blockpos_cases.py seed={args.seed} count={args.count}",
        "# x y z (doubles); a saída é a coordenada de bloco e os vizinhos",
    ]

    for case in edge_cases():
        lines.append(" ".join(repr(v) for v in case))

    for _ in range(args.count):
        point = [rng.coord(-64.0, 64.0) for _ in range(3)]
        if rng.next() % 4 == 0:
            point = [rng.double(1023 - 30, 1023 + 30) for _ in range(3)]
        lines.append(" ".join(repr(v) for v in point))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    print(f"{args.out}: {len(lines) - 3} casos ({len(edge_cases())} de fronteira + {args.count} aleatórios)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
