#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos de teste para o harness de `Vec3`.

Casos são **entrada nossa**, por isso podem ser versionados: são só números. A
saída do harness (o que o jogo responde) é que nunca entra no repositório.

Metade dos casos vem de padrões de bits aleatórios, não de decimais "bonitos":
o que quebra paridade de `Vec3` é 1 ULP em `normalize`/`lerp`/`dot`, e isso só
aparece com mantissa arbitrária.

Expoentes ficam limitados a ±500 (|v| ≲ 1e150) para que nenhuma operação estoure
para infinito — `inf - inf` produz NaN, e payload de NaN não é comparável.

Uso:
    python3 tools/parity/gen_vec3_cases.py --out tests/parity/vec3-0001.txt
"""

from __future__ import annotations

import argparse
import os

from prng import XorShift64Star, double_from_bits

MIN_SUBNORMAL = 5e-324


def edge_cases() -> list[list[float]]:
    """Casos que a aleatoriedade dificilmente acerta."""
    cases: list[list[float]] = []

    zero = [0.0, 0.0, 0.0]
    # Vetor nulo: normalizar não pode virar NaN nem (1,0,0).
    cases.append(zero + zero + [0.0])
    # Zero negativo em todas as componentes.
    cases.append([-0.0, -0.0, -0.0] + zero + [0.5])
    # Limiar de `normalize`: exatamente 1e-4, acima e abaixo.
    cases.append([1.0e-4, 0.0, 0.0] + zero + [0.0])
    cases.append([9.999999999999999e-5, 0.0, 0.0] + zero + [0.0])
    cases.append([1.0000000000000002e-4, 0.0, 0.0] + zero + [0.0])
    # Limiar real de `normalize`: `(double)1e-5f` = 0x3EE4F8B580000000, com os
    # dois vizinhos em bits. 1e-5 em double erra 147 dos 2.020 casos.
    cases.append([1.0e-5, 0.0, 0.0] + zero + [0.0])
    threshold = 9.999999747378752e-06
    cases.append([threshold, 0.0, 0.0] + zero + [0.0])
    cases.append([9.99999974737875e-06, 0.0, 0.0] + zero + [0.0])
    cases.append([9.999999747378753e-06, 0.0, 0.0] + zero + [0.0])
    cases.append([6.0e-6, 8.0e-6, 0.0] + zero + [0.0])
    cases.append([1.0e-5, 1.0e-5, 1.0e-5] + zero + [0.0])
    cases.append([9.0e-6, 0.0, 0.0] + zero + [0.0])
    # Eixos unitários e múltiplos exatos de 1/8.
    cases.append([1.0, 0.0, 0.0] + [0.0, 1.0, 0.0] + [0.5])
    cases.append([0.125, 0.25, 0.5] + [-0.125, 0.25, -0.5] + [0.25])
    # Potências de dois (exatas) e o menor subnormal.
    cases.append([2048.0, 1024.0, 512.0] + [0.5, 0.25, 0.125] + [0.75])
    cases.append([MIN_SUBNORMAL, 0.0, MIN_SUBNORMAL] + [MIN_SUBNORMAL, 0.0, 0.0] + [1.0])
    # Magnitudes grandes (sem estourar): |v|² ≈ 1e300.
    big = 1.0e150
    cases.append([big, -big, big] + [big, big, -big] + [0.5])
    # Vetores iguais: distância zero e lerp constante.
    same = [3.0, -4.0, 12.0]
    cases.append(same + same + [0.5])
    # Alphas de fronteira, inclusive fora de [0,1].
    for alpha in (0.0, 1.0, -1.0, 1.0 + 2.220446049250313e-16):
        cases.append([1.5, -2.5, 0.125] + [-0.5, 3.25, -8.0] + [alpha])
    return cases


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/vec3-0001.txt")
    parser.add_argument("--count", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=20261027)
    args = parser.parse_args()

    rng = XorShift64Star(args.seed)
    lines: list[str] = []
    lines.append("# casos do harness de Vec3 — entrada nossa, pode ser versionada")
    lines.append(f"# gerado por tools/parity/gen_vec3_cases.py seed={args.seed} count={args.count}")
    lines.append("# ax ay az bx by bz  |  alpha")

    for case in edge_cases():
        lines.append(" ".join(repr(value) for value in case))

    for _ in range(args.count):
        # Expoente 1023 = |v| em [1,2); a faixa ±500 cobre 1e-150 a 1e150.
        a = [rng.double(1023 - 500, 1023 + 500) for _ in range(3)]
        b = [rng.double(1023 - 500, 1023 + 500) for _ in range(3)]
        # Metade dos casos com valores pequenos: é onde o limiar de normalize mora.
        if rng.next() % 2 == 0:
            a = [rng.double(1023 - 40, 1023 + 40) for _ in range(3)]
            b = [rng.double(1023 - 40, 1023 + 40) for _ in range(3)]
        alpha = rng.coord(-4.0, 4.0)
        lines.append(" ".join(repr(v) for v in a + b + [alpha]))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    print(f"{args.out}: {len(lines) - 3} casos ({len(edge_cases())} de fronteira + {args.count} aleatórios)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
