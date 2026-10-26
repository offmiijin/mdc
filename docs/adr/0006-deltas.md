# ADR 0006 — Política de divergência (deltas)

**Status:** aceito (2026-10-26)

## Contexto

O ADR 0002 fixou "paridade como base" e "divergência como camada declarada",
mas não disse **como** uma divergência entra, nem o que a torna legítima. Sem
esse mecanismo, duas coisas ruins acontecem:

1. toda diferença vira discussão caso a caso, e "está diferente de propósito"
   passa a ser desculpa universal;
2. o harness perde valor: se divergência é livre, o vermelho deixa de significar
   bug.

O projeto tem ambição explícita de mudar coisas ("reescrita à nossa maneira",
mods nativos, mecânicas novas) — então o mecanismo precisa **permitir** essa
liberdade sem dissolver o critério.

## Decisão

Divergência é permitida, mas é **registrada, testada e reversível**.

1. **Registro obrigatório.** Toda diferença de comportamento em relação ao 26.3
   entra em `docs/deltas/NNNN-slug.md` **antes do merge**, seguindo o template.
2. **Escrito antes do código.** O delta descreve o comportamento do original
   **com evidência** (medida, classe observada ou harness) e o nosso.
3. **Desligado por padrão** quando muda o que o jogador percebe. Quem quer
   vanilla tem vanilla.
4. **Com harness que demonstra a divergência.** O harness precisa ficar vermelho
   **exatamente** no ponto descrito — e em nenhum outro.
5. **Reversível.** Se não dá para desligar sem reescrever, não é delta: é fork
   de comportamento e exige ADR próprio.
6. **Nunca apagado.** Delta vira `revogado` quando voltamos ao vanilla; o
   arquivo permanece como histórico.
7. **Delta não carrega material da Mojang.** Nenhum asset, nenhum código.

O que **não** é delta: bug, recurso ainda não implementado, divergência de
arredondamento de 1 ULP, falta de um item do degrau atual.

## Consequências

- O vermelho do harness continua significando alguma coisa: ou bug, ou delta
  registrado. Não existe terceira categoria.
- "Mudar de opinião" fica barato e auditável: revoga-se o delta, o arquivo
  fica, e o motivo de ter tentado também.
- O `git log` continua irrelevante para entender decisões — quem conta a
  história são os ADRs e os deltas (coerente com o lote 3 ter sido feito antes
  do código).
- Custo por delta: um arquivo de ~30 linhas e um caso de teste. Barato o
  suficiente para não desencorajar; caro o suficiente para não ser reflexo.

## Alternativas rejeitadas

| Alternativa | Motivo |
|---|---|
| **Sem registro, divergência livre** | o harness perde o sentido; "está diferente de propósito" vira desculpa |
| **Paridade total, zero divergência** | contradiz o objetivo do projeto (mecânicas novas, mods nativos) |
| **Um ADR por divergência** | peso excessivo para mudanças pequenas e reversíveis; ADR é para decisão difícil de desfazer |
| **Registrar depois de implementar** | vira justificativa retroativa do que já foi feito, não decisão |
| **Delta não desligável** | quebra a promessa de "paridade como base" |

## Relacionados

- [`0002-escopo.md`](0002-escopo.md) — paridade como base, critério de sucesso
- [`docs/deltas/`](../deltas/) — registro e template
- [`docs/HARNESS.md`](../HARNESS.md) — como a divergência é demonstrada
