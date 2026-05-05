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


## Certification and audit claim notice

This repository does not claim ISO certification, formal ISO compliance, or accredited external audit status. Any ISO/IEC references are internal checklist references or methodological alignment notes only. Certification requires an external accredited audit process and is outside the scope of this repository.

Este repositório não declara certificação formal baseada em ISO, conformidade ISO formal nem auditoria externa acreditada. Qualquer referência a ISO/IEC é apenas checklist interno, referência metodológica ou alinhamento preliminar de boas práticas. Certificação exige processo externo acreditado e está fora do escopo deste repositório.

### Audit/benchmark/runtime trail
- `docs/AUDIT_CLAIMS_POLICY.md`
- `reports/vectra_grade_benchmarks.md`
- `reports/device_runtime_smoke.md`
- `reports/rmr_equivalence.md`
- CI validação não equivale a validação em device real.
- Benchmark definido não equivale a benchmark medido.
