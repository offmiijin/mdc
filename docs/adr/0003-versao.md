# ADR 0003 — Versão de referência: 26.3 congelada

**Status:** aceito (2026-10-26)

## Contexto

Em outubro de 2026 o Java Edition está na `26.3` (release em 2026-09-15), com
`26.4-snapshot-5` já publicado. A Mojang passou a usar esquema anual: `26.1`,
`26.2`, `26.3`, `27.1`… Ou seja, uma versão nova a cada poucos meses.

Consequência: perseguir "a versão atual" significa que o alvo se move mais
rápido do que um projeto desse tamanho pode acompanhar.

Ao mesmo tempo, desde `26.1` o jogo é **distribuído sem ofuscação** — é a única
faixa em que dá para ler a implementação real como fonte de comportamento.

## Decisão

**Congelar a referência em 26.3**, registrada em `refs/minecraft.lock` com:

| Campo | Valor verificado |
|---|---|
| Versão | `26.3` (release 2026-09-15) |
| version JSON | `61fff1316bd50312624a858bbb5eb18fa45208ed` |
| `client.jar` sha1 | `e877b6a07acd633fb3bb475002175cec036e7b87` (41.483.720 bytes) |
| `server.jar` sha1 | `33680f5f2ac32864d6d7cf5e56a705fdb3e05f4c` (62.294.556 bytes) |
| asset index | id `34`, size 597.035 |
| protocolo | `777` |
| world version | `5023` |
| resource/data pack | `97.1` / `121.0` |
| Java | 25 (`java-runtime-epsilon`) |
| bibliotecas | 114 |

**Política de porte:** subir de versão só depois do degrau **M5** (save/load),
e sempre como um ADR novo. Nenhum harness mudará de versão de referência sem
que a suíte inteira passe na versão anterior.

## Consequências

- Os harnesses comparam **sempre** contra o mesmo binário, então uma regressão
  é sempre causada pelo nosso código — nunca por atualização do jogo.
- Um fork que queira 26.4 vai precisar de outro lock e de ajustar os harnesses;
  a arquitetura não muda.
- Enquanto o alvo for "singleplayer jogável", a defasagem de versão é invisível
  para quem joga offline.

## Alternativas rejeitadas

| Alternativa | Motivo |
|---|---|
| Acompanhar a versão mais recente | nunca estabiliza; cada release custa re-trabalho de protocolo e datafixer |
| Beta 1.7.3 | escopo muito menor, mas só existe como bytecode ofuscado; exigiria engenharia reversa às cegas |
| Suportar múltiplas versões desde já | multiplica o custo por N sem entregar valor de aprendizado |
