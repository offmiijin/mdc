#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos de teste para o harness de resolução de colisão.

Cada linha: `n  minX minY minZ maxX maxY maxZ  moveX moveY moveZ` + `n` caixas
sólidas (6 doubles cada).

O mundo do caso é uma **lista explícita de AABBs** — sem chunk, sem bloco, sem
`Level`: o que o degrau M1 prova é a geometria da resolução.

Uso:
    python3 tools/parity/gen_collision_cases.py --out tests/parity/collision-0001.txt
"""

from __future__ import annotations

import argparse
import os

from prng import XorShift64Star

# Caixas de entidade reais: o harness não inventa dimensão, copia as do jogo.
PLAYER = (0.6, 1.8)
ITEM = (0.25, 0.25)
SLIME = (0.98, 0.98)


def box(min_x: float, min_y: float, min_z: float, sx: float, sy: float, sz: float) -> list[float]:
    return [min_x, min_y, min_z, min_x + sx, min_y + sy, min_z + sz]


def edge_cases() -> list[tuple[list[float], list[float], list[list[float]]]]:
    cases: list[tuple[list[float], list[float], list[list[float]]]] = []

    player = box(0.0, 0.0, 0.0, *PLAYER, PLAYER[0])
    floor_slab = box(-8.0, -1.0, -8.0, 16.0, 1.0, 16.0)

    # Queda livre até o chão: aplica -0.5 e para em 0.
    cases.append((player, [0.0, -0.5, 0.0], [floor_slab]))
    # Mesma queda com 1 ULP de folga: o movimento é aplicado inteiro.
    cases.append((player, [0.0, -0.9999999999999999, 0.0], [floor_slab]))
    # Subindo: o chão abaixo não interfere.
    cases.append((player, [0.0, 0.5, 0.0], [floor_slab]))
    # Movimento horizontal com o chão embaixo: desliza inteiro.
    cases.append((player, [0.5, -0.5, 0.0], [floor_slab]))
    # Parede: bate e para encostado.
    wall = box(2.0, 0.0, -4.0, 1.0, 4.0, 8.0)
    cases.append((player, [3.0, 0.0, 0.0], [wall]))
    cases.append((player, [3.0, -1.0, 0.0], [wall, floor_slab]))
    # Quina: parede e chão ao mesmo tempo (ordem y,x,z fica visível).
    cases.append((player, [3.0, -2.0, 0.0], [wall, floor_slab]))
    # Dois sólidos no mesmo eixo: vence o mais próximo.
    near = box(2.0, 0.0, 0.0, 1.0, 2.0, 1.0)
    far = box(4.0, 0.0, 0.0, 1.0, 2.0, 1.0)
    cases.append((player, [5.0, 0.0, 0.0], [far, near]))
    cases.append((player, [5.0, 0.0, 0.0], [near, far]))
    # Movimento nulo e lista vazia.
    cases.append((player, [0.0, 0.0, 0.0], [floor_slab]))
    cases.append((player, [0.5, -0.5, 0.5], []))
    # Já sobreposto ao sólido (estado "preso"): não empurra.
    cases.append((player, [0.0, -0.5, 0.0], [box(-1.0, 0.0, -1.0, 3.0, 1.0, 3.0)]))
    # Caixa de entidade pequena (item) e larga (slime) contra o chão.
    cases.append((box(0.0, 0.0, 0.0, *ITEM, ITEM[0]), [0.0, -1.0, 0.0], [floor_slab]))
    cases.append((box(0.0, 0.0, 0.0, *SLIME, SLIME[0]), [0.0, -1.0, 0.0], [floor_slab]))
    # Teto: subindo, é barrado pela face inferior.
    ceiling = box(-8.0, 3.0, -8.0, 16.0, 1.0, 16.0)
    cases.append((player, [0.0, 2.0, 0.0], [ceiling]))
    # Sólido de espessura zero (laje degenerada).
    cases.append((player, [0.0, -0.5, 0.0], [[-8.0, 0.0, -8.0, 8.0, 0.0, 8.0]]))
    # Tocar a lateral sem colidir na vertical.
    cases.append((player, [0.0, -0.5, 0.0],
                  [[player[3], 0.0, -1.0, player[3] + 1.0, 1.0, 1.0]]))
    # Coordenadas grandes.
    cases.append((box(1.0e7, 0.0, 0.0, *PLAYER, PLAYER[0]), [0.0, -0.5, 0.0],
                  [box(-1.0e7, -1.0, -1.0e7, 2.0e7, 1.0, 2.0e7)]))
    return cases


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/collision-0001.txt")
    parser.add_argument("--count", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=20261029)
    args = parser.parse_args()

    rng = XorShift64Star(args.seed)
    lines: list[str] = []
    lines.append("# casos do harness de collision — entrada nossa, pode ser versionada")
    lines.append(f"# gerado por tools/parity/gen_collision_cases.py seed={args.seed} count={args.count}")
    lines.append("# n | minX minY minZ maxX maxY maxZ | moveX moveY moveZ | n × (6 doubles)")

    def emit(entity: list[float], movement: list[float], solids: list[list[float]]) -> None:
        fields = [str(len(solids))] + [repr(v) for v in entity + movement]
        for solid in solids:
            fields += [repr(v) for v in solid]
        lines.append(" ".join(fields))

    for entity, movement, solids in edge_cases():
        emit(entity, movement, solids)

    for _ in range(args.count):
        width = rng.coord(0.25, 1.0)
        height = rng.coord(0.25, 2.0)
        entity = box(rng.coord(-4.0, 4.0), rng.coord(-2.0, 2.0), rng.coord(-4.0, 4.0), width, height,
                     width)
        movement = [rng.coord(-1.0, 1.0), rng.coord(-1.0, 1.0), rng.coord(-1.0, 1.0)]
        # 0 a 4 sólidos, com o chão presente na maioria dos casos.
        solid_count = rng.next() % 5
        solids: list[list[float]] = []
        if rng.next() % 4 != 0:
            solids.append(box(rng.coord(-1.0, 1.0), rng.coord(-3.0, -1.0), rng.coord(-1.0, 1.0),
                              8.0, 1.0, 8.0))
        while len(solids) < solid_count:
            sx = rng.coord(0.5, 4.0)
            sy = rng.coord(0.5, 3.0)
            sz = rng.coord(0.5, 4.0)
            solid = box(rng.coord(-5.0, 5.0), rng.coord(-2.0, 3.0), rng.coord(-5.0, 5.0), sx, sy, sz)
            if solid[3] > solid[0] and solid[4] > solid[1] and solid[5] > solid[2]:
                solids.append(solid)
        emit(entity, movement, solids)

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    print(f"{args.out}: {len(lines) - 3} casos ({len(edge_cases())} de fronteira + {args.count} aleatórios)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
