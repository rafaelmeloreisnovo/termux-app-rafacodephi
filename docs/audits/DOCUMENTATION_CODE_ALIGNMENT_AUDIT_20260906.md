# Documentation ↔ Code Alignment Audit — 2026-09-06

> Repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Initial audited baseline: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Reconciliation baseline: `3f97ef42ae9756b9f7fb4965b941b5b3048fc8d1` (PR #415 merged while this audit was in progress).
> Scope: documentation only. No executable source, workflow, build script, test or runtime behavior is modified by this audit branch.
> Audit posture: source-first, fail-closed, append-only reasoning. `TOKEN_VAZIO` means evidence not yet observed; it is never promoted to PASS.

## 1. Role separation

1. **Audit — Class A:** inspect source, workflows, tests, contracts and receipts; identify contradictions, stale claims, scope collisions and missing evidence.
2. **Review — Class B:** normalize terminology, references, status language and navigation without widening technical claims.
3. **Technical writing — Class B:** update normative documentation so it describes what the current source actually does.

Documentation changes MUST NOT silently become implementation claims.

## 2. Evidence hierarchy

```text
CURRENT SOURCE / TEST / WORKFLOW
    > CURRENT MACHINE-READABLE CONTRACT
    > CURRENT RECEIPT / REPORT
    > NORMATIVE DOCUMENTATION
    > HISTORICAL REPORT
    > HYPOTHESIS / INFERRED EXAMPLE
```

An inferred snippet is not evidence that the snippet exists in the repository. A historical bug report is not evidence that the bug remains open. A CI structural PASS is not physical-device proof.

## 3. Source observations

### 3.1 Termux-packages source contract

`data/contracts/termux-packages-rafcodephi-pin.v1.json` defines:

- `canonical`: `837afec42ecf5f9ac1bd8b00e65d143bc23a380b`, state `MERGED_BASELINE`;
- `candidate`: `0ffb24a5a6be58316236383a6d249544c39eb3e3`, state `CURRENT_MAIN_PIN_VALIDATION`.

The candidate supersedes stale commit `1fc540b0c296581c5793c109e3834589f85a0114`; historical PR #89 is recorded as merged. Neither channel widens `claim_allowed`; `physical_android` remains `TOKEN_VAZIO`.

### 3.2 Pin resolver

`scripts/resolve_termux_packages_pin.py` validates repository identity, package name, prefix, ABI set and claim/device boundaries before resolving `canonical`, `candidate` or an exact SHA.

Documentation must name workflow + selector/ref + resolved commit. "Current pin" by itself is ambiguous.

### 3.3 libLLVM18 beta workflow

`.github/workflows/beta-build-libllvm18-unblock.yml` performs source-capability preflight before the expensive source build. It checks the exact checkout, required source files, `libllvm18` host closure, manifest/schema/naming tokens and architecture capability.

The strict receipt runs only on `success()`. On upstream failure the workflow writes state `UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE`, records present/missing downstream evidence and preserves the primary-cause boundary.

```text
UPSTREAM FAILURE != DOWNSTREAM FILE MISSING AS ROOT CAUSE
```

### 3.4 Main beta workflow has a separate explicit-pin route

`.github/workflows/beta-build.yml` carries its own exact default `termux-packages` SHA (`2538114ca05a7f9c0849d9a1e6bf764702f038a0`) and supports an explicit `workflow_dispatch` ref.

This route must not be conflated with the channel-resolved libLLVM18 route.

### 3.5 API/sharedUserId drift

`tests/test_termux_api_access_contract.py` requires the Termux API permission with `signature` protection and explicitly asserts that `android:sharedUserId` is absent from the main manifest. Current documentation claiming the main app uses `sharedUserId="com.termux"` is stale.

### 3.6 Bootstrap prefix

Current Termux RAFCODEPHI surfaces use:

```text
package = com.termux.rafacodephi
prefix  = /data/data/com.termux.rafacodephi/files/usr
```

Legacy `/data/data/com.termux/files/usr` references must be qualified as upstream/legacy/risk/historical.

### 3.7 Module-scoped attractor cardinality — critical finding

The repository contains different current cardinalities in different modules.

RMR/VECTRA pulse:

```text
rmr/Rrr/attractor_table.h → count=41, period=41, indices 0..40
rmr/Rrr/vectra_pulse.S    → period 41 / indices 0..40
```

RAFAELIA Verbovivo graph:

```text
rafaelia/verbovivo_graph.h → ATTRACTOR_COUNT 42u
rafaelia/t7_toroid_builder.c → consumes ATTRACTOR_COUNT
```

Therefore:

```text
RMR_ATTRACTOR_COUNT(41) != VERBOVIVO_ATTRACTOR_COUNT(42)
```

unless an explicit bridge/decision proves equivalence. A global 42→41 rewrite would be a regression.

### 3.8 CTI, ZrManifest, Lyapunov and hotfix source contradict old inferred snippets

Observed current source:

- CTI scanner uses its actual traversal/index logic; the old `scan_idx++` example was inferred.
- `cti_scanner_barrier.h` and `cti_race_condition_validator.c` exist with a Makefile gate.
- `zipraf_manifest_pool.*` and `zipraf_index.h` provide current ZrManifest mitigation structure.
- `scripts/hotfix_ate_compilar.sh` uses `set -euo pipefail`; the old fake hash snippet was not current source.
- Lyapunov has `lyapunov_convergence.c`, validator and Makefile gate; the old hypothetical macro is not source authority.

### 3.9 Reconciliation with PR #415 — freestanding ARM gate

While the audit branch was open, PR #415 advanced `master` from `b207970...` to `3f97ef42...`.

The changed files of #415 did not overlap the 12 documentation files already modified by this audit. The audit branch was then merged with the new master ancestry without force-push.

New observed surfaces:

```text
bootstrap/proot_freestanding.c
bootstrap/proot_syscall_bridge.h
scripts/build_freestanding_real_arm_bootstrap.py
.github/workflows/freestanding-runtime-gate.yml
docs/FREESTANDING_PROOT_PKG_GATE_V1.md
```

The gate itself is freestanding; `pkg`, apt/dpkg, PRoot, Ninja, Clang, CMake and QEMU remain package payloads and are not reclassified as freestanding binaries.

The workflow statically builds ARMv7 and AArch64 gate ELFs and rejects `PT_INTERP`, `DT_NEEDED` and undefined external symbols. Its receipt deliberately preserves:

```text
build_state=BUILD_PROVEN
device_runtime_state=TOKEN_VAZIO
claim_allowed=false
```

This delta was integrated into `ENGINEERING_RUNBOOK_RAFCODEPHI.md` and `RUNTIME_TRUTH_TABLE.md`.

## 4. Documentation drift found and disposition

| Finding | Severity | Disposition |
|---|---:|---|
| old owner presented as current | HIGH | corrected in normative/indexed docs; historical reports remain custody |
| inferred snippets treated as implementation | HIGH | removed/reclassified in active bug docs |
| current `sharedUserId` conflict claim | HIGH | superseded by executable test contract |
| bootstrap failure causality undocumented | HIGH | runbook/source-contract/truth-table updated |
| multiple pin authorities collapsed | HIGH | routes explicitly separated |
| RMR 41 vs Verbovivo 42 conflated | CRITICAL DOC RISK | module-scope invariant added |
| old BUG index mixed historical/current | HIGH | converted to evidence ledger |
| old action plan prescribed already-superceded fixes | HIGH | replaced with evidence-driven plan |
| structural-risk doc asserted hypotheses as source facts | HIGH | rewritten with evidence classes |
| freestanding gate could be misread as making PRoot/pkg/Ninja freestanding | HIGH | explicit control-core vs payload boundary documented |

## 5. Required documentation states

Every active technical assertion should be expressible as one of:

- `SOURCE_OBSERVED`
- `TEST_ENFORCED`
- `WORKFLOW_WIRED`
- `BUILD_PROVEN`
- `RUNTIME_PROVEN`
- `DEVICE_PROVEN`
- `REPRODUCED`
- `HISTORICAL`
- `HYPOTHESIS`
- `STALE`
- `TOKEN_VAZIO`

`RESOLVED` alone is insufficient without a source/test/receipt pointer.

## 6. Documentation-only change set

Updated/created:

- `CLAUDE.md`
- `docs/AUDIT_CLAIMS_POLICY.md`
- `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`
- `docs/BOOTSTRAP_SOURCE_CONTRACT.md`
- `docs/RUNTIME_TRUTH_TABLE.md`
- `docs/00_BUG_MASTER_INDEX.md`
- `docs/01_BUG_ATTRACTOR_TABLE_INCOMPLETA.md`
- `docs/03_BUG_VECTRA_PULSE_AARCH64.md`
- `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md`
- `docs/05_FALHAS_ESTRUTURAIS_ARQUITETURA.md`
- `docs/06_PLANO_ACAO_EXECUCAO.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`

No executable file is modified by this audit PR.

## 7. Historical archive policy

Historical audit/report files may still contain previous owner names, previous RMR designs, already-executed action plans, old CI state or inferred snippets. They are retained for custody rather than silently rewritten. They must not outrank current source.

## 8. Current claim boundary

```text
claim_allowed = false
physical_android = TOKEN_VAZIO
production_release = BLOCKED unless current release gates independently prove otherwise
```

Documentation may describe structural implementation and CI wiring but cannot convert them into physical-device proof.

## 9. Audit invariants

```text
DOCUMENTATION_TRUTH(t) = CURRENT_SOURCE(t) + CURRENT_EVIDENCE(t) - STALE_INFERENCE(t)
MODULE_SCOPE precedes NUMERIC_UNIFICATION
freestanding_control_gate != freestanding_package_payload
TOKEN_VAZIO remains valid until evidence exists
```
