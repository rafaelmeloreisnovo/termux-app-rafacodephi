# BUG-04..08 — Registro auditado de Bootstrap e Bugs Sistêmicos

> Repository atual: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Baseline auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Histórico anterior permanece recuperável no Git. Este documento substitui trechos inferidos por apontamentos verificáveis do source atual.

## Regra de evidência

```text
source/test/workflow > contract > receipt > documentation > historical hypothesis
```

Exemplos antigos marcados como `inferido`, `provável` ou `presumido` não são tratados como código real.

---

## BUG-04 — Bootstrap package/prefix/sharedUserId

### Estado documental atual

```text
SOURCE_OBSERVED + TEST_ENFORCED + WORKFLOW_WIRED
DEVICE_PROVEN = TOKEN_VAZIO
claim_allowed = false
```

### O que o source atual sustenta

Identidade/prefixo normativos:

```text
package = com.termux.rafacodephi
prefix  = /data/data/com.termux.rafacodephi/files/usr
```

O teste `tests/test_termux_api_access_contract.py` exige:

- permissão `${TERMUX_PACKAGE_NAME}.permission.TERMUX_API`;
- `android:protectionLevel="signature"`;
- ausência de `android:sharedUserId` no manifest principal.

Portanto, a narrativa antiga de que o app atual ainda usa `android:sharedUserId="com.termux"` é `STALE` para este baseline.

A fonte real ARM/ARM64 e a cadeia de bootstrap estão descritas em:

- `docs/BOOTSTRAP_SOURCE_CONTRACT.md`
- `.github/workflows/beta-build.yml`
- `.github/workflows/beta-build-libllvm18-unblock.yml`
- `scripts/prepare_bootstrap_env.sh`
- `scripts/generate_usable_beta_receipt.py`

### Falha tardia do manifest

O manifest:

```text
RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt
```

é produto do source-build de `termux-packages`; não é arquivo para ser fabricado manualmente quando um consumidor downstream reclama da ausência.

O workflow `beta-build-libllvm18-unblock.yml` agora faz preflight da capacidade da fonte antes do build caro e só executa o receipt estrito em `success()`.

Em falha upstream, registra:

```text
state=UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE
physical_android=TOKEN_VAZIO
claim_allowed=false
release_allowed=false
```

Regra causal:

```text
producer failed before artifact creation
→ artifact missing is consequence
→ do not replace the primary failure with "missing artifact"
```

### Gate restante

A correção estrutural/documental não prova instalação física, shell físico, `pkg update` ou `pkg install`.

---

## BUG-05 — `ZrManifest` / prevenção de stack overflow

### Estado observado

```text
SOURCE_OBSERVED
```

O baseline contém:

```text
rmr/Rrr/zipraf_index.h
rmr/Rrr/zipraf_index.c
rmr/Rrr/zipraf_manifest_pool.h
rmr/Rrr/zipraf_manifest_pool.c
```

`zipraf_index.h` contém guarda explícita referente a alocação acidental em stack, e existe pool estático dedicado a `ZrManifest`.

Documentos específicos de implementação/resolução também existem, incluindo:

- `docs/BUG05_ZRMANIFEST_RESOLUTION.md`
- `docs/BUG_05_ZRMANIFEST_STACK_OVERFLOW_FIX.md`

### Limite do claim

A presença do pool/guard prova implementação estrutural, não ausência universal de qualquer uso incorreto em todo runtime/device. Para claim mais amplo, usar gate/teste/receipt correspondente.

---

## BUG-06 — CTI / TOROID / concorrência

### Correção do documento antigo

O trecho antigo que inventava:

```text
int scan_idx;
s->scan_idx++;
```

foi explicitamente marcado como inferido e NÃO aparece como descrição do `rmr/Rrr/cti_raw_reader.c` atual. O scanner atual usa índices locais de iteração na rotina de scan.

O baseline também contém uma implementação/gate específico de barreira:

```text
rmr/Rrr/cti_scanner_barrier.h
rmr/Rrr/cti_race_condition_validator.c
Makefile target: cti-race-condition-gate
```

### Estado documental

```text
SOURCE_OBSERVED + GATE_WIRED
DEVICE_PROVEN = TOKEN_VAZIO unless a physical receipt is attached
```

Não usar o snippet antigo de `_Atomic scan_idx` como se fosse a implementação corrente.

---

## BUG-07 — integridade/BLAKE3 no pipeline

### Correção do documento antigo

O exemplo antigo afirmava que `hotfix_ate_compilar.sh` continha um mismatch BLAKE3 silencioso, mas o exemplo era explicitamente `inferido`.

O arquivo real atual está em:

```text
scripts/hotfix_ate_compilar.sh
```

Ele usa `set -euo pipefail` e delega preflight/hashes para `prepare_bootstrap_env.sh` e/ou o pipeline de matriz. Logo, o snippet histórico com `EXPECTED_HASH="abc123..."` não pode ser citado como código atual.

### Estado documental

```text
OLD_SNIPPET = STALE/HYPOTHESIS
CURRENT_SCRIPT = SOURCE_OBSERVED
```

Qualquer claim de integridade deve apontar ao verificador/receipt atual que materializou os hashes, não ao exemplo histórico.

---

## BUG-08 — Lyapunov `φ=(1-H)·C`

### Correção do documento antigo

A macro hipotética `VECTRA_ASSERT_LYAPUNOV(...)` mostrada na versão anterior deste documento não aparece como autoridade atual do source pesquisado.

O baseline possui implementação/gate dedicado:

```text
rmr/Rrr/lyapunov_convergence.c
rmr/Rrr/lyapunov_convergence_validator.c
Makefile target: lyapunov-convergence-gate
```

O Makefile compila e executa o `lyapunov_validator` no gate correspondente.

### Estado documental

```text
SOURCE_OBSERVED + GATE_WIRED
DEVICE/RUNTIME GENERALIZATION = TOKEN_VAZIO unless independently observed
```

Não substituir a implementação atual pela macro hipotética antiga.

---

## Matriz auditada BUG-04..08

| ID | Claim antigo problemático | Estado após auditoria | Autoridade atual |
|---|---|---|---|
| BUG-04 | hardcode/sharedUserId atual | parcialmente STALE | source contract + API access test + beta workflows |
| BUG-05 | risco descrito apenas por exemplo | implementação estrutural observada | ZIPRAF headers/pool + records específicos |
| BUG-06 | `scan_idx++` inferido | snippet STALE; gate real observado | `cti_raw_reader.c`, barrier header, race validator |
| BUG-07 | mismatch silencioso inferido | snippet STALE | `scripts/hotfix_ate_compilar.sh` + pipeline atual |
| BUG-08 | macro hipotética como fix | snippet HYPOTHESIS/STALE | Lyapunov source + validator + Makefile gate |

## Fronteira final

Nada neste documento converte estrutura em prova física:

```text
physical_android=TOKEN_VAZIO
claim_allowed=false
```

Auditoria completa de alinhamento documental:

- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
