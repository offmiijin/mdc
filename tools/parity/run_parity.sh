#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
#
# Harness de paridade — roda o probe Java contra o SEU client.jar e compara com
# o nosso binário, bit a bit.
#
# Uso:
#   tools/parity/run_parity.sh <harness> [casos.txt]
#
#   harness    aabb | vec3 | clip | blockpos | collision
#   casos      padrão: tests/parity/<harness>-0001.txt
#   client.jar MDC_MC_JAR, senão o jar achado no layout de $MDC_MC_DIR
#
# Saída do jogo (fixture) vai para .local/fixtures/ — nunca para o repositório.
# Ver docs/HARNESS.md.
set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"

if [ $# -lt 1 ]; then
    echo "uso: $0 <harness> [casos.txt]" >&2
    echo "     harness: aabb vec3 clip blockpos collision palette blockstate atlas tint" >&2
    exit 2
fi

harness="$1"
cases="${2:-tests/parity/$harness-0001.txt}"

# Nome do probe e do binário por harness. Explícito, e não "capitalize": o
# mapeamento é contrato, não convenção de string.
case "$harness" in
    aabb)      probe="AabbProbe" ;;
    vec3)      probe="Vec3Probe" ;;
    clip)      probe="ClipProbe" ;;
    blockpos)  probe="BlockPosProbe" ;;
    collision) probe="CollisionProbe" ;;
    palette)   probe="PaletteProbe" ;;
    blockstate) probe="BlockStateProbe" ;;
    atlas)     probe="AtlasProbe" ;;
    tint)      probe="TintProbe" ;;
    *) echo "erro: harness desconhecido: $harness" >&2; exit 2 ;;
esac

mc_dir="${MDC_MC_DIR:-$HOME/.minecraft}"

# O classpath vem do version json (ordem exata das bibliotecas), não de `find`:
# instalações com duas versões de DataFixerUpper quebram se a errada vier antes.
# Suporta launcher oficial e MultiMC/UltimMC/Prism (ver tools/parity/classpath.py).
CP=$(python3 tools/parity/classpath.py "$mc_dir" 26.3) || exit 2
if [ -n "${MDC_MC_JAR:-}" ]; then
    CP="$MDC_MC_JAR:$CP"
fi
jar=$(printf '%s' "$CP" | cut -d: -f1)

[ -f "$jar" ] || { echo "erro: client.jar não encontrado ($jar)" >&2; exit 2; }
[ -f "$cases" ] || { echo "erro: casos não encontrados em $cases" >&2
                     echo "      gere com: python3 tools/parity/gen_${harness}_cases.py" >&2; exit 2; }

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

# O atlas é o único harness que precisa do pack de recursos e de nativos
# (decodificar PNG usa NativeImage -> LWJGL). Os dois vêm da instalação do
# usuário; nada é baixado.
if [ "$harness" = "atlas" ] || [ "$harness" = "tint" ]; then
    assets="${MDC_MC_ASSETS:-$root/.sandbox-cache/mc-assets}"
    if [ ! -d "$assets/assets" ]; then
        echo "==> extraindo assets do jar para $assets (uma vez)"
        mkdir -p "$assets"
        unzip -q -o "$jar" 'assets/*' -d "$assets" || {
            echo "erro: não consegui extrair assets do jar (unzip)" >&2; exit 2; }
        printf '{"pack":{"pack_format":97,"description":"atlas probe"}}' > "$assets/pack.mcmeta"
    fi
    export MDC_MC_ASSETS="$assets"

    natives="${MDC_LWJGL_NATIVES:-$root/.sandbox-cache/lwjgl-natives/jars}"
    native_jars=$(find "$natives" -name '*natives-linux*.jar' 2>/dev/null | tr '\n' ':')
    if [ -z "$native_jars" ]; then
        echo "erro: nativos do LWJGL ausentes em $natives" >&2
        echo "      copie os *-natives-linux.jar da sua instalação (libraries/org/lwjgl)" >&2
        echo "      ou aponte MDC_LWJGL_NATIVES para eles" >&2
        exit 2
    fi
    CP="$CP:$native_jars"
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

out_native=".local/fixtures/$harness"
out_ours="build/parity"
mkdir -p "$out_native" "$out_ours/classes"

echo "==> 1/4 compilando os probes (JDK $major)"
# O pacote inteiro, não só o probe pedido: há fontes compartilhadas (StubLevel).
javac -cp "$CP" -d "$out_ours/classes" $(find tools/oracle/java -name '*.java')

echo "==> 2/4 gerando a transcrição do jogo"
CP="$CP:$out_ours/classes"
java -cp "$CP" "dev.mdc.oracle.$probe" "$cases" > "$out_native/vanilla.jsonl"

echo "==> 3/4 gerando a nossa transcrição"
native=""
for candidate in "build/dev/bin/mdc-parity-$harness" "build/headless/bin/mdc-parity-$harness" \
                 "build/ci/bin/mdc-parity-$harness"; do
    if [ -x "$candidate" ]; then native="$candidate"; break; fi
done
if [ -z "$native" ]; then
    echo "    binário ausente; configurando e compilando (preset dev)"
    cmake --preset dev >/dev/null
    cmake --build --preset dev --target "mdc-parity-$harness" >/dev/null
    native="build/dev/bin/mdc-parity-$harness"
fi
"$native" "$cases" > "$out_ours/$harness.jsonl"

echo "==> 4/4 comparando"
if diff -u "$out_native/vanilla.jsonl" "$out_ours/$harness.jsonl" > "$out_ours/$harness.diff"; then
    cases_done=$(tail -1 "$out_ours/$harness.jsonl" | sed -E 's/.*"cases":([0-9]+).*/\1/')
    echo "✔ paridade $harness: $cases_done casos, 0 diferenças"
    exit 0
fi

echo "✖ divergências encontradas em $harness:"
head -40 "$out_ours/$harness.diff" | sed 's/^/    /'
total=$(diff "$out_native/vanilla.jsonl" "$out_ours/$harness.jsonl" | grep -c '^<' || true)
echo "    $total linha(s) divergentes — diff completo em $out_ours/$harness.diff"
echo "    próximo passo: minimizar o caso (menos casos, ver docs/HARNESS.md §5)"
exit 1
