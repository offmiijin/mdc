// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
#include "mdc/res/resource_pack.hpp"

#include <cstdlib>

namespace mdc::res {

ResourceId split_id(std::string_view id) {
    ResourceId result;
    const std::size_t colon = id.find(':');
    if (colon == std::string_view::npos) {
        result.path = std::string(id);
        return result;
    }
    result.namespace_id = std::string(id.substr(0, colon));
    result.path = std::string(id.substr(colon + 1));
    return result;
}

std::optional<ResourcePack> ResourcePack::open_jar(const std::string& jar_path, std::string* error) {
    std::optional<ZipArchive> archive = ZipArchive::open(jar_path, error);
    if (!archive.has_value()) {
        return std::nullopt;
    }
    ResourcePack pack;
    pack.archive_ = std::move(*archive);
    return pack;
}

std::optional<ResourcePack> ResourcePack::from_environment(std::string* error) {
    const char* jar = std::getenv("MDC_MC_JAR");
    if (jar == nullptr || *jar == '\0') {
        if (error != nullptr) {
            *error = "MDC_MC_JAR não definido";
        }
        return std::nullopt;
    }
    return open_jar(jar, error);
}

std::optional<std::string> ResourcePack::read_text(std::string_view namespace_id,
                                                   std::string_view path,
                                                   std::string* error) const {
    std::string full = "assets/";
    full += namespace_id;
    full += '/';
    full += path;
    return archive_.read(full, error);
}

std::optional<JsonValue> ResourcePack::read_json(std::string_view namespace_id,
                                                 std::string_view path, std::string* error) const {
    std::optional<std::string> text = read_text(namespace_id, path, error);
    if (!text.has_value()) {
        return std::nullopt;
    }
    return JsonValue::parse(*text, error);
}

std::optional<JsonValue> ResourcePack::blockstate(std::string_view id, std::string* error) const {
    const ResourceId resource = split_id(id);
    std::string path = "blockstates/";
    path += resource.path;
    path += ".json";
    return read_json(resource.namespace_id, path, error);
}

std::optional<JsonValue> ResourcePack::model(std::string_view id, std::string* error) const {
    const ResourceId resource = split_id(id);
    std::string path = "models/";
    path += resource.path;
    path += ".json";
    return read_json(resource.namespace_id, path, error);
}

} // namespace mdc::res
