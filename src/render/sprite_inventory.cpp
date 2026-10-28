// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/render/sprite_inventory.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

namespace mdc::render {

namespace {

struct ImageSize {
    int width{0};
    int height{0};
};

/// Lê só o IHDR do PNG: largura e altura estão nos bytes 16..23, big-endian.
[[nodiscard]] bool png_size(std::string_view bytes, ImageSize& size) {
    if (bytes.size() < 24) {
        return false;
    }
    const auto read_u32 = [&bytes](std::size_t offset) {
        return (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset])) << 24U) |
               (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 16U) |
               (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 8U) |
               static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3]));
    };
    size.width = static_cast<int>(read_u32(16));
    size.height = static_cast<int>(read_u32(20));
    return size.width > 0 && size.height > 0;
}

/// Tamanho do frame: a regra do `.mcmeta` de animação (medida no harness).
[[nodiscard]] ImageSize frame_size(const res::ResourcePack& pack, const std::string& namespace_id,
                                  const std::string& texture_path, ImageSize image,
                                  std::string* error) {
    // O `.mcmeta` acompanha o PNG: o sufixo vem **depois** de `.png`.
    std::string metadata_path = "textures/";
    metadata_path += texture_path;
    metadata_path += ".png.mcmeta";
    const std::optional<res::JsonValue> metadata = pack.read_json(namespace_id, metadata_path, error);
    if (!metadata.has_value()) {
        return image;
    }
    const res::JsonValue* animation = metadata->find("animation");
    if (animation == nullptr || !animation->is_object()) {
        return image;
    }

    ImageSize frame = image;
    const res::JsonValue* width = animation->find("width");
    const res::JsonValue* height = animation->find("height");
    if (width != nullptr && width->is_number()) {
        frame.width = width->as_int();
    }
    if (height != nullptr && height->is_number()) {
        frame.height = height->as_int();
    } else if (image.width > 0 && image.height % image.width == 0) {
        // Sem `height` declarado: frame quadrado do lado da largura (tira
        // vertical de quadros).
        frame.height = image.width;
    }
    return frame;
}

void add_texture(const res::ResourcePack& pack, const std::string& namespace_id,
                 const std::string& texture_path, const std::string& sprite_id,
                 std::vector<SpriteEntry>& out, std::string* error) {
    std::string png_path = "textures/";
    png_path += texture_path;
    png_path += ".png";
    const std::optional<std::string> bytes = pack.read_text(namespace_id, png_path, error);
    ImageSize image;
    if (!bytes.has_value() || !png_size(*bytes, image)) {
        return;
    }
    const ImageSize frame = frame_size(pack, namespace_id, texture_path, image, error);
    out.push_back(SpriteEntry{sprite_id, frame.width, frame.height});
}

} // namespace

std::vector<SpriteEntry> sprite_inventory(const res::ResourcePack& pack, std::string_view atlas_id,
                                         std::string* error) {
    const res::ResourceId atlas = res::split_id(atlas_id);
    std::string atlas_path = "atlases/";
    atlas_path += atlas.path;
    atlas_path += ".json";

    std::vector<SpriteEntry> sprites;
    const std::optional<res::JsonValue> definition = pack.read_json(atlas.namespace_id, atlas_path, error);
    if (!definition.has_value()) {
        return sprites;
    }
    const res::JsonValue* sources = definition->find("sources");
    if (sources == nullptr || !sources->is_array()) {
        return sprites;
    }

    for (std::size_t i = 0; i < sources->size(); ++i) {
        const res::JsonValue& source = sources->at(i);
        const std::string_view type = source.string_or("type", "");
        if (type == "minecraft:single") {
            const res::ResourceId texture = res::split_id(source.string_or("resource", ""));
            add_texture(pack, texture.namespace_id, texture.path,
                        texture.namespace_id + ":" + texture.path, sprites, error);
            continue;
        }
        if (type != "minecraft:directory") {
            continue;
        }

        // `directory`: todo PNG sob `textures/<source>/`; o id é `prefix` + o
        // caminho relativo, sem o `.png`.
        const std::string prefix = std::string(source.string_or("prefix", ""));
        const std::string directory = std::string(source.string_or("source", ""));
        std::string root = "assets/";
        root += atlas.namespace_id;
        root += "/textures/";
        root += directory;
        root += '/';

        for (const std::string& name : pack.archive().names()) {
            if (!name.starts_with(root) || !name.ends_with(".png")) {
                continue;
            }
            const std::string relative = name.substr(root.size());
            const std::string texture_path = directory + "/" + relative.substr(0, relative.size() - 4);
            add_texture(pack, atlas.namespace_id, texture_path,
                        atlas.namespace_id + ":" + prefix + relative.substr(0, relative.size() - 4),
                        sprites, error);
        }
    }

    // O jogo sempre inclui o sprite de fallback no atlas (16×16).
    sprites.push_back(SpriteEntry{"minecraft:missingno", 16, 16});

    std::sort(sprites.begin(), sprites.end(),
              [](const SpriteEntry& a, const SpriteEntry& b) { return a.id < b.id; });
    sprites.erase(std::unique(sprites.begin(), sprites.end(),
                              [](const SpriteEntry& a, const SpriteEntry& b) { return a.id == b.id; }),
                  sprites.end());
    return sprites;
}

} // namespace mdc::render
