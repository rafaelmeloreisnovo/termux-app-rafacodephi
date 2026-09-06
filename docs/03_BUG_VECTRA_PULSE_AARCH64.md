# BUG-03 — `vectra_pulse.S` AArch64 — registro auditado

> Repository atual: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Baseline auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`
> O documento histórico anterior descrevia quatro bugs como abertos e incluía assembly inferido. O source atual declara e implementa o fechamento estrutural desses quatro pontos.

## Estado atual

```text
SOURCE_OBSERVED
GATE/VALIDATOR EVIDENCE = verificar pelo target/receipt correspondente
DEVICE_PROVEN = TOKEN_VAZIO
```

Arquivo autoritativo:

```text
rmr/Rrr/vectra_pulse.S
```

O próprio source atual declara:

- BUG-03-A: load-use hazard mitigado com instrução independente;
- BUG-03-B: indexing corrigido para `sizeof=20` + bounds;
- BUG-03-C: `dmb ish` após atualização de estado;
- BUG-03-D: wrapping com `subs/csel` em vez de `udiv/msub`.

## Contrato atual observado

O source documenta:

```text
x0 = state_ptr
x1 = C (Q16.16)
x2 = H (Q16.16)
x3 = phase [0..40]
x4 = attractor_idx [0..40]
x5 = φ = (1-H)·C
period = 41
```

Isso substitui a descrição antiga de índices `[0..41]`/período 42 neste documento.

## Implementação corrente observada

### Bounds/indexação

O source faz bound check contra `#41`, usa fallback para índice 0 e calcula o offset com multiplicação por `20` bytes.

### Load-use

Há instrução independente entre cálculo/lookup e uso subsequente, explicitamente marcada no source como mitigação do BUG-03-A.

### Phase wrap

O source soma `phase + delta_r`, compara/subtrai `41` e usa `csel`, sem `udiv/msub` no path documentado.

### Memória

O source escreve os campos de estado e executa:

```asm
dmb ish
```

antes do retorno de `vectra_pulse_step`.

## O que foi removido deste documento

A versão antiga continha trechos classificados como "código problemático (inferido)" e uma versão completa proposta do assembly. Eles foram removidos desta superfície normativa porque:

```text
INFERRED_SNIPPET != CURRENT_SOURCE
```

Quando se quer revisar a implementação, deve-se ler `rmr/Rrr/vectra_pulse.S`, não reconstruí-la a partir deste Markdown.

## Pontos que NÃO estão automaticamente provados

A inspeção do source não autoriza, sozinha, afirmar:

- ≤30 ciclos em hardware real;
- ausência de regressões em todos os microarquiteturas ARM64;
- execução física em Android;
- corretude do bulk path sob toda combinação de input;
- benchmark PMU/perf de dispositivo.

Esses claims exigem receipts/gates próprios. Na ausência deles: `TOKEN_VAZIO`.

## Gate documental de manutenção

Qualquer alteração futura em:

```text
period/cardinality
AttractorState layout
register ABI
offset/sizeof
memory ordering
phase wrapping
Lyapunov representation
```

deve atualizar simultaneamente o source, validators/tests relacionados e esta documentação.

## Relações

- `rmr/Rrr/attractor_table.h` define o espaço atual de 41 estados.
- `docs/01_BUG_ATTRACTOR_TABLE_INCOMPLETA.md` agora registra a transição histórica→corrente.
- `docs/BUG03_IMPLEMENTATION_RECORD.md`, quando presente, é registro histórico; o source atual permanece autoridade.

## Auditoria relacionada

- `docs/00_BUG_MASTER_INDEX.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
