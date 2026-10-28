// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.Optional;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/**
 * Probe de paridade de {@code AABB.clip} (raycast caixa-alinhada).
 *
 * Casos por linha: {@code minX minY minZ maxX maxY maxZ fromX fromY fromZ toX toY toZ}.
 * Saída: {@code hit} e o ponto de impacto (ou {@code null}).
 *
 * Regras desta pasta (ver tools/oracle/README.md): {@code package dev.mdc.*},
 * apenas <i>chama</i> a API do jogo, e a saída vai para .local/fixtures/.
 */
public final class ClipProbe {

    private ClipProbe() {}

    public static void main(String[] args) throws Exception {
        final String path = args.length > 0 ? args[0] : "-";
        final BufferedReader in = new BufferedReader(new InputStreamReader(
                "-".equals(path) ? System.in : new FileInputStream(path), StandardCharsets.UTF_8));
        final PrintStream out = new PrintStream(
                new java.io.BufferedOutputStream(new java.io.FileOutputStream(java.io.FileDescriptor.out)),
                false, StandardCharsets.UTF_8);

        out.println("{\"schema\":1,\"harness\":\"clip\"}");

        long index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            final String trimmed = line.trim();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                continue;
            }
            final String[] parts = trimmed.split("\\s+");
            if (parts.length != 12) {
                throw new IllegalArgumentException("linha " + index + ": esperados 12 campos, vieram " + parts.length);
            }
            final double[] v = new double[12];
            for (int i = 0; i < 12; i++) {
                v[i] = Double.parseDouble(parts[i]);
            }

            final AABB box = new AABB(v[0], v[1], v[2], v[3], v[4], v[5]);
            final Vec3 from = new Vec3(v[6], v[7], v[8]);
            final Vec3 to = new Vec3(v[9], v[10], v[11]);

            final Optional<Vec3> hit = box.clip(from, to);

            final StringBuilder sb = new StringBuilder(128);
            sb.append("{\"case\":").append(index);
            sb.append(",\"hit\":").append(hit.isPresent());
            sb.append(",\"point\":");
            if (hit.isPresent()) {
                final Vec3 p = hit.get();
                sb.append('[');
                appendBits(sb, p.x);
                sb.append(',');
                appendBits(sb, p.y);
                sb.append(',');
                appendBits(sb, p.z);
                sb.append(']');
            } else {
                sb.append("null");
            }
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
}
