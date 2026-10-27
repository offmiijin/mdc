# ADR 0007 — Exceção de mods adiada

**Status:** aceito (2026-10-26)
**Substitui parcialmente:** [0001](0001-licenca.md) item 2 e [0005](0005-mods.md) item 2

## Contexto

O ADR 0001 decidiu duas coisas ao mesmo tempo: licença AGPL-3.0-only **e** uma
permissão adicional (§7) para que mods ligados ao motor pudessem ter outra
licença. Essa permissão depende de duas condições que não se cumprem:

1. a redação precisa de revisão jurídica;
2. a fronteira técnica que ela pressupõe (ABI C estável + processo/WASM) **não
   existe** — o ADR 0005 diz que a ABI só é congelada depois do degrau M2.

Conceder permissão é fácil; **retirar depois é caro** — com DCO, exigiria o
consentimento de cada contribuidor. Conceder agora significa assumir risco
jurídico por um mecanismo que não existe, em troca de nenhum benefício imediato.

## Decisão

**Não conceder a exceção.** No estado atual:

- `NOTICE` **não** contém permissão adicional (§7). O programa é AGPL-3.0-only, ponto.
- Um mod que linke contra o motor é obra derivada e **precisa ser AGPL-3.0-only**.
- A fronteira técnica do ADR 0005 (ABI C + processo/WASM) continua sendo o plano.
- A permissão poderá ser concedida **depois**, por um ADR novo, quando (a) a ABI
  existir e estiver congelada e (b) houver revisão jurídica da redação.

## Por que esta ordem, e não a inversa

| Conceder depois | Retirar depois |
|---|---|
| adicionar permissão é um ato unilateral do licenciante | retirar exige consentimento de todos os contribuidores (DCO, sem CLA) |
| ninguém depende dela ainda | mods já publicados contariam com ela |
| reflete o estado real (a ABI não existe) | cria obrigação para um mecanismo não implementado |

## Consequências

- O ecossistema de mods **de terceiros** fica indisponível até a ABI existir.
  Isso não bloqueia nada do M0–M7: mods são do M7+.
- `docs/LEGAL.md` e `README.md` descrevem a exceção como adiada, não como vigente.
- O `NOTICE` tem três seções e nenhuma permissão extra — menor e mais fácil de auditar.
- O ADR 0005 mantém a decisão de arquitetura (a fronteira é necessária para o AGPL
  não se propagar aos mods), mas a parte de licença fica pendente de um ADR futuro.

## Alternativas rejeitadas

| Alternativa | Motivo |
|---|---|
| **Conceder a exceção já** | assume risco jurídico por zero benefício: a ABI não existe |
| **Condicionar o projeto à revisão jurídica** | bloqueia o andamento por semanas para uma permissão sem uso |
| **Desistir da exceção para sempre** | fecha a porta para mods de terceiros, que é objetivo declarado do projeto |
