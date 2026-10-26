# Política de propriedade intelectual

> **NOT AN OFFICIAL MINECRAFT PRODUCT. NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.**

Este documento é a regra do repositório. O gate `tools/check_compliance.py` automatiza o que está aqui — se você discorda de uma regra, mude este documento **e** o gate na mesma alteração.

> Não é aconselhamento jurídico. É uma política de engenharia, construída a partir do texto público do EULA e das Usage Guidelines da Mojang.

---

## 1. A regra de ouro

**Nunca commite nada que tenha saído da sua instalação do Minecraft.**

O EULA da Mojang não proíbe reescrever o jogo. Proíbe *distribuir* o que é dela:

> *"You must not **distribute anything we've made** … [incluindo] modified versions of a game, part of those things."*

E define o que **pode** ser distribuído:

> *"By 'Mods,' we mean **something original** that you or someone else created that **doesn't contain a substantial part of our copyrightable code or content**."*
>
> *"Basically, **Mods are okay to distribute**; hacked versions or Modded Versions of the game client or server software are not okay to distribute."*

Consequência prática: um repositório de reimplementação limpa é permitido **desde que** não contenha nem distribua material da Mojang, e desde que não se apresente como oficial nem seja monetizado sem permissão.

Observação importante: o EULA é unilateral — *"We have the final say on what constitutes a Mod and what doesn't."* Portanto conformidade aqui é um juízo da Mojang, não uma garantia. A política deste repositório é deliberadamente conservadora.

---

## 2. Pode / não pode

| ✅ Permitido | ❌ Proibido |
|---|---|
| Código C++ escrito por você, a partir de comportamento observado | Qualquer `.jar`, `.class` ou bytecode do jogo |
| **Probe Java em `tools/oracle/java/`**, que apenas **chama** a API do seu jar | Código Java decompilado, mesmo "só para referência" |
| Ferramentas que leem a **sua** cópia local e escrevem fora do repositório | Texturas, sons, fontes, modelos, traduções (`.lang`) do jogo |
| Documentação descrevendo comportamento, protocolo, formatos | Arquivos de mundo (`.mca`, `.mcr`, `level.dat`, `.nbt`) |
| Assets de obra própria ou de licença compatível (ex.: texturas FOSS), com crédito | O `server.jar`, o `client.jar` ou o launcher |
| Números e medidas extraídas por você (ex.: "1.383 blocos registrados") | Dumps de registries, datapacks, receitas ou loot tables do jogo |
| Transcrições de harness em `.local/fixtures/` (privadas) | Transcrições `.jsonl` **versionadas** |
| Citar pequenos trechos do EULA/Usage Guidelines para explicar a política | Qualquer coisa com o nome do jogo no **nome do produto** |

Regra de bolso para assets: **se veio do jar, não entra. Se foi feito por você (ou tem licença explícita), entra com crédito.**

---

## 3. As regras do gate

| Código | Verifica | Por quê |
|---|---|---|
| `R001` | segmentos de caminho reservados (`.sandbox-cache`, `decompiled`, `decomp`, `minecraft`, `versions`, `libraries`, `crash-reports`, `.local`) | são exatamente onde material derivado aparece |
| `R002` | extensões de artefato do jogo (`.jar .class .java .jfr .hprof .mca .mcr .nbt .mcmeta .mcassetsroot .fsh .vsh`) | por construção, vêm da Mojang |
| `R003` | arquivo > 8 MB | binário grande é como assets entram escondidos |
| `R004` | texto contém `net/minecraft/…` ou `net.minecraft.…` | código derivado/portado do jar |
| `R005` | texto contém cabeçalho de obfuscation map da Microsoft | mappings redistribuídos são proibidos pela própria licença deles |
| `R006` | texto contém `piston-data.mojang.com` / `piston-meta.mojang.com` | código não deve baixar o jogo por conta própria |
| `R007` | `README.md` sem o aviso `NOT AN OFFICIAL MINECRAFT PRODUCT…` | exigido pelas Usage Guidelines |
| `R008` | fonte `.c/.cc/.cpp/.h/.hpp/.inl` **e scripts** (`.py`, `.sh`, `.yml`) sem `SPDX-License-Identifier` | todo arquivo declara origem e licença de forma legível por máquina |
| `R009` | arquivos compactados (`.zip .tar .gz .7z .xz .zst`) | podem transportar o jogo dentro |
| `R010` | `SPDX-License-Identifier` diferente de `AGPL-3.0-only`, ou `LICENSE`/`NOTICE` ausente ou não-AGPL | impede drift de licença e perda da exceção de mods |
| `R011` | `.java` **só** em `tools/oracle/java/**`, com `package dev.mdc.…` e **sem** `package net.minecraft`/`com.mojang` | o oráculo do harness é o único Java legítimo; a trava impede código da Mojang disfarçado de probe |
| `R012` | diretório em `mods/` sem `LICENSE` | mod é trabalho independente; a licença declarada é o que torna a fronteira auditável |
| `R013` | qualquer `.jsonl` versionado | transcrição de harness é fixture: material derivado do jogo |

Arquivos markdown e `docs/` são isentos de `R004`–`R006`: a documentação **precisa** poder citar Mojang, URLs e nomes de classe para explicar o que está sendo reimplementado.

---

## 4. Exceções (allowlist)

`.compliance/allowlist.json` — cada entrada precisa de **caminho, regra e motivo**:

```json
{
  "entries": [
    {
      "path": "tools/check_compliance.py",
      "rule": "R004",
      "reason": "o próprio gate contém as assinaturas que procura"
    }
  ]
}
```

Regras:
- `path` aceita glob (`tools/*.py`).
- O motivo tem de explicar **por que** aquilo não é material da Mojang — "precisei" não é motivo.
- Exceção para `R003` deve trazer também `sha256` do arquivo, para que ele não mude sem revisão.

---

## 5. Se algo entrar por engano

1. **Pare de fazer push** e avise quem tem o repositório clonado.
2. Remova do index e do disco: `git rm --cached <arquivo>`.
3. Reescreva o histórico — deletar num commit posterior **não basta**, o blob continua acessível:
   ```sh
   git filter-repo --path <arquivo> --invert-paths
   ```
4. Force-push em todas as branches e tags, e avise forks.
5. Rode `python3 tools/check_compliance.py --history` para confirmar.
6. Se o material veio de um jar e chegou a ser público, trate como incidente de licença: registre em `docs/incidents/` com data, escopo e remediação.

---

## 6. Marca e nome

- O produto **não** se chama "Minecraft", nem "Minecraft <algo>", nem usa o logo.
- Uso descritivo ("reimplementação do Minecraft: Java Edition") é permitido e esperado — desde que acompanhado do aviso de não-oficial em README, site e releases.
- Sem monetização, doação com recompensa ou patrocínio sem permissão escrita da Mojang. As Usage Guidelines tratam até distribuição gratuita como uso comercial quando há divulgação.

---

## 7. Licença do projeto (decidido em ADR 0001)

| Item | Licença | O que isso significa |
|---|---|---|
| Código | **AGPL-3.0-only** | quem modificar e distribuir tem de publicar o código; quem rodar servidor modificado tem de oferecer o código aos usuários (§13). **Vender é permitido; fechar não é** |
| Assets originais | **CC BY-SA 4.0** | crédito obrigatório; derivados sob a mesma licença |
| Contribuições | **DCO** (`git commit -s`) | o contribuidor mantém o copyright e declara ter o direito de enviar; **não** transferimos direitos, e por isso o projeto não pode ser relicenciado sem o consentimento de todos |
| Mods de terceiros | **AGPL-3.0-only** (sem exceção) | a permissão adicional (§7) foi adiada — ver `docs/adr/0007-excecao-de-mods-adiada.md`. Um mod que linke contra o motor é obra derivada |

Dois pontos que costumam ser mal entendidos:

1. **A nossa licença não concede nada em relação à Mojang** e não protege ninguém de reivindicação dela. O que nos mantém conformes é não distribuir material da Mojang — é o que o gate garante.
2. **"Nunca monetizar" é um compromisso do projeto, não uma restrição da licença.** A AGPL permite que terceiros vendam o software; o que ela impede é que o fechem. Se algum dia for necessário proibir a venda, isso exige troca de licença — e, com DCO, o consentimento de todos os contribuidores.

## 8. Mods

O projeto pretende suportar mods de terceiros. Isso é uma decisão **arquitetural e de licença** ao mesmo tempo (ADR 0005):

- o motor publica uma **ABI C estável**; mods rodam em processo separado ou em sandbox WASM;
- mods **não podem** embarcar código ou assets do Minecraft — nem do nosso projeto, exceto pelo que a interface exige;
- todo mod em `mods/<nome>/` precisa de **`LICENSE`** próprio (regra `R012`);
- **nesta versão não há exceção de linking**: mod de terceiro é AGPL-3.0-only até a ABI existir (ADR 0007);
- mods entram em `mods/**` e passam pelo gate como qualquer outro código.

## 8.1 Oráculo do harness

O harness precisa de um probe em Java que **chame** a API do jogo para produzir a referência de comparação. Esse é o **único** ponto do repositório onde `.java` é aceito:

- diretório: `tools/oracle/java/**` (fora dele, `R002` bloqueia);
- `package dev.mdc.…` obrigatório e `package net.minecraft`/`com.mojang` proibido (regra `R011`);
- o probe **chama**, nunca copia: corpo de método, campos e estruturas da Mojang são proibidos;
- transcrições geradas ficam em `.local/fixtures/`, nunca no repositório (`R013`).

Contrato completo em `tools/oracle/README.md`.

---

## 9. Referências

- Minecraft EULA — <https://www.minecraft.net/en-us/eula>
- Minecraft Usage Guidelines — <https://www.minecraft.net/en-us/usage-guidelines>
- Licença dos obfuscation maps da Mojang (veda redistribuição integral dos mappings) — ver `docs/` do gate
- ADRs do projeto — `docs/adr/0001` a `0005`
