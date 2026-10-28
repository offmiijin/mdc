# Harness — como se prova paridade

Um harness roda **a mesma entrada** nas duas implementações e compara a saída.

```
entrada determinística (seed, ticks, input, estado inicial)
        │
        ├──► vanilla 26.3  ──► probe Java ──► transcrição ──┐
        │                                                   ├──► diff bit a bit
        └──► nosso C++     ──────────────► transcrição ─────┘         │
                                                                      ▼
                                                          0 linhas = paridade
```

Três peças, três destinos:

| Peça | O que é | Onde fica |
|---|---|---|
| **Oráculo** | o 26.3 rodando na **sua** cópia | sua máquina |
| **Probe** | programa **nosso** que alimenta e serializa | `tools/oracle/java/**` (regra `R011`) |
| **Fixture** | transcrição capturada | `.local/fixtures/` — **nunca** versionada (`R013`) |

Dois tipos:

| Tipo | Como funciona | Quando |
|---|---|---|
| **Diferencial** | roda o vanilla e o nosso lado na hora | desenvolvimento; pega regressão no mesmo dia |
| **Golden** | grava a saída do vanilla uma vez e compara com o fixture | CI sem Java, mais rápido |

---

## 1. Determinismo — o requisito que quebra tudo se falhar

Se a entrada não for determinística, o `diff` é ruído e o harness não vale nada.

| Regra | Consequência se violar |
|---|---|
| **Seed explícita**, nunca aleatória | duas execuções do oráculo divergem entre si |
| **Ordem de tick fixa**, simulação single-thread na comparação | interleaving de threads muda o resultado |
| **Sem relógio de parede** — o tempo é a contagem de ticks | `deltaTime` variável contamina física |
| **Sem I/O** no caminho medido (disco, rede, locale) | resultado depende da máquina |
| **Nada de `HashMap`/`HashSet`** no caminho comparado | ordem de iteração não é especificada em nenhuma das linguagens |
| **Coleções comparadas como ordenadas** ou como conjunto ordenado | idem |

## 2. Ponto flutuante — a armadilha número um

O Java Edition usa `double` e `float` IEEE-754 em toda a física e no worldgen.
A partir do **Java 17** (JEP 306) todo cálculo é *strictfp*: sem precisão
estendida, sem surpresa de x87. O C++ **não** é assim por padrão.

Regras obrigatórias no nosso lado:

| Item | Regra |
|---|---|
| Flags de compilação | **proibido** `-ffast-math`, `-Ofast`, `-mfpmath=387`. Usar `-ffp-contract=off` e `-fno-fast-math` — já embutido no alvo `mdc_fp_strict` |
| FMA | proibido contrair `a*b+c` em `fma` — o Java não contrai |
| Ordem das operações | preservar a expressão como está no original; `(a+b)+c ≠ a+(b+c)` |
| Precisão | `float` é 32 bits de verdade; nunca promover silenciosamente para `double` |
| Comparação | **bits exatos**, via `bit_cast`/`doubleToRawLongBits` |
| Transcendentais | `Math.sin/cos/atan2/pow/exp/log` delegam a **fdlibm** (via `StrictMath`). Portar a mesma implementação — a `libm` do sistema **não** é bit-idêntica |
| Aleatoriedade | `java.util.Random` (LCG de 48 bits) e as fontes do worldgen (`net/minecraft/world/level/levelgen/`) têm de ser reimplementadas bit a bit, não "equivalentes" |

### Divergências conhecidas — o teste unitário não pega, o harness pega

Três armadilhas que passam em teste de unidade e só aparecem na comparação
contra o original:

**1. `AABB.getCenter()` não é média.** O original usa `Mth.lerp(0.5, min, max)`
= `min + 0.5*(max-min)`. A "média óbvia" `(min+max)/2` diverge em **~9,9%** dos
casos (1–2 ULP).

**2. O construtor de `AABB` normaliza.** `AABB(minX,…,maxX,…)` aplica
`Math.min`/`Math.max` a cada par — caixa invertida não existe no jogo. Sem isso,
**1.865 de 2.007 casos** divergiam.

**3. `Math.min` do Java ≠ `std::min` do C++ no zero.** Entre `+0.0` e `-0.0`, Java
devolve `-0.0` (e `+0.0` no `max`); `std::min` devolve o primeiro argumento e erra
o bit de sinal. O caso degenerado que expõe isso é `min_x == max_x == -510.625`:
tratar "iguais e negativos" como zero erra o bit.

**4. "Consertar" o resultado diverge.** `AABB.clip` devolve o `lerp` puro, e
esse ponto pode cair 1 ULP **fora** da caixa (caso 1420). Grampear o resultado
aos limites da caixa — que parece a coisa certa a fazer — diverge do original em
1 bit. Paridade é reproduzir, não corrigir.

**5. Ordem de resolução não é a "óbvia".** A colisão do original resolve **y
primeiro** e depois o eixo de maior |movimento| entre x e z (empate vai para x).
A ordem intuitiva (y, x, z) diverge em ~0,3% dos casos diagonais: o movimento de
um eixo muda a sobreposição que decide o seguinte. Nenhuma ordem fixa explica o
conjunto todo — a regra só apareceu com casos reduzidos a um sólido.

**6. Sólido de espessura zero não colide.** `Shapes.create(AABB)` com `min == max`
em algum eixo produz shape vazio; tratar como laje barra a queda de quem já está
apoiado nela.

**7. Constantes de limiar podem ser `float` promovido a `double`.** O limiar de
`Vec3.normalize()` não é `1e-5` em double (`0x3EE4F8B588E368F1`) e sim
`(double) 1e-5f` (`0x3EE4F8B580000000`) — 27 bits de mantissa a menos. Errar
isso zera ou normaliza 147 de 2.020 casos, e nenhum teste de unidade escrito a
partir do "bom senso" detecta. A fronteira foi achada por **bissecção no probe**:
o último vetor zerado e o primeiro normalizado são vizinhos dela.

Depois das três correções: **2.007 casos, 0 diferenças, md5 da transcrição igual
nos dois lados**.

Divergência de 1 ULP **é falha**. Tolerância não é ajustável por harness: o
padrão é zero (regra do ADR 0002).

## 3. Formato da transcrição

`JSONL`: uma linha JSON por amostra, campos em ordem determinística.

> **Números vão em bits.** Cada `double` é escrito como 16 dígitos hex do seu
> padrão IEEE-754 (`3fe0000000000000` = 0.5), não como decimal nem como
> "hex float". Motivo: Java (`0x0.0p0`) e C (`0x0p+0`) formatam hex float de
> formas diferentes, e a comparação viraria discussão de formatação. Bits não
> têm dialeto.

Primeira linha é o **cabeçalho de proveniência**, idêntico nos dois lados:

```jsonl
{"schema":1,"harness":"aabb"}
{"case":0,"intersects":false,"center_a":["3fe0000000000000","3fe0000000000000","3fe0000000000000"],"move_a":[ … 6 bits … ],"inflate_a":[ … 6 bits … ]}
{"cases":2007}
```

Regras do formato:

- **campos ordenados** (ordem declarada no harness) — nunca ordem de iteração;
- **doubles como bits** (16 hex), inclusive `-0.0` (`8000000000000000`);
- **booleanos** como `true`/`false`, nulos como `null` explícito;
- **sem timestamps, sem paths, sem nomes de máquina** nos registros comparados;
- última linha traz a contagem de casos, para detectar transcrição truncada.

## 3.1 Casos de entrada são nossos; saída é do jogo

Essa distinção é o que permite versionar os casos:

| | Origem | Versão no repositório? |
|---|---|---|
| **Casos** (`tests/parity/*.txt`) | gerados por nós, números puros | **sim** |
| **Transcrição do jogo** | resposta do original | **não** — `.local/fixtures/` (regra `R013`) |
| **Nossa transcrição** | nosso código | não — `build/` |

Os geradores (`tools/parity/gen_<harness>_cases.py`) são determinísticos pelo
PRNG próprio em `tools/parity/prng.py` (xorshift64*), não pelo `random` da
linguagem — a mesma seed produz o mesmo arquivo em qualquer versão de Python.

## 4. Fixtures

```
.local/fixtures/<harness>/<versao>/<hash-da-entrada>.jsonl
```

- **Nunca** versionado (`.gitignore` + regra `R013`). Fixture é saída do jogo.
- Tamanho: se passar de alguns MB, o harness está amostrando demais — grave os
  ticks de interesse, não todos.
- Proveniência obrigatória no cabeçalho: `manifest_sha1` de `refs/minecraft.lock`,
  seed, hash das fontes dos dois lados. Fixture sem proveniência não é prova.
- Regenerar é sempre possível na máquina de quem tem o jogo; por isso não se
  compartilha fixture.

## 5. Ciclo de trabalho

```
1. escrever o probe (Java) + o caso no C++ + o gerador (`gen_<harness>_cases.py`)
2. rodar os dois → duas transcrições
3. diff → se 0, harness verde; se não:
     a. minimizar o caso (menos ticks, menos entidades)
     b. é divergência intencional? → docs/deltas/ (ADR 0006) e o harness passa
        a esperar aquela diferença exata
     c. senão → é bug: conserta o C++
4. commitar só com o harness verde
```

## 6. O que **não** vale como prova

| Prática | Por que não |
|---|---|
| Screenshot "parece igual" | não é bit-exato; serve para inspeção visual, nunca como critério |
| Comparar log de texto | formato instável, ordem variável |
| Tolerância por padrão | esconde o erro de 0,001/tick que quebra no tick 1.000 |
| Harness que não falha quando você estraga o código de propósito | não está testando nada — teste a falha também |
| Fixture sem hash de origem | não se sabe contra o que foi comparado |

## 7. Pré-requisito

**JDK 25.** O 26.3 é class file v69; `javac 21` falha com *"class file has wrong
version 69.0, should be 65.0"* (verificado). O probe roda contra o jar indicado
por argumento/variável de ambiente — o harness **nunca** baixa o jogo.

Ver também: [`PARITY.md`](PARITY.md) (o que cada degrau prova),
[`adr/0002-escopo.md`](adr/0002-escopo.md) (critério de sucesso),
[`deltas/`](deltas/) (como registrar divergência intencional).
