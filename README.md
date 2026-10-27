# mdc

> **NOT AN OFFICIAL MINECRAFT PRODUCT. NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.**

Reimplementação independente do Minecraft: Java Edition **26.3**, em C++, escrita do zero a partir do comportamento observável do jogo.

**Isto não é um mod.** Um mod é um delta sobre o binário original; aqui o que se distribui é um programa independente, que exige uma cópia legal do jogo para rodar. O projeto **não contém** código, assets, dados ou binários da Mojang.

> `mdc` é o nome provisório (veio do diretório). O produto **não** usa "Minecraft" no nome, por exigência das [Usage Guidelines](https://www.minecraft.net/en-us/usage-guidelines).

## Estado

**Fase 1 — fundação + primeiro harness.** O que existe hoje e funciona:

| Item | Estado |
|---|---|
| licença, política legal e ADRs | ✅ 0001–0007 |
| versão de referência congelada e verificável (`refs/minecraft.lock`) | ✅ 26.3 |
| gate de conformidade com **13 regras**, hooks e CI | ✅ |
| relógio de tick 20 Hz + janela SDL3 + input | ✅ **M0 = 1** — 12.000 ticks/600 s, drift 0; WASD + mouse capturado |
| geometria AABB + harness bit a bit | ✅ **M1 = 0,50** — 2.007 casos, 0 diferenças |
| render, mundo, rede, sobrevivência | ⛔ M2–M7 |

Progresso medido, com critério de aceite por degrau: [`docs/PARITY.md`](docs/PARITY.md).

## Decisões (fechadas)

| Decisão | Escolha | ADR |
|---|---|---|
| Licença do código | **AGPL-3.0-only** + exceção de mods (§7) | [0001](docs/adr/0001-licenca.md) |
| Licença dos assets | **CC BY-SA 4.0** | [0001](docs/adr/0001-licenca.md) |
| Contribuições | **DCO** (`git commit -s`), sem CLA | [0001](docs/adr/0001-licenca.md) |
| Escopo | reimplementação independente; paridade como base; divergências declaradas em `docs/deltas/` | [0002](docs/adr/0002-escopo.md) |
| Dentro de escopo | singleplayer → LAN → servidor dedicado offline-mode | [0002](docs/adr/0002-escopo.md) |
| Fora de escopo | login Microsoft/online-mode, Realms, Marketplace, cross-play Bedrock | [0002](docs/adr/0002-escopo.md) |
| Versão de referência | **26.3 congelada** (protocolo 777, world version 5023) | [0003](docs/adr/0003-versao.md) |
| Stack | C++23, CMake + FetchContent, **SDL3 + Vulkan** + RHI própria, `shaderc`, doctest | [0004](docs/adr/0004-stack.md) |
| Plataforma | **Linux x86-64 + Vulkan primeiro** | [0004](docs/adr/0004-stack.md) |
| Mods | ABI C + fronteira de processo/WASM; **sem exceção de linking por ora** | [0005](docs/adr/0005-mods.md) · [0007](docs/adr/0007-excecao-de-mods-adiada.md) |
| Divergência do original | registrada em `docs/deltas/`, testada e reversível | [0006](docs/adr/0006-deltas.md) |
| Como se prova paridade | harness diferencial, floats por bits, fixtures privados | [HARNESS](docs/HARNESS.md) |
| Ordem do trabalho | degraus M0–M7, nenhum pulado | [PARITY](docs/PARITY.md) |

## Build

Requisitos: **CMake ≥ 3,24**, **compilador C++23** (GCC 13+ ou Clang 16+), **Ninja**.
SDL3 é opcional: se estiver instalado é usado; se não, o CMake baixa (`release-3.4.18`).

```sh
cmake --preset dev            # Debug, cliente + testes, avisos = erro
cmake --build --preset dev
ctest --preset dev            # 20 casos / 94 asserções
```

Binários em `build/dev/bin/`. Sem GPU ou sem SDL:

```sh
cmake --preset headless && cmake --build --preset headless && ctest --preset headless
```

Critério do degrau M0 (relógio de tick), sem abrir janela:

```sh
./build/dev/bin/mdc-client --headless --seconds=600
# [headless] frames=36001 ticks=12000 simulados=600.017s esperado=12000 drift=0 descartados=0
```

O mesmo critério com janela real — e o único jeito de ver o input chegar ao
tick (uma linha por segundo simulado; `W..D...` = W e D pressionados):

```sh
./build/dev/bin/mdc-client --seconds=600
# [mouse] captura relativa ativa
# [input] t=300s teclas=W..D... mouse=(640.0, 360.0)
# [janela] ticks=12000 simulados=600.000s esperado=12000 drift=0 descartados=0 frames=36001 fps=60.0
```

Mapeamento (scancode, posição física): `WASD` mover, `espaço` pular, `shift`
agachar, `ctrl` correr, `ESC` encerrar. O mouse fica em captura relativa — o
cursor não bate na borda da janela.

Presets: `dev`, `headless`, `ci` (Release, SDL3 via FetchContent).
`MDC_WERROR=ON` transforma aviso em erro — padrão em `dev` e `ci`.

### Diagnóstico da janela

O cliente imprime a primeira linha **antes** de abrir a janela (stdout em modo
linha, então aparece na hora mesmo com saída redirecionada):

```
[video] driver=x11 displays=2
```

| Driver | Significa |
|---|---|
| `x11` | X11 ou XWayland — funciona; caso comum em sessão Wayland com XWayland ativo |
| `wayland` | Wayland nativo |
| `dummy` / `offscreen` | **sem saída gráfica** (sandbox/container sem socket, ou sessão sem display). O app aborta explicando |

Se vier `dummy`: use `--headless` (relógio sem janela) ou force `SDL_VIDEODRIVER=x11`.
Para rodar assim mesmo: `--allow-headless-driver`.

A janela alterna entre dois tons escuros a 1 Hz e mostra a contagem de ticks no
título. **É intencional**: prova visual de que frames são apresentados enquanto
não existe renderer — sai no M2, junto com o render de verdade.

### Harness de paridade

Compara o nosso código com o jogo original, bit a bit. Precisa de **JDK 25** e de
uma instalação do jogo (própria, legal).

```sh
tools/parity/run_aabb.sh                              # usa ~/.minecraft
MDC_MC_DIR=~/Games/UltimMC tools/parity/run_aabb.sh   # MultiMC/UltimMC/Prism
```

Estado: **2.007 casos, 0 diferenças**. Contrato e armadilhas em
[`tools/oracle/README.md`](tools/oracle/README.md).

No fim, o relatório traz o pacing real:

```
[janela] ticks=60 simulados=3.000s esperado=60 drift=0 descartados=0 frames=181 fps=60.3
```

## Conformidade (o que protege o projeto)

Três camadas, um motor de regras:

```sh
tools/install_hooks.sh                        # uma vez, por clone
python3 tools/check_compliance.py --staged    # pre-commit
python3 tools/check_compliance.py --all       # árvore
python3 tools/check_compliance.py --history   # histórico + blobs grandes (CI)
python3 tools/selftest_compliance.py          # testes do próprio gate
```

| Código | Bloqueia |
|---|---|
| `R001` | caminhos de material derivado (`.sandbox-cache/`, `decompiled/`, `minecraft/`, `versions/`, `libraries/`, …) |
| `R002` | `.jar`, `.class`, `.java`, `.mca`, `.nbt`, `.mcmeta`, shaders `.fsh/.vsh` |
| `R003` | arquivo > 8 MB sem justificativa |
| `R004`–`R006` | assinaturas de código derivado: `net/minecraft`, cabeçalho de obfuscation map, URLs de distribuição do jar |
| `R007` | README sem o aviso obrigatório |
| `R008` | código/script sem `SPDX-License-Identifier` |
| `R009` | arquivos compactados |
| `R010` | SPDX que **não** é `AGPL-3.0-only`, ou `LICENSE`/`NOTICE` ausente ou não-AGPL |
| `R011` | `.java` fora de `tools/oracle/java/**`, ou probe sem `package dev.mdc.…`, ou com `package net.minecraft` |
| `R012` | diretório em `mods/` sem `LICENSE` |
| `R013` | qualquer `.jsonl` versionado (transcrição de harness = fixture) |

Exceções só em [`.compliance/allowlist.json`](.compliance/allowlist.json), com caminho, regra e motivo. Política completa em [`docs/LEGAL.md`](docs/LEGAL.md).

## Estrutura

```
LICENSE                      AGPL-3.0, texto íntegro e não modificado
NOTICE                       eleição de versão, exceção de mods, assets, terceiros
LICENSES/                    CC BY-SA 4.0 (assets) + registro de terceiros
README.md · CONTRIBUTING.md
docs/LEGAL.md                política de propriedade intelectual
docs/HARNESS.md              como se prova paridade (determinismo, transcrição, fixtures)
docs/PARITY.md               escada M0–M7 com critério de aceite por degrau
docs/deltas/                 divergências intencionais (uma por arquivo, com harness)
docs/adr/                    decisões de arquitetura (0001–0007)
refs/minecraft.lock          26.3 congelada: hashes, protocolo 777, world version
mods/                        mods de terceiros (cada um com LICENSE própria — R012)
tools/oracle/                probe Java do harness (único lugar com .java)

CMakeLists.txt · CMakePresets.json · cmake/     build
include/mdc/                 headers públicos (core; futuro ABI de mods)
src/core/                    núcleo sem dependências: relógio de tick, Vec3/AABB
src/client/                  binário do cliente (SDL3)
src/harness/                 lado C++ dos harnesses de paridade (sem SDL)
tests/                       testes unitários (doctest)
tests/parity/                casos do harness (entrada nossa, versionada)
tools/parity/                gerador de casos + runner do diff

.compliance/allowlist.json
.githooks/                   pre-commit e pre-push
.github/workflows/           CI de conformidade (compliance.yml) e de build (build.yml)
tools/                       gate + testes + instalador de hooks
```

## Como contribuir

Leia [`CONTRIBUTING.md`](CONTRIBUTING.md). Regra de ouro: **nunca** commite nada que tenha saído da sua instalação do Minecraft. Assine seus commits (`git commit -s`).

## Licença

Código sob **AGPL-3.0-only**; assets originais sob **CC BY-SA 4.0**. Ver [`LICENSE`](LICENSE) e [`NOTICE`](NOTICE). Isso cobre apenas o trabalho original deste repositório — Minecraft, seu código, assets e nome pertencem à Mojang e à Microsoft, sob a [Minecraft EULA](https://www.minecraft.net/en-us/eula).
