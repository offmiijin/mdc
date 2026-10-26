# ADR 0002 — Escopo e critério de sucesso

**Status:** aceito (2026-10-26)

## Contexto

O objetivo declarado é "entregar o Minecraft", mas a implementação é nossa e
pode divergir do comportamento original. Sem critério objetivo, "pronto" deixa
de existir: sempre haverá uma opinião a mudar.

A medição do `client.jar` 26.3 dá a escala do alvo: **774.696 linhas de Java,
7.301 arquivos, 565 pacotes**, mais 114 bibliotecas de terceiros.

## Decisão

1. **Reimplementação independente, não um "mod".** Um mod é um delta sobre o
   binário original e o EULA proíbe distribuir "Modded Versions". O que
   distribuímos é um programa independente, que exige cópia legal do jogo.
   "Modificação" descreve **o que mudamos em relação ao original**, nunca
   classifica o produto.
2. **Paridade como base.** Todo subsistema nasce medido contra o original por
   harness diferencial (ver `docs/HARNESS.md`, lote 3).
3. **Divergência como camada declarada.** Toda mudança intencional de
   comportamento entra em `docs/deltas/` com: o que o vanilla faz, o que
   fazemos, por quê, se há toggle, e quais harnesses devem divergir.
4. **Versão de referência: 26.3** (ver ADR 0003).

## Matriz de topologias

| Topologia | Precisa | Escopo |
|---|---|---|
| Singleplayer (cliente + servidor integrado) | só o jogo | **dentro — primeiro** |
| LAN (hospedar e entrar na rede local) | protocolo de play, sem cifra | **dentro** |
| Servidor dedicado offline-mode | protocolo 777 completo | **dentro** |
| Online-mode / servidores públicos | login Microsoft/Xbox (MSA, XSTS, chaves de perfil) | **fora** |
| Realms / Marketplace | ser parceiro da Mojang | **fora (impossível)** |
| Cross-play Bedrock | outro protocolo, outro código | **fora** |

Fora de escopo não é "talvez depois": é escopo **recusado**, para não prometer o
que não se entrega.

## Consequências

- Temos um critério binário por degrau: ou o harness bate, ou não bate.
- Um delta não registrado é bug — não feature.
- O cliente vem antes do servidor porque **é superconjunto**: as 7.762 classes do
  servidor dedicado estão integralmente dentro das 11.383 do cliente (0
  exclusivas). Fazer o cliente significa escrever a simulação uma vez só.
- Sem login Microsoft, não entramos em servidor público e não podemos ser
  banidos por anticheat — o que também é uma vantagem para um projeto de estudo.

## Alternativas rejeitadas

| Alternativa | Motivo |
|---|---|
| "Jogo próprio inspirado" | perde o oráculo (não há com o que comparar) e o critério de pronto |
| Paridade 100% rígida, sem deltas | impede qualquer mudança própria, contradiz o objetivo |
| Começar pelo servidor dedicado | reescreveria a simulação duas vezes |
| Perseguir a versão mais recente | nunca termina nada |
