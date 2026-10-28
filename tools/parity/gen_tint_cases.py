#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos do harness de tint (degrau M2).

Cada caso é `(tipo, temperatura, precipitação)`. A grade cobre os cantos da
tabela de cores, os múltiplos de 1/255 (limites de célula) e valores fora de
[0,1], que o original grampeia.

Uso:
    python3 tools/parity/gen_tint_cases.py --out tests/parity/tint-0001.txt
"""

from __future__ import annotations

import argparse
import os

from prng import XorShift64Star

KINDS = ("grass", "foliage", "dry_foliage")


def edge_values() -> list[float]:
    # O domínio do original é [0,1]^2: fora dele o índice da tabela estoura,
    # então o caso não existe (ver docs/HARNESS.md §2 sobre não inventar caso).
    values = [0.0, 1.0, -0.0, 0.5]
    for step in (255, 256):
        for i in range(0, step + 1, max(1, step // 8)):
            values.append(i / step)
    # Vizinhança de um limite de célula: é onde o índice erra por 1.
    values += [254.5 / 255.0, 255.0 / 255.0, 1.0 / 255.0, 0.9999999, 1.0000001]
    return values


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/tint-0001.txt")
    parser.add_argument("--count", type=int, default=600)
    parser.add_argument("--seed", type=int, default=20261029)
    args = parser.parse_args()

    rng = XorShift64Star(args.seed)
    lines = [
        "# casos do harness de tint — entrada nossa, pode ser versionada",
        f"# gerado por tools/parity/gen_tint_cases.py seed={args.seed} count={args.count}",
        "# <tipo> <temperatura> <precipitação>",
    ]

    edges = edge_values()
    for kind in KINDS:
        for temperature in (0.0, 0.5, 1.0):
            for downfall in (0.0, 0.5, 1.0):
                lines.append(f"{kind} {temperature!r} {downfall!r}")
        for value in edges:
            lines.append(f"{kind} {value!r} {0.5!r}")
            lines.append(f"{kind} {0.5!r} {value!r}")

    for i in range(args.count):
        kind = KINDS[i % len(KINDS)]
        lines.append(f"{kind} {rng.coord(0.0, 1.0)!r} {rng.coord(0.0, 1.0)!r}")

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print(f"{args.out}: {len(lines) - 3} casos")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
