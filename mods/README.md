# Mods

Cada mod é um **trabalho independente**, com licença própria. O motor é
AGPL-3.0-only, mas a exceção de linking (`NOTICE` §2) permite que mods usem
**qualquer** licença — desde que se comuniquem apenas pela interface publicada.

> A ABI C ainda **não existe**. Ela só é congelada depois do degrau M2, quando os
> pontos de extensão reais forem conhecidos. Ver `docs/adr/0005-mods.md`.

## Layout

```
mods/
└── <nome-do-mod>/
    ├── LICENSE          ← obrigatório (regra R012)
    ├── mod.json         ← manifesto (formato a definir)
    └── ...              ← código-fonte ou .wasm, assets originais
```

## Regras (verificadas pelo gate)

| Regra | O que exige |
|---|---|
| `R012` | todo diretório em `mods/` precisa de `LICENSE`, `LICENSE.md`, `LICENSE.txt` ou `COPYING` |
| `R002` | nada de `.jar` nem `.class` (mod Java não roda; mods são nativos via ABI ou WASM) |
| `R001` | nenhum caminho com segmento `minecraft/` (bloqueia `assets/minecraft/…`) |
| `R003` | nenhum arquivo acima de 8 MB sem justificativa |

## O que um mod **não** pode conter

- texturas, sons, modelos, traduções ou dados extraídos do jar do jogo;
- código da Mojang, de qualquer forma — incluindo "só para referência";
- o próprio jogo (`client.jar`, `server.jar`, launcher).

Assets originais ou de terceiros com licença explícita: registre em
`LICENSES/README.md` antes de commitar.

## Motivo da exigência de LICENSE

Por dois motivos ao mesmo tempo:

1. **Jurídico** — a exceção do `NOTICE` só funciona se a fronteira entre motor e
   mod for demonstrável. A licença declarada é parte dessa prova.
2. **De conformidade** — um diretório de mod sem licença é o lugar mais fácil do
   mundo para alguém despejar um pack de texturas sem origem e ninguém notar.
