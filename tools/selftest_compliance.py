#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
"""Testes do gate de conformidade. Sem dependências, sem rede.

    python3 tools/selftest_compliance.py
"""

from __future__ import annotations

import contextlib
import importlib.util
import io
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SPEC = importlib.util.spec_from_file_location("check_compliance", os.path.join(HERE, "check_compliance.py"))
cc = importlib.util.module_from_spec(SPEC)
sys.modules["check_compliance"] = cc
SPEC.loader.exec_module(cc)

failures: list[str] = []


def expect(label: str, got, want) -> None:
    ok = got == want
    print(f"  {'ok  ' if ok else 'FAIL'}  {label}")
    if not ok:
        failures.append(f"{label}: esperado {want!r}, obtido {got!r}")


def rules(findings) -> list[str]:
    return sorted({f.rule for f in findings})


def test_paths() -> None:
    print("caminhos")
    expect("jar do jogo", rules(cc.check_path("assets/client.jar")), ["R002"])
    expect("bytecode", rules(cc.check_path("tmp/Minecraft.class")), ["R002"])
    expect("java", rules(cc.check_path("tools/Oracle.java")), ["R002"])
    expect("região do mundo", rules(cc.check_path("worlds/r.0.0.mca")), ["R002"])
    expect("diretório minecraft/", rules(cc.check_path("src/minecraft/world.cpp")), ["R001"])
    expect("cache do sandbox", rules(cc.check_path(".sandbox-cache/clones/x/source/A.java")), ["R001", "R002"])
    expect("decompilado", rules(cc.check_path("decompiled/net/minecraft/A.java")), ["R001", "R002"])
    expect("arquivo compactado", rules(cc.check_path("deps/lib.zip")), ["R009"])
    expect("fonte próprio", rules(cc.check_path("src/render/chunk_mesher.cpp")), [])
    expect("asset próprio", rules(cc.check_path("assets/textures/grass.png")), [])


def test_content() -> None:
    print("conteúdo")
    expect("referência ao jogo", rules(cc.check_content("src/a.cpp", b"#include \"net/minecraft/A.h\"")), ["R004"])
    expect("mapping header", rules(cc.check_content(
        "src/a.cpp", b'# These mappings are provided "as-is"')), ["R005"])
    expect("url do jar", rules(cc.check_content(
        "src/a.cpp", b"https://piston-data.mojang.com/v1/objects/x/client.jar")), ["R006"])
    expect("markdown é isento", rules(cc.check_content("docs/LEGAL.md", b"net/minecraft/")), [])
    expect("binário é isento", rules(cc.check_content("src/a.cpp", b"\x00\x01net/minecraft/")), [])
    expect("sem assinatura", rules(cc.check_content("src/a.cpp", b"int main() { return 0; }")), [])


def test_source_header() -> None:
    print("R008 — SPDX presente")
    expect("c++ sem SPDX", rules(cc.check_source_header("src/a.cpp", b"int main();\n")), ["R008"])
    expect("c++ com SPDX", rules(cc.check_source_header(
        "src/a.cpp", b"// SPDX-License-Identifier: AGPL-3.0-only\nint main();\n")), [])
    expect("header .hpp também", rules(cc.check_source_header("src/a.hpp", b"#pragma once\n")), ["R008"])
    expect("python", rules(cc.check_source_header("tools/x.py", b"print(1)\n")), ["R008"])
    expect("shell hook", rules(cc.check_source_header(".githooks/pre-commit", b"#!/bin/sh\nexit 0\n")), ["R008"])
    expect("não-código é isento", rules(cc.check_source_header("src/CMakeLists.txt", b"x")), [])


def test_spdx_value() -> None:
    print("R010 — SPDX casa com o projeto")
    ok = b"// SPDX-License-Identifier: AGPL-3.0-only\n"
    bad = b"// SPDX-License-Identifier: MIT\n"
    expect("AGPL ok", rules(cc.check_spdx_value("src/a.cpp", ok)), [])
    expect("MIT diverge", rules(cc.check_spdx_value("src/a.cpp", bad)), ["R010"])
    expect("GPL-3.0 diverge", rules(cc.check_spdx_value(
        "src/a.cpp", b"// SPDX-License-Identifier: GPL-3.0-only\n")), ["R010"])
    expect("sem SPDX não é R010", rules(cc.check_spdx_value("src/a.cpp", b"int x;\n")), [])
    expect("yml também é conferido", rules(cc.check_spdx_value(
        ".github/workflows/ci.yml", b"# SPDX-License-Identifier: MIT\n")), ["R010"])


def test_license_files() -> None:
    print("R010 — LICENSE e NOTICE")
    with tempfile.TemporaryDirectory() as tmp:
        expect("sem LICENSE nem NOTICE", rules(cc.check_license_files(tmp)), ["R010"])
        with open(os.path.join(tmp, "LICENSE"), "w", encoding="utf-8") as f:
            f.write("MIT License\n")
        with open(os.path.join(tmp, "NOTICE"), "w", encoding="utf-8") as f:
            f.write("x\n")
        expect("LICENSE não-AGPL", rules(cc.check_license_files(tmp)), ["R010"])
        with open(os.path.join(tmp, "LICENSE"), "w", encoding="utf-8") as f:
            f.write("GNU AFFERO GENERAL PUBLIC LICENSE\nVersion 3\n")
        expect("LICENSE AGPL + NOTICE", rules(cc.check_license_files(tmp)), [])


def test_size_and_allowlist() -> None:
    print("tamanho e allowlist")
    expect("arquivo grande", rules(cc.check_size("data/pack.bin", 9 * 1024 * 1024)), ["R003"])
    expect("arquivo normal", rules(cc.check_size("src/a.cpp", 1024)), [])
    allow = [{"path": "tools/*.py", "rule": "R006", "reason": "doc"}]
    expect("allowlist por glob", cc.allowed(allow, "tools/fetch.py", "R006"), True)
    expect("allowlist não cobre outra regra", cc.allowed(allow, "tools/fetch.py", "R002"), False)


def test_disclaimer() -> None:
    print("R007 — aviso obrigatório")
    with tempfile.TemporaryDirectory() as tmp:
        expect("sem README", rules(cc.check_disclaimer(tmp)), ["R007"])
        with open(os.path.join(tmp, "README.md"), "w", encoding="utf-8") as f:
            f.write("# proj\n\nnada.\n")
        expect("README sem aviso", rules(cc.check_disclaimer(tmp)), ["R007"])
        with open(os.path.join(tmp, "README.md"), "w", encoding="utf-8") as f:
            f.write(f"# proj\n\n> {cc.DISCLAIMER}\n")
        expect("README com aviso", rules(cc.check_disclaimer(tmp)), [])


def run_main(args: list[str]) -> int:
    """Chama o gate silenciando a saída — inclusive os casos que devem falhar."""
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        return cc.main(args)


def test_end_to_end() -> None:
    print("integração (main)")
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "src")
        os.makedirs(src)
        with open(os.path.join(tmp, "README.md"), "w", encoding="utf-8") as f:
            f.write(f"# proj\n\n{cc.DISCLAIMER}\n")
        with open(os.path.join(src, "ok.cpp"), "w", encoding="utf-8") as f:
            f.write("// SPDX-License-Identifier: AGPL-3.0-only\nint main() { return 0; }\n")
        with open(os.path.join(tmp, "LICENSE"), "w", encoding="utf-8") as f:
            f.write("GNU AFFERO GENERAL PUBLIC LICENSE\n")
        with open(os.path.join(tmp, "NOTICE"), "w", encoding="utf-8") as f:
            f.write("x\n")
        expect("árvore limpa", run_main(["--root", tmp, "--all", "--quiet"]), 0)

        with open(os.path.join(src, "bad.cpp"), "w", encoding="utf-8") as f:
            f.write('// sem SPDX\n#include "net/minecraft/A.h"\n')
        expect("árvore com violação", run_main(["--root", tmp, "--all", "--quiet"]), 1)


def test_untracked() -> None:
    print("--all inclui arquivo novo não rastreado")
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run(["git", "init", "-q"], cwd=tmp, check=True)
        with open(os.path.join(tmp, "README.md"), "w", encoding="utf-8") as f:
            f.write(f"# proj\n\n{cc.DISCLAIMER}\n")
        with open(os.path.join(tmp, "LICENSE"), "w", encoding="utf-8") as f:
            f.write("GNU AFFERO GENERAL PUBLIC LICENSE\n")
        with open(os.path.join(tmp, "NOTICE"), "w", encoding="utf-8") as f:
            f.write("x\n")
        # nada commitado, nada staged: só existe no disco
        with open(os.path.join(tmp, "novo.cpp"), "w", encoding="utf-8") as f:
            f.write("// SPDX-License-Identifier: AGPL-3.0-only\nint main(){}\n")
        expect("novo arquivo limpo", run_main(["--root", tmp, "--all", "--quiet"]), 0)
        with open(os.path.join(tmp, "penetrante.jar"), "wb") as f:
            f.write(b"\x00\x01")
        expect("novo arquivo proibido", run_main(["--root", tmp, "--all", "--quiet"]), 1)


def test_oracle() -> None:
    print("R011 — oráculo Java")
    oracle = "tools/oracle/java/dev/mdc/oracle/MobAiProbe.java"
    probe = b"package dev.mdc.oracle;\nimport net.minecraft.world.entity.Mob;\n"
    expect("java fora do oracle bloqueado", rules(cc.check_path("tools/Probe.java")), ["R002"])
    expect("java no oracle liberado", rules(cc.check_path(oracle)), [])
    expect("probe com pacote próprio", rules(cc.check_oracle_namespace(oracle, probe)), [])
    expect("R004 não dispara no oracle", rules(cc.check_content(oracle, probe)), [])
    expect("R004 dispara fora do oracle", rules(cc.check_content("src/a.cpp", b"net/minecraft/A")), ["R004"])
    expect("código do jogo disfarçado", rules(cc.check_oracle_namespace(
        oracle, b"package net.minecraft.world.entity;\npublic class Mob {}\n")), ["R011"])
    expect("probe sem pacote próprio", rules(cc.check_oracle_namespace(
        oracle, b"public class Probe {}\n")), ["R011"])
    expect("exige SPDX também no oracle", rules(cc.check_source_header(
        oracle, b"package dev.mdc.oracle;\n")), ["R008"])


def test_mods() -> None:
    print("R012 — licença por mod")
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "mods", "com-licenca"))
        os.makedirs(os.path.join(tmp, "mods", "sem-licenca"))
        with open(os.path.join(tmp, "mods", "com-licenca", "LICENSE"), "w") as f:
            f.write("SPDX-License-Identifier: MIT\n")
        with open(os.path.join(tmp, "mods", "sem-licenca", "main.wasm"), "wb") as f:
            f.write(b"\x00asm")
        paths = ["mods/com-licenca/LICENSE", "mods/sem-licenca/main.wasm", "mods/README.md"]
        found = cc.check_mod_licenses(tmp, paths)
        expect("mod sem LICENSE é barrado", [f.path for f in found], ["mods/sem-licenca/"])
        expect("somente R012", rules(found), ["R012"])
        expect("mods/ vazio não gera achado", cc.check_mod_licenses(tmp, []), [])


def test_fixtures() -> None:
    print("R013 — fixture/transcrição")
    expect("jsonl bloqueado", rules(cc.check_path("tools/oracle/fixtures/run.jsonl")), ["R013"])
    expect("jsonl em .local também", rules(cc.check_path(".local/fixtures/run.jsonl")), ["R001", "R013"])
    expect("json permitido", rules(cc.check_path("docs/exemplo.json")), [])


def test_repo_guardrails() -> None:
    print("guarda-corpo do próprio repo")
    gitignore = os.path.join(os.path.dirname(HERE), ".gitignore")
    if not os.path.exists(gitignore):
        print("  (fora do repo, ignorado)")
        return
    text = open(gitignore, encoding="utf-8").read()
    for entry in (".local/", ".sandbox-cache/", "*.jsonl"):
        expect(f".gitignore contém {entry}", entry in text, True)


def main() -> int:
    print("selftest do gate de conformidade\n")
    test_paths()
    test_content()
    test_source_header()
    test_spdx_value()
    test_oracle()
    test_mods()
    test_fixtures()
    test_repo_guardrails()
    test_license_files()
    test_size_and_allowlist()
    test_disclaimer()
    test_untracked()
    test_end_to_end()

    print()
    if failures:
        print(f"✖ {len(failures)} teste(s) falharam:")
        for failure in failures:
            print(f"   - {failure}")
        return 1
    print("✔ todos os testes passaram")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
