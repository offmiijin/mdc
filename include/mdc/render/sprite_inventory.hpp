// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "mdc/res/resource_pack.hpp"

namespace mdc::render {

/// Um sprite do atlas com o tamanho de **frame** (não o da imagem).
struct SpriteEntry {
    std::string id;
    int width{0};
    int height{0};
};

/// Inventário de um atlas: quais sprites entram e de que tamanho.
///
/// As fontes vêm do JSON do atlas (`directory` e `single`) e o tamanho do frame
/// tem duas regras que foram **medidas** no harness `atlas`, não deduzidas:
///
/// - textura animada (`.mcmeta` com `animation`): o frame é quadrado de lado
///   igual à largura da imagem — 16×512 vira 16×16 —, a menos que o `.mcmeta`
///   declare `width`/`height` (o conduíte declara `height: 32`);
/// - textura sem animação: o frame é o próprio tamanho da imagem.
///
/// O sprite sintético `missingno` faz parte do atlas que o jogo monta e é
/// adicionado aqui pelo mesmo motivo.
[[nodiscard]] std::vector<SpriteEntry> sprite_inventory(const res::ResourcePack& pack,
                                                       std::string_view atlas_id,
                                                       std::string* error = nullptr);

} // namespace mdc::render
