#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Gate de conformidade do repositório.

Impede que material da Mojang (código, assets, artefatos derivados) entre no
repositório, e que a documentação perca os avisos exigidos pelas
Minecraft Usage Guidelines.

Somente biblioteca padrão do Python. Sem rede. Rápido o suficiente para rodar
em pre-commit.

Uso:
    check_compliance.py --all        # todos os arquivos versionados (CI)
    check_compliance.py --staged     # apenas o que está no index (pre-commit)
    check_compliance.py --history    # todo o histórico + blobs grandes (CI)
    check_compliance.py --path A B   # caminhos explícitos (testes/debug)

Saída: uma linha por violação, código + caminho + motivo. Exit 1 se houver.
"""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass

# ---------------------------------------------------------------- configuração

# R001 — segmentos de caminho que nunca podem ser versionados.
# Nomes que, num projeto C++ limpo, só aparecem como artefato da Mojang.
PRIVATE_SEGMENTS = {
    ".local",
    ".sandbox-cache",
    "decompiled",
    "decomp",
    "minecraft",
    "versions",
    "libraries",
    "crash-reports",
}

# R002 — extensões que são, por construção, artefato do jogo Java.
GAME_EXTENSIONS = {
    ".jar", ".class", ".java", ".jfr", ".hprof",
    ".mca", ".mcr", ".nbt", ".mcmeta", ".mcassetsroot",
    ".fsh", ".vsh",
}

# R011 — único lugar do repositório onde .java é permitido: o oráculo do harness.
# O probe é código NOSSO que apenas chama a API do jar do usuário; nunca contém
# código da Mojang. Por isso exige pacote próprio e proíbe pacotes do jogo.
ORACLE_JAVA_ROOT = "tools/oracle/java/"
ORACLE_PACKAGE_PREFIXES = ("dev.mdc.",)
GAME_PACKAGE_PREFIXES = ("package net.minecraft", "package com.mojang")

# R012 — cada mod declara a própria licença.
MODS_ROOT = "mods/"
MOD_LICENSE_NAMES = ("LICENSE", "LICENSE.md", "LICENSE.txt", "COPYING")

# R013 — transcrição de harness é fixture: material derivado, nunca versionado.
FIXTURE_EXTENSIONS = {".jsonl"}

# R003 — nenhum arquivo versionado acima disso sem justificativa.
LARGE_FILE_LIMIT = 8 * 1024 * 1024

# R004/R005/R006 — assinaturas de material derivado, varridas em arquivos de texto.
SIGNATURES = [
    ("R004", "referência a classe do jogo (net/minecraft ou net.minecraft)"),
    ("R005", "cabeçalho de obfuscation map da Microsoft"),
    ("R006", "URL de distribuição do jar do jogo"),
]
SIGNATURE_MARKERS = {
    "R004": (b"net/minecraft/", b"net.minecraft."),
    "R005": (b'These mappings are provided "as-is"', b"(c) 2020 Microsoft Corporation"),
    "R006": (b"piston-data.mojang.com", b"piston-meta.mojang.com"),
}

# R007 — aviso exigido pelas Usage Guidelines (comparação normalizada).
DISCLAIMER = (
    "NOT AN OFFICIAL MINECRAFT PRODUCT. NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT."
)
DISCLAIMER_FILE = "README.md"

# R008/R010 — todo código declara licença, e ela casa com o LICENSE do projeto.
SOURCE_EXTENSIONS = {
    ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl",
    ".py", ".sh", ".yml", ".yaml", ".java",
}
# Hooks de git não têm extensão.
HEADER_GLOBS = (".githooks/*",)
SPDX_MARKER = "SPDX-License-Identifier:"
PROJECT_SPDX = "AGPL-3.0-only"
LICENSE_FILE = "LICENSE"
NOTICE_FILE = "NOTICE"
LICENSE_MARKER = "GNU AFFERO GENERAL PUBLIC LICENSE"

# R009 — formatos que podem carregar o jogo/assets dentro.
ARCHIVE_EXTENSIONS = {".zip", ".7z", ".tar", ".gz", ".tgz", ".xz", ".zst"}

# Arquivos de texto que legitimamente citam Mojang/Minecraft, ou que são
# textos legais e precisam poder citar URLs e nomes.
CONTENT_SKIP_GLOBS = (
    "docs/*", "*.md", ".compliance/*", "LICENSE*", "NOTICE*", "LICENSES/*",
)

ALLOWLIST_PATH = ".compliance/allowlist.json"


@dataclass(frozen=True)
class Finding:
    rule: str
    path: str
    message: str

    def render(self) -> str:
        return f"{self.rule}\t{self.path}\t{self.message}"


# --------------------------------------------------------------------- utilidades

def load_allowlist(root: str) -> list[dict]:
    path = os.path.join(root, ALLOWLIST_PATH)
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8") as handle:
        data = json.load(handle)
    return data.get("entries", [])


def allowed(allowlist: list[dict], path: str, rule: str) -> bool:
    for entry in allowlist:
        if entry.get("rule") not in (rule, "*"):
            continue
        if fnmatch.fnmatch(path, entry.get("path", "")):
            return True
    return False


def normalize(text: str) -> str:
    return " ".join(text.upper().split())


def is_binary(blob: bytes) -> bool:
    return b"\x00" in blob[:8192]


def git(root: str, *args: str) -> str:
    # stderr silenciado: fora de um repositório o fallback para os.walk é normal.
    return subprocess.check_output(["git", *args], cwd=root, text=True, stderr=subprocess.DEVNULL)


# --------------------------------------------------------------------- as regras

def is_oracle_java(path: str) -> bool:
    """O único lugar onde .java é permitido (ver R011)."""
    return path.startswith(ORACLE_JAVA_ROOT)


def check_path(path: str) -> list[Finding]:
    """Regras que dependem só do caminho."""
    findings: list[Finding] = []
    parts = path.split("/")
    lowered = [part.lower() for part in parts]

    for segment in lowered[:-1]:
        if segment in PRIVATE_SEGMENTS:
            findings.append(Finding(
                "R001", path,
                f"diretório '{segment}/' é material derivado do jogo e não pode ser versionado"))
            break

    name = lowered[-1]
    base = name.lstrip(".")
    if name in PRIVATE_SEGMENTS or base in PRIVATE_SEGMENTS - {"minecraft"}:
        findings.append(Finding("R001", path, f"nome de arquivo reservado '{parts[-1]}'"))

    extension = os.path.splitext(name)[1]
    if extension in GAME_EXTENSIONS and not (extension == ".java" and is_oracle_java(path)):
        hint = "" if extension != ".java" else " (o único .java permitido é o do oráculo, em tools/oracle/java/)"
        findings.append(Finding(
            "R002", path,
            f"'{extension}' é artefato do jogo (jar/bytecode/dados) — nunca entra no repositório{hint}"))

    if extension in FIXTURE_EXTENSIONS:
        findings.append(Finding(
            "R013", path,
            f"'{extension}' é transcrição/fixture de harness: material derivado, fica em .local/fixtures/"))

    if extension in ARCHIVE_EXTENSIONS:
        findings.append(Finding(
            "R009", path,
            f"arquivo compactado ('{extension}') pode conter o jogo — descompacte e versione só a origem"))

    return findings


def check_content(path: str, blob: bytes) -> list[Finding]:
    """Regras que dependem do conteúdo (só arquivos de texto pequenos)."""
    if any(fnmatch.fnmatch(path, glob) for glob in CONTENT_SKIP_GLOBS):
        return []
    if len(blob) > 2 * 1024 * 1024 or is_binary(blob):
        return []

    findings: list[Finding] = []
    seen: set[str] = set()
    for rule, _label in SIGNATURES:
        if rule in seen:
            continue
        # O oráculo referencia classes do jogo de propósito; o que o contém é R011.
        if rule == "R004" and is_oracle_java(path):
            continue
        for marker in SIGNATURE_MARKERS[rule]:
            if marker in blob:
                label = dict(SIGNATURES)[rule]
                findings.append(Finding(rule, path, label))
                seen.add(rule)
                break
    return findings


def header_checked(path: str) -> bool:
    """R008/R010 valem para código e scripts."""
    if os.path.splitext(path)[1] in SOURCE_EXTENSIONS:
        return True
    return any(fnmatch.fnmatch(path, glob) for glob in HEADER_GLOBS)


def check_source_header(path: str, blob: bytes) -> list[Finding]:
    """R008: todo código declara licença/origem de forma legível por máquina."""
    if not header_checked(path):
        return []
    head = blob[:2048].decode("utf-8", "replace")
    if SPDX_MARKER in head:
        return []
    return [Finding("R008", path, f"sem '{SPDX_MARKER} …' nas primeiras linhas")]


def check_spdx_value(path: str, blob: bytes) -> list[Finding]:
    """R010: o SPDX declarado tem de ser o do projeto (impede drift de licença)."""
    if not header_checked(path):
        return []
    head = blob[:2048].decode("utf-8", "replace")
    match = re.search(re.escape(SPDX_MARKER) + r"\s*(\S+)", head)
    if not match:
        return []  # ausência é R008, não R010
    declared = match.group(1).strip()
    if declared == PROJECT_SPDX:
        return []
    return [Finding("R010", path, f"SPDX '{declared}' diverge do projeto ('{PROJECT_SPDX}')")]


def check_oracle_namespace(path: str, blob: bytes) -> list[Finding]:
    """R011: .java só no oráculo, e lá só com pacote do projeto.

    A exceção que libera `.java` (R002) existe para o probe do harness, não para
    carregar código do jogo. Duas travas: pacote próprio obrigatório, pacote da
    Mojang proibido.
    """
    if not path.endswith(".java") or not is_oracle_java(path):
        return []  # fora do oráculo, R002 já reportou
    text = blob[:4096].decode("utf-8", "replace")
    for forbidden in GAME_PACKAGE_PREFIXES:
        if forbidden in text:
            return [Finding("R011", path,
                            "declara pacote do jogo — código da Mojang disfarçado de oráculo")]
    if not any(f"package {prefix}" in text for prefix in ORACLE_PACKAGE_PREFIXES):
        return [Finding("R011", path, "probe sem pacote próprio ('package dev.mdc.…')")]
    return []


def check_mod_licenses(root: str, paths: list[str]) -> list[Finding]:
    """R012: todo mod sob `mods/` declara a própria licença.

    O mod é um trabalho independente. Exigir o LICENSE é o que mantém a
    separação auditável — e o que impede um mod de virar depósito de asset do
    jogo sem ninguém perceber.
    """
    names: set[str] = set()
    for path in paths:
        if not path.startswith(MODS_ROOT):
            continue
        rest = path[len(MODS_ROOT):]
        if "/" not in rest:
            continue  # arquivo solto em mods/ (README, etc.)
        names.add(rest.split("/")[0])
    findings: list[Finding] = []
    for name in sorted(names):
        base = os.path.join(root, "mods", name)
        if not os.path.isdir(base):
            continue
        if not any(os.path.exists(os.path.join(base, option)) for option in MOD_LICENSE_NAMES):
            findings.append(Finding(
                "R012", f"mods/{name}/",
                "mod sem LICENSE — declare a licença do mod (e nenhum asset do jogo)"))
    return findings


def check_license_files(root: str) -> list[Finding]:
    """R010: o LICENSE do projeto tem de existir e ser AGPL."""
    findings: list[Finding] = []
    license_path = os.path.join(root, LICENSE_FILE)
    if not os.path.exists(license_path):
        findings.append(Finding("R010", LICENSE_FILE, "arquivo de licença ausente"))
    else:
        text = open(license_path, encoding="utf-8", errors="replace").read()
        if LICENSE_MARKER not in text.upper():
            findings.append(Finding(
                "R010", LICENSE_FILE,
                f"não contém '{LICENSE_MARKER}' — o projeto declara {PROJECT_SPDX}"))
    if not os.path.exists(os.path.join(root, NOTICE_FILE)):
        findings.append(Finding("R010", NOTICE_FILE, "NOTICE ausente (eleição de versão + exceção de mods)"))
    return findings


def check_disclaimer(root: str) -> list[Finding]:
    path = os.path.join(root, DISCLAIMER_FILE)
    if not os.path.exists(path):
        return [Finding("R007", DISCLAIMER_FILE, "arquivo ausente — o aviso é obrigatório")]
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = normalize(handle.read())
    if normalize(DISCLAIMER) in text:
        return []
    return [Finding("R007", DISCLAIMER_FILE, "falta o aviso 'NOT AN OFFICIAL MINECRAFT PRODUCT…'")]


def check_size(path: str, size: int) -> list[Finding]:
    if size <= LARGE_FILE_LIMIT:
        return []
    limit = LARGE_FILE_LIMIT // (1024 * 1024)
    return [Finding("R003", path, f"arquivo com {size / 1048576:.1f} MB (> {limit} MB) — binário grande esconde asset do jogo")]


# --------------------------------------------------------------------- coletores

def is_git_repo(root: str) -> bool:
    try:
        return git(root, "rev-parse", "--is-inside-work-tree").strip() == "true"
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def files_all(root: str) -> list[str]:
    """Tudo que *seria* commitado: rastreados + não rastreados não ignorados.

    Fora de um repositório git, varre a árvore. Incluir os não rastreados é
    essencial: sem isso, um arquivo novo (ainda não `git add`) passaria batido
    num `--all` local.
    """
    if is_git_repo(root):
        tracked = git(root, "ls-files", "-z").split("\0")
        untracked = git(root, "ls-files", "--others", "--exclude-standard", "-z").split("\0")
        return sorted({p for p in tracked + untracked if p})
    collected: list[str] = []
    for base, dirs, names in os.walk(root):
        dirs[:] = [d for d in dirs if d != ".git"]
        for name in names:
            rel = os.path.relpath(os.path.join(base, name), root).replace(os.sep, "/")
            collected.append(rel)
    return sorted(collected)


def files_staged(root: str) -> list[str]:
    out = git(root, "diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z")
    return [p for p in out.split("\0") if p]


def read_root_file(root: str, rel: str) -> bytes | None:
    path = os.path.join(root, rel)
    if not os.path.exists(path) or os.path.islink(path):
        return None
    with open(path, "rb") as handle:
        return handle.read()


def run_file_rules(root: str, paths: list[str], allow: list[dict]) -> list[Finding]:
    findings: list[Finding] = []
    for rel in paths:
        chunk = check_path(rel)
        blob = read_root_file(root, rel)
        if blob is not None:
            chunk += check_content(rel, blob)
            chunk += check_source_header(rel, blob)
            chunk += check_spdx_value(rel, blob)
            chunk += check_oracle_namespace(rel, blob)
            chunk += check_size(rel, len(blob))
        for item in chunk:
            if not allowed(allow, item.path, item.rule):
                findings.append(item)
    return findings


def history_findings(root: str, allow: list[dict]) -> list[Finding]:
    """Varre todo o histórico: caminhos já commitados e blobs grandes."""
    findings: list[Finding] = []
    seen_paths: set[str] = set()
    raw = git(root, "log", "--format=", "--name-only", "-z", "--all")
    for path in raw.split("\0"):
        path = path.lstrip("\n")
        if not path or path in seen_paths:
            continue
        seen_paths.add(path)
        for item in check_path(path):
            if not allowed(allow, item.path, item.rule):
                findings.append(Finding(item.rule, f"{item.path} (histórico)", item.message))

    objects = git(root, "rev-list", "--objects", "--all")
    if objects.strip():
        batch = subprocess.run(
            ["git", "cat-file", "--batch-check=%(objecttype) %(objectname) %(objectsize) %(rest)"],
            cwd=root, input=objects, text=True, capture_output=True, check=True,
        )
        for line in batch.stdout.splitlines():
            fields = line.split(" ", 3)
            if len(fields) < 4 or fields[0] != "blob":
                continue
            _, _, size, path = fields
            if not path:
                continue
            for item in check_size(path, int(size)):
                if not allowed(allow, item.path, item.rule):
                    findings.append(Finding(item.rule, f"{path} (histórico)", item.message))
    return findings


# --------------------------------------------------------------------- principal

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", default=".", help="raiz do repositório (padrão: .)")
    parser.add_argument("--all", action="store_true", help="arquivos versionados/rastreados")
    parser.add_argument("--staged", action="store_true", help="apenas o index (pre-commit)")
    parser.add_argument("--history", action="store_true", help="todo o histórico do git")
    parser.add_argument("--path", nargs="+", default=None, help="caminhos explícitos")
    parser.add_argument("--quiet", action="store_true", help="só o resumo")
    args = parser.parse_args(argv)

    root = os.path.abspath(args.root)
    allow = load_allowlist(root)

    if args.staged:
        paths = files_staged(root)
    elif args.path:
        paths = args.path
    elif args.history:
        paths = files_all(root)
    else:
        paths = files_all(root)

    findings = run_file_rules(root, paths, allow)
    # No pre-commit o README normalmente não está no index; mas se estiver, o
    # aviso obrigatório tem de ser verificado antes do commit existir.
    if not args.staged or DISCLAIMER_FILE in paths:
        findings += check_disclaimer(root)
    if not args.staged or LICENSE_FILE in paths:
        findings += check_license_files(root)
    if not args.staged or any(p.startswith(MODS_ROOT) for p in paths):
        findings += check_mod_licenses(root, paths)
    if args.history:
        findings += history_findings(root, allow)

    if not args.quiet:
        for item in findings:
            print(item.render(), file=sys.stderr)

    checked = len(paths)
    if findings:
        print(f"\n✖ {len(findings)} violação(ões) em {checked} arquivo(s) verificado(s).", file=sys.stderr)
        print("  Política: docs/LEGAL.md · allowlist justificada: " + ALLOWLIST_PATH, file=sys.stderr)
        return 1
    print(f"✔ conformidade ok — {checked} arquivo(s) verificado(s), 0 violação(ões).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
