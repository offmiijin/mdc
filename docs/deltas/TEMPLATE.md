# NNNN — título curto do delta

- **Status:** proposto <!-- proposto | aceito | implementado | revogado -->
- **Degrau:** M? <!-- em qual marco M0–M7 isso entra -->
- **Autor:** @
- **Data:** AAAA-MM-DD

## Comportamento do original (26.3)

O que o vanilla faz, com **evidência**: caminho da classe/classe observada,
valor medido, ou harness que demonstra. "Achei que fazia X" não vale.

```
exemplo:
  física do jogador: velocidade terminal em queda = 3,92 blocos/tick
  medido por: harness player-walk, seed 5, 200 ticks
```

## Nosso comportamento

O que faremos, de forma precisa o suficiente para alguém implementar sem
perguntar nada.

## Motivo

Por que vale divergir. Se a resposta for "porque é mais fácil de implementar",
provavelmente não vale — e o certo é registrar como limitação, não como delta.

## Impacto

| Item | Valor |
|---|---|
| Perceptível pelo jogador? | sim / não |
| Desligado por padrão? | sim / não |
| Toggle | nome da opção, ou "não há" |
| Quebra save existente? | sim / não |
| Quebra compatibilidade de rede? | sim / não |

## Harness afetado

| Harness | Antes | Depois | Esperado |
|---|---|---|---|
| `player-walk` | verde | vermelho em 200/200 casos | divergência **esperada** nesse campo |

> O harness tem de **falhar exatamente aqui** e em nenhum outro lugar. Se
> falhar em outro ponto, o delta está mal delimitado.

## Alternativas consideradas

O que foi descartado e por quê. (Se nada foi descartado, o delta não foi pensado.)

## Histórico

| Data | Estado | Nota |
|---|---|---|
| AAAA-MM-DD | proposto | — |
