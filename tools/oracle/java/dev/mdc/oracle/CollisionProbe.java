// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.level.Level;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;
import net.minecraft.world.phys.shapes.CollisionContext;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * Probe de paridade da resolução de colisão.
 *
 * Cada caso: {@code n  minX minY minZ maxX maxY maxZ  moveX moveY moveZ} seguido
 * de {@code n} caixas sólidas (6 doubles cada). O resultado é o movimento
 * <b>aplicado</b> depois de resolver contra os sólidos — calculado pelo resolvedor
 * do próprio jogo ({@code Entity.collideBoundingBox}) com um {@link StubLevel}
 * que não contribui com colisões de mundo.
 *
 * Saída por caso: {@code applied} (3 doubles em bits) — o movimento que o
 * resolvedor do jogo deixaria acontecer.
 *
 * Regras desta pasta (tools/oracle/README.md): {@code package dev.mdc.*}, apenas
 * <i>chama</i> a API do jogo, e a saída vai para .local/fixtures/.
 */
public final class CollisionProbe {

    private CollisionProbe() {}

    public static void main(String[] args) throws Exception {
        // Sem versão definida o bootstrap falha, e sem bootstrap o registro não
        // existe: a classe Level não inicializa.
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        final Level level = StubLevel.create();

        final String path = args.length > 0 ? args[0] : "-";
        final BufferedReader in = new BufferedReader(new InputStreamReader(
                "-".equals(path) ? System.in : new FileInputStream(path), StandardCharsets.UTF_8));
        final PrintStream out = new PrintStream(
                new java.io.BufferedOutputStream(new java.io.FileOutputStream(java.io.FileDescriptor.out)),
                false, StandardCharsets.UTF_8);

        out.println("{\"schema\":1,\"harness\":\"collision\"}");

        long index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            final String trimmed = line.trim();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                continue;
            }
            final String[] parts = trimmed.split("\\s+");
            final int solidCount = Integer.parseInt(parts[0]);
            if (parts.length != 1 + 6 + 3 + 6 * solidCount) {
                throw new IllegalArgumentException("linha " + index + ": esperados "
                        + (1 + 6 + 3 + 6 * solidCount) + " campos, vieram " + parts.length);
            }
            int at = 1;
            final AABB box = new AABB(num(parts, at), num(parts, at + 1), num(parts, at + 2),
                    num(parts, at + 3), num(parts, at + 4), num(parts, at + 5));
            at += 6;
            final Vec3 movement = new Vec3(num(parts, at), num(parts, at + 1), num(parts, at + 2));
            at += 3;

            final List<VoxelShape> shapes = new ArrayList<>(solidCount);
            for (int i = 0; i < solidCount; i++) {
                final AABB solid = new AABB(num(parts, at), num(parts, at + 1), num(parts, at + 2),
                        num(parts, at + 3), num(parts, at + 4), num(parts, at + 5));
                at += 6;
                shapes.add(Shapes.create(solid));
            }

            final Vec3 applied = Entity.collideBoundingBox(CollisionContext.empty(), movement, box,
                    level, shapes);
            final StringBuilder sb = new StringBuilder(128);
            sb.append("{\"case\":").append(index);
            sb.append(",\"applied\":");
            appendVec(sb, applied);
            sb.append('}');
            out.println(sb);
            index++;
        }

        out.println("{\"cases\":" + index + "}");
        out.flush();
    }

    private static double num(String[] parts, int index) {
        return Double.parseDouble(parts[index]);
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
