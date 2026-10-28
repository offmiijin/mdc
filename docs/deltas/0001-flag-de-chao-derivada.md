# 0001 — Flag de chão derivada do movimento barrado

- **Status:** aceito
- **Degrau:** M1
- **Autor:** @offmiijin
- **Data:** 2026-10-27

## Comportamento do original (26.3)

Duas coisas separadas no jogo:

1. **A resolução do movimento** — `Entity.collideBoundingBox(CollisionContext, Vec3, AABB, Level, List<VoxelShape>)`
   devolve o `Vec3` que o corpo realmente anda. É geometria pura e o harness
   `collision` a compara **bit a bit** em 10.018 casos.
2. **As flags** — `onGround`, `verticalCollision`, `horizontalCollision` são
   campos da **entidade**, escritos por `Entity.move`, que exige `Level` e
   entidade vivos (mundo carregado, posição, chunk). Não há como observá-las num
   harness de geometria: o probe não consegue instanciar uma entidade real.

Evidência: `javap` mostra `setOnGround(...)`, `verticalCollision` como membros de
instância de `Entity`; o resolvedor estático só devolve o `Vec3`. O harness
`collision` compara exatamente o que o resolvedor devolve.

## Nosso comportamento

A flag é **derivada** do movimento barrado, no mesmo lugar da resolução:

```cpp
result.on_ground = dy != movement.y && movement.y < 0.0;   // desceu menos do que pediu
result.horizontal_collision = (dx != movement.x || dz != movement.z);
```

Coberta por testes de unidade (`tests/collision_test.cpp`), não pelo harness.

## Motivo

Observar a flag exigiria um mundo real dentro do probe — `Level` concreto,
chunk carregado, entidade instanciada —, e isso traria o M2 (chunk) para dentro
do M1, que é explicitamente "colisão de blocos estáticos e AABBs". A parte da
flag que depende de mundo (empurrar, subir degrau, escorregar) é do M3.

A definição escolhida é a única consistente com a resolução: só há "chão"
quando o movimento vertical foi **reduzido** e era **para baixo**. Comparar com
a flag do original só será possível no M3, com jogador de verdade.

## Impacto

- Nenhum caso do harness `collision` depende da flag — ele compara `applied`.
- `docs/PARITY.md` registra que a nota **1** do M1 cobre posição/movimento
  bit a bit e que a flag é derivada, não comparada.
- No M3, quando houver entidade e mundo, esta derivação precisa ser substituída
  pela comparação direta — e este delta vira "implementado" ou "revogado".
