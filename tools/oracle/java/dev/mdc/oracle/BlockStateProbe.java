// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Optional;
import net.minecraft.SharedConstants;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * Harness `blockstate` (degrau M2): o contrato de estado de bloco.
 *
 * Compara o que é observável sem assets e sem GPU: as propriedades do bloco na
 * ordem de declaração (e o domínio de cada uma), os valores do estado default,
 * e as consultas que o mesher e a luz vão usar depois.
 *
 * Entrada (uma linha por caso): `&lt;id&gt; [prop=valor ...]`, com `id` no formato
 * `minecraft:stone`.
 */
public final class BlockStateProbe {
    /**
     * `System.out` capturado **antes** do bootstrap: o jogo instala um layout de
     * log por cima do stream, e a transcrição tem de sair crua para o diff.
     */
    private static final PrintStream OUT = System.out;

    public static void main(String[] args) throws Exception {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        BufferedReader in = args.length == 1
            ? new BufferedReader(new FileReader(args[0], StandardCharsets.UTF_8))
            : new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));

        OUT.println("{\"schema\":1,\"harness\":\"blockstate\"}");
        int index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty() || line.charAt(0) == '#') {
                continue;
            }
            case_(index++, line.split("\\s+"));
        }
        OUT.println("{\"cases\":" + index + "}");
    }

    private static void case_(int index, String[] fields) {
        Identifier id = Identifier.parse(fields[0]);
        Block block = BuiltInRegistries.BLOCK.getValue(id);
        if (block == null) {
            throw new IllegalArgumentException("bloco inexistente: " + id);
        }

        BlockState state = block.defaultBlockState();
        for (int i = 1; i < fields.length; i++) {
            int eq = fields[i].indexOf('=');
            String name = fields[i].substring(0, eq);
            String raw = fields[i].substring(eq + 1);
            state = withProperty(state, name, raw);
        }

        StringBuilder out = new StringBuilder();
        out.append("{\"case\":").append(index)
           .append(",\"id\":\"").append(id).append("\",\"props\":[");
        List<Property<?>> properties = List.copyOf(block.getStateDefinition().getProperties());
        for (int i = 0; i < properties.size(); i++) {
            Property<?> property = properties.get(i);
            if (i > 0) {
                out.append(',');
            }
            out.append("[\"").append(property.getName()).append("\",[");
            List<?> values = property.getPossibleValues();
            for (int v = 0; v < values.size(); v++) {
                if (v > 0) {
                    out.append(',');
                }
                out.append('"').append(property.getName(unchecked(property, values.get(v)))).append('"');
            }
            out.append("]]");
        }

        BlockState defaults = block.defaultBlockState();
        out.append("],\"default\":[");
        for (int i = 0; i < properties.size(); i++) {
            if (i > 0) {
                out.append(',');
            }
            out.append(propertyIndex(properties.get(i), defaults));
        }
        out.append("],\"state\":[");
        for (int i = 0; i < properties.size(); i++) {
            if (i > 0) {
                out.append(',');
            }
            out.append(propertyIndex(properties.get(i), state));
        }
        out.append("],\"render_shape\":\"").append(state.getRenderShape().name())
           .append("\",\"solid_render\":").append(state.isSolidRender())
           .append(",\"light_emission\":").append(state.getLightEmission())
           .append(",\"light_dampening\":").append(state.getLightDampening())
           .append(",\"skylight_down\":").append(state.propagatesSkylightDown())
           .append(",\"shape_for_light\":").append(state.useShapeForLightOcclusion())
           .append("}");
        OUT.println(out);
    }

    @SuppressWarnings("unchecked")
    private static <T extends Comparable<T>> T unchecked(Property<?> property, Object value) {
        return (T) value;
    }

    private static <T extends Comparable<T>> BlockState withProperty(BlockState state, String name,
                                                                    String raw) {
        Property<?> property = state.getProperties().stream()
            .filter(p -> p.getName().equals(name))
            .findFirst()
            .orElseThrow(() -> new IllegalArgumentException("propriedade inexistente: " + name));
        return set(state, property, raw);
    }

    private static <T extends Comparable<T>> BlockState set(BlockState state, Property<T> property,
                                                            String raw) {
        Optional<T> value = property.getValue(raw);
        if (value.isEmpty()) {
            throw new IllegalArgumentException("valor inválido: " + property.getName() + "=" + raw);
        }
        return state.setValue(property, value.get());
    }

    /** Índice do valor da propriedade nesse estado, na ordem de `getPossibleValues`. */
    private static <T extends Comparable<T>> int propertyIndex(Property<T> property, BlockState state) {
        List<T> values = property.getPossibleValues();
        T current = state.getValue(property);
        for (int i = 0; i < values.size(); i++) {
            if (values.get(i).equals(current)) {
                return i;
            }
        }
        throw new IllegalStateException("valor fora do domínio: " + current);
    }
}
