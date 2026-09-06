# Documentation ↔ Code Alignment Audit — 2026-09-06

> Repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Audited baseline: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Scope: documentation only. No executable source, workflow, build script, test or runtime behavior is modified by this audit branch.
> Audit posture: source-first, fail-closed, append-only reasoning. `TOKEN_VAZIO` means evidence not yet observed; it is never promoted to PASS.

## 1. Role separation

This repository must keep three activities distinct:

1. **Audit — Class A:** inspect source, workflows, tests, contracts and receipts; identify contradictions, stale claims and missing evidence.
2. **Review — Class B:** normalize terminology, references, status language and navigation without widening technical claims.
3. **Technical writing — Class B:** update normative documentation so it describes what the current source actually does.

Documentation changes MUST NOT silently become implementation claims.

## 2. Evidence hierarchy for documentation

Use this precedence when documents disagree:

```text
CURRENT SOURCE / TEST / WORKFLOW
    > CURRENT MACHINE-READABLE CONTRACT
    > CURRENT RECEIPT / REPORT
    > NORMATIVE DOCUMENTATION
    > HISTORICAL REPORT
    > HYPOTHESIS / INFERRED EXAMPLE
```

An inferred snippet is not evidence that the snippet exists in the repository.
A historical bug report is not evidence that the bug remains open.
A CI structural PASS is not device/runtime proof.

## 3. Source observations from baseline

### 3.1 Termux-packages source contract

`data/contracts/termux-packages-rafcodephi-pin.v1.json` defines two named channels:

- `canonical`: `837afec42ecf5f9ac1bd8b00e65d143bc23a380b`, state `MERGED_BASELINE`;
- `candidate`: `0ffb24a5a6be58316236383a6d249544c39eb3e3`, state `CURRENT_MAIN_PIN_VALIDATION`.

The candidate explicitly supersedes stale commit `1fc540b0c296581c5793c109e3834589f85a0114`; historical PR #89 is recorded as merged. Neither channel widens `claim_allowed`; `physical_android` remains `TOKEN_VAZIO`.

### 3.2 Pin resolver

`scripts/resolve_termux_packages_pin.py` validates repository identity, package name, prefix, ABI set, claim boundary and physical-device boundary before resolving `canonical`, `candidate`, or an exact 40-character SHA.

Documentation therefore must name the route it describes. The phrase "current termux-packages pin" is ambiguous unless the workflow/channel is identified.

### 3.3 libLLVM18 beta workflow

`.github/workflows/beta-build-libllvm18-unblock.yml` now performs a source-capability preflight before the expensive source build. The preflight verifies the checked-out SHA, required source files, `libllvm18` host closure, manifest schema token, bootstrap ZIP naming token and `--architectures` capability.

The strict semantic custody receipt runs only on `success()`. On upstream failure, the workflow writes `usable-beta-receipt.json` in state `UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE`, records present/missing downstream evidence, preserves `TOKEN_VAZIO`, and states that missing downstream evidence is a consequence rather than the root cause.

This is the canonical documentation rule for the incident class:

```text
UPSTREAM FAILURE != DOWNSTREAM FILE MISSING AS ROOT CAUSE
```

### 3.4 Main beta workflow uses its own explicit pin

`.github/workflows/beta-build.yml` currently carries its own exact default `termux-packages` SHA (`2538114ca05a7f9c0849d9a1e6bf764702f038a0`) and supports an explicit `workflow_dispatch` ref.

This is a separate route from the channel-resolved `libLLVM18` workflow. Documentation must not collapse the two into one global pin authority.

### 3.5 API/sharedUserId documentation drift

`tests/test_termux_api_access_contract.py` asserts that the main manifest contains the `signature` permission and explicitly asserts that `android:sharedUserId` is absent. Therefore documentation claiming the current main app "uses android:sharedUserId=com.termux" is stale for the audited baseline.

### 3.6 Bootstrap prefix

Current source and validation surfaces consistently use:

```text
package = com.termux.rafacodephi
prefix  = /data/data/com.termux.rafacodephi/files/usr
```

Historical documents describing `/data/data/com.termux/files/usr` as the active RAFCODEPHI runtime path must be marked historical, risk/example, or stale unless tied to a specific legacy artifact.

## 4. Documentation drift found

| Finding | Severity | Class | Corrective documentation action |
|---|---:|---|---|
| old repository identity `exacordex-crypto/termux-app-rafacodephi` appears in active docs | HIGH | provenance drift | use current repository identity; preserve old owner only when explicitly historical |
| inferred code snippets are presented inside bug documents as if describing current code | HIGH | evidence drift | label as historical hypothesis/example; do not use as closure/open-state proof |
| BUG-04 document claims current `sharedUserId` conflict | HIGH | source contradiction | supersede with current signature-permission/no-sharedUserId source evidence |
| source-built bootstrap receipt failure semantics were undocumented | HIGH | incident-diagnosis drift | document preflight + primary-cause preservation |
| multiple pin authorities can be mistaken for one global pin | MEDIUM | route ambiguity | always name workflow + channel/exact SHA |
| runtime table predates the 2026-09-06 source-capability preflight | MEDIUM | freshness drift | add rows for pin resolution, preflight and failure receipt semantics |
| old BUG master index mixes resolved, historical and inferred statements | HIGH | epistemic mixing | convert index into navigational/status ledger with explicit evidence classes |

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

`RESOLVED` alone is insufficient for a current technical claim unless a source/test/receipt pointer accompanies it.

## 6. Documentation-only change set for this audit

This branch updates the following authoritative surfaces:

- `docs/AUDIT_CLAIMS_POLICY.md`
- `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`
- `docs/BOOTSTRAP_SOURCE_CONTRACT.md`
- `docs/RUNTIME_TRUTH_TABLE.md`
- `docs/00_BUG_MASTER_INDEX.md`
- `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md`

No executable files are in scope.

## 7. Remaining audit pointers

The following documents contain historical/inferred wording and must be read under the evidence hierarchy above until individually revalidated against current source:

- `docs/01_BUG_ATTRACTOR_TABLE_INCOMPLETA.md`
- `docs/03_BUG_VECTRA_PULSE_AARCH64.md`
- `docs/05_FALHAS_ESTRUTURAIS_ARQUITETURA.md`
- `docs/06_PLANO_ACAO_EXECUCAO.md`
- historical audit/report files that name an old repository owner

Their existence is preserved for chain-of-custody. This audit does not delete historical reasoning; it prevents it from outranking current source.

## 8. Current claim boundary

```text
claim_allowed = false
physical_android = TOKEN_VAZIO
production_release = BLOCKED unless current release gates independently prove otherwise
```

The documentation may describe structural implementation and CI wiring, but must not convert them into physical-device proof.

## 9. Audit invariant

```text
DOCUMENTATION_TRUTH(t) = CURRENT_SOURCE(t) + CURRENT_EVIDENCE(t) - STALE_INFERENCE(t)
```

When evidence is absent, record `TOKEN_VAZIO`; do not manufacture a conclusion.
