# ADR 0008 — `development` integra, `main` publica

**Status:** aceito (2026-10-27)

## Contexto

O projeto nasceu com um único commit em `main` e a regra prática "enquanto não
houver push, corrija com `--amend`". Isso já não vale: `main` foi publicada, e
`--amend` nela reescreveria histórico que outras pessoas têm.

Faltava declarar **onde o trabalho entra**. Com uma branch só, duas coisas
diferentes ficam indistinguíveis no mesmo ponteiro:

- o que já foi **publicado e verificado** (o que alguém pode clonar e esperar
  que funcione);
- o que está em **integração** (PRs abertos, degraus em andamento, harness
  ainda não verde).

Misturar as duas significa que "main está verde" deixa de dizer qualquer coisa:
um PR recém-mergeado e um degrau fechado ficam no mesmo lugar, e a única forma
de saber o que é release seria a data do commit.

Havia também um efeito mecânico: os PRs #1 e #2 foram abertos contra `main`
enquanto o degrau M0 estava em andamento. Funcionou, mas nada impedia que um
PR pela metade fosse mergeado direto no que as pessoas clonam.

## Decisão

**Duas branches, papéis disjuntos.**

| Branch | Papel | Commit direto | PR |
|---|---|---|---|
| `development` | branch de desenvolvimento: base de todo trabalho, destino de **todos** os PRs | não | **sim, sempre** |
| `main` | espelho do que foi publicado | não | **nunca** |

Regras que decorrem disso:

1. **Todo PR aponta para `development`.** PR aberto contra `main` é fechado e
   reaberto apontando para `development` — não é mergeado na `main`.
2. **`main` avança só por promoção de `development`**, feita por mantenedor em
   momento de release, com o gate verde (`core` + `compliance`). Não é fluxo
   automático: publicar é decisão, não consequência de merge.
3. **Trabalho novo sai de branch curta** (`feat/`, `fix/`, `ci/`, `docs/`,
   `test/`) criada a partir de `development`, com o número da tarefa no fim do
   nome — é o formato que o hook de commit aceita.
4. **Nenhuma das duas recebe commit direto.** Alteração entra por PR; o gate
   roda em toda branch, então a barreira é a mesma em qualquer caminho.

## Consequências

- `main` volta a significar algo: "isto foi publicado com o gate verde".
- Um PR pela metade pode ser mergeado sem contaminar o que as pessoas clonam.
- Ajuste no workflow: PR de release (`development` → `main`) é exceção
  consciente e documentada, o único caminho de escrita em `main`.
- Custo: um passo a mais em cada promoção (merge `development` → `main`) e a
  obrigação de manter `development` verde — caso contrário a promoção nunca
  acontece.

## Alternativas rejeitadas

| Alternativa | Por que não |
|---|---|
| **Branch única (`main`)** | é o estado anterior: integração e publicação no mesmo ponteiro; `main` deixa de ser sinal de "publicado e verde" |
| **`main` + feature branches, sem `development`** | PRs mergeados passam a ser publicação imediata; falta o estágio onde a integração de vários PRs é testada junta antes de virar release |
| **`develop` (nome do GitFlow)** | `development` já é o nome usado neste repositório desde o primeiro ADR; trocar gera confusão por motivo cosmético |
| **Release por tag, sem `main`** | tags não resolvem "o que está em integração"; o problema era separar os dois papéis, não versionar |
