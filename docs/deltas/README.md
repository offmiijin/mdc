# Deltas — divergências intencionais do original

O projeto nasce com **paridade** como base. Toda diferença de comportamento em
relação ao 26.3 é uma decisão registrada aqui — não um capricho espalhado no
código.

Política em [`../adr/0006-deltas.md`](../adr/0006-deltas.md).

## O que é (e o que não é) um delta

| É delta | Não é delta |
|---|---|
| mudar uma regra de física porque preferimos assim | bug de implementação |
| menu próprio, atalho extra, opção nova | funcionalidade ainda não implementada |
| corrigir algo que o vanilla faz e consideramos errado | arredondamento nosso que diverge por 1 ULP |
| distância de render maior que a do vanilla | falta de um recurso do degrau atual |

Regra de bolso: **se o harness ficou vermelho e você decidiu que está certo
assim, é delta.** Se ficou vermelho e você não sabe por quê, é bug.

## Ciclo de vida

```
proposto → aceito → implementado → (revogado)
```

| Estado | Significa |
|---|---|
| `proposto` | escrito antes do código; ainda em discussão |
| `aceito` | aprovado; o harness já pode esperar a divergência |
| `implementado` | no código, com teste que demonstra a divergência |
| `revogado` | voltamos ao comportamento do vanilla; o arquivo permanece como histórico |

Delta **nunca é apagado** — é revogado. O histórico de "o que já tentamos mudar"
é parte do valor do projeto.

## Numeração

`NNNN-slug-em-minusculas.md`, sequencial, sem reaproveitar número:

```
deltas/0001-iluminacao-suave-opcional.md
deltas/0002-distancia-de-render-1024.md
```

## Regras

1. **Um delta por arquivo.** Delta com dois assuntos é dois deltas.
2. **Escrito antes do merge**, nunca depois.
3. **Desligado por padrão** quando muda aparência ou comportamento que o
   jogador percebe. Quem quer vanilla, tem vanilla.
4. **Com harness.** O delta precisa de um teste que *demonstre* a divergência —
   e que falhe se ela desaparecer sem aviso.
5. **Reversível.** Se não dá para desligar sem reescrever, não é delta: é um
   fork de comportamento, e precisa de ADR.
6. **Sem material da Mojang.** Delta não é desculpa para encostar asset do jogo
   (ver [`../LEGAL.md`](../LEGAL.md)).

## Template

Copie [`TEMPLATE.md`](TEMPLATE.md) e preencha. Campo vazio é campo que não foi
pensado.
