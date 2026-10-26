#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Harness de paridade de AABB — roda o probe Java contra o SEU client.jar e
# compara com o nosso binário, bit a bit.
#
# Uso:
#   tools/parity/run_aabb.sh [client.jar] [casos.txt]
#
#   client.jar  padrão: $MDC_MC_JAR, senão ~/.minecraft/versions/26.3/26.3.jar
#   casos       padrão: tests/parity/aabb-0001.txt
#
# Saída do jogo (fixture) vai para .local/fixtures/ — nunca para o repositório.
set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"

mc_dir="${MDC_MC_DIR:-$HOME/.minecraft}"
cases="${2:-tests/parity/aabb-0001.txt}"

# O classpath vem do version json (ordem exata das bibliotecas), não de `find`:
# instalações com duas versões de DataFixerUpper quebram se a errada vier antes.
# Suporta launcher oficial e MultiMC/UltimMC/Prism (ver tools/parity/classpath.py).
if [ $# -ge 1 ] && [ -f "$1" ]; then
    jar="$1"
    CP=$(python3 tools/parity/classpath.py "$mc_dir" 26.3) || exit 2
    CP="$jar:$CP"
else
    CP=$(python3 tools/parity/classpath.py "$mc_dir" 26.3) || exit 2
    jar=$(printf '%s' "$CP" | cut -d: -f1)
fi

[ -f "$jar" ] || { echo "erro: client.jar não encontrado ($jar)" >&2; exit 2; }
[ -f "$cases" ] || { echo "erro: casos não encontrados em $cases" >&2
                     echo "      gere com: python3 tools/parity/gen_aabb_cases.py" >&2; exit 2; }

# Proveniência: o jar tem de ser o 26.3 que o lock registra.
expected=$(grep -o '"sha1": "[0-9a-f]\{40\}"' refs/minecraft.lock | head -1 | cut -d'"' -f4)
if [ -n "$expected" ]; then
    actual=$(sha1sum "$jar" | cut -d' ' -f1)
    if [ "$actual" != "$expected" ]; then
        echo "erro: sha1 do client.jar não confere com refs/minecraft.lock" >&2
        echo "      esperado $expected" >&2
        echo "      obtido   $actual" >&2
        exit 2
    fi
fi

command -v javac >/dev/null 2>&1 || { echo "erro: javac não encontrado (precisa de JDK 25)" >&2; exit 2; }
major=$(javac -version 2>&1 | sed -E 's/^javac ([0-9]+).*/\1/')
if [ "$major" -lt 25 ]; then
    echo "erro: javac $major encontrado; o 26.3 é class file v69 e exige JDK 25" >&2
    echo "      (javac 21 falha com: wrong version 69.0, should be 65.0)" >&2
    exit 2
fi
# O runtime também precisa ser 25: o probe carrega classes v69.
run_major=$(java -version 2>&1 | sed -nE 's/.*version "([0-9]+).*/\1/p' | head -1)
if [ -z "$run_major" ] || [ "$run_major" -lt 25 ]; then
    echo "erro: java $run_major no PATH, mas o probe roda classes v69 (precisa 25)" >&2
    echo "      ajuste o PATH para o mesmo JDK do javac" >&2
    exit 2
fi

out_native=".local/fixtures/aabb"
out_ours="build/parity"
mkdir -p "$out_native" "$out_ours/classes"

echo "==> 1/4 compilando o probe (JDK $major)"
javac -cp "$CP" -d "$out_ours/classes" tools/oracle/java/dev/mdc/oracle/AabbProbe.java

echo "==> 2/4 gerando a transcrição do jogo"
CP="$CP:$out_ours/classes"
java -cp "$CP" dev.mdc.oracle.AabbProbe "$cases" > "$out_native/vanilla.jsonl"

echo "==> 3/4 gerando a nossa transcrição"
native=""
for candidate in build/dev/bin/mdc-parity-aabb build/headless/bin/mdc-parity-aabb \
                 build/ci/bin/mdc-parity-aabb; do
    if [ -x "$candidate" ]; then native="$candidate"; break; fi
done
if [ -z "$native" ]; then
    echo "    binário ausente; configurando e compilando (preset dev)"
    cmake --preset dev >/dev/null
    cmake --build --preset dev --target mdc-parity-aabb >/dev/null
    native=build/dev/bin/mdc-parity-aabb
fi
"$native" "$cases" > "$out_ours/ours.jsonl"

echo "==> 4/4 comparando"
if diff -u "$out_native/vanilla.jsonl" "$out_ours/ours.jsonl" > "$out_ours/aabb.diff"; then
    cases_done=$(tail -1 "$out_ours/ours.jsonl" | sed -E 's/.*"cases":([0-9]+).*/\1/')
    echo "✔ paridade: $cases_done casos, 0 diferenças"
    exit 0
fi

echo "✖ divergências encontradas:"
head -40 "$out_ours/aabb.diff" | sed 's/^/    /'
total=$(diff "$out_native/vanilla.jsonl" "$out_ours/ours.jsonl" | grep -c '^<' || true)
echo "    $total linha(s) divergentes — diff completo em $out_ours/aabb.diff"
echo "    próximo passo: minimizar o caso (menos casos, ver docs/HARNESS.md §5)"
exit 1
