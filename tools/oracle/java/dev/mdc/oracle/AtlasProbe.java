// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.TreeMap;
import net.minecraft.SharedConstants;
import net.minecraft.network.chat.Component;
import net.minecraft.client.renderer.texture.SpriteContents;
import net.minecraft.client.renderer.texture.atlas.SpriteResourceLoader;
import net.minecraft.client.renderer.texture.atlas.SpriteSource;
import net.minecraft.client.renderer.texture.atlas.SpriteSourceList;
import net.minecraft.client.renderer.texture.atlas.SpriteSources;
import net.minecraft.resources.Identifier;
import net.minecraft.server.Bootstrap;
import net.minecraft.server.packs.PackType;
import net.minecraft.server.packs.PackLocationInfo;
import net.minecraft.server.packs.PathPackResources;
import net.minecraft.server.packs.repository.PackSource;
import net.minecraft.server.packs.resources.MultiPackResourceManager;
import net.minecraft.server.packs.resources.ResourceManager;

/**
 * Harness `atlas` (degrau M2): inventário de sprites do atlas.
 *
 * Compara **quais** sprites entram e **de que tamanho** — o layout de posições
 * (Stitcher) é outra metade do degrau. Sem isso a comparação de UV do modelo
 * não tem base.
 *
 * Entrada: uma linha por atlas (`minecraft:blocks`). O `assets/` extraído do
 * jar vem por `MDC_MC_ASSETS` — o probe nunca baixa nem empacota asset.
 */
public final class AtlasProbe {
    /** `System.out` capturado antes do bootstrap: o jogo embrulha o stream. */
    private static final PrintStream OUT = System.out;

    public static void main(String[] args) throws Exception {
        String assetsPath = System.getenv("MDC_MC_ASSETS");
        if (assetsPath == null || assetsPath.isEmpty()) {
            throw new IllegalStateException("MDC_MC_ASSETS não definido");
        }
        // O log do jogo sai no mesmo stdout e embrulha a transcrição; o harness
        // compara bytes, então o appender é desligado antes de qualquer carga.
        org.apache.logging.log4j.core.config.Configurator.setRootLevel(
            org.apache.logging.log4j.Level.OFF);
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
        // O código de despacho das fontes de sprite só existe depois deste
        // registro; sem ele o atlas não é nem parseado.
        SpriteSources.bootstrap();

        ResourceManager resources = new MultiPackResourceManager(PackType.CLIENT_RESOURCES,
            List.of(new PathPackResources(
                new PackLocationInfo("assets", Component.literal("atlas probe"), PackSource.BUILT_IN,
                                     Optional.empty()),
                Path.of(assetsPath))));

        BufferedReader in = args.length == 1
            ? new BufferedReader(new FileReader(args[0], StandardCharsets.UTF_8))
            : new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));

        OUT.println("{\"schema\":1,\"harness\":\"atlas\"}");
        int index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty() || line.charAt(0) == '#') {
                continue;
            }
            dump(index++, Identifier.parse(line), resources);
        }
        OUT.println("{\"cases\":" + index + "}");
    }

    private static void dump(int index, Identifier atlas, ResourceManager resources) {
        SpriteSourceList sources = SpriteSourceList.load(resources, atlas);
        SpriteResourceLoader loader = SpriteResourceLoader.create(Set.of());

        Map<String, int[]> sprites = new TreeMap<>();
        for (SpriteSource.Loader source : sources.list(resources)) {
            try (SpriteContents contents = source.get(loader)) {
                if (contents != null) {
                    sprites.put(contents.name().toString(), new int[] {contents.width(), contents.height()});
                }
            }
        }

        StringBuilder out = new StringBuilder();
        out.append("{\"case\":").append(index).append(",\"atlas\":\"").append(atlas)
           .append("\",\"count\":").append(sprites.size()).append(",\"sprites\":[");
        boolean first = true;
        for (Map.Entry<String, int[]> entry : sprites.entrySet()) {
            if (!first) {
                out.append(',');
            }
            first = false;
            out.append("[\"").append(entry.getKey()).append("\",")
               .append(entry.getValue()[0]).append(',').append(entry.getValue()[1]).append(']');
        }
        out.append("]}");
        OUT.println(out);
    }
}
