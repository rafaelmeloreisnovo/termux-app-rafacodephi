# RAFCODEΦ First Beta Orchestrator · Wizard / Bootstrap / Evidence Contract V1

Status: `IMPLEMENTED_CANDIDATE / CLAIM_LIMITED / DEVICE_PROOF_REQUIRED`

## 1. Purpose

This contract unifies the normal operator path without collapsing independent evidence domains:

```text
Wizard + Bootstrap readiness
        ↓
PA physical observation
        ↓
optional governed n=30 series
        ↓
series analysis
        ↓
industrial methods / gap export
        ↓
canonical local orchestration receipt
        ↓
best-effort app-specific external mirror
```

Specialized screens remain available as expert tools. Existing manifest/settings entry points are preserved through compatibility classes.

## 2. Invariant

For stage states `s_i`, no downstream claim may exceed the weakest evidence required by that claim:

```text
claim_allowed(C) = ∧ required(C, i) · admissible(s_i)
```

where `admissible(PASS)=1` only for the specific gate that produced the PASS. `TOKEN_VAZIO`, `UNAVAILABLE`, `BLOCKED`, `FAIL`, `INVALIDATED` and unrelated PASS states never substitute for required evidence.

A local orchestrator PASS means only:

```text
all selected local orchestration stages completed under their own contracts
```

It does **not** mean release, certification, isolated-silicon proof or cross-device superiority.

The canonical app-private receipt is authoritative for the orchestration result. A copied/exported receipt is a convenience mirror and cannot independently promote a claim.

## 3. Shared Bootstrap Readiness Gate

The shared read-only gate is `BootstrapReadinessGate`.

Mandatory runtime-resolved targets:

```text
$PREFIX
$PREFIX/bin
$HOME
$HOME/storage
$PREFIX/bin/sh
$PREFIX/bin/pkg
$PREFIX/bin/apkmanager
$PREFIX/bin/shellbash
$PREFIX/bin/busybox-safe
$PREFIX/bin/proot-safe
```

Optional observations, not readiness requirements:

```text
$PREFIX/bin/busybox
$PREFIX/bin/proot
```

This resolves the previous semantic divergence where UI readiness could depend on real optional utility binaries while the documented bootstrap contract required safe shims.

The gate is read-only. Installation/repair remains owned by `TermuxInstaller` and bootstrap import remains owned by `BootstrapWizardSource`.

## 4. Operator UX

The primary screen exposes explicit checkboxes:

- bootstrap/readiness preflight — mandatory and not deselectable;
- one PA observation — default enabled;
- governed n=30 series — opt-in for a full evidence run;
- governed-history analysis — default enabled;
- industrial methods/gap export — default enabled.

Two normal actions exist:

```text
RUN SELECTED PIPELINE
RUN FULL BETA EVIDENCE PIPELINE
```

The full action selects all optional stages before execution.

Cancellation is cooperative:

```text
STOP AFTER CURRENT ATOMIC STAGE
```

A currently running PA trial is retained. No partial evidence is deleted to manufacture a cleaner series.

The operator can also refresh the shared readiness state and re-open the last canonical receipt without rerunning the benchmark. The UI displays both canonical and external-export paths when available.

## 5. State Machine

```text
IDLE
  ↓
BOOTSTRAP_PREFLIGHT
  ├─ BLOCKED → persist canonical receipt → STOP
  └─ PASS
       ↓
PA_SINGLE?
  ├─ FAIL → persist canonical receipt → STOP
  └─ PASS / not selected
       ↓
PA_SERIES_N30?
  ├─ USER_CANCELLED → BLOCKED → persist canonical receipt → STOP
  ├─ TRIAL_FAIL → FAIL → persist canonical receipt → STOP
  └─ TARGET_REACHED / not selected
       ↓
SERIES_ANALYSIS?
  ├─ INVALIDATED → FAIL → persist canonical receipt → STOP
  └─ state preserved / not selected
       ↓
METHOD_EXPORT?
  ├─ FAIL → persist canonical receipt → STOP
  └─ PASS / not selected
       ↓
LOCAL_ORCHESTRATOR_PASS
       ↓
CANONICAL_RECEIPT_ATOMIC
       ↓
EXTERNAL_APP_SPECIFIC_MIRROR?
  ├─ PASS → record export path
  ├─ UNAVAILABLE → preserve canonical receipt
  └─ FAIL → preserve canonical receipt + record export failure
```

## 6. Watchdog / Failsafe / Failover / Rollback

### Watchdog

Each PA trial uses the existing `PaBenchmarkRunner` bounded process execution:

```text
PROCESS_TIMEOUT_MS = 60_000
```

The orchestration layer does not weaken or replace that timeout.

### Failsafe

Mandatory stage failure stops dependent execution. There is no continue-on-error promotion path.

If the Activity is destroyed while work is active, it requests cooperative cancellation after the current atomic stage/trial. A destroyed Activity is not allowed to keep receiving UI updates. This bounds the lifecycle leak while retaining the current trial receipt.

### Failover

Bootstrap BLOCKED does not trigger a hidden alternate runtime. The operator is routed to the Wizard. Failover means a safe recovery route, not a gate bypass.

External export failure falls back only to the canonical private receipt; it never changes a failed benchmark gate into PASS.

### Rollback

Measurement stages are non-destructive. Orchestrator receipts are written with `AtomicFile`; failed publication cannot intentionally create a half-written latest receipt. Bootstrap filesystem mutation remains within `TermuxInstaller` and is not falsely advertised as transactionally reversible by this orchestrator.

External mirroring follows a fail-closed sequence: the canonical receipt first survives with `external_export_state=NOT_MEASURED`; only a successful mirror may be recorded as `PASS`. An external failure is written back to the canonical receipt as `FAIL` or `UNAVAILABLE` without deleting prior evidence.

## 7. Statistical Boundary

The orchestrator reuses `PaBenchmarkSeriesAnalyzer.MIN_DISTRIBUTION_N = 30` and does not alter V3 rules:

```text
R0…R5 remain separate metric families
cross-series pooling = false
ad-hoc → governed promotion = false
silent warm-up deletion = false
silent outlier deletion = false
```

For workload family `k`, if a governed homogeneous series reaches `n ≥ 30`, the analyzer may summarize its distribution with the existing statistics contract. This does not independently imply reproducibility or cross-device comparability.

The V3 interpretation remains:

```text
n >= 30 ⇒ distribution summary may become admissible
n >= 30 ⇏ reproducibility
n >= 30 ⇏ environmental stability
n >= 30 ⇏ cross-device comparability
```

## 8. Evidence Receipt

Schema:

```text
rafcodephi.beta-evidence-orchestrator/v1
```

Canonical persistence:

```text
$FILES/rafcodephi-beta-orchestrator/history/<run_id>.json
$FILES/rafcodephi-beta-orchestrator/latest.json
```

Best-effort app-specific external mirror when Android exposes `getExternalFilesDir("beta-evidence")`:

```text
$EXTERNAL_APP_FILES/beta-evidence/history/<run_id>.json
$EXTERNAL_APP_FILES/beta-evidence/latest.json
```

Export states are explicit:

```text
NOT_MEASURED
UNAVAILABLE
PASS
FAIL
```

`UNAVAILABLE` or `FAIL` for the mirror does not invalidate the already-written canonical receipt; it does prevent any claim that an external result artifact exists successfully.

Mandatory top-level release boundaries remain false:

```text
claim_allowed_release=false
claim_allowed_certification=false
claim_allowed_cross_device_comparison=false
claim_allowed_isolated_silicon=false
```

## 9. Beta Closure Matrix

| Domain | Current code contract | Physical/device evidence |
|---|---|---|
| Shared Wizard/bootstrap readiness | implemented candidate | required on installed APK |
| PA one-shot orchestration | implemented candidate | required |
| governed n=30 orchestration | implemented candidate | TOKEN_VAZIO until executed |
| series analysis | existing V3 + orchestrated | depends on receipts |
| methods/gap export | existing V3 + orchestrated | generated on device |
| canonical receipt atomicity | implemented | filesystem smoke/failure-path desirable |
| external receipt mirror | implemented best-effort | device/filesystem smoke required |
| latest receipt recovery | implemented bounded read | reopen/rotation smoke required |
| cancellation | implemented between atomic stages/trials | smoke required |
| lifecycle teardown failsafe | implemented cooperative cancel | rotation/back smoke required |
| PA watchdog | existing 60 s runner timeout | timeout smoke desirable |
| rollback of bootstrap mutation | not claimed by orchestrator | separate installer contract |
| release/certification | BLOCKED | independent CI/review/device evidence required |

## 10. Security / Reliability Failure Model

The following are explicitly considered:

- missing/malformed bootstrap target;
- wrong runtime path / relocated Android-assigned path;
- missing executable bit;
- optional real busybox/proot absent while safe shim exists;
- PA ELF missing;
- linker unavailable;
- process timeout;
- stdout truncation;
- canonical receipt write failure;
- external export directory unavailable;
- external export write failure;
- oversized/unreadable latest receipt;
- governed trial runtime/timing failure;
- deterministic series identity drift;
- thermal interference retained in history;
- analysis invalidation;
- method export failure;
- user cancellation;
- Activity destruction during a run;
- duplicate start attempt.

No claim of NASA, NSA, FBI, Pentagon, military certification or governmental approval is made. The engineering objective is hardened, auditable, fail-closed behavior using explicit evidence contracts.

## 11. Remaining TOKEN_VAZIO / P0

1. Compile/build proof for the final PR head.
2. Automated contract tests on the final PR head.
3. Install candidate APK on the target Android device.
4. Wizard/readiness smoke with actual runtime prefix.
5. One PA protocol-v2 receipt.
6. Full governed n=30 device series if distribution evidence is desired.
7. Cancellation smoke during a governed series.
8. Activity teardown/reopen/latest-receipt smoke.
9. Canonical/external receipt filesystem failure-path test where practical.
10. Human review of UI wording/usability.
11. Only after the above: evaluate first-beta promotion under the repository's release policy.
