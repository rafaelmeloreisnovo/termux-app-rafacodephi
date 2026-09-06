# BUG-01 — Attractor Table — registro auditado

> Repository atual: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Baseline auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`
> O texto histórico deste arquivo descrevia uma tabela 42-state ausente e propunha valores inferidos. Essa descrição não corresponde ao source atual.

## Estado atual

```text
SOURCE_OBSERVED
GATE_WIRED
DEVICE_PROVEN = TOKEN_VAZIO
```

O baseline contém:

```text
rmr/Rrr/attractor_table.c
rmr/Rrr/attractor_table.h
rmr/Rrr/attractor_table_validator.c
```

O header atual declara explicitamente um espaço toroidal de **41 estados**:

```text
count  = 41
period = 41
index  = [0..40]
dim    = 7
```

A função pública `attractor_lookup(uint32_t idx)` documenta acesso `[0..40]`, e `attractor_validate()` é o validador estrutural.

## Correção do drift histórico

As seguintes afirmações da versão antiga deste documento são `STALE` para o baseline auditado:

- `attractor_table` ausente/stub;
- “40/42 estados faltando” como estado corrente;
- tabela corrente de 42 posições;
- estado #22 propositalmente vazio aguardando decisão;
- instrução de gerar uma nova tabela a partir dos valores hipotéticos escritos neste documento.

Essas ideias são material histórico de projeto e podem ser recuperadas pelo histórico do Git, mas não devem ser copiadas de volta ao código como correção atual.

## Autoridade técnica corrente

Use, nesta ordem:

1. `rmr/Rrr/attractor_table.c`
2. `rmr/Rrr/attractor_table.h`
3. `rmr/Rrr/attractor_table_validator.c`
4. target `attractor-table-complete-gate` do `Makefile`
5. `docs/BUG01_IMPLEMENTATION_RECORD.md` como registro histórico de implementação
6. `docs/BUG02_DECISION_RECORD.md` para a decisão que levou à cardinalidade atual

## Invariante documental

```text
CURRENT_CARDINALITY = 41
CURRENT_INDEX_RANGE = 0..40
HISTORICAL_42_STATE_PROPOSALS != CURRENT_SOURCE
```

Qualquer alteração futura da cardinalidade deve modificar source, validator, testes/gates e documentação no mesmo ciclo; não pode nascer apenas de um documento propositivo.

## Evidência e limite de claim

A existência da tabela e do validador permite afirmar implementação estrutural. Ela não prova, isoladamente:

- execução AArch64 física;
- estabilidade em todos os SoCs;
- desempenho em ciclos;
- comportamento de device/release.

Esses itens permanecem dependentes de evidência própria; quando ausente, usar `TOKEN_VAZIO`.

## Relações

- BUG-02: decisão de cardinalidade/VOID histórica.
- BUG-03: `vectra_pulse.S` consome o espaço de atratores atual.
- BUG-08: convergência Lyapunov possui implementação/validator próprios.

## Auditoria relacionada

- `docs/00_BUG_MASTER_INDEX.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
