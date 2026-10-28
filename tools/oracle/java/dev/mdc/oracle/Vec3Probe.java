// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import net.minecraft.world.phys.Vec3;

/**
 * Probe de paridade de {@code Vec3}.
 *
 * Lê casos (texto, um por linha: {@code ax ay az bx by bz alpha}) e escreve uma
 * transcrição JSONL com os resultados do jogo original em <b>bits</b>, no mesmo
 * formato e na mesma ordem de campos que o lado C++.
 *
 * Regras desta pasta (ver tools/oracle/README.md): {@code package dev.mdc.*},
 * apenas <i>chama</i> a API do jogo, e a saída vai para .local/fixtures/.
 */
public final class Vec3Probe {

    private Vec3Probe() {}

    public static void main(String[] args) throws Exception {
        final String path = args.length > 0 ? args[0] : "-";
        final BufferedReader in = new BufferedReader(new InputStreamReader(
                "-".equals(path) ? System.in : new FileInputStream(path), StandardCharsets.UTF_8));
        final PrintStream out = new PrintStream(
                new java.io.BufferedOutputStream(new java.io.FileOutputStream(java.io.FileDescriptor.out)),
                false, StandardCharsets.UTF_8);

        out.println("{\"schema\":1,\"harness\":\"vec3\"}");

        long index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            final String trimmed = line.trim();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                continue;
            }
            final String[] parts = trimmed.split("\\s+");
            if (parts.length != 7) {
                throw new IllegalArgumentException("linha " + index + ": esperados 7 campos, vieram " + parts.length);
            }
            final double[] v = new double[7];
            for (int i = 0; i < 7; i++) {
                v[i] = Double.parseDouble(parts[i]);
            }

            final Vec3 a = new Vec3(v[0], v[1], v[2]);
            final Vec3 b = new Vec3(v[3], v[4], v[5]);
            final double alpha = v[6];

            final StringBuilder sb = new StringBuilder(512);
            sb.append("{\"case\":").append(index);
            sb.append(",\"add\":");
            appendVec(sb, a.add(b));
            sb.append(",\"subtract\":");
            appendVec(sb, a.subtract(b));
            sb.append(",\"dot\":");
            appendBits(sb, a.dot(b));
            sb.append(",\"cross\":");
            appendVec(sb, a.cross(b));
            sb.append(",\"length_a\":");
            appendBits(sb, a.length());
            sb.append(",\"length_sqr_a\":");
            appendBits(sb, a.lengthSqr());
            sb.append(",\"normalize_a\":");
            appendVec(sb, a.normalize());
            sb.append(",\"distance_sqr\":");
            appendBits(sb, a.distanceToSqr(b));
            sb.append(",\"lerp_ab\":");
            appendVec(sb, a.lerp(b, alpha));
            sb.append(",\"scale_a\":");
            appendVec(sb, a.scale(alpha));
            sb.append('}');
            out.println(sb);
            index++;
        }

        out.println("{\"cases\":" + index + "}");
        out.flush();
    }

    private static void appendBits(StringBuilder sb, double value) {
        sb.append('"').append(String.format(Locale.ROOT, "%016x", Double.doubleToRawLongBits(value))).append('"');
    }

    private static void appendVec(StringBuilder sb, Vec3 vec) {
        sb.append('[');
        appendBits(sb, vec.x);
        sb.append(',');
        appendBits(sb, vec.y);
        sb.append(',');
        appendBits(sb, vec.z);
        sb.append(']');
    }
}
