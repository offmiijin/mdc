#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gera casos do harness de atlas (degrau M2).

Um atlas por linha. O `blocks` é o que o terreno usa; os outros entram quando o
M3 precisar deles (item, partícula).

Uso:
    python3 tools/parity/gen_atlas_cases.py --out tests/parity/atlas-0001.txt
"""

from __future__ import annotations

import argparse
import os

ATLASES = ["minecraft:blocks"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default="tests/parity/atlas-0001.txt")
    args = parser.parse_args()

    lines = [
        "# casos do harness de atlas — entrada nossa, pode ser versionada",
        "# gerado por tools/parity/gen_atlas_cases.py",
        "# <id do atlas>",
    ]
    lines.extend(ATLASES)

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print(f"{args.out}: {len(ATLASES)} caso(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
