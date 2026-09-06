# RAFAELIA / VECTRA_OS — Bug Master Index

> Repository atual: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Baseline documental auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Este arquivo é um índice navegável. Documentos antigos podem preservar estados históricos, hipóteses e nomes de forks anteriores; eles não ultrapassam o source atual.

## 1. Regra de leitura

Use a seguinte precedência:

```text
CURRENT SOURCE / TEST / WORKFLOW
  > CURRENT CONTRACT
  > CURRENT RECEIPT / REPORT
  > NORMATIVE DOC
  > HISTORICAL DOC
  > INFERRED / HYPOTHETICAL SNIPPET
```

Estados aceitos neste índice:

```text
SOURCE_OBSERVED
TEST_ENFORCED
WORKFLOW_WIRED
BUILD_PROVEN
RUNTIME_PROVEN
DEVICE_PROVEN
REPRODUCED
HISTORICAL
HYPOTHESIS
STALE
TOKEN_VAZIO
```

`RESOLVED` sem apontamento de evidência não é suficiente para afirmar estado corrente.

## 2. Regra crítica de escopo — 41 e 42 coexistem em módulos diferentes

A auditoria do baseline encontrou duas cardinalidades reais em superfícies distintas:

### RMR/VECTRA pulse

```text
rmr/Rrr/attractor_table.h
rmr/Rrr/attractor_table.c
rmr/Rrr/vectra_pulse.S
```

Estado observado:

```text
count/period = 41
index range  = 0..40
```

### RAFAELIA Verbovivo graph

```text
rafaelia/verbovivo_graph.h
rafaelia/verbovivo_graph.c
rafaelia/t7_toroid_builder.c
```

Estado observado:

```text
ATTRACTOR_COUNT = 42
```

Portanto:

```text
RMR_ATTRACTOR_COUNT(41) != VERBOVIVO_ATTRACTOR_COUNT(42)
```

Não existe autorização para "uniformizar" os dois por documentação. Toda claim de cardinalidade deve nomear o módulo.

## 3. Índice dos BUG-01..08

| ID | Componente | Documento histórico/específico | Estado documental atual | Evidência/apontamento atual |
|---|---|---|---|---|
| BUG-01 | `rmr/Rrr/attractor_table` | `docs/01_BUG_ATTRACTOR_TABLE_INCOMPLETA.md` | narrativa antiga 42-state/stub é `STALE`; implementação RMR atual é `SOURCE_OBSERVED` | `rmr/Rrr/attractor_table.c`, `.h`, `attractor_table_validator.c`, `attractor-table-complete-gate` |
| BUG-02 | cardinalidade/VOID histórico do RMR | `docs/02_BUG_VOID_PARADOX_ATRATOR_22.md`, `docs/BUG02_DECISION_RECORD.md` | decisão histórica deve ser lida junto do RMR source atual | RMR usa 41; isso não altera automaticamente Verbovivo 42 |
| BUG-03 | `rmr/Rrr/vectra_pulse.S` AArch64 | `docs/03_BUG_VECTRA_PULSE_AARCH64.md` | implementação atual é `SOURCE_OBSERVED`; snippets inferidos antigos foram superseded | `rmr/Rrr/vectra_pulse.S` + gates/testes correspondentes |
| BUG-04 | bootstrap/package/prefix | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | narrativa antiga de hardcode/sharedUserId parcialmente `STALE`; contrato atual source-built/prefix-safe estrutural | `docs/BOOTSTRAP_SOURCE_CONTRACT.md`, API access test, workflows beta |
| BUG-05 | `ZrManifest` / stack | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | mitigação estrutural observada | `zipraf_index.h`, `zipraf_manifest_pool.*`, records específicos |
| BUG-06 | CTI / concorrência | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | exemplo `scan_idx` inferido não descreve o scanner corrente | `cti_raw_reader.c`, `cti_scanner_barrier.h`, race validator/gate |
| BUG-07 | integridade/hash | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | snippet BLAKE3 inferido é `STALE/HYPOTHESIS` | scripts/workflows/receipts reais são autoridade |
| BUG-08 | invariante `φ=(1-H)·C` | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | macro hipotética antiga não é autoridade; implementação/gate atuais existem | `lyapunov_convergence.c`, validator, Makefile gate |

## 4. Contradições documentais já detectadas

### 4.1 BUG-01

O documento antigo descrevia `attractor_table` como ausente/stub, mas o baseline contém o source/validator RMR. Logo, "arquivo faltante" é `STALE` como descrição do baseline atual.

### 4.2 Cardinalidade

"42-state" não é globalmente errado: é source real do módulo Verbovivo. O erro é usar 42 como autoridade sobre o RMR atual, ou usar 41 para reescrever Verbovivo sem source-level decision.

### 4.3 BUG-04 / sharedUserId

O teste corrente `tests/test_termux_api_access_contract.py` exige:

```text
android:sharedUserId NOT present in main manifest
TERMUX_API permission protectionLevel=signature
```

Logo, a afirmação de presença atual de `sharedUserId="com.termux"` é `STALE` no baseline auditado.

### 4.4 Prefixo

Prefixo corrente normativo da superfície Termux RAFCODEPHI:

```text
/data/data/com.termux.rafacodephi/files/usr
```

`/data/data/com.termux/files/usr` só deve aparecer como upstream/legacy/risk ou registro histórico.

### 4.5 Pin de termux-packages

Não existe um único "pin global" documentalmente seguro. Há:

- canais `canonical`/`candidate` no contrato semântico;
- workflow beta principal com SHA exato próprio;
- possibilidade de exact SHA no `workflow_dispatch`.

Sempre registrar rota + selector/ref + commit resolvido.

## 5. Bootstrap/CI — incidente de falha tardia

O workflow `.github/workflows/beta-build-libllvm18-unblock.yml` foi endurecido para:

1. resolver candidate pelo contrato;
2. preflight da capacidade da fonte antes do build caro;
3. receipt estrito somente em `success()`;
4. em falha upstream, `UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE`;
5. não mascarar a primeira falha com downstream evidence ausente.

```text
producer failed → produced artifact absent = consequence
not automatically root cause
```

## 6. Áreas que permanecem sem promoção automática

Exigem evidência própria:

- runtime físico ARM32;
- runtime físico ARM64;
- package repository custom publicado/assinado;
- `pkg update` real em device;
- `pkg install` real em device;
- dual-ARM matrix física;
- release final assinada/reproduzida independentemente.

Enquanto não observados:

```text
physical_android=TOKEN_VAZIO
claim_allowed=false
```

## 7. Documentos históricos e normativos

Os antigos documentos BUG-01, BUG-03, BUG-04, análise estrutural e plano de ação foram reclassificados/revisados na auditoria documental de 2026-09-06. Outros relatórios antigos podem preservar owner anterior, estado 42-state do RMR histórico ou instruções já executadas.

Preservar história não significa dar a ela precedência sobre o source atual.

Documentos normativos para começar:

- `AGENTS.md`
- `CLAUDE.md`
- `docs/AUDIT_CLAIMS_POLICY.md`
- `docs/BOOTSTRAP_SOURCE_CONTRACT.md`
- `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`
- `docs/RUNTIME_TRUTH_TABLE.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`

## 8. Invariantes do índice

```text
BUG_STATUS_CURRENT = evidence(current_source, current_test, current_receipt)
BUG_STATUS_CURRENT != historical_sentence
MODULE_A_CONSTANT != MODULE_B_CONSTANT unless an explicit bridge says so
```

Se o source atual ainda não foi examinado para um claim específico, usar `TOKEN_VAZIO`/`HYPOTHESIS`, não preencher por memória.
