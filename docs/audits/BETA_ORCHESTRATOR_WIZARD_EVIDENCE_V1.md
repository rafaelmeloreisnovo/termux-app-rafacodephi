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
local orchestration receipt
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

## 5. State Machine

```text
IDLE
  ↓
BOOTSTRAP_PREFLIGHT
  ├─ BLOCKED → persist receipt → STOP
  └─ PASS
       ↓
PA_SINGLE? 
  ├─ FAIL → persist receipt → STOP
  └─ PASS / not selected
       ↓
PA_SERIES_N30?
  ├─ USER_CANCELLED → BLOCKED → persist receipt → STOP
  ├─ TRIAL_FAIL → FAIL → persist receipt → STOP
  └─ TARGET_REACHED / not selected
       ↓
SERIES_ANALYSIS?
  ├─ INVALIDATED → FAIL → persist receipt → STOP
  └─ state preserved / not selected
       ↓
METHOD_EXPORT?
  ├─ FAIL → persist receipt → STOP
  └─ PASS / not selected
       ↓
LOCAL_ORCHESTRATOR_PASS
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

### Failover

Bootstrap BLOCKED does not trigger a hidden alternate runtime. The operator is routed to the Wizard. Failover means a safe recovery route, not a gate bypass.

### Rollback

Measurement stages are non-destructive. Orchestrator receipts are written with `AtomicFile`; failed publication cannot intentionally create a half-written latest receipt. Bootstrap filesystem mutation remains within `TermuxInstaller` and is not falsely advertised as transactionally reversible by this orchestrator.

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

## 8. Evidence Receipt

Schema:

```text
rafcodephi.beta-evidence-orchestrator/v1
```

Persistence:

```text
$FILES/rafcodephi-beta-orchestrator/history/<run_id>.json
$FILES/rafcodephi-beta-orchestrator/latest.json
```

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
| cancellation | implemented between atomic stages/trials | smoke required |
| PA watchdog | existing 60 s runner timeout | smoke required |
| receipt atomicity | implemented | filesystem failure-path test desirable |
| rollback of bootstrap mutation | not claimed | separate installer contract |
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
- receipt write failure;
- governed trial runtime/timing failure;
- deterministic series identity drift;
- thermal interference retained in history;
- analysis invalidation;
- export failure;
- user cancellation;
- duplicate start attempt.

No claim of NASA, NSA, FBI, Pentagon, military certification or governmental approval is made. The engineering objective is hardened, auditable, fail-closed behavior using explicit evidence contracts.

## 11. Remaining TOKEN_VAZIO / P0

1. Compile/build proof for this branch.
2. Automated contract tests on CI.
3. Install candidate APK on the target Android device.
4. Wizard/readiness smoke with actual runtime prefix.
5. One PA protocol-v2 receipt.
6. Full governed n=30 device series if distribution evidence is desired.
7. Cancellation smoke during a governed series.
8. Receipt filesystem failure-path test where practical.
9. Human review of UI wording/usability.
10. Only after the above: evaluate first-beta promotion under the repository's release policy.
