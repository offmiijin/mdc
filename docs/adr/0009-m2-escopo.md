# ADR 0009 — Escopo do M2: chunk, paleta, mesher e render de terreno

**Status:** aceito (2026-10-28)

## Contexto

O degrau M2 foi declarado em [`../PARITY.md`](../PARITY.md) com três problemas,
todos medidos no `client.jar` 26.3 da cópia de estudo:

1. **A superfície declarada não contém o mesher.** Estão listados
   `world/level/chunk` e `client/renderer/{chunk,extract}`, mas o tesselador
   mora em `client/renderer/block` (95 classes) e a geometria de bloco nasce em
   `client/resources/model` (92 classes). Sem esses dois não existe face nenhuma
   para comparar. `client/renderer/extract` tem 3 classes e serve a dados de
   entidade — é do M3, não do M2.
2. **O critério depende do M4.** "Mesma seed → mesmos vértices" exige geração de
   mundo, que é o degrau seguinte. Nenhum degrau pode ser pulado
   ([`../PARITY.md`](../PARITY.md) regra 1), então o M2 não pode nascer de uma
   seed.
3. **Metade do degrau não é comparável por harness.** "Render de terreno"
   inclui GPU (Vulkan/RHI própria, ADR 0004), e não existe oráculo para pixels.
   Deixar isso implícito faria a nota do degrau virar opinião.

Medições que sustentam a decisão (assinaturas públicas, `javap`, sem corpo de
método):

| Caminho do pipeline | Precisa de GPU? |
|---|---|
| `SpriteLoader.loadAndStitch(ResourceManager, Identifier, mip, Executor, Set)` | **não** — a maior dimensão de textura entra como `int` |
| `MaterialBaker(Preparations, Preparations)` | não |
| `LoadedBlockModels.bake(...)` → `BlockStateModelSet` | não |
| `ModelBlockRenderer.tesselateBlock(BlockQuadOutput, …)` | não — o `BlockAndTintGetter` é interface, cabe stub |
| `SectionBufferBuilderPack` + `MeshData.vertexBuffer()` | não |
| `SectionCompiler.compile` | não, **mas** exige `RenderSectionRegion(ClientLevel, …)` |
| upload, pipeline, swapchain | sim |

## Decisão

1. **Superfície do M2 expandida** para incluir o tesselador e a geometria:
   `world/level/chunk`, `client/renderer/chunk`, `client/renderer/block`,
   `client/resources/model`, `client/color` (tint) e o empacotamento de bits de
   `util` (`BitStorage`). `client/renderer/extract` sai do M2 e entra no M3.
2. **Os dados de seção vêm do arquivo de caso**, gerados pelo PRNG do nosso
   gerador — não de worldgen. O critério "mesma seed" é substituído por "mesma
   entrada sintética → mesmos vértices e mesmas faces visíveis". Worldgen
   continua sendo M4, e nada do M2 depende dele.
3. **Atlas e UV fazem parte do M2**, com harness `atlas` próprio. Comparar
   vértice sem UV compararia metade do vértice; e adiar o atlas esconderia a
   parte mais frágil (ordem do `Stitcher`).
4. **Fluidos e camada `TRANSLUCENT` ficam fora do M2.** Fluido é renderizador
   separado e a camada translúcida depende de ordenação por câmera — ou seja, de
   um jogador real (M3). As camadas `SOLID` e `CUTOUT` estão dentro.
5. **Iluminação suave (AO) fica fora**, como `PARITY.md` já declarava. O probe
   instancia o original com AO desligado, e o mapa do degrau registra o que
   precisa ser medido para isso (o significado dos dois booleanos do construtor
   do tesselador e do compilador de seção **não** é conhecido por assinatura).
   Consequência aceita: o visual do M2 não é o default do jogo.
6. **O degrau tem duas metades com provas diferentes.** M2.0–M2.5 são CPU e
   entram na nota por harness. M2.6 (RHI Vulkan, câmera, frustum) **não tem
   oráculo**: vale por smoke visual e por revisão, e fica fora da nota.
7. **A unidade de comparação é a seção de 16³**: 1.000 seções, hash por seção e
   por camada, além do registro por quad para minimizar divergência.

## Consequências

- O M2 deixa de ser "um" harness e passa a ser seis, cada um com entrada própria:
  `palette`, `blockstate`, `model`, `atlas`, `tint`, `chunk-mesh` (+ `visgraph`).
  Ordem e contrato em [`../PARITY.md`](../PARITY.md).
- Entram dependências que o M0/M1 não tinham: leitura de ZIP, JSON e PNG do jar
  do usuário (e, no M2.6, Vulkan). Nenhuma delas pode entrar em `mdc_core`.
- `docs/PARITY.md` passa a registrar a nota do M2 como "parte comparável"; a
  apresentação em GPU é verificada por inspeção, não por diff.
- O critério vira reprodutível em CI sem Java (transcrição golden) e sem GPU.

## Alternativas rejeitadas

| Alternativa | Motivo |
|---|---|
| Deixar UV/atlas fora do M2 (delta declarado) | o delta esconderia o `Stitcher` e o UV quantizado; `model` e `chunk-mesh` comparariam metade do vértice |
| Exigir worldgen mínimo antes do M2 | inverte a escada: M2 é pré-requisito de M4 (render), e o M4 é que fecha a seed |
| Colocar fluidos no M2 | traz `FluidRenderer` + camada translúcida + ordenação por câmera; sem jogador (M3) não há como comparar a ordenação |
| Revelar a nota do M2 só quando o Vulkan estiver pronto | mistura prova com inspeção; a parte comparável pode fechar antes |
| Comparar pixels do M2.6 contra captura do original | capture do jogo não é oráculo (docs/HARNESS.md §6) e depende de driver |
