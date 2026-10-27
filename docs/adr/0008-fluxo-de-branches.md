# ADR 0008 — `development` integra, `main` publica

**Status:** aceito (2026-10-27)

## Contexto

Um repositório com uma branch só mistura duas coisas que precisam ser distinguíveis:

- o que foi **publicado e verificado** — o que alguém clona e espera que funcione;
- o que está em **integração** — PRs abertos, degraus em andamento, harness ainda
  não verde.

Com as duas no mesmo ponteiro, "`main` está verde" deixa de dizer qualquer coisa:
um PR recém-mergeado e um degrau fechado ocupam o mesmo lugar, e a única forma de
saber o que é release seria a data do commit. Pior: nada impede que trabalho pela
metade chegue ao que as pessoas clonam.

## Decisão

**Duas branches, papéis disjuntos.**

| Branch | Papel | Commit direto | PR |
|---|---|---|---|
| `development` | branch de desenvolvimento: base de todo trabalho, destino de **todos** os PRs | não | **sim, sempre** |
| `main` | espelho do que foi publicado | não | **nunca** |

Regras que decorrem disso:

1. **Todo PR aponta para `development`.** PR com base em `main` é redirecionado
   para `development` — não é mergeado em `main`.
2. **`main` avança só por promoção de `development`**, feita por mantenedor em
   momento de release, com o gate verde (`core` + `compliance`). Publicar é
   decisão, não consequência de merge.
3. **Trabalho novo sai de branch curta** (`feat/`, `fix/`, `ci/`, `docs/`,
   `test/`) criada a partir de `development`, com o número da tarefa no fim do
   nome — o formato que o hook de commit aceita.
4. **Nenhuma das duas recebe commit direto.** Alteração entra por PR, e o gate
   roda em toda branch: a barreira é a mesma em qualquer caminho.

## Consequências

- `main` significa "isto foi publicado com o gate verde".
- Um PR pela metade pode ser mergeado sem contaminar o que as pessoas clonam.
- A escrita em `main` tem um único caminho: promoção de `development`
  (PR `development` → `main` no momento de release).
- Custo: um passo a mais em cada promoção e a obrigação de manter `development`
  verde — caso contrário a promoção nunca acontece.

## Alternativas rejeitadas

| Alternativa | Por que não |
|---|---|
| **Branch única (`main`)** | integração e publicação no mesmo ponteiro; `main` deixa de ser sinal de "publicado e verde" |
| **`main` + feature branches, sem `development`** | PR mergeado passa a ser publicação imediata; falta o estágio onde a integração de vários PRs é testada junta antes de virar release |
| **`develop` (nome do GitFlow)** | `development` é o nome que este repositório usa; traduzir por hábito de outro projeto só quebra link e memória muscular |
| **Release por tag, sem `main`** | tag não resolve "o que está em integração"; o problema é separar os dois papéis, não versionar |
