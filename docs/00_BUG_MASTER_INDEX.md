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

## 2. Índice dos BUG-01..08

| ID | Componente | Documento histórico/específico | Estado documental atual | Evidência/apontamento atual |
|---|---|---|---|---|
| BUG-01 | `attractor_table` | `docs/01_BUG_ATTRACTOR_TABLE_INCOMPLETA.md` | documento original contém narrativa histórica/stale; implementação atual é `SOURCE_OBSERVED` | `rmr/Rrr/attractor_table.c`, `.h`, `attractor_table_validator.c`, alvo `attractor-table-complete-gate` no Makefile |
| BUG-02 | atrator #22 / cardinalidade | `docs/02_BUG_VOID_PARADOX_ATRATOR_22.md`, `docs/BUG02_DECISION_RECORD.md` | decisão histórica implementada precisa ser lida junto do source atual | código atual referencia espaço de 41 estados; não usar textos 42-state antigos como autoridade sem revalidação |
| BUG-03 | `vectra_pulse.S` AArch64 | `docs/03_BUG_VECTRA_PULSE_AARCH64.md` | implementação é `SOURCE_OBSERVED`; trechos marcados como inferidos no documento antigo são `HYPOTHESIS/HISTORICAL` | `rmr/Rrr/vectra_pulse.S` + gates/testes correspondentes |
| BUG-04 | bootstrap/package/prefix | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | narrativa antiga de hardcode/sharedUserId está parcialmente `STALE`; contrato atual é source-built/prefix-safe estrutural | `docs/BOOTSTRAP_SOURCE_CONTRACT.md`, `tests/test_termux_api_access_contract.py`, workflows beta |
| BUG-05 | `ZrManifest` / stack | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | histórico; estado técnico só deve ser promovido após inspeção direta do source atual | localizar instâncias reais e gate correspondente antes de claim |
| BUG-06 | CTI / concorrência | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | histórico; exemplos inferidos não são prova de race atual | `rmr/Rrr/cti_raw_reader.*` e testes/gates atuais são autoridade |
| BUG-07 | integridade/hash | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | histórico; exemplos inferidos não são prova de comportamento corrente | workflows/scripts/receipts atuais são autoridade |
| BUG-08 | invariante `φ=(1-H)·C` | `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md` | implementação matemática e claims de runtime devem permanecer separados | source/validator/report atual; device/runtime não é inferido |

## 3. Contradições documentais já detectadas

### 3.1 BUG-01

O documento antigo descreve `attractor_table` como ausente/stub, mas o baseline atual contém:

```text
rmr/Rrr/attractor_table.c
rmr/Rrr/attractor_table.h
rmr/Rrr/attractor_table_validator.c
```

Logo, "arquivo faltante" é `STALE` como descrição do baseline atual.

### 3.2 BUG-04 / sharedUserId

O documento antigo afirma conflito atual de `android:sharedUserId="com.termux"`. O teste corrente `tests/test_termux_api_access_contract.py` exige:

```text
android:sharedUserId NOT present in main manifest
TERMUX_API permission protectionLevel=signature
```

Logo, a afirmação de presença atual de `sharedUserId` é `STALE` no baseline auditado.

### 3.3 Prefixo

Prefixo corrente normativo:

```text
/data/data/com.termux.rafacodephi/files/usr
```

`/data/data/com.termux/files/usr` só deve aparecer como upstream/legacy/risk ou em registro histórico, nunca como prefixo RAFCODEPHI corrente sem qualificação.

### 3.4 Pin de termux-packages

Não existe um único "pin global" documentalmente seguro. Há pelo menos:

- canais `canonical`/`candidate` no contrato semântico;
- workflow beta principal com SHA exato próprio;
- possibilidade de exact SHA em `workflow_dispatch`.

Sempre registrar rota + selector/ref + commit resolvido.

## 4. Bootstrap/CI — incidente de falha tardia

O workflow `.github/workflows/beta-build-libllvm18-unblock.yml` foi endurecido para:

1. resolver o candidate pin pelo contrato;
2. fazer preflight de capacidade da fonte antes do build caro;
3. só gerar receipt estrito em `success()`;
4. em falha upstream, registrar `UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE`;
5. não mascarar a primeira falha com `missing downstream evidence`.

Regra do índice:

```text
producer failed → produced artifact absent = consequence
not automatically root cause
```

## 5. Áreas que permanecem sem promoção automática

Os itens abaixo exigem evidência própria e não podem ser declarados pelo simples fato de existirem source/tests:

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

## 6. Documentos que contêm material inferido/histórico

Até revalidação individual, tratar como histórico/hipótese onde o próprio texto usa `inferido`, `presumido` ou descreve owner antigo:

- `docs/01_BUG_ATTRACTOR_TABLE_INCOMPLETA.md`
- `docs/03_BUG_VECTRA_PULSE_AARCH64.md`
- `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md`
- `docs/05_FALHAS_ESTRUTURAIS_ARQUITETURA.md`
- `docs/06_PLANO_ACAO_EXECUCAO.md`
- relatórios/auditorias que citam `exacordex-crypto/termux-app-rafacodephi` como owner atual

Preservar esses arquivos para cadeia de custódia; corrigir o status/qualificação, não apagar o histórico.

## 7. Documentos normativos atuais

Para bootstrap, claims e runtime truth, começar por:

- `docs/AUDIT_CLAIMS_POLICY.md`
- `docs/BOOTSTRAP_SOURCE_CONTRACT.md`
- `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`
- `docs/RUNTIME_TRUTH_TABLE.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`

## 8. Invariante do índice

```text
BUG_STATUS_CURRENT = evidence(current_source, current_test, current_receipt)
BUG_STATUS_CURRENT != historical_sentence
```

Se o source atual ainda não foi examinado para um claim específico, usar `TOKEN_VAZIO`/`HYPOTHESIS`, não preencher por memória.
