#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos do harness de estado de bloco (degrau M2).

Cada caso é um bloco e, opcionalmente, um estado específico. A tabela é
**curada**: são os blocos que o mesher vai encontrar primeiro no terreno e nas
construções, escolhidos para exercitar cada tipo de propriedade (enum, booleano,
inteiro) e cada combinação de luz/oclusão.

Uso:
    python3 tools/parity/gen_blockstate_cases.py --out tests/parity/blockstate-0001.txt
"""

from __future__ import annotations

import argparse
import os

# (id, [estados específicos]) — o estado vazio é sempre incluído,
# porque o default do bloco é parte do contrato.
BLOCKS: list[tuple[str, list[str]]] = [
    ("minecraft:air", []),
    ("minecraft:stone", []),
    ("minecraft:dirt", []),
    ("minecraft:grass_block", ["snowy=true"]),
    ("minecraft:sand", []),
    ("minecraft:gravel", []),
    ("minecraft:cobblestone", []),
    ("minecraft:bedrock", []),
    ("minecraft:coal_ore", []),
    ("minecraft:iron_ore", []),
    ("minecraft:oak_log", ["axis=x", "axis=z"]),
    ("minecraft:oak_planks", []),
    ("minecraft:oak_leaves", ["distance=1", "distance=7 persistent=true", "waterlogged=true"]),
    ("minecraft:oak_stairs", ["facing=east half=top", "facing=west shape=inner_left",
                              "facing=south shape=outer_right waterlogged=true"]),
    ("minecraft:stone_slab", ["type=top", "type=double waterlogged=true"]),
    ("minecraft:glass", []),
    ("minecraft:sandstone", []),
    ("minecraft:snow_block", []),
    ("minecraft:ice", []),
    ("minecraft:farmland", ["moisture=7"]),
    ("minecraft:water", ["level=7"]),
    ("minecraft:lava", ["level=15"]),
    ("minecraft:glowstone", []),
    ("minecraft:sea_lantern", []),
]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/blockstate-0001.txt")
    args = parser.parse_args()

    lines = [
        "# casos do harness de estado de bloco — entrada nossa, pode ser versionada",
        "# gerado por tools/parity/gen_blockstate_cases.py",
        "# <id> [prop=valor ...]",
    ]
    for block, states in BLOCKS:
        lines.append(block)
        for state in states:
            lines.append(f"{block} {state}")

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    print(f"{args.out}: {len(lines) - 3} casos")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
