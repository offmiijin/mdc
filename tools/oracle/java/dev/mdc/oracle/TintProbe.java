// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.awt.image.BufferedImage;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import javax.imageio.ImageIO;
import net.minecraft.world.level.DryFoliageColor;
import net.minecraft.world.level.FoliageColor;
import net.minecraft.world.level.GrassColor;

/**
 * Harness `tint` (degrau M2): cor de grama, folhagem e folhagem seca.
 *
 * A tabela de cores (colormap) é um PNG do pack; o que este probe compara é a
 * **consulta** do jogo — o índice na tabela a partir de (temperatura,
 * precipitação). O PNG é decodificado aqui com `ImageIO` e entregue ao jogo por
 * `init(int[])`, então o harness também pega erro do nosso decodificador.
 *
 * Entrada: `grassa|foliage|dry_foliage <temperatura> <precipitação>`.
 */
public final class TintProbe {
    private static final PrintStream OUT = System.out;

    public static void main(String[] args) throws Exception {
        String assets = System.getenv("MDC_MC_ASSETS");
        if (assets == null || assets.isEmpty()) {
            throw new IllegalStateException("MDC_MC_ASSETS não definido");
        }
        String colormaps = assets + "/assets/minecraft/textures/colormap/";
        GrassColor.init(pixels(new File(colormaps + "grass.png")));
        FoliageColor.init(pixels(new File(colormaps + "foliage.png")));
        DryFoliageColor.init(pixels(new File(colormaps + "dry_foliage.png")));

        BufferedReader in = args.length == 1
            ? new BufferedReader(new FileReader(args[0], StandardCharsets.UTF_8))
            : new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));

        OUT.println("{\"schema\":1,\"harness\":\"tint\"}");
        int index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty() || line.charAt(0) == '#') {
                continue;
            }
            String[] fields = line.split("\\s+");
            double temperature = Double.parseDouble(fields[1]);
            double downfall = Double.parseDouble(fields[2]);
            int color = switch (fields[0]) {
                case "grass" -> GrassColor.get(temperature, downfall);
                case "foliage" -> FoliageColor.get(temperature, downfall);
                case "dry_foliage" -> DryFoliageColor.get(temperature, downfall);
                default -> throw new IllegalArgumentException("tipo inválido: " + fields[0]);
            };
            OUT.println("{\"case\":" + index + ",\"kind\":\"" + fields[0]
                + "\",\"temperature\":\"" + hex(temperature)
                + "\",\"downfall\":\"" + hex(downfall)
                + "\",\"color\":" + color + "}");
            index++;
        }
        OUT.println("{\"cases\":" + index + "}");
    }

    /** ARGB de cada pixel, na ordem de leitura (linha a linha). */
    private static int[] pixels(File file) throws Exception {
        BufferedImage image = ImageIO.read(file);
        if (image == null) {
            throw new IllegalStateException("não decodifiquei " + file);
        }
        int[] out = new int[image.getWidth() * image.getHeight()];
        image.getRGB(0, 0, image.getWidth(), image.getHeight(), out, 0, image.getWidth());
        return out;
    }

    /** Bits do double: a comparação é exata, sem dialeto de formatação. */
    private static String hex(double value) {
        return String.format("%016x", Double.doubleToRawLongBits(value));
    }
}
