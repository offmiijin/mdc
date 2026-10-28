# Oráculo do harness — `tools/oracle/`

O oráculo é o **lado de referência** da comparação: um probe Java que carrega as
classes do **seu** `client.jar` 26.3, roda o jogo de forma controlada (seed fixa,
ordem de tick fixa) e despeja o estado resultante numa **transcrição**.

O nosso C++ produz a mesma transcrição. `diff` bit a bit → paridade.

```
entrada fixa ──► probe Java (este diretório) ──► transcript.jsonl  (fixture, privada)
             └─► nosso C++                  ──► transcript.jsonl
                                                  └─► diff = 0?
```

## Regras (verificadas pelo gate)

| Regra | O que exige | Por quê |
|---|---|---|
| `R002` (exceção) | `.java` é permitido **só** aqui | é o único lugar do repositório onde Java é legítimo |
| `R011` | `package dev.mdc.…` obrigatório | prova que o arquivo é nosso |
| `R011` | `package net.minecraft` / `package com.mojang` **proibido** | pega código da Mojang disfarçado de probe |
| `R008`/`R010` | cabeçalho SPDX `AGPL-3.0-only` | mesma regra de qualquer código |
| `R013` | nenhum `.jsonl` versionado | transcrição é material derivado |

## O que é permitido escrever aqui

```java
// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import net.minecraft.world.entity.Mob;   // importar a API: permitido
import net.minecraft.world.phys.AABB;   // — o probe só *chama*

public final class CollisionProbe {
    // lê o jar do usuário, monta o cenário, chama o jogo, serializa o resultado
}
```

**Permitido:** importar e chamar a API do jogo, como qualquer mod faria.
**Proibido:** copiar corpo de método, campos ou estruturas da Mojang; colar
arquivo decompilado; redistribuir o jar; baixar o jogo (o caminho do jar vem por
argumento ou variável de ambiente).

## Formato da transcrição

- Uma linha JSON por amostra (`JSONL`).
- Floats em **hex** (`0x1.fffffffffffffp-2`), nunca decimal — comparação é por
  bits exatos.
- Campos ordenados de forma determinística.
- O arquivo gerado **nunca** entra no repositório: fica em `.local/fixtures/`.

## Pré-requisito

**JDK 25.** O 26.3 compila para class file v69; `javac 21` falha com
*"class file has wrong version 69.0, should be 65.0"*. Ver `docs/adr/0004-stack.md`.

## Estado

Probes implementados: **`AabbProbe`**, **`Vec3Probe`**, **`ClipProbe`**, **`CollisionProbe`**
e **`BlockPosProbe`** (geometria, colisão e coordenada de bloco — degrau M1). O `CollisionProbe` usa **`StubLevel`**, um `Level`
mínimo que responde "nenhuma colisão de mundo": assim o harness compara o
resolvedor do próprio jogo, e não uma reimplementação nossa.

### Como rodar um harness

```sh
# 1. casos (entrada nossa, versionada)
python3 tools/parity/gen_<harness>_cases.py --out tests/parity/<harness>-0001.txt

# 2. os dois lados + diff (precisa de JDK 25 e da instalação do jogo)
tools/parity/run_parity.sh <harness>
```

`harness` ∈ `aabb`, `vec3`, `clip`, `blockpos`, `collision`. Por padrão o script
procura a instalação em `~/.minecraft`; para outro launcher:

```sh
MDC_MC_DIR=~/Games/UltimMC tools/parity/run_parity.sh aabb
```

O script: resolve o classpath → confere o **sha1 do jar contra `refs/minecraft.lock`**
→ compila o probe → gera `.local/fixtures/<harness>/vanilla.jsonl` → gera
`build/parity/<harness>.jsonl` → `diff`. Sai 0 se idênticos, 1 com o diff.

**Resultado atual: 2.007 casos, 0 diferenças** — verificado com o jar do UltimMC
(sha1 `e877b6a0…`, o mesmo do lock) e 56 bibliotecas resolvidas para linux/x86.

### Instalação do jogo: dois layouts

`tools/parity/classpath.py` resolve os dois, e monta o classpath **na ordem do
version json** — não por `find`. Motivo: a pasta `libraries/` do UltimMC tem
DataFixerUpper 6.0.8 e 10.0.21 ao mesmo tempo (outras instâncias); pegar a errada
primeiro quebra o probe.

| launcher | client jar | version json |
|---|---|---|
| oficial | `versions/26.3/26.3.jar` | `versions/26.3/26.3.json` |
| MultiMC / UltimMC / Prism | `libraries/com/mojang/minecraft/26.3/minecraft-26.3-client.jar` | `meta/net.minecraft/26.3.json` |

O json lista bibliotecas de **todas as plataformas**; o script filtra pelo bloco
`rules` (OS/arquitetura) e descarta as entradas `natives-*`. Sem esse filtro,
numa instalação Linux faltam 48 jars — todos de macOS/Windows.

### Duas armadilhas do lado Java (custaram uma iteração cada)

**1. O classpath é o jogo inteiro, não só o `client.jar`.** E não é só uma
biblioteca: **duas cadeias independentes** puxam dependências mesmo num probe
que só toca geometria:

```
Vec3.CODEC   →  com.mojang.serialization.Codec      (DataFixerUpper)
AABB         →  net.minecraft.util.Util.<clinit>
             →  com.mojang.logging.LogUtils.<clinit>
             →  org.slf4j.MarkerFactory              (slf4j-api)
```

`Util.java` tem `private static final Logger LOGGER = LogUtils.getLogger();` —
um campo estático, ou seja, qualquer classe que inicialize `Util` paga o preço.

Testado: removendo **só** `slf4j-api-2.0.17.jar` (69 KB) do classpath, o probe morre
com `NoClassDefFoundError: org/slf4j/MarkerFactory` **antes da primeira linha de
saída**. Não existe "só um pouquinho do jogo": carregar um tipo de geometria
inicializa o stack de logging. É por isso que o classpath vem da lista completa
do version json.

**2. Sobrecargas que puxam bibliotecas externas.** `AABB.move(Vec3)` está
sobrecarregado com `move(Vector3f)`, e `org.joml.Vector3f` **não está** no
`client.jar`. O compilador precisa carregar o tipo para resolver a sobrecarga e
falha com *"class file for org.joml.Vector3f not found"*. Solução: chamar a
sobrecarga de três doubles (`move(x, y, z)`).

Regra geral: **prefira a sobrecarga com tipos primitivos** em qualquer probe.

O que o probe compara (por caso):

| Campo | Operação do original |
|---|---|
| `intersects` | `AABB.intersects(AABB)` — comparações estritas |
| `center_a` | `AABB.getCenter()` — via `Mth.lerp(0.5, …)` |
| `move_a` | `AABB.move(Vec3)` |
| `inflate_a` | `AABB.inflate(x,y,z)` |

O que ainda **não** existe:

- `clip`/raycast e resolução de colisão (o próximo probe, e o que dá trabalho: a
  ordem das operações do slab test tem de bater bit a bit);
- harness de tick, física, worldgen;
- qualquer harness que abra o jogo com janela (todos são headless).
