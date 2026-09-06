# Plano de Ação Documental e de Evidência — RAFAELIA/VECTRA_OS

> Repository atual: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Baseline auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`
> A versão histórica deste arquivo prescrevia implementação de BUG-01..08. Várias dessas prescrições já foram superadas pelo source atual. Este plano agora é **evidence-driven** e não manda reaplicar patches históricos.

## 1. Regra central

```text
OBSERVAR SOURCE → LOCALIZAR GATE → LER RECEIPT → CLASSIFICAR CLAIM → ATUALIZAR DOC
```

Nunca:

```text
grep vazio → concluir patch ausente → aplicar patch automaticamente
```

A ausência de uma string não prova ausência semântica de uma correção.

## 2. Itens históricos já superados

Não executar como plano corrente:

- gerar uma nova `attractor_table[42]` a partir de valores hipotéticos deste Markdown;
- adicionar state #22 dual-mode conforme proposta histórica;
- aplicar novamente os quatro fixes de `vectra_pulse.S` descritos no documento antigo;
- adicionar `_Atomic int scan_idx` apenas porque o exemplo inferido o sugeria;
- adicionar `VECTRA_ASSERT_LYAPUNOV` apenas porque o exemplo hipotético o sugeria;
- substituir `sharedUserId` presumido sem antes observar o manifest/test atual;
- editar `hotfix_ate_compilar.sh` com base no snippet BLAKE3 inventado no documento antigo.

Essas instruções eram hipóteses/planos históricos, não autoridade atual.

## 3. Estado estrutural corrente observado

### Attractor table

```text
rmr/Rrr/attractor_table.c
rmr/Rrr/attractor_table.h
rmr/Rrr/attractor_table_validator.c
```

Cardinalidade corrente documentada no header: 41, índices `[0..40]`.

### AArch64 pulse

```text
rmr/Rrr/vectra_pulse.S
```

O source declara e contém as quatro correções estruturais BUG-03-A..D.

### CTI

```text
rmr/Rrr/cti_raw_reader.c
rmr/Rrr/cti_scanner_barrier.h
rmr/Rrr/cti_race_condition_validator.c
```

### Lyapunov

```text
rmr/Rrr/lyapunov_convergence.c
rmr/Rrr/lyapunov_convergence_validator.c
```

### ZrManifest

```text
rmr/Rrr/zipraf_index.h
rmr/Rrr/zipraf_manifest_pool.h
rmr/Rrr/zipraf_manifest_pool.c
```

### Termux API identity

`tests/test_termux_api_access_contract.py` exige permissão `signature` e ausência de `android:sharedUserId` no manifest principal.

## 4. Gates estruturais a usar como referência

O `Makefile` possui gates específicos, incluindo:

```text
attractor-table-complete-gate
lyapunov-convergence-gate
cti-race-condition-gate
```

A existência do target é `GATE_WIRED`; o resultado de uma execução só é PASS quando houver execução/receipt correspondente.

## 5. Bootstrap/package stack

### Contrato semântico

```text
data/contracts/termux-packages-rafcodephi-pin.v1.json
```

No baseline:

```text
canonical = 837afec42ecf5f9ac1bd8b00e65d143bc23a380b
candidate = 0ffb24a5a6be58316236383a6d249544c39eb3e3
```

### Resolver

```bash
python3 scripts/resolve_termux_packages_pin.py canonical --json
python3 scripts/resolve_termux_packages_pin.py candidate --json
```

### Workflow de candidate/libLLVM18

`.github/workflows/beta-build-libllvm18-unblock.yml` deve ser lido como cadeia:

```text
resolve pin
→ checkout exact SHA
→ preflight source capabilities
→ source-build ARM/AArch64 + manifest
→ gate bootstrap pair
→ import/semantic validation
→ APK matrix
→ strict receipt on success
```

Se houver falha upstream:

```text
UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE
```

preserva o primeiro erro e registra evidências downstream faltantes sem mascarar a causa.

## 6. Plano corrente de fechamento documental

### D1 — identidade/proveniência

Todo documento normativo deve usar:

```text
rafaelmeloreisnovo/termux-app-rafacodephi
```

Owners antigos só podem permanecer qualificados como históricos.

### D2 — eliminar código inferido como autoridade

Para todo Markdown que use `inferido`, `provável`, `presumido`:

- preservar como `HYPOTHESIS` quando tiver valor histórico;
- apontar o source atual correspondente;
- nunca deixar snippet inferido parecer implementação corrente.

### D3 — claims

Todo claim técnico ativo deve carregar uma classe:

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

### D4 — produtor/consumidor de artefatos

Documentar sempre:

```text
producer → artifact → consumer → validator → receipt
```

para impedir erro secundário de arquivo ausente virar falsa causa-raiz.

### D5 — pinning

Nunca documentar apenas "pin atual". Registrar:

```text
workflow
selector_or_exact_ref
resolved_commit
contract/schema
```

### D6 — runtime físico

Não promover estrutura/CI para device:

```text
physical_android=TOKEN_VAZIO
```

até receipt físico atual.

## 7. Checklist documental de release

Antes de uma documentação dizer que existe release funcional, exigir apontamentos para:

```text
[ ] app commit exato
[ ] workflow exato
[ ] termux-packages ref/commit exato
[ ] bootstrap manifest + ARM ZIP + ARM64 ZIP
[ ] hashes/receipt do bootstrap
[ ] APK receipt
[ ] assinatura aplicável
[ ] package repository state
[ ] ARM32 physical receipt
[ ] ARM64 physical receipt
[ ] runtime/package smoke
[ ] claim_allowed correspondente
```

Qualquer item ausente é `TOKEN_VAZIO` ou `BLOCKED`, conforme o contrato; não deve ser preenchido por linguagem otimista.

## 8. Estado do plano antigo

```text
HISTORICAL_IMPLEMENTATION_PLAN = SUPERSEDED
CURRENT_PLAN = DOCUMENTATION_AND_EVIDENCE_ALIGNMENT
```

O histórico permanece disponível no Git para cadeia de custódia.

## 9. Documentos normativos para navegação

- `docs/AUDIT_CLAIMS_POLICY.md`
- `docs/00_BUG_MASTER_INDEX.md`
- `docs/BOOTSTRAP_SOURCE_CONTRACT.md`
- `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`
- `docs/RUNTIME_TRUTH_TABLE.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`

## 10. Invariante final

```text
PLANO_ATUAL(t) = source_current(t) + evidence_current(t)
                 - stale_instruction(t)
```

`TOKEN_VAZIO` permanece estado válido até a evidência existir.
