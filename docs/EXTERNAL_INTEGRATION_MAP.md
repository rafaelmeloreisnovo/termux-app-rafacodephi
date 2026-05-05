# External Integration Map

## Scope
This document defines how termux_rafcodephi integrates with optional external components without vendoring their core code into this repository.

## Integration Roles

### blake3_rmr
- Role: bootstrap integrity, hash generation, custody records, and integration reports.
- Integration contract:
  - Not vendored by default.
  - Accessed via script backend selection (`python-blake3` or `external-rmr-pai`).
  - Used to generate integrity artifacts for bootstrap packages.
- Expected artifacts:
  - `reports/bootstrap_integrity.json`
  - `reports/bootstrap_integrity.tsv`
  - `reports/bootstrap_integrity.md`

### vectra_rmr
- Role: industrial benchmark contract, state promotion signals, policy pipeline evidence, route decisions, memory tier observations.
- Integration contract:
  - Not vendored by default.
  - Accessed via optional local path or script/artifact integration.
  - When enabled, must emit minimal benchmark contract artifacts.
- Expected artifacts:
  - `reports/vectra_integration_report.md`
  - `reports/vectra_integration_report.json`

### termux_rafcodephi
- Role: Android runtime, bootstrap packaging/validation, APK build pipeline, ABI support matrix, JNI bridge, local C/ASM lowlevel.
- Responsibilities:
  - Keep local bootstrap flow deterministic and offline-compatible.
  - Validate hash and `BOOTSTRAP_INFO` contracts for strict release flows.
  - Keep ARM32 (`armeabi-v7a`) and universal artifact support.
  - Keep native lowlevel runtime fallback paths active.

## Contracts and Boundaries
- External integrations are optional and explicit.
- Release strict mode must fail when mandatory integrity/bench contracts are enabled and missing.
- Debug mode may continue with warning-level fallback when external backends are unavailable.
- No source vendoring of blake3_rmr or vectra_rmr in this stage.

## Operational Entry Points
- Integrity: `scripts/raf_external_integrity.sh`
- Benchmark contract: `scripts/raf_external_vectra_bench.sh`
- Gradle checks:
  - `validateExternalIntegrity`
  - `validateVectraGradeBenchContract`
