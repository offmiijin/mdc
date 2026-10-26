# Paridade — a escada M0–M7

O alvo é o cliente **26.3** (ver [`adr/0003-versao.md`](adr/0003-versao.md)).
"Pronto" não é uma opinião: cada degrau tem critério de aceite **binário**,
verificado por harness ([`HARNESS.md`](HARNESS.md)).

> **Regra dura:** degrau sem harness não existe. Se não dá para comparar com o
> original, não está pronto — está "parecendo".

## Escala de paridade de um subsistema

| Nota | Significado |
|---:|---|
| **0** | ausente |
| **0,25** | iniciado, não jogável |
| **0,50** | jogável, com funcionalidade vanilla importante faltando |
| **0,75** | jogável, quase completo |
| **1** | completo e verificado contra o original por teste diferencial |

`1` significa **100% do que o harness compara**, não "não tem diferença
nenhuma". A lista de diferenças conhecidas é viva e mora em
[`deltas/`](deltas/) e na seção "fora de escopo" do ADR 0002.

## Os degraus

| # | Objetivo | Superfície tocada | Harness de aceite | Critério de pronto |
|---|---|---|---|---|
| **M0** | janela + laço fixo de 20 Hz + input | `client/main`, `Minecraft.runTick` | *tick-clock* | 10 min contínuos: 12.000 ticks, drift 0, sem tick perdido ou duplicado; janela visível apresentando frames |
| **M1** | geometria: `Vec3`, `AABB`, `BlockPos`, colisão | `core` (11.641 linhas), `world/phys` (1.082) | *aabb*, *collision* | 10.000 casos de colisão/movimento idênticos **bit a bit** (posição, velocidade, flag de chão) |
| **M2** | chunk, paleta, mesher, render de terreno | `world/level/chunk`, `client/renderer/{chunk,extract}` | *chunk-mesh* | mesma seed → mesmos vértices e mesmas faces visíveis (hash igual); 1.000 chunks |
| **M3** | jogador: física, câmera, quebrar/colocar, drops | `world/entity/player`, `client/player` | *player-walk*, *break-place* | 1.000 operações de movimento e 500 de quebrar/colocar idênticas |
| **M4** | geração de mundo: ruído, biomas, estruturas | `world/level/levelgen` (o maior bloco de `world`) | *worldgen* | 200 chunks por dimensão: hash do bloco a bloco igual ao vanilla |
| **M5** | save/load Anvil + `level.dat` | `world/level/storage`, datafixers | *save-roundtrip* | mundo salvo pelo nosso lado abre no vanilla **e** vice-versa, sem diff de NBT |
| **M6** | rede: protocolo 777 + servidor dedicado + LAN | `network` (24.109 linhas, 187 pacotes) | *protocol-join* | um cliente vanilla entra no nosso servidor offline-mode e vê o mesmo mundo; nós entramos em servidor vanilla |
| **M7** | sobrevivência: mobs, IA, crafting, redstone, comandos | `world/entity`, `world/item`, `server/commands` | vários, por subsistema | cada subsistema com nota **1** na tabela abaixo |

## Critério por degrau: o que **não** está coberto

Cada degrau declara explicitamente o que fica de fora — senão "pronto" volta a
ser opinião.

| # | Fora de escopo no degrau |
|---|---|
| M0 | qualquer conteúdo de mundo; só o relógio e o input |
| M1 | entidades; só blocos estáticos e AABBs |
| M2 | iluminação suave, transparência, entidades no chunk |
| M3 | mobs, veículos, efeitos, dano |
| M4 | geração de estruturas complexas e *features* raras (entram no M7) |
| M5 | migração de versões anteriores (datafixers completos) |
| M6 | **login Microsoft / online-mode** (recusado no ADR 0002) |
| M7 | Bedrock, Realms, Marketplace |

## Baseline medido (do `client.jar` 26.3)

Referência para dimensionar cada degrau. Os números vêm da análise estática do
`client.jar` 26.3 feita na **sua** cópia de estudo (fora do repositório, por serem
derivados — ver `docs/LEGAL.md`), reproduzível com as ferramentas em
`tools/` da árvore de estudo.

| Subsistema | Linhas | Arquivos |
|---|---:|---:|
| `world` | 315.648 | 2.715 |
| `client` | 185.404 | 1.891 |
| `util` | 60.831 | 735 |
| `server` | 54.099 | 432 |
| `data` (datagen — **não** entra no escopo) | 38.213 | 174 |
| `network` | 24.109 | 416 |
| `com/mojang/renderpearl` (RHI) | 17.921 | 146 |
| `commands` | 12.075 | 124 |
| `core` | 11.641 | 123 |
| **total** | **774.696** | **7.301** |

Núcleo por fan-in (quem todos usam): `core` (5.643 refs), `util` (4.129),
`world.entity` (2.620), `world.level` (1.989), `world.level.block.state`
(**1.868 refs em 6 arquivos** — maior alavancagem do projeto).

## Status atual

| # | Degrau | Nota | Observação |
|---|---|---:|---|
| M0 | laço + janela | **0,50** | relógio verificado (12.000 ticks/600 s, drift 0, sem descarte); janela SDL3 apresentando frames na máquina-alvo (**driver x11/XWayland, 60,3 fps**) com título contador. **Falta:** input de teclado/mouse e captura de mouse — sem isso não se anda no mundo |
| M1 | geometria/colisão | **0,50** | `Aabb` (constructor normalizado, `intersects`, `center` via lerp, `move`, `inflate`) com 8 casos de teste; **harness verde: 2.007 casos, 0 diferenças, md5 idêntico** contra o 26.3. Falta `clip`/raycast e resolução de colisão (o critério do degrau pede 10.000 casos de movimento) |
| M2 | chunk/mesher/render | 0 | — |
| M3 | jogador | 0 | — |
| M4 | worldgen | 0 | — |
| M5 | save/load | 0 | — |
| M6 | rede | 0 | offline-mode; auth Microsoft fora de escopo |
| M7 | sobrevivência | 0 | — |

Esta tabela é atualizada **no mesmo commit** que fecha um degrau. Tabela
desatualizada é bug de documentação.

## Regras do jogo

1. **Nenhum degrau é pulado.** M2 sem M1 produz render que não colide.
2. **Um degrau só fecha com o harness verde** e o critério acima satisfeito.
3. **Divergência intencional** entra em [`deltas/`](deltas/) antes do merge —
   e o harness passa a esperar exatamente aquela diferença.
4. **A versão de referência não muda** dentro de um degrau (ADR 0003).
5. **Licença e conformidade valem em todos os degraus**: o gate
   (`tools/check_compliance.py`) roda em cada commit, sem exceção.
