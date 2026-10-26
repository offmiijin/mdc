# ADR 0004 — Stack de implementação

**Status:** aceito (2026-10-26)

## Contexto

Antes da primeira linha de C++ era preciso decidir linguagem/padrão, build,
camada de plataforma, camada de GPU, testes e plataforma-alvo. A decisão foi
tomada **contra evidência do próprio 26.3**, não contra preferência.

O que o `client.jar` 26.3 realmente usa (medido na árvore decompilada):

| Camada | Tecnologia | Evidência |
|---|---|---|
| Plataforma (janela, input, cursor, monitores, clipboard, messagebox) | **SDL3** | 25 arquivos importam `org.lwjgl.sdl`, todos em `com/mojang/blaze3d/platform/` (`SDLEventHandler`, `MonitorManager`, `ClipboardManager`, `CursorType`, `MessageBox`) |
| Janela | `SDL_CreateWindow` | 2 usos |
| GPU | **Vulkan** + backend **OpenGL** | 29 arquivos em `renderpearl/backend/vulkan/`, 15 com `org.lwjgl.opengl` |
| Abstração gráfica de terceiro | nenhuma (RHI própria) | zero referências a `SDL_GPU` |
| Shaders | `shaderc` em runtime | 3 arquivos |
| Áudio | OpenAL | `lwjgl-openal` |
| **GLFW** | **não usa** | 0 imports; `lwjgl-glfw` ausente das 114 dependências |

## Decisão

| Item | Escolha |
|---|---|
| Linguagem | **C++23** |
| Build | **CMake** + presets; dependências por **FetchContent** |
| Plataforma | **SDL3** (janela, input, cursor, monitor, clipboard) |
| GPU | **Vulkan direto** via `volk` + **VMA**, atrás de **RHI própria** (espelhando `renderpearl`) |
| Shaders | `shaderc` em runtime (GLSL → SPIR-V), como o original |
| Testes | **doctest** |
| Harness diferencial | probe em Java (JDK 25) + comparação bit a bit — ver `docs/HARNESS.md` |
| Alvo de plataforma | **Linux x86-64 + Vulkan primeiro**; macOS/Windows depois |
| Licença | AGPL-3.0-only (ADR 0001) |

## Por que não as alternativas

| Alternativa | Motivo da rejeição |
|---|---|
| **GLFW** | mais leve, mas é um desvio da camada do original: não traz clipboard completo, messagebox, gestão de monitores nem cursor de sistema — tudo isso teria de ser reescrito ou perdido |
| **SDL3 + SDL_GPU** | desvio duplo: `SDL_GPU` é uma abstração sobre Vulkan/Metal/D3D12, e nós queremos a **nossa** RHI. O original não usa `SDL_GPU` |
| **Vulkan sem camada de plataforma** | exigiria escrever X11 + Wayland + Win32 + Cocoa antes do primeiro triângulo (~1.000 linhas) |
| **vcpkg** | melhor para dependências pesadas, mas exige bootstrap; FetchContent resolve as 7 dependências previstas. Migrar quando `shaderc`/`spirv-tools` doerem no CI |
| **Catch2** | mais features, compilação mais lenta; o volume de testes aqui é de comportamento determinístico, não de mocking |
| **GoogleTest + GMock** | seria a escolha se houvesse muitos mocks de RHI; os testes principais comparam estado, não interação |
| **Cross-platform desde o início** | metade dos projetos nesse estágio morre por isso; Linux + Vulkan first, porte depois |
| **OpenGL primeiro** | o backend Vulkan é o caminho novo e o alvo do próprio 26.x; GL entra como backend secundário da RHI, não como base |

## Consequências

- A RHI é o componente mais caro do M0 e o que mais se parece com o original —
  é o que permite comparar render de verdade.
- `volk` evita link em tempo de carga contra `libvulkan.so`, o que facilita rodar
  em máquinas sem driver (llvmpipe, CI).
- FetchContent compila tudo do zero: o primeiro build será lento. Aceitável até
  `shaderc` entrar; aí a decisão volta (novo ADR).
- Sem GLFW, o "messagebox" e o cursor do sistema vêm de graça — e é exatamente o
  conjunto de classes que o `blaze3d/platform/` do original tem.

## Pendências

- Confirmar `OpenAL Soft` (LGPL-2.1+) vs `miniaudio` (domínio público) quando o
  áudio entrar — ver `LICENSES/README.md`.
- JDK **25** é obrigatório para o probe do harness (26.3 é class file v69;
  `javac 21` falha com *"class file has wrong version 69.0, should be 65.0"*).
