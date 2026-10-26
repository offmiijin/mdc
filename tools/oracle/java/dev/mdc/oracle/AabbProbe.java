// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/**
 * Probe de paridade de AABB.
 *
 * Lê casos (texto, um por linha) e escreve uma transcrição JSONL com os
 * resultados do jogo original em <b>bits</b> (16 dígitos hex do padrão IEEE-754),
 * exatamente no mesmo formato que o lado C++ produz.
 *
 * Regras desta pasta (ver tools/oracle/README.md):
 *   - package dev.mdc.* — este arquivo é nosso, não da Mojang;
 *   - apenas <i>chama</i> a API do jogo; nunca copia corpo/campos do original;
 *   - a saída é material derivado e vai para .local/fixtures/, nunca para o repo.
 */
public final class AabbProbe {

    private AabbProbe() {}

    public static void main(String[] args) throws Exception {
        final String path = args.length > 0 ? args[0] : "-";
        final BufferedReader in = new BufferedReader(new InputStreamReader(
                "-".equals(path) ? System.in : new FileInputStream(path), StandardCharsets.UTF_8));
        final PrintStream out = new PrintStream(
                new java.io.BufferedOutputStream(new java.io.FileOutputStream(java.io.FileDescriptor.out)),
                false, StandardCharsets.UTF_8);

        out.println("{\"schema\":1,\"harness\":\"aabb\"}");

        long index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            final String trimmed = line.trim();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                continue;
            }
            final String[] parts = trimmed.split("\\s+");
            if (parts.length != 13) {
                throw new IllegalArgumentException("linha " + index + ": esperados 13 campos, vieram " + parts.length);
            }
            final double[] v = new double[13];
            for (int i = 0; i < 13; i++) {
                v[i] = Double.parseDouble(parts[i]);
            }

            final AABB a = new AABB(v[0], v[1], v[2], v[3], v[4], v[5]);
            final AABB b = new AABB(v[6], v[7], v[8], v[9], v[10], v[11]);
            final Vec3 delta = new Vec3(v[12], v[12] * 0.5, -v[12]);
            final Vec3 inflate = new Vec3(v[12], v[12] * 0.25, v[12] * 2.0);

            final StringBuilder sb = new StringBuilder(256);
            sb.append("{\"case\":").append(index);
            sb.append(",\"intersects\":").append(a.intersects(b));
            sb.append(",\"center_a\":");
            appendVec(sb, a.getCenter());
            sb.append(",\"move_a\":");
            // move(double,double,double), não move(Vec3): a sobrecarga move(Vector3f)
            // exige org.joml no classpath do probe — e joml não está no client.jar.
            appendBox(sb, a.move(delta.x, delta.y, delta.z));
            sb.append(",\"inflate_a\":");
            appendBox(sb, a.inflate(inflate.x, inflate.y, inflate.z));
            sb.append('}');
            out.println(sb);
            index++;
        }

        out.println("{\"cases\":" + index + "}");
        out.flush();
    }

    private static void appendBits(StringBuilder sb, double value) {
        // Bits crus: comparação binária, sem ambiguidade de formatação entre Java e C++.
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

    private static void appendBox(StringBuilder sb, AABB box) {
        sb.append('[');
        appendBits(sb, box.minX);
        sb.append(',');
        appendBits(sb, box.minY);
        sb.append(',');
        appendBits(sb, box.minZ);
        sb.append(',');
        appendBits(sb, box.maxX);
        sb.append(',');
        appendBits(sb, box.maxY);
        sb.append(',');
        appendBits(sb, box.maxZ);
        sb.append(']');
    }
}
