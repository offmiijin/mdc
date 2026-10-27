# Como contribuir

## Antes do primeiro commit

```sh
tools/install_hooks.sh          # aponta core.hooksPath para .githooks
python3 tools/selftest_compliance.py
```

Os hooks rodam o gate automaticamente. Se você pular (`git commit --no-verify`), a CI vai reprovar de qualquer forma — e o histórico fica sujo de um jeito que exige reescrita.

## Fluxo de branches

`development` é a **branch de desenvolvimento**; `main` é o **espelho publicado**.

| Branch | Papel | Commit direto | PR |
|---|---|---|---|
| `development` | base de todo trabalho e destino de **todos** os PRs | não | **sim, sempre** |
| `main` | espelho do que foi publicado e verificado | não | **nunca** |

Trabalho novo sai de uma branch curta criada a partir de `development`, com o número da tarefa no fim do nome:

```sh
git checkout development && git pull
git checkout -b feat/clip-raycast-4
# ... commits com -s ...
git push -u origin feat/clip-raycast-4
# abra o PR com base em `development`
```

**PR aberto contra `main` é fechado e reaberto apontando para `development`** — não é mergeado em `main`. `main` só avança quando um mantenedor promove `development` (merge de release), com o gate verde. Nenhuma das duas branches recebe commit direto.

Motivo, alternativas e consequências: [`docs/adr/0008-fluxo-de-branches.md`](docs/adr/0008-fluxo-de-branches.md).

## O que nunca entra num commit

A lista completa está em [`docs/LEGAL.md`](docs/LEGAL.md). O resumo:

- `.jar`, `.class`, qualquer bytecode do jogo;
- código Java decompilado, mesmo como "referência";
- texturas, sons, fontes, modelos, traduções;
- arquivos de mundo (`.mca`, `.nbt`, `level.dat`);
- dumps de registries, receitas, loot tables;
- `server.jar`, `client.jar`, o launcher.

Se você precisa comparar com o jogo, use **sua** cópia e mantenha o material **fora do repositório** (`.sandbox-cache/` e `.local/` já estão no `.gitignore`).

## Assinatura obrigatória (DCO)

O projeto usa **DCO**, não CLA: você mantém o copyright da sua contribuição e declara ter o direito de enviá-la sob a licença do projeto.

```sh
git commit -s -m "feat: descrição"     # -s adiciona o Signed-off-by
```

Commits sem `Signed-off-by` são recusados na revisão. Não há formulário, bot nem transferência de direitos.

## Convenções

- **Cabeçalho SPDX** em todo código e script (`.c`, `.cc`, `.cpp`, `.h`, `.hpp`, `.inl`, `.py`, `.sh`, `.yml`) e nos hooks, nas primeiras linhas:
  ```cpp
  // SPDX-License-Identifier: AGPL-3.0-only
  // SPDX-FileCopyrightText: 2026 mdc contributors
  ```
  A regra `R010` recusa qualquer outro identificador — MIT, GPL, Apache. Se você copiou código de outro lugar, ele **não entra**: reimplemente.
- **Commits semânticos** (`feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `chore:`), um assunto por commit.
- **PRs pequenos**, com a saída do gate colada na descrição. O destino é sempre `development` (ver [Fluxo de branches](#fluxo-de-branches)).

## Licença da sua contribuição

Ao enviar, sua contribuição passa a ser licenciada sob **AGPL-3.0-only** (código) ou **CC BY-SA 4.0** (assets). Você continua sendo o detentor do copyright. A exceção de mods (`NOTICE` §2) é parte da licença do projeto e se aplica igualmente.

## Mudança de comportamento

Se o seu PR muda comportamento em relação ao original, ele precisa de um registro em `docs/deltas/` — o que o vanilla faz, o que fazemos, por quê, e quais harnesses devem divergir. Sem isso, é regressão, não feature.

## Ao adicionar uma exceção

Só via `.compliance/allowlist.json`, com `path`, `rule` e `reason` explicando **por que** aquilo não é material da Mojang. Exceções de tamanho (`R003`) exigem também `sha256`.

## Casos especiais

**Oráculo do harness (`tools/oracle/java/`)** — é o único lugar do repositório onde `.java` é aceito. Regras (`R011`):

- `package dev.mdc.…` obrigatório;
- `package net.minecraft` / `package com.mojang` **proibido** (é código da Mojang disfarçado);
- pode **importar e chamar** a API do jogo; não pode **copiar** corpo de método, campos ou estruturas;
- transcrições geradas ficam em `.local/fixtures/`, nunca no repo (`R013`).

Contrato: [`tools/oracle/README.md`](tools/oracle/README.md).

**Mods (`mods/<nome>/`)** — cada mod precisa do próprio `LICENSE` (`R012`), não pode conter `.jar`/`.class` nem qualquer asset do jogo. Ver [`mods/README.md`](mods/README.md).

## Ao mudar uma regra

Mude `docs/LEGAL.md` e `tools/check_compliance.py` na **mesma** PR, e adicione um caso em `tools/selftest_compliance.py`. Regra sem teste não existe.

## Dúvida sobre conformidade

Abra uma issue **antes** de commitar, descrevendo o que pretende adicionar e de onde veio. É mais barato perguntar do que reescrever histórico.
