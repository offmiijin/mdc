# ADR 0001 — Licença

**Status:** aceito (2026-10-26); item 2 (exceção de mods) **adiado** por [0007](0007-excecao-de-mods-adiada.md)
**Substitui:** a licença MIT do esqueleto inicial (nunca publicada)

## Contexto

O projeto é uma reimplementação do Minecraft: Java Edition, distribuída
publicamente, com o objetivo declarado de aprendizado e **sem monetização**.
Isso levanta duas perguntas separadas:

1. **Perante a Mojang** — o que podemos distribuir sem violar o EULA e as Usage
   Guidelines?
2. **Perante a comunidade** — o que outras pessoas podem fazer com o nosso código?

A resposta 1 não depende da licença escolhida: o EULA proíbe *distribuir
material da Mojang*, e o projeto não distribui nenhum. A resposta 2 é decidida
inteiramente por nós.

## Decisão

1. **Código: `AGPL-3.0-only`.** Copyleft forte, com concessão de patente e
   cláusula de rede (§13). "Only" — não "or any later version".
2. **Exceção de linking para mods (AGPL §7).** Mods que usam apenas a interface
   publicada podem ter qualquer licença. Ver ADR 0005.
3. **Assets: `CC BY-SA 4.0`.** Copyleft de conteúdo, consistente com o código.
4. **Contribuições: DCO** (`git commit -s`), **sem CLA.**

## Por que não as alternativas

| Alternativa | Motivo da rejeição |
|---|---|
| **MIT** | Permitiria que qualquer fork vendesse ou fechasse o código. É incompatível com o propósito declarado do projeto, e o próprio autor seria o único obrigado pela promessa |
| **PolyForm Noncommercial** | Deixaria de ser open source (viola o critério "sem discriminação de área de atuação" da OSI); afastaria contribuidores e distros; e, sem CLA, travaria o projeto para sempre naquela licença |
| **LGPL para motor + CC BY-SA para conteúdo** | Modelo do Luanti, bom para mods, mas enfraquece o copyleft justamente no componente onde queremos reciprocidade |
| **GPL-3.0** | A cláusula de rede do AGPL é relevante: se alguém rodar um servidor modificado, o código tem de continuar aberto |
| **CLA** | Só serve para relicenciar ou vender licença comercial. Contradiz "nunca monetizar" e adiciona atrito e infraestrutura |

## Consequências

- Quem modificar e distribuir **tem de publicar o código** sob AGPL.
- Quem rodar um servidor modificado **tem de oferecer o código** aos usuários (§13).
- **Vender é permitido** (software livre pode ser vendido); **fechar não é**. Se
  o objetivo fosse proibir a venda, só a licença não comercial faria isso.
- **Com DCO não podemos relicenciar** sem o consentimento de cada contribuidor.
  Por isso a exceção de mods precisou existir **antes** da primeira contribuição.
- O selo "open source" é mantido (OSI aprova AGPL-3.0), o que ajuda adoção,
  empacotamento por distros e contribuições.

## Pendência

O texto da exceção em `NOTICE` §2 é rascunho de engenharia. **Revisar com
advogado de PI antes do primeiro release público.**
