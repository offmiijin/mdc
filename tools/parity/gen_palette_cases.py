#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos do harness de paleta (degrau M2).

Dois tipos de caso, no mesmo arquivo:

- `storage <bits> <size> <índice>=<valor> ...` — empacotamento de bits;
- `container <x>,<y>,<z>=<token> ...` — paleta + container paletizado.

Os tokens são um espaço nosso (`0..4095`), não identificadores do jogo: o probe
usa um `IdMap` identidade, então nada do original entra na entrada nem na saída.

O que a fronteira precisa cobrir (medido no original, não deduzido):

- `bits = 0` (storage degenerado) e valores que atravessam a fronteira de um
  `long` — o original **não** divide um valor entre dois longs;
- crescimento da paleta: 1 valor (single), 16 (linear), 17 e 256 (hashmap),
  257 e 4096 (paleta global);
- paleta que **não encolhe**: sobrescrever valores não devolve a paleta ao
  tamanho menor.

Uso:
    python3 tools/parity/gen_palette_cases.py --out tests/parity/palette-0001.txt
"""

from __future__ import annotations

import argparse
import os

from prng import XorShift64Star

ENTRY_COUNT = 4096


def storage_edge_cases() -> list[str]:
    """Casos de fronteira do empacotamento de bits."""
    cases: list[str] = []
    # Bits onde a divisão de 64 não é inteira: o último long fica parcial.
    # Tamanho pequeno de propósito: a transcrição carrega o `long[]` inteiro.
    for bits in (1, 2, 3, 5, 6, 7, 9, 12, 17, 31, 32):
        sizes = (1, 16, 64, 256) if bits > 8 else (1, 16, 64, 256, 4096)
        for size in sizes:
            # `-1` é "todos os bits" para 32 bits: o valor tem de caber em int.
            full = (1 << 31) - 1 if bits == 32 else (1 << bits) - 1
            ops = []
            for index in (0, 1, size - 1, size // 2):
                if 0 <= index < size:
                    ops.append(f"{index}={full if index % 2 else 0}")
            cases.append(f"storage {bits} {size} {' '.join(ops)}")
    # bits = 0: storage degenerado, sempre zero.
    cases.append("storage 0 4096 0=0 4095=0")
    # Todos os valores no máximo: expõe padding mal zerado.
    cases.append("storage 5 4096 " + " ".join(f"{i}={31}" for i in range(0, 4096, 331)))
    return cases


def container_edge_cases() -> list[str]:
    """Casos de fronteira da paleta: 1, 16, 17, 256, 257 e 4096 valores."""
    cases: list[str] = []
    # Um valor só: paleta de valor único.
    cases.append("container 0,0,0=7 15,15,15=7 3,4,5=7")
    # Exatamente no limite linear (16) e um além (17).
    for count in (16, 17):
        ops = []
        for token in range(count):
            ops.append(f"{token & 15},{(token >> 4) & 15},{(token >> 8) & 15}={token}")
        cases.append("container " + " ".join(ops))
    # Limite do hashmap (256) e um além (257) — este último já usa paleta global.
    for count in (256, 257):
        ops = []
        for token in range(count):
            ops.append(f"{token & 15},{(token >> 4) & 15},{(token >> 8) & 15}={token}")
        cases.append("container " + " ".join(ops))
    # Paleta que não encolhe: 20 valores, depois 19 deles voltam para 0.
    ops = [f"{t & 15},{(t >> 4) & 15},0={t}" for t in range(1, 21)]
    ops += [f"{t & 15},{(t >> 4) & 15},0=0" for t in range(1, 20)]
    cases.append("container " + " ".join(ops))
    # Seção cheia com a mesma paleta em todos os 4096: índice de borda.
    cases.append("container " + " ".join(f"{x},{y},{z}={(x + y * 16 + z * 256) % 3}"
                                         for x in range(16) for y in range(16) for z in range(16)))
    return cases


def random_storage(rng: XorShift64Star) -> str:
    bits = 1 + rng.next() % 32
    size = (1, 16, 64, 256)[rng.next() % 4]
    count = 1 + rng.next() % 40
    ops = []
    for _ in range(count):
        index = rng.next() % size
        # `set` do original valida o valor como int: 32 bits fica limitado a 2^31-1.
        value = rng.next() % (1 << min(bits, 31))
        ops.append(f"{index}={value}")
    return f"storage {bits} {size} " + " ".join(ops)


def random_container(rng: XorShift64Star) -> str:
    """Sorteia quantos valores distintos e quantas escritas."""
    distinct = (1, 2, 3, 8, 16, 17, 40, 100, 256, 257, 1000, 4096)[rng.next() % 12]
    writes = 1 + rng.next() % 200
    ops = []
    for _ in range(writes):
        x = rng.next() % 16
        y = rng.next() % 16
        z = rng.next() % 16
        ops.append(f"{x},{y},{z}={rng.next() % distinct}")
    return "container " + " ".join(ops)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/palette-0001.txt")
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--seed", type=int, default=20261028)
    args = parser.parse_args()

    rng = XorShift64Star(args.seed)
    lines = [
        "# casos do harness de paleta — entrada nossa, pode ser versionada",
        f"# gerado por tools/parity/gen_palette_cases.py seed={args.seed} count={args.count}",
        "# storage <bits> <size> <índice>=<valor> ... | container <x>,<y>,<z>=<token> ...",
    ]
    lines.extend(storage_edge_cases())
    lines.extend(container_edge_cases())
    for i in range(args.count):
        lines.append(random_storage(rng) if i % 2 == 0 else random_container(rng))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    total = len(lines) - 3
    print(f"{args.out}: {total} casos "
          f"({len(storage_edge_cases()) + len(container_edge_cases())} de fronteira + {args.count})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
