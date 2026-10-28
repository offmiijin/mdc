#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos de teste para o harness de `Aabb::clip` (raycast).

Casos são **entrada nossa**, por isso podem ser versionados. Cada linha:
`minX minY minZ maxX maxY maxZ fromX fromY fromZ toX toY toZ`.

Metade dos casos é de fronteira (o que a aleatoriedade não acerta): partida
dentro/na face, chegada dentro/na face, eixos paralelos, comprimento zero,
empate de eixos, caixa degenerada e coordenadas grandes.

Uso:
    python3 tools/parity/gen_clip_cases.py --out tests/parity/clip-0001.txt
"""

from __future__ import annotations

import argparse
import os

from prng import XorShift64Star


def box(min_x: float, min_y: float, min_z: float, sx: float, sy: float, sz: float) -> list[float]:
    return [min_x, min_y, min_z, min_x + sx, min_y + sy, min_z + sz]


UNIT = box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0)


def edge_cases() -> list[list[float]]:
    cases: list[list[float]] = []

    def add(box6: list[float], frm: tuple[float, float, float], to: tuple[float, float, float]) -> None:
        cases.append(box6 + list(frm) + list(to))

    # Cruzando de fora para fora, pelos três eixos.
    add(UNIT, (-1.0, 0.5, 0.5), (2.0, 0.5, 0.5))
    add(UNIT, (0.5, -1.0, 0.5), (0.5, 2.0, 0.5))
    add(UNIT, (0.5, 0.5, -1.0), (0.5, 0.5, 2.0))
    # Sentido invertido.
    add(UNIT, (2.0, 0.5, 0.5), (-1.0, 0.5, 0.5))
    add(UNIT, (0.5, 2.0, 0.5), (0.5, -1.0, 0.5))
    # Partida dentro e partida na face: sem face de entrada -> vazio.
    add(UNIT, (0.5, 0.5, 0.5), (2.0, 0.5, 0.5))
    add(UNIT, (0.0, 0.5, 0.5), (2.0, 0.5, 0.5))
    add(UNIT, (-0.0, 0.5, 0.5), (2.0, 0.5, 0.5))
    add(UNIT, (1.0, 1.0, 1.0), (2.0, 2.0, 2.0))
    # Chegada dentro e chegada exatamente na face (t == 1 não conta).
    add(UNIT, (-1.0, 0.5, 0.5), (0.5, 0.5, 0.5))
    add(UNIT, (-1.0, 0.5, 0.5), (0.0, 0.5, 0.5))
    add(UNIT, (-1.0, 0.5, 0.5), (1.0, 0.5, 0.5))
    # Eixos paralelos: fora (vazio) e sobre a face (rasante).
    add(UNIT, (-1.0, 2.0, 0.5), (2.0, 2.0, 0.5))
    add(UNIT, (-1.0, 1.0, 0.5), (2.0, 1.0, 0.5))
    add(UNIT, (-1.0, 0.0, 0.5), (2.0, 0.0, 0.5))
    add(UNIT, (-1.0, 0.5, -3.0), (-1.0, 0.5, 3.0))
    # Comprimento zero.
    add(UNIT, (0.5, 0.5, 0.5), (0.5, 0.5, 0.5))
    add(UNIT, (1.5, 0.5, 0.5), (1.5, 0.5, 0.5))
    add(UNIT, (-1.0, 0.5, 0.5), (-1.0, 0.5, 0.5))
    # Empate de eixos: dois e três ao mesmo tempo.
    add(UNIT, (-1.0, -1.0, 0.5), (0.5, 0.5, 0.5))
    add(UNIT, (-1.0, -1.0, -1.0), (0.5, 0.5, 0.5))
    add(UNIT, (2.0, 2.0, 2.0), (0.5, 0.5, 0.5))
    # Vértice exato e diagonal longa.
    add(UNIT, (-1.0, -1.0, -1.0), (2.0, 2.0, 2.0))
    add(UNIT, (-100.0, -100.0, -100.0), (100.0, 100.0, 100.0))
    # Caixa degenerada em um eixo e em todos.
    add(box(0.0, 0.0, 0.0, 0.0, 1.0, 1.0), (-1.0, 0.5, 0.5), (2.0, 0.5, 0.5))
    add([2.0, 2.0, 2.0, 2.0, 2.0, 2.0], (0.0, 2.0, 2.0), (4.0, 2.0, 2.0))
    add([2.0, 2.0, 2.0, 2.0, 2.0, 2.0], (0.0, 0.0, 0.0), (4.0, 4.0, 4.0))
    # Caixa invertida: o construtor normaliza, o caso continua válido.
    add([1.0, 1.0, 1.0, 0.0, 0.0, 0.0], (-1.0, 0.5, 0.5), (2.0, 0.5, 0.5))
    # Coordenadas grandes e negativas.
    big = box(-1.0e7, -1.0e7, -1.0e7, 2.0e7, 2.0e7, 2.0e7)
    add(big, (-2.0e7, 0.0, 0.0), (2.0e7, 0.0, 0.0))
    add(big, (-2.0e7, 1.0e7, 1.0e7), (-1.5e7, 1.0e7, 1.0e7))
    return cases


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/clip-0001.txt")
    parser.add_argument("--count", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=20261028)
    args = parser.parse_args()

    rng = XorShift64Star(args.seed)
    lines: list[str] = []
    lines.append("# casos do harness de clip — entrada nossa, pode ser versionada")
    lines.append(f"# gerado por tools/parity/gen_clip_cases.py seed={args.seed} count={args.count}")
    lines.append("# minX minY minZ maxX maxY maxZ  |  fromX fromY fromZ  |  toX toY toZ")

    for case in edge_cases():
        lines.append(" ".join(repr(value) for value in case))

    for _ in range(args.count):
        min_x, min_y, min_z = (rng.coord(-4.0, 4.0) for _ in range(3))
        sx, sy, sz = (rng.coord(0.0, 4.0) for _ in range(3))
        box6 = box(min_x, min_y, min_z, sx, sy, sz)

        # Metade dos raios parte de longe (cruzando a caixa), metade de perto.
        if rng.next() % 2 == 0:
            fx, fy, fz = (rng.coord(-8.0, 8.0) for _ in range(3))
        else:
            fx, fy, fz = (rng.coord(-2.0, 2.0) for _ in range(3))
        dx, dy, dz = (rng.coord(-8.0, 8.0) for _ in range(3))
        lines.append(" ".join(repr(v) for v in (box6 + [fx, fy, fz, fx + dx, fy + dy, fz + dz])))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    print(f"{args.out}: {len(lines) - 3} casos ({len(edge_cases())} de fronteira + {args.count} aleatórios)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
