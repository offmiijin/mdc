#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos de teste para o harness de AABB.

Casos são **entrada nossa**, por isso podem ser versionados: não são derivados
do jogo, são só números. A saída do harness (o que o jogo responde) é que nunca
entra no repositório.

Gerador determinístico (xorshift64*), sem depender de `random` do Python, para
que a mesma seed produza o mesmo arquivo em qualquer versão.

Uso:
    python3 tools/parity/gen_aabb_cases.py --out tests/parity/aabb-0001.txt
"""

from __future__ import annotations

import argparse
import os

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


def normalise(min_v: float, size: float) -> tuple[float, float]:
    """Aceita tamanho negativo (caixa invertida) — o jogo não valida."""
    return min_v, min_v + size


def edge_cases() -> list[list[float]]:
    """Casos de fronteira que a aleatoriedade dificilmente acerta."""
    cases: list[list[float]] = []

    def box(mx: float, my: float, mz: float, sx: float, sy: float, sz: float) -> list[float]:
        return [mx, my, mz, mx + sx, my + sy, mz + sz]

    unit = box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0)
    # Encostando face a face: interseção estrita -> false.
    cases.append(unit + box(1.0, 0.0, 0.0, 1.0, 1.0, 1.0) + [0.0])
    # Sobreposição mínima possível em double (nextafter de 1.0).
    tiny = 1.0 + 2.220446049250313e-16
    cases.append(unit + box(tiny, 0.0, 0.0, 1.0, 1.0, 1.0) + [0.0])
    # Caixa degenerada (volume zero).
    cases.append(box(2.0, 2.0, 2.0, 0.0, 0.0, 0.0) + unit + [0.0])
    # Caixa invertida (min > max).
    cases.append([5.0, 5.0, 5.0, 1.0, 1.0, 1.0] + unit + [0.0])
    # Idênticas.
    cases.append(unit + unit + [0.0])
    # Valores negativos grandes e deltas negativos.
    cases.append(box(-3.0e4, -3.0e4, -3.0e4, 16.0, 16.0, 16.0) + unit + [-1024.5])
    # Zero negativo nos limites.
    cases.append([-0.0, -0.0, -0.0, 1.0, 1.0, 1.0] + unit + [0.0])
    return cases


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/aabb-0001.txt")
    parser.add_argument("--count", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=20261026)
    parser.add_argument("--range", type=float, default=512.0)
    args = parser.parse_args()

    rng = XorShift64Star(args.seed)
    lines: list[str] = []
    lines.append("# casos do harness de AABB — entrada nossa, pode ser versionada")
    lines.append(f"# gerado por tools/parity/gen_aabb_cases.py seed={args.seed} count={args.count} range={args.range}")
    lines.append("# minX minY minZ maxX maxY maxZ  |  minX minY minZ maxX maxY maxZ  |  delta")

    for case in edge_cases():
        lines.append(" ".join(f"{value!r}" for value in case))

    half = args.range
    for _ in range(args.count):
        ax, ay, az = (rng.coord(-half, half) for _ in range(3))
        asx, asy, asz = (rng.coord(-8.0, 8.0) for _ in range(3))
        bx, by, bz = (rng.coord(-half, half) for _ in range(3))
        bsx, bsy, bsz = (rng.coord(-8.0, 8.0) for _ in range(3))
        delta = rng.coord(-16.0, 16.0)

        amx, amy, amz = normalise(ax, asx)[0], normalise(ay, asy)[0], normalise(az, asz)[0]
        amxx, amyx, amzx = normalise(ax, asx)[1], normalise(ay, asy)[1], normalise(az, asz)[1]
        bmx, bmy, bmz = normalise(bx, bsx)[0], normalise(by, bsy)[0], normalise(bz, bsz)[0]
        bmxx, bmyx, bmzx = normalise(bx, bsx)[1], normalise(by, bsy)[1], normalise(bz, bsz)[1]

        lines.append(" ".join(f"{v!r}" for v in (
            amx, amy, amz, amxx, amyx, amzx, bmx, bmy, bmz, bmxx, bmyx, bmzx, delta)))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    print(f"{args.out}: {len(lines) - 3} casos ({len(edge_cases())} de fronteira + {args.count} aleatórios)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
