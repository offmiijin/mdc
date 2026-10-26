#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Monta o classpath do jogo a partir de uma instalação local.

Resolve dois layouts:

| launcher | client jar | version json |
|---|---|---|
| oficial | `versions/<v>/<v>.jar` | `versions/<v>/<v>.json` |
| MultiMC / UltimMC / Prism | `libraries/com/mojang/minecraft/<v>/minecraft-<v>-client.jar` | `meta/net.minecraft/<v>.json` |

O classpath é montado **na ordem do version json**, não por `find`: instalações
com mais de uma versão de uma biblioteca (ex.: DataFixerUpper 6.0.8 e 10.0.21)
quebram se a versão errada vier primeiro.

Saída: o classpath em uma linha (separado por `:`). Erros vão para stderr.

Uso: classpath.py <mc_dir> <versao>
"""

from __future__ import annotations

import json
import os
import platform
import sys

# Nomes de OS no version json ("os": {"name": ...}) diferem do platform.system().
OS_NAMES = {"Linux": "linux", "Darwin": "osx", "Windows": "windows"}


def current_os_and_arch() -> tuple[str, str]:
    system = platform.system()
    os_name = OS_NAMES.get(system, system.lower())
    machine = platform.machine().lower()
    arch = "arm64" if machine in ("arm64", "aarch64") else "x86"
    return os_name, arch


def allowed_on_this_platform(library: dict, os_name: str, arch: str) -> bool:
    """Avalia o bloco `rules` do version json (ausência = permitido em tudo)."""
    rules = library.get("rules")
    if not rules:
        return True
    result = False
    for rule in rules:
        target = rule.get("os", {})
        matches = True
        if "name" in target and target["name"] != os_name:
            matches = False
        if matches and "arch" in target and target["arch"] != arch:
            matches = False
        if matches:
            result = rule.get("action") == "allow"
    return result


def is_native(library: dict) -> bool:
    """`grupo:artefato:versao:natives-*` — não é classe Java, é binário de plataforma."""
    parts = library.get("name", "").split(":")
    return len(parts) >= 4 and parts[3].startswith("natives-")


def find_client_jar(mc_dir: str, version: str) -> str | None:
    candidates = (
        os.path.join(mc_dir, "versions", version, f"{version}.jar"),
        os.path.join(mc_dir, "libraries", "com", "mojang", "minecraft", version,
                     f"minecraft-{version}-client.jar"),
    )
    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate
    return None


def find_version_json(mc_dir: str, version: str) -> str | None:
    candidates = (
        os.path.join(mc_dir, "versions", version, f"{version}.json"),
        os.path.join(mc_dir, "meta", "net.minecraft", f"{version}.json"),
    )
    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate
    return None


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print("uso: classpath.py <mc_dir> <versao>", file=sys.stderr)
        return 2

    mc_dir, version = argv[1], argv[2]

    jar = find_client_jar(mc_dir, version)
    if jar is None:
        print(f"erro: client jar de {version} não encontrado em {mc_dir}\n"
              f"      procurei em:\n"
              f"        versions/{version}/{version}.jar\n"
              f"        libraries/com/mojang/minecraft/{version}/minecraft-{version}-client.jar",
              file=sys.stderr)
        return 1

    entries = [os.path.abspath(jar)]

    version_json = find_version_json(mc_dir, version)
    if version_json is None:
        print(f"aviso: version json de {version} não encontrado; "
              f"classpath só com o jar (vai falhar em runtime)", file=sys.stderr)
        print(":".join(entries))
        return 0

    with open(version_json, encoding="utf-8") as handle:
        metadata = json.load(handle)

    os_name, arch = current_os_and_arch()
    missing: list[str] = []
    skipped = 0
    for library in metadata.get("libraries", []):
        if is_native(library) or not allowed_on_this_platform(library, os_name, arch):
            skipped += 1
            continue
        artifact = library.get("downloads", {}).get("artifact")
        if not artifact or "path" not in artifact:
            continue
        path = os.path.join(mc_dir, "libraries", artifact["path"])
        if os.path.isfile(path):
            entries.append(os.path.abspath(path))
        else:
            missing.append(artifact["path"])

    print(f"classpath: {len(entries)} jar(s) para {os_name}/{arch}, "
          f"{skipped} biblioteca(s) de outra plataforma ignorada(s)", file=sys.stderr)

    if missing:
        print(f"erro: {len(missing)} biblioteca(s) ausente(s) em {mc_dir}/libraries:", file=sys.stderr)
        for path in missing[:10]:
            print(f"  {path}", file=sys.stderr)
        if len(missing) > 10:
            print(f"  … e mais {len(missing) - 10}", file=sys.stderr)
        print("      abra a versão uma vez no launcher para baixá-las", file=sys.stderr)
        return 1

    print(":".join(entries))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
