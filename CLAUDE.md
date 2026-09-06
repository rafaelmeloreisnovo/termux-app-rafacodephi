# CLAUDE.md — Claude Code adapter for `termux-app-rafacodephi`

@AGENTS.md
@docs/00_BUG_MASTER_INDEX.md
@docs/AUDIT_CLAIMS_POLICY.md
@docs/RUNTIME_TRUTH_TABLE.md

This file is an adapter, not an architectural authority. `AGENTS.md` defines repository-wide governance and explicitly forbids importing another module's assembly/register/attractor rules as root authority.

## Session start

Before editing:

1. Read `AGENTS.md`.
2. Read `docs/00_BUG_MASTER_INDEX.md` and the current source for the target module.
3. Read the exact test/workflow/contract that governs the claim.
4. Record repository, branch, HEAD and target paths.
5. Distinguish source observation from build/runtime/device evidence.
6. Do not merge without explicit human authorization.

## Evidence order

```text
CURRENT SOURCE / TEST / WORKFLOW
  > CURRENT MACHINE-READABLE CONTRACT
  > CURRENT RECEIPT / REPORT
  > NORMATIVE DOCUMENTATION
  > HISTORICAL REPORT
  > HYPOTHESIS / INFERRED EXAMPLE
```

`TOKEN_VAZIO != PASS`.

## Critical scope boundary: attractor cardinality is module-specific

Do **not** flatten the repository into one global attractor constant.

### RMR/VECTRA pulse surface

Current source:

```text
rmr/Rrr/attractor_table.h
rmr/Rrr/attractor_table.c
rmr/Rrr/vectra_pulse.S
```

This surface currently declares:

```text
count/period = 41
index range  = 0..40
```

### RAFAELIA Verbovivo graph surface

Current source:

```text
rafaelia/verbovivo_graph.h
rafaelia/verbovivo_graph.c
rafaelia/t7_toroid_builder.c
```

This surface currently declares:

```text
ATTRACTOR_COUNT = 42
```

Therefore:

```text
RMR_ATTRACTOR_COUNT(41) != VERBOVIVO_ATTRACTOR_COUNT(42)
```

unless an explicit source-level bridge proves equivalence. Never change one module merely to make it numerically match the other.

## Historical BUG-01..08 documents

Old bug documents contained inferred snippets and an obsolete blocking cascade. They are not command authority.

Current navigation/status authority is:

- `docs/00_BUG_MASTER_INDEX.md`
- `docs/01_BUG_ATTRACTOR_TABLE_INCOMPLETA.md`
- `docs/03_BUG_VECTRA_PULSE_AARCH64.md`
- `docs/04_BUG_BOOTSTRAP_E_SISTEMICOS.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`

Always read the source pointer listed there before editing.

## Current RMR observations

### Attractor table

`rmr/Rrr/attractor_table.h` exposes 41-state metadata and validation API.

### AArch64 pulse

`rmr/Rrr/vectra_pulse.S` states and contains structural fixes for the historical four BUG-03 items: bounds/indexing, anti-hazard scheduling, memory barrier and division-free wrap.

These are source observations, not physical-device proof.

### Lyapunov

Use the current implementation/validator and Makefile gate:

```text
rmr/Rrr/lyapunov_convergence.c
rmr/Rrr/lyapunov_convergence_validator.c
make lyapunov-convergence-gate
```

Do not implement a hypothetical macro from an old Markdown unless current source requirements independently call for it.

### CTI

Use:

```text
rmr/Rrr/cti_raw_reader.c
rmr/Rrr/cti_scanner_barrier.h
rmr/Rrr/cti_race_condition_validator.c
```

Do not add `_Atomic scan_idx` based solely on the historical inferred example; the current scanner must be reviewed directly.

## Bootstrap/package source

Machine-readable route authority:

```text
data/contracts/termux-packages-rafcodephi-pin.v1.json
scripts/resolve_termux_packages_pin.py
```

The `beta-build-libllvm18-unblock.yml` route resolves the `candidate` channel and performs source-capability preflight before the expensive source build.

A downstream manifest missing after an upstream failure is a consequence unless evidence proves otherwise.

```text
UPSTREAM_FAILURE != DOWNSTREAM_MISSING_ARTIFACT_AS_ROOT_CAUSE
```

## Android/Termux API identity

`tests/test_termux_api_access_contract.py` is the current executable contract for the main-manifest access boundary. It requires signature permission and absence of `android:sharedUserId` in the main manifest.

Do not revive old documentation claiming current dependency on `sharedUserId="com.termux"`.

## Claim discipline

Keep these layers separate:

```text
SOURCE_OBSERVED
TEST_ENFORCED
WORKFLOW_WIRED
BUILD_PROVEN
RUNTIME_PROVEN
DEVICE_PROVEN
REPRODUCED
```

A source file, compiled artifact, APK, CI PASS, Android install and runtime receipt are different evidence objects.

## Coding/editing discipline

- Never alter binary layouts silently.
- Never suppress gate failures with `|| true` or unconditional success.
- Preserve ARM32 support unless the exact scope explicitly says otherwise.
- Preserve `claim_allowed=false` and `physical_android=TOKEN_VAZIO` unless bounded evidence promotes them.
- Use current module constants; do not transplant constants across RMR/Verbovivo/Vectra/Termux boundaries.
- Historical text may be preserved, but it must be labeled `HISTORICAL`, `STALE` or `HYPOTHESIS` where appropriate.

## Handoff

Finish material work with:

```text
F_ok   = actually changed/observed/demonstrated
F_gap  = unknown/blocked/contradicted/unexecuted
F_next = smallest reproducible next verification
```

Never fill a missing link with narrative confidence.
