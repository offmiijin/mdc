// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Testes do leitor de pack: JSON e ZIP. O teste contra o jar do jogo é
// condicional (`MDC_MC_JAR`), porque o jar não é versionado nem vem no CI.
#include <doctest/doctest.h>

#include <cstdlib>
#include <string>

#include "mdc/res/json.hpp"
#include "mdc/res/png.hpp"
#include "mdc/res/resource_pack.hpp"
#include "mdc/res/zip.hpp"

using mdc::res::JsonValue;
using mdc::res::ResourcePack;
using mdc::res::ZipArchive;

TEST_CASE("JSON: objeto, array, número, escape e ordem dos membros") {
    const auto json = JsonValue::parse(R"({
        "parent": "block/cube_all",
        "textures": {"all": "block/stone"},
        "rotation": {"x": 90, "y": -270.5},
        "uv": [0, 0, 16, 16],
        "cullface": null,
        "tintindex": true,
        "note": "linha\ncom \"aspas\" e \u00e9"
    })");
    REQUIRE(json.has_value());
    CHECK(json->is_object());
    CHECK(json->size() == 7);
    CHECK(json->key_at(0) == "parent");
    CHECK(json->string_or("parent", "") == "block/cube_all");
    const JsonValue* textures = json->find("textures");
    REQUIRE(textures != nullptr);
    CHECK(textures->string_or("all", "") == "block/stone");
    const JsonValue* rotation = json->find("rotation");
    REQUIRE(rotation != nullptr);
    CHECK(rotation->number_or("x", 0) == 90.0);
    CHECK(rotation->number_or("y", 0) == -270.5);
    const JsonValue* uv = json->find("uv");
    REQUIRE(uv != nullptr);
    CHECK(uv->is_array());
    CHECK(uv->size() == 4);
    CHECK(uv->at(3).as_number() == 16.0);
    CHECK(json->find("cullface")->is_null());
    CHECK(json->bool_or("tintindex", false));
    CHECK(json->string_or("note", "") == "linha\ncom \"aspas\" e \xc3\xa9");
}

TEST_CASE("JSON: entrada inválida volta como nulo, com mensagem") {
    std::string error;
    CHECK_FALSE(JsonValue::parse("{", &error).has_value());
    CHECK_FALSE(error.empty());
    CHECK_FALSE(JsonValue::parse("{\"a\": 1} lixo").has_value());
    CHECK_FALSE(JsonValue::parse("[1,]").has_value());
    CHECK_FALSE(JsonValue::parse("").has_value());
}

TEST_CASE("ZIP: não é ZIP, o erro é explícito") {
    std::string error;
    CHECK_FALSE(ZipArchive::open("/nao/existe.zip", &error).has_value());
    CHECK_FALSE(error.empty());
}

TEST_CASE("pack do jogo: abre o jar e lê um blockstate (condicional)") {
    std::string error;
    const auto pack = ResourcePack::from_environment(&error);
    if (!pack.has_value()) {
        MESSAGE("MDC_MC_JAR ausente: teste do jar pulado (" << error << ")");
        return;
    }
    const auto blockstate = pack->blockstate("minecraft:stone", &error);
    REQUIRE_MESSAGE(blockstate.has_value(), error);
    CHECK(blockstate->contains("variants"));

    const auto model = pack->model("minecraft:block/cube_all", &error);
    REQUIRE_MESSAGE(model.has_value(), error);
    CHECK(model->string_or("parent", "").starts_with("block/"));

    // Textura: binário no ZIP, lido como bytes crus.
    const auto png = pack->read_text("minecraft", "textures/block/stone.png", &error);
    REQUIRE_MESSAGE(png.has_value(), error);
    CHECK(png->size() > 8);
    CHECK((*png)[1] == 'P');
}

TEST_CASE("PNG: decodifica o pack do usuário (condicional ao jar)") {
    std::string error;
    const auto pack = ResourcePack::from_environment(&error);
    if (!pack.has_value()) {
        MESSAGE("MDC_MC_JAR ausente: teste de PNG pulado (" << error << ")");
        return;
    }

    const auto colormap = pack->read_text("minecraft", "textures/colormap/grass.png", &error);
    REQUIRE_MESSAGE(colormap.has_value(), error);
    const auto image = mdc::res::decode_png(*colormap, &error);
    REQUIRE_MESSAGE(image.has_value(), error);
    CHECK(image->width == 256);
    CHECK(image->height == 256);
    CHECK(image->rgba.size() == 256U * 256U * 4U);
    // Canal alfa vem preenchido mesmo em PNG sem transparência.
    CHECK(image->rgba[3] == 0xFF);

    // PNG RGBA (transparência de verdade) e PNG não quadrado: o decodificador
    // não assume simetria. Textura indexada/cinza ainda não é suportada — entra
    // quando o M2.5 precisar de pixel de bloco; aqui o alvo é o colormap.
    const auto rgba = pack->read_text("minecraft", "textures/block/acacia_trapdoor.png", &error);
    REQUIRE_MESSAGE(rgba.has_value(), error);
    const auto trapdoor = mdc::res::decode_png(*rgba, &error);
    REQUIRE_MESSAGE(trapdoor.has_value(), error);
    CHECK(trapdoor->width == 16);
    CHECK(trapdoor->height == 16);

    const auto wide = pack->read_text("minecraft", "textures/block/fire_0.png", &error);
    REQUIRE_MESSAGE(wide.has_value(), error);
    const auto strip = mdc::res::decode_png(*wide, &error);
    REQUIRE_MESSAGE(strip.has_value(), error);
    CHECK(strip->width == 16);
    CHECK(strip->height == 512);

    // Entrada que não é PNG é recusada, não "quase decodificada".
    CHECK_FALSE(mdc::res::decode_png("nao e png", &error).has_value());
    CHECK_FALSE(error.empty());
}
