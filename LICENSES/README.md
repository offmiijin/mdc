# Licenças de terceiros e de assets

Registro obrigatório: **nada entra no repositório sem constar aqui.**

Duas categorias, regras diferentes:

| Categoria | Licença padrão do projeto | Exceção |
|---|---|---|
| Código | `AGPL-3.0-only` | dependências de terceiros mantêm a própria licença (precisam ser compatíveis com AGPL) |
| Assets originais | `CC-BY-SA-4.0` | assets de terceiros mantêm a própria licença |

Compatibilidade com AGPL-3.0: permissivas (MIT, BSD, zlib, Apache-2.0, ISC) são
compatíveis para uso e redistribuição. **GPL-2.0-only não é.** GPL-3.0 é
(AGPL §13 / GPL §13). Na dúvida, não adote.

## Dependências de código

### Adotadas

Nenhuma é vendorizada: o repositório guarda apenas o pino de versão
(`cmake/Dependencies.cmake`).

| Dependência | Versão | Licença | Uso |
|---|---|---|---|
| SDL3 | `release-3.4.18` | zlib | janela, input, cursor, monitores, clipboard (M0) |
| doctest | `v2.4.12` | MIT | testes unitários (build-time apenas) |

### Previstas (confirmar licença no momento da adoção)

| Dependência | Versão | Licença | Uso previsto | Status |
|---|---|---|---|---|
| Vulkan-Headers | 1.4.x | Apache-2.0 | tipos e enums Vulkan (M2) | prevista |
| volk | 1.x | MIT | loader de Vulkan sem link em runtime (M2) | prevista |
| VulkanMemoryAllocator | 3.x | MIT | alocação de memória GPU (M2) | prevista |
| shaderc | 2025.x | Apache-2.0 | GLSL → SPIR-V em runtime (M2) | prevista |
| OpenAL Soft | 1.24.x | LGPL-2.1-or-later | áudio (link dinâmico) | prevista |

> `OpenAL Soft` é a única não permissiva da lista: LGPL exige link dinâmico e
> oferecer o código da biblioteca (não do seu programa). Confirmar antes de adotar;
> alternativa: `miniaudio` (domínio público / MIT-0).

## Assets de terceiros

| Asset | Origem | Licença | Obrigações | Arquivo |
|---|---|---|---|---|
| — | — | — | — | — |

Regras ao adicionar um asset:

1. Licença explícita e compatível com `CC-BY-SA-4.0` (CC0, CC-BY, CC-BY-SA, OFL).
2. Crédito com nome do autor e link para a licença.
3. Copiar o texto da licença para `LICENSES/` se ainda não existir.
4. Fontes: SIL OFL exige manter o copyright notice e proíbe vender a fonte isolada.

**Nunca:** assets extraídos do jar do Minecraft, screenshots do jogo como se
fossem arte própria, ou "recuperados" de packs sem licença. Ver `docs/LEGAL.md`.
