# Registros de decisão de arquitetura (ADRs)

Formato: contexto → decisão → consequências → alternativas rejeitadas.
Um ADR é **imutável**: para mudar uma decisão, escreva um novo ADR que a substitui.

| # | Decisão | Status |
|---|---|---|
| [0001](0001-licenca.md) | Licença: AGPL-3.0-only, DCO, exceção de mods, assets CC BY-SA 4.0 | aceito |
| [0002](0002-escopo.md) | Escopo: reimplementação independente, paridade como base, deltas declarados | aceito |
| [0003](0003-versao.md) | Versão de referência: 26.3 congelada | aceito |
| [0004](0004-stack.md) | Stack: C++23, CMake+FetchContent, SDL3+Vulkan+RHI própria, doctest | aceito |
| [0005](0005-mods.md) | Fronteira de mods: C ABI + processo/WASM (licença adiada por 0007) | aceito |
| [0006](0006-deltas.md) | Política de divergência: delta registrado, testado e reversível | aceito |
| [0007](0007-excecao-de-mods-adiada.md) | Exceção de mods adiada: publicar AGPL puro, conceder junto com a ABI | aceito |

## O que **não** é um ADR

Decisões de implementação reversíveis em uma tarde (nome de variável, layout de
pasta, qual container usar) não precisam de ADR. Regra prática: se desfazer
custa mais de um dia ou mais de uma pessoa, escreva o ADR.
