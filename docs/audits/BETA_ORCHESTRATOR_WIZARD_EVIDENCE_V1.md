# RAFCODEΦ First Beta Orchestrator · Wizard / Bootstrap / Evidence Contract V1

Status: `IMPLEMENTED_CANDIDATE / CLAIM_LIMITED / DEVICE_PROOF_REQUIRED`

## 1. Purpose

The normal operator path is now one composition without collapsing independent evidence domains:

```text
Wizard / Bootstrap repair
        ↓
READ-ONLY BOOTSTRAP READINESS
        ↓
PA physical observation
        ↓
optional governed n=30 series
        ↓
series analysis
        ↓
industrial V3 methods / gap export
        ↓
canonical orchestration receipt
        ↓
app-specific mirror + optional user-selected SAF copy
```

Expert diagnostic screens remain available. Historical manifest/settings/Vectra entry points are preserved through compatibility classes.

## 2. Fundamental evidence invariant

For claim `C` and evidence gates `g_i`:

```text
claim_allowed(C) = ∧ᵢ [ required(C,gᵢ) ⇒ admissible(gᵢ) ]
```

`PASS` is local to the gate that produced it. `TOKEN_VAZIO`, `UNAVAILABLE`, `BLOCKED`, `FAIL`, `INVALIDATED`, `OBSERVED_LIMITED` or an unrelated `PASS` cannot substitute for required evidence.

The orchestrator therefore distinguishes:

```text
orchestration_execution_state = PASS
state                         = OBSERVED_LIMITED
publication_gate_state        = BLOCKED
claim_allowed_release         = false
```

when all selected local work executes successfully but publication/release evidence is still absent. This is intentional: successful orchestration is not equivalent to full evidentiary promotion.

## 3. Shared Bootstrap Readiness Gate

`BootstrapReadinessGate` is the one read-only runtime readiness contract consumed by both Wizard and Beta Orchestrator.

The current source has **two distinct scopes**. `evaluateStartup()` requires only these runtime-resolved targets:

```text
$PREFIX                  directory read/write/execute
$PREFIX/bin              directory read/write/execute
$HOME                    directory read/write/execute
$PREFIX/bin/sh           executable
$PREFIX/bin/pkg          executable
```

These are **observations, not startup blockers**: `$HOME/storage`;
compatibility wrappers `apkmanager`, `shellbash`, `busybox-safe`,
`proot-safe`; and native binaries `apt`, `apt-get`, `dpkg`, `bash`,
`busybox`, `proot`. Their absence stays visible as `UNAVAILABLE`.
The full package runtime independently inventories native executables; its
aggregate PASS requires startup PASS **and** the installed profile contract,
rather than treating an optional wrapper as a substitute for a native package
manager.

### 3.1 Installed profile contract (FULL_PACKAGE_RUNTIME only)

`$PREFIX/BOOTSTRAP_PROFILE.json` is mandatory for the **full** package
readiness gate `evaluate()`, not the minimal startup gate. The current
source requires:

```text
schema               = rafcodephi-bootstrap-profile/v1
profile              = real-pkg
package_layer        = real-pkg
runtime_materialized = true
package_name         = current Android package
prefix               = runtime-resolved PREFIX
arch                 = current ABI mapping
claim_allowed        = false
release_allowed      = false
device_validation    = TOKEN_VAZIO
```

`required_entries` must be bounded, relative and remain canonically inside
`$PREFIX`. Materialized targets must exist; `SYMLINKS.txt` is a
source-archive installation instruction consumed by the installer and is
not required as an installed runtime file. A `bridge` profile may describe
another source/build path, but it **does not** satisfy this full real-pkg gate.

This section supersedes the older combined-target list, which incorrectly
treated optional wrappers and storage as startup blockers and allowed a
`bridge` profile in the real-pkg full-runtime contract. The device export
remains separately authoritative for **installed** state: current source
correctness is not physical proof that the observed APK contains it.

The profile is read with a 64 KiB bound. The readiness gate contains no `mkdir`, `chmod`, delete, bootstrap install or repair operation. Mutation belongs to `TermuxInstaller`/Wizard. This avoids using `BootstrapBaremetalGuard.validateAfterBootstrap()` as a UI readiness oracle because that install-time guard may create/chmod directories and its debug strictness policy is not equivalent to a fail-closed read-only gate.

## 4. Operator UX

The primary surface exposes understandable selectable work:

- **Bootstrap/Wizard readiness** — mandatory, selected and not deselectable;
- **One PA observation** — default selected;
- **Governed n=30 series** — optional for fast smoke, selected by the full-beta action;
- **Governed-history analysis** — default selected;
- **Industrial V3 methods/gap export** — default selected.

Actions:

```text
RUN SELECTED PIPELINE
RUN FULL BETA EVIDENCE PIPELINE
STOP AFTER CURRENT ATOMIC STAGE
OPEN BOOTSTRAP / PERMISSIONS WIZARD
OPEN VECTRA EXPERT DIAGNOSTICS
REFRESH READINESS + PROCESS STATE + LATEST RECEIPT
EXPORT LATEST RECEIPT…
```

An empty optional plan is rejected; bootstrap preflight by itself cannot manufacture a successful benchmark orchestration.

## 5. Process-wide single-flight invariant

Concurrent evidence pipelines inside the Android process are forbidden:

```text
N_active_evidence_pipeline ≤ 1
```

The lock is process-wide, not Activity-local. This prevents two screens/instances from running simultaneous PA series and contaminating scheduler, memory, thermal and DVFS observations.

A recreated Activity can observe that a process-wide run is still active. It cannot start another one.

## 6. State machine

```text
IDLE
  ↓
BOOTSTRAP_PREFLIGHT
  ├─ BLOCKED → canonical receipt → STOP
  └─ PASS
       ↓
PA_SINGLE?
  ├─ FAIL → canonical receipt → STOP
  └─ PASS / not selected
       ↓
PA_SERIES_N30?
  ├─ CANCEL → BLOCKED → canonical receipt → STOP
  ├─ TRIAL_FAIL → FAIL → canonical receipt → STOP
  └─ TARGET_REACHED / not selected
       ↓
SERIES_ANALYSIS?
  ├─ INVALIDATED → FAIL → canonical receipt → STOP
  └─ state retained / not selected
       ↓
METHOD_EXPORT?
  ├─ FAIL → canonical receipt → STOP
  └─ PASS / not selected
       ↓
LOCAL EXECUTION COMPLETED
  orchestration_execution_state=PASS
  evidence state=OBSERVED_LIMITED
  publication=BLOCKED
       ↓
CANONICAL RECEIPT
       ↓
BEST-EFFORT MIRRORS
```

## 7. Watchdog / failsafe / failover / rollback

### Watchdog

Each PA process preserves the existing runner bound:

```text
PROCESS_TIMEOUT_MS = 60_000
```

The orchestrator does not replace or weaken that watchdog.

### Failsafe

A mandatory dependent gate stops on failure. There is no continue-on-error route that promotes the overall evidence claim.

Cancellation is cooperative. The current atomic PA trial is retained; cancellation is observed before the next trial/stage. Partial evidence is never deleted merely to make a series look cleaner.

When the Activity is destroyed, it requests cooperative cancellation and stops accepting callbacks into the dead UI.

### Failover

A BLOCKED bootstrap does not silently switch runtime or bypass the gate. Recovery means opening the Wizard/repair route.

An external export failure falls back only to the canonical app-private receipt; it never changes measurement state.

### Rollback

Measurement work is non-destructive. Receipt publication uses Android `AtomicFile` + flush + `fsync`.

Bootstrap filesystem changes remain under the installer’s separate rollback contract. The orchestrator does not falsely claim transactional rollback over bootstrap mutation.

## 8. Statistical boundary

The orchestrator reuses `PaBenchmarkSeriesAnalyzer.MIN_DISTRIBUTION_N = 30` and retains V3 invariants:

```text
R0…R5 metric families remain separate
heterogeneous workload pooling = false
cross-series pooling            = false
ad-hoc → governed promotion     = false
silent warm-up deletion         = false
silent outlier deletion         = false
```

For homogeneous governed workload family `k`:

```text
n ≥ 30 ⇒ distribution summary may become admissible
n ≥ 30 ⇏ reproducibility
n ≥ 30 ⇏ environmental stability
n ≥ 30 ⇏ cross-device comparability
```

The analyzer’s `NOT_MEASURED`, `INVALIDATED` and `OBSERVED_LIMITED` states are preserved rather than normalized to PASS.

## 9. Receipt and export model

Schema:

```text
rafcodephi.beta-evidence-orchestrator/v1
```

### 9.1 Canonical authority

```text
$FILES/rafcodephi-beta-orchestrator/history/<run_id>.json
$FILES/rafcodephi-beta-orchestrator/latest.json
```

The canonical app-private receipt is authoritative for orchestration state.

### 9.2 App-specific external mirror

When Android exposes `getExternalFilesDir("beta-evidence")`:

```text
$EXTERNAL_APP_FILES/beta-evidence/history/<run_id>.json
$EXTERNAL_APP_FILES/beta-evidence/latest.json
```

State is explicit:

```text
NOT_MEASURED | UNAVAILABLE | PASS | FAIL
```

Canonical evidence is written conservatively with external export `NOT_MEASURED` before any mirror can be promoted. Mirror failure is written back as failure/unavailability without deleting canonical evidence.

### 9.3 User-selected SAF copy

`EXPORT LATEST RECEIPT…` uses Android Storage Access Framework `ACTION_CREATE_DOCUMENT` with MIME `application/json`. The operator chooses the destination. The copy is flushed and fsynced.

Its authority is explicitly:

```text
copy_only
canonical_receipt_remains_authoritative
```

No broad storage permission is required for this export route.

## 10. Claim boundary

These remain false at the orchestrator level:

```text
claim_allowed_release=false
claim_allowed_certification=false
claim_allowed_cross_device_comparison=false
claim_allowed_isolated_silicon=false
```

No claim of NASA, NSA, FBI, Pentagon, military certification, government approval, SPEC certification, MLPerf certification or equivalent conformance is made. The engineering target is hardened, auditable, fail-closed operation using practices analogous to high-assurance/industrial systems where applicable.

## 11. Beta closure matrix

| Domain | Code state | Required remaining evidence |
|---|---|---|
| Shared Wizard/bootstrap readiness | implemented candidate | installed-device smoke |
| Strict read-only installed-profile gate | implemented candidate | installed-device smoke/adversarial profile test |
| PA one-shot orchestration | implemented candidate | physical protocol-v2 receipt |
| Governed n=30 orchestration | implemented candidate | physical n=30 execution |
| Series analysis | V3 existing + orchestrated | governed receipts |
| Process-wide single-flight | implemented | multi-Activity/device smoke |
| Cooperative cancellation | implemented | cancellation during n=30 smoke |
| Lifecycle teardown failsafe | implemented | back/rotation/reopen smoke |
| PA watchdog | existing 60 s | timeout-path smoke desirable |
| Canonical receipt atomicity | implemented | filesystem failure-path smoke desirable |
| App-specific mirror | implemented | device/filesystem smoke |
| User-selected SAF export | implemented | chooser/cancel/write smoke |
| Methods/gap export | V3 existing + orchestrated | generated device artifact |
| Bootstrap transactional rollback | installer-owned, not claimed here | separate installer validation |
| Publication/release/certification | BLOCKED | independent CI + physical + review evidence |

## 12. Failure model covered by code/contracts

- wrong/missing runtime prefix;
- missing required directories/executable bits;
- missing/malformed/oversized bootstrap profile;
- package/prefix/ABI/profile mismatch;
- `claim_allowed` or `release_allowed` incorrectly true;
- `device_validation` not TOKEN_VAZIO;
- unsafe/missing profile required entry;
- optional real busybox/proot absent;
- PA ELF/linker unavailable;
- PA timeout/stdout bound;
- runtime/timing trial failure;
- duplicate process-wide start;
- empty optional plan;
- user cancellation;
- Activity destruction/recreation;
- deterministic series identity drift;
- thermal interference retained rather than deleted;
- analysis invalidation;
- method export failure;
- canonical receipt write failure;
- external mirror unavailable/write failure;
- SAF destination cancelled/unavailable/write failure;
- oversized/unreadable latest receipt.

## 13. Remaining TOKEN_VAZIO / P0 before first-beta promotion

1. CI/build proof on the **final PR head**.
2. Automated contract tests on that same head.
3. Build artifact/ABI policy proof for candidate APK.
4. Install candidate APK on the target Android device.
5. Shared Wizard/readiness smoke against actual installed runtime/profile.
6. One physical PA protocol-v2 receipt.
7. Governed n=30 device series when distribution evidence is required.
8. Cancellation during series smoke.
9. Back/rotation/reopen/single-flight smoke.
10. Canonical + app-specific + SAF export smoke.
11. Human usability/release review.
12. Only then evaluate first-beta promotion under release policy.
