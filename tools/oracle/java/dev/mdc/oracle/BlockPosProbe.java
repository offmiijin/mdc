// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import net.minecraft.core.BlockPos;
import net.minecraft.world.phys.Vec3;

/**
 * Probe de paridade de {@code BlockPos.containing} e vizinhos.
 *
 * Casos por linha: {@code x y z}. Saída: o bloco que contém o ponto, os
 * deslocamentos acima/abaixo, e o empacotamento em {@code long}.
 *
 * Regras desta pasta (tools/oracle/README.md): {@code package dev.mdc.*} e a
 * saída vai para .local/fixtures/.
 */
public final class BlockPosProbe {

    private BlockPosProbe() {}

    public static void main(String[] args) throws Exception {
        final String path = args.length > 0 ? args[0] : "-";
        final BufferedReader in = new BufferedReader(new InputStreamReader(
                "-".equals(path) ? System.in : new FileInputStream(path), StandardCharsets.UTF_8));
        final PrintStream out = new PrintStream(
                new java.io.BufferedOutputStream(new java.io.FileOutputStream(java.io.FileDescriptor.out)),
                false, StandardCharsets.UTF_8);

        out.println("{\"schema\":1,\"harness\":\"blockpos\"}");

        long index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            final String trimmed = line.trim();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                continue;
            }
            final String[] parts = trimmed.split("\\s+");
            if (parts.length != 3) {
                throw new IllegalArgumentException("linha " + index + ": esperados 3 campos, vieram " + parts.length);
            }

            final BlockPos pos = BlockPos.containing(new Vec3(Double.parseDouble(parts[0]),
                    Double.parseDouble(parts[1]), Double.parseDouble(parts[2])));

            final StringBuilder sb = new StringBuilder(96);
            sb.append("{\"case\":").append(index);
            sb.append(",\"pos\":[").append(pos.getX()).append(',').append(pos.getY()).append(',')
                    .append(pos.getZ()).append(']');
            sb.append(",\"above\":").append(pos.above().asLong());
            sb.append(",\"below\":").append(pos.below().asLong());
            sb.append(",\"as_long\":").append(pos.asLong());
            sb.append('}');
            out.println(sb);
            ++index;
        }

        out.println("{\"cases\":" + index + "}");
        out.flush();
    }
}
