// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
package dev.mdc.oracle;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Optional;
import java.util.stream.LongStream;
import net.minecraft.core.IdMap;
import net.minecraft.util.BitStorage;
import net.minecraft.util.SimpleBitStorage;
import net.minecraft.util.ZeroBitStorage;
import net.minecraft.world.level.chunk.PalettedContainer;
import net.minecraft.world.level.chunk.PalettedContainerRO;
import net.minecraft.world.level.chunk.Strategy;

/**
 * Harness `palette` (degrau M2): empacotamento de bits, paletas e container
 * paletizado.
 *
 * O espaço de valores é <b>nosso</b>: um {@link IdMap} identidade sobre
 * {@code 0..entryCount-1}. Nenhum identificador do jogo entra na transcrição, e
 * o caso continua sendo entrada nossa.
 *
 * Nenhuma classe de bloco é carregada: só a máquina de paleta, o que mantém o
 * probe independente de assets e de GPU.
 *
 * Entrada (uma linha por caso, campos separados por espaço):
 * <pre>
 * storage &lt;bits&gt; &lt;size&gt; &lt;índice&gt;=&lt;valor&gt; ...
 * container &lt;x&gt;,&lt;y&gt;,&lt;z&gt;=&lt;token&gt; ...
 * </pre>
 * Saída: JSONL em bits, com doubles/int em hex quando forem float
 * (ver docs/HARNESS.md §3).
 */
public final class PaletteProbe {
    /** Quantos valores distintos o IdMap aceita (o jogo usa 4096 para blocos). */
    private static final int ENTRY_COUNT = 4096;
    /** Quantas leituras amostradas por caso: barato e pega erro de índice. */
    private static final int SAMPLE_COUNT = 8;

    /**
     * Valor do nosso espaço de tokens: um objeto **interno**, como os estados de
     * bloco do jogo. Internar importa para paridade: a paleta compara valores
     * por identidade, então um `Integer` recém-criado (> 127, fora do cache do
     * JDK) geraria uma entrada duplicada na paleta — artefato do probe, não do
     * original.
     */
    private record Token(int id) {
        @Override
        public String toString() {
            return Integer.toString(id);
        }
    }

    /** IdMap identidade sobre tokens internos. Nada do jogo vaza na saída. */
    private static final class TokenIdMap implements IdMap<Token> {
        private final Token[] values;

        TokenIdMap(int size) {
            this.values = new Token[size];
            for (int i = 0; i < size; i++) {
                values[i] = new Token(i);
            }
        }

        Token token(int id) {
            return values[id];
        }

        @Override
        public int getId(Token value) {
            return value.id();
        }

        @Override
        public Token byId(int id) {
            return values[id];
        }

        @Override
        public int size() {
            return values.length;
        }

        @Override
        public java.util.Iterator<Token> iterator() {
            return java.util.Arrays.asList(values).iterator();
        }
    }

    public static void main(String[] args) throws Exception {
        TokenIdMap map = new TokenIdMap(ENTRY_COUNT);
        Strategy<Token> strategy = Strategy.createForBlockStates(map);

        // O caminho vem por argumento, como nos outros probes; stdin fica como
        // conveniência para inspeção manual.
        if (args.length > 1) {
            throw new IllegalArgumentException("uso: PaletteProbe [casos.txt]");
        }
        BufferedReader in = args.length == 1
            ? new BufferedReader(new java.io.FileReader(args[0], StandardCharsets.UTF_8))
            : new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
        System.out.println("{\"schema\":1,\"harness\":\"palette\"}");

        int index = 0;
        String line;
        while ((line = in.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty() || line.charAt(0) == '#') {
                continue;
            }
            String[] parts = line.split("\\s+");
            switch (parts[0]) {
                case "storage" -> storageCase(index, parts);
                case "container" -> containerCase(index, parts, strategy, map);
                default -> throw new IllegalArgumentException("caso desconhecido: " + parts[0]);
            }
            index++;
        }
        System.out.println("{\"cases\":" + index + "}");
    }

    private static void storageCase(int index, String[] parts) {
        int bits = Integer.parseInt(parts[1]);
        int size = Integer.parseInt(parts[2]);
        BitStorage storage = bits == 0 ? new ZeroBitStorage(size) : new SimpleBitStorage(bits, size);
        for (int i = 3; i < parts.length; i++) {
            int eq = parts[i].indexOf('=');
            storage.set(Integer.parseInt(parts[i].substring(0, eq)),
                        Integer.parseInt(parts[i].substring(eq + 1)));
        }

        StringBuilder out = new StringBuilder();
        out.append("{\"case\":").append(index)
           .append(",\"kind\":\"storage\",\"bits\":").append(storage.getBits())
           .append(",\"size\":").append(storage.getSize()).append(",\"raw\":[");
        appendHexLongs(out, storage.getRaw());
        out.append("],\"gets\":[");
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            if (i > 0) {
                out.append(',');
            }
            out.append(storage.get(sampleIndex(i, size)));
        }
        out.append("]}");
        System.out.println(out);
    }

    private static void containerCase(int index, String[] parts, Strategy<Token> strategy,
                                    TokenIdMap map) {
        PalettedContainer<Token> container = new PalettedContainer<>(map.token(0), strategy);
        for (int i = 1; i < parts.length; i++) {
            int eq = parts[i].indexOf('=');
            String[] xyz = parts[i].substring(0, eq).split(",");
            container.set(Integer.parseInt(xyz[0]), Integer.parseInt(xyz[1]), Integer.parseInt(xyz[2]),
                          map.token(Integer.parseInt(parts[i].substring(eq + 1))));
        }

        PalettedContainerRO.PackedData<Token> packed = container.pack(strategy);
        StringBuilder out = new StringBuilder();
        out.append("{\"case\":").append(index)
           .append(",\"kind\":\"container\",\"bits\":").append(container.bitsPerEntry())
           .append(",\"packed_bits\":").append(packed.bitsPerEntry()).append(",\"palette\":[");
        List<Token> entries = packed.paletteEntries();
        for (int i = 0; i < entries.size(); i++) {
            if (i > 0) {
                out.append(',');
            }
            out.append(strategy.globalMap().getId(entries.get(i)));
        }
        out.append("],\"storage\":[");
        Optional<LongStream> storage = packed.storage();
        appendHexLongs(out, storage.isPresent() ? storage.get().toArray() : new long[0]);
        out.append("],\"gets\":[");
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            if (i > 0) {
                out.append(',');
            }
            int idx = sampleIndex(i, 4096);
            out.append(container.get(idx & 15, (idx >> 8) & 15, (idx >> 4) & 15).id());
        }
        out.append("]}");
        System.out.println(out);
    }

    /** Longs em 16 dígitos: o mesmo formato do lado C++, para o diff valer. */
    private static void appendHexLongs(StringBuilder out, long[] values) {
        for (int i = 0; i < values.length; i++) {
            if (i > 0) {
                out.append(',');
            }
            out.append('"').append(String.format("%016x", values[i])).append('"');
        }
    }

    /** Índice amostrado: espalha pela seção sem depender de aleatoriedade. */
    private static int sampleIndex(int i, int size) {
        return (int) (((long) i * 977L + 13L) % size);
    }
}
