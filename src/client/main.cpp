// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// mdc-client — M0: janela + laço de tick fixo a 20 Hz + input.
//
// Dois modos:
//   padrão      janela SDL3, deltas reais, limite de 60 fps
//   --headless  sem janela e sem espera, deltas sintéticos: verifica o relógio
//               de tick de forma determinística, inclusive em CI sem GPU.
#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mdc/core/input.hpp"
#include "mdc/core/tick_clock.hpp"

namespace {

constexpr std::int64_t kNanosecondsPerSecond = 1'000'000'000;
constexpr std::int64_t kHeadlessFrameNs = kNanosecondsPerSecond / 60; // 60 fps sintéticos
constexpr std::uint64_t kFrameBudgetNs = static_cast<std::uint64_t>(kNanosecondsPerSecond / 60);

struct Options {
    bool headless = false;
    bool allow_headless_driver = false; ///< aceitar driver dummy/offscreen em modo janela
    std::int64_t seconds = 0;           ///< 0 = sem limite de tempo
    std::int64_t ticks = 0;             ///< 0 = sem limite de ticks
    int width = 1280;
    int height = 720;
};

void print_usage(const char* program) {
    std::printf(
        "uso: %s [opções]\n"
        "  --headless         sem janela; deltas sintéticos (determinístico)\n"
        "  --seconds=N        encerra após N segundos simulados\n"
        "  --ticks=N          encerra após N ticks\n"
        "  --size=WxH         tamanho inicial da janela (padrão 1280x720)\n"
        "  --allow-headless-driver  aceita rodar 'janela' sem driver gráfico real\n"
        "  --help             esta mensagem\n",
        program);
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--headless") == 0) {
            options.headless = true;
        } else if (std::strcmp(arg, "--allow-headless-driver") == 0) {
            options.allow_headless_driver = true;
        } else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            return false;
        } else if (std::strncmp(arg, "--seconds=", 10) == 0) {
            options.seconds = std::strtoll(arg + 10, nullptr, 10);
        } else if (std::strncmp(arg, "--ticks=", 8) == 0) {
            options.ticks = std::strtoll(arg + 8, nullptr, 10);
        } else if (std::strncmp(arg, "--size=", 7) == 0) {
            if (std::sscanf(arg + 7, "%dx%d", &options.width, &options.height) != 2 ||
                options.width <= 0 || options.height <= 0) {
                std::fprintf(stderr, "valor inválido para --size: %s\n", arg + 7);
                return false;
            }
        } else {
            std::fprintf(stderr, "opção desconhecida: %s\n", arg);
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

bool reached_target(const Options& options, std::int64_t ticks, std::int64_t simulated_ns) {
    if (options.ticks > 0 && ticks >= options.ticks) {
        return true;
    }
    if (options.seconds > 0 && simulated_ns >= options.seconds * kNanosecondsPerSecond) {
        return true;
    }
    return false;
}

void report(const char* mode, const mdc::core::TickClock& clock, std::int64_t simulated_ns,
            std::int64_t frames = -1) {
    const std::int64_t expected = simulated_ns / mdc::core::TickClock::kTickDurationNs;
    const std::int64_t drift = clock.ticks() - expected;
    std::printf("[%s] ticks=%lld simulados=%.3fs esperado=%lld drift=%lld descartados=%lld",
                mode, static_cast<long long>(clock.ticks()),
                static_cast<double>(simulated_ns) / static_cast<double>(kNanosecondsPerSecond),
                static_cast<long long>(expected), static_cast<long long>(drift),
                static_cast<long long>(clock.dropped_ticks()));
    if (frames > 0) {
        const double seconds = static_cast<double>(simulated_ns) /
                               static_cast<double>(kNanosecondsPerSecond);
        std::printf(" frames=%lld fps=%.1f", static_cast<long long>(frames),
                    seconds > 0.0 ? static_cast<double>(frames) / seconds : 0.0);
    }
    std::printf("\n");
    std::fflush(stdout);
}

/// Modo determinístico: nenhuma janela, nenhuma espera, nenhum relógio real.
int run_headless(const Options& options) {
    mdc::core::TickClock clock;
    std::int64_t simulated_ns = 0;
    std::int64_t frames = 0;

    while (!reached_target(options, clock.ticks(), simulated_ns)) {
        const int ticks = clock.advance(kHeadlessFrameNs);
        for (int i = 0; i < ticks; ++i) {
            // Corpo do tick (vazio em M0).
        }
        simulated_ns += kHeadlessFrameNs;
        ++frames;
    }

    std::printf("[headless] frames=%lld ", static_cast<long long>(frames));
    fflush(stdout);
    report("headless", clock, simulated_ns);
    return clock.dropped_ticks() == 0 ? 0 : 1;
}

/// Desenha um frame e o apresenta. Sem isso, no Wayland a janela nunca é
/// mapeada: um xdg_toplevel sem buffer commitado simplesmente não aparece.
void present(SDL_Window* window, std::int64_t ticks) {
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    if (surface != nullptr) {
        // Duas tonalidades alternando a 1 Hz: prova visual de que frames saem.
        const bool bright = (ticks / mdc::core::TickClock::kTicksPerSecond) % 2 == 0;
        const Uint32 color = bright ? SDL_MapSurfaceRGB(surface, 0x18, 0x1C, 0x22)
                                    : SDL_MapSurfaceRGB(surface, 0x24, 0x2A, 0x33);
        SDL_FillSurfaceRect(surface, nullptr, color);
        SDL_UpdateWindowSurface(window);
    }

    char title[128];
    std::snprintf(title, sizeof title, "mdc — %lld ticks (%.1f s)",
                  static_cast<long long>(ticks),
                  static_cast<double>(ticks) / mdc::core::TickClock::kTicksPerSecond);
    SDL_SetWindowTitle(window, title);
}

bool driver_is_headless(const char* driver) {
    return driver != nullptr && (std::strcmp(driver, "dummy") == 0 ||
                                 std::strcmp(driver, "offscreen") == 0);
}

/// Traduz o estado do teclado em máscara de ações.
///
/// Scancode (posição física), não keycode (símbolo impresso): WASD fica no mesmo
/// lugar em ABNT2, US e AZERTY, como no jogo.
std::uint16_t read_keys() {
    const bool* keys = SDL_GetKeyboardState(nullptr);
    if (keys == nullptr) {
        return 0;
    }
    std::uint16_t mask = 0;
    if (keys[SDL_SCANCODE_W]) {
        mask |= mdc::core::mask_of(mdc::core::InputAction::kForward);
    }
    if (keys[SDL_SCANCODE_S]) {
        mask |= mdc::core::mask_of(mdc::core::InputAction::kBack);
    }
    if (keys[SDL_SCANCODE_A]) {
        mask |= mdc::core::mask_of(mdc::core::InputAction::kLeft);
    }
    if (keys[SDL_SCANCODE_D]) {
        mask |= mdc::core::mask_of(mdc::core::InputAction::kRight);
    }
    if (keys[SDL_SCANCODE_SPACE]) {
        mask |= mdc::core::mask_of(mdc::core::InputAction::kJump);
    }
    if (keys[SDL_SCANCODE_LSHIFT]) {
        mask |= mdc::core::mask_of(mdc::core::InputAction::kSneak);
    }
    if (keys[SDL_SCANCODE_LCTRL]) {
        mask |= mdc::core::mask_of(mdc::core::InputAction::kSprint);
    }
    return mask;
}

/// Prova observável de que o input chegou ao tick: uma linha por segundo
/// simulado. Sem render (M2) é o único jeito de ver tecla e mouse chegando.
void report_input(std::int64_t second, std::uint16_t keys, double mouse_dx, double mouse_dy) {
    static constexpr mdc::core::InputAction kOrder[] = {
        mdc::core::InputAction::kForward, mdc::core::InputAction::kBack,
        mdc::core::InputAction::kLeft,    mdc::core::InputAction::kRight,
        mdc::core::InputAction::kJump,    mdc::core::InputAction::kSneak,
        mdc::core::InputAction::kSprint};
    static constexpr char kGlyphs[] = {'W', 'S', 'A', 'D', 'J', 'N', 'R'};

    char held[sizeof kGlyphs + 1];
    for (std::size_t i = 0; i < sizeof kGlyphs; ++i) {
        held[i] = (keys & mdc::core::mask_of(kOrder[i])) != 0U ? kGlyphs[i] : '.';
    }
    held[sizeof kGlyphs] = '\0';

    std::printf("[input] t=%llds teclas=%s mouse=(%.1f, %.1f)\n", static_cast<long long>(second),
                held, mouse_dx, mouse_dy);
    std::fflush(stdout);
}

int run_windowed(const Options& options) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }

    const char* driver = SDL_GetCurrentVideoDriver();
    int display_count = 0;
    if (SDL_DisplayID* displays = SDL_GetDisplays(&display_count); displays != nullptr) {
        SDL_free(displays);
    }
    std::printf("[video] driver=%s displays=%d\n", driver != nullptr ? driver : "(nenhum)",
                display_count);
    std::fflush(stdout); // diagnóstico tem de aparecer na hora, não no fim do processo

    if (driver_is_headless(driver) && !options.allow_headless_driver) {
        std::fprintf(stderr,
                     "erro: SDL escolheu o driver '%s' — não há saída gráfica real.\n"
                     "  Causas comuns: rodar dentro de sandbox/container sem acesso ao socket\n"
                     "  do Wayland/X11, ou sessão sem display.\n"
                     "  Alternativas: --headless (relógio sem janela) ou SDL_VIDEODRIVER=x11.\n",
                     driver);
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("mdc", options.width, options.height, 0);
    if (window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow falhou: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);
    present(window, 0); // primeiro frame: é o que faz a janela aparecer no Wayland

    // Captura de mouse: sem ela o cursor bate na borda da janela e o olhar
    // trava nela. Falha não é fatal — driver sem suporte (offscreen/dummy) só
    // significa rodar sem captura, e isso tem de ser dito, não silenciado.
    if (SDL_SetWindowRelativeMouseMode(window, true)) {
        std::printf("[mouse] captura relativa ativa\n");
    } else {
        std::printf("[mouse] captura relativa indisponível: %s\n", SDL_GetError());
    }
    std::fflush(stdout);

    mdc::core::TickClock clock;
    mdc::core::InputBuffer input;
    std::int64_t simulated_ns = 0;
    std::uint64_t previous = SDL_GetTicksNS();
    std::uint64_t next_frame_ns = previous;
    std::int64_t frames = 0;
    std::int64_t trace_second = -1; ///< última linha de input já impressa
    std::uint16_t keys_now = 0;
    double mouse_dx = 0.0; ///< acumulado do segundo corrente
    double mouse_dy = 0.0;
    bool running = true;
    bool event_forced_redraw = false;
    int exit_code = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // Em captura relativa xrel/yrel são o movimento puro do mouse,
                // sem o cursor preso à borda e sem aceleração do sistema.
                input.add_mouse_delta(static_cast<double>(event.motion.xrel),
                                      static_cast<double>(event.motion.yrel));
            } else if (event.type == SDL_EVENT_WINDOW_EXPOSED) {
                // Compositor pediu redesenho (redimensionou, destapou): reapresenta.
                event_forced_redraw = true;
            }
        }

        // Teclado é nível: a máscara é reescrita a cada quadro, então tecla
        // solta não depende de evento nenhum ser entregue.
        input.set_keys(read_keys());

        const std::uint64_t now = SDL_GetTicksNS();
        const auto elapsed = static_cast<std::int64_t>(now - previous);
        previous = now;
        simulated_ns += elapsed > 0 ? elapsed : 0;

        const int ticks = clock.advance(elapsed);
        for (int i = 0; i < ticks; ++i) {
            // Corpo do tick: em M0 só consome o input (a física é M3).
            const mdc::core::InputFrame frame = input.consume();
            keys_now = frame.keys;
            mouse_dx += frame.mouse_dx;
            mouse_dy += frame.mouse_dy;
        }

        const std::int64_t second = clock.ticks() / mdc::core::TickClock::kTicksPerSecond;
        if (second > trace_second) {
            trace_second = second;
            report_input(second, keys_now, mouse_dx, mouse_dy);
            mouse_dx = 0.0;
            mouse_dy = 0.0;
        }
        if (ticks > 0 || event_forced_redraw) {
            present(window, clock.ticks());
            event_forced_redraw = false;
        }

        if (reached_target(options, clock.ticks(), simulated_ns)) {
            running = false;
        }

        // Limite de 60 fps: sobra do frame vira espera, não trabalho.
        ++frames;

        // Pacing por prazo: dorme só o que falta para o próximo quadro de 60 Hz.
        // Um sono fixo somaria o custo do quadro ao orçamento e derrubaria o fps.
        next_frame_ns += kFrameBudgetNs;
        const std::uint64_t after_frame = SDL_GetTicksNS();
        if (next_frame_ns > after_frame) {
            SDL_DelayNS(next_frame_ns - after_frame);
        } else {
            next_frame_ns = after_frame; // atrasado: não acumula dívida
        }
    }

    report("janela", clock, simulated_ns, frames);
    if (clock.dropped_ticks() != 0) {
        std::fprintf(stderr, "atenção: %lld ticks descartados (teto por chamada)\n",
                     static_cast<long long>(clock.dropped_ticks()));
        exit_code = 1;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}

} // namespace

int main(int argc, char** argv) {
    // Diagnóstico imediato: sem isso o stdout fica em buffer de bloco quando não
    // é um terminal e as mensagens só aparecem no fim do processo.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    Options options;
    if (!parse_options(argc, argv, options)) {
        return 2;
    }
    return options.headless ? run_headless(options) : run_windowed(options);
}
