# Workflow Control Plane Refactor — 2026-08-14 — V1

## Baseline observed

The `master` workflow tree contained 50 YAML workflows before this refactor branch. The repository already had an ownership document, a `ci_track`/`ci_abis` compatibility validator, a unified RAFAELIA pipeline, ARM32 compatibility lanes, bootstrap gates, APK evidence gates, release workflows, benchmarks and Vectras specialists.

The operational problem was not absence of workflows; it was **operator surface fragmentation and duplicated implementation**. A non-expert user could reasonably ask “which workflow do I run?” and encounter dozens of similarly named actions.

## V1 intervention

This branch adds a single human entrypoint:

- `.github/workflows/00-rafaelia-control-plane.yml`
- visible name: `🧭 RAFAELIA — Executar / Diagnosticar`

Four user intents are exposed:

1. `diagnostico`
2. `arm32-v7`
3. `bootstrap-arm32`
4. `completo-seguro`

ARM32 toolchain selection remains explicit (`canonical` or `ndk29`).

## Structural refactor

The duplicated heavy logic of:

- `compatibility-arm32.yml`
- `compatibility-arm32-ndk29.yml`

was extracted into:

- `_reusable-arm32-compat.yml`

The original workflow names remain as compatibility wrappers to reduce branch-protection/check-name regression risk.

The following workflows were made callable by the control plane while retaining their specialist/manual roles:

- `run_tests.yml`
- `beta-real-bootstrap-contract.yml`
- `apk-evidence-gate.yml`

## Whole-tree governance

`scripts/ci/workflow_control_plane.py` scans every workflow and emits JSON/Markdown inventory with SHA-256, metadata, trigger/callability and safety-structure observations. Missing metadata is represented as `TOKEN_VAZIO`/warning unless strict mode is explicitly requested.

This means the phrase “all YML” is handled in V1 as **whole-tree discovery + governance + migration topology**, not as a destructive 50-file big-bang rewrite.

## Why no big-bang rewrite

Changing every trigger/job/check name in one commit would create unnecessary risk:

- branch-protection check names could disappear;
- release/signing lanes could accidentally change semantics;
- historical specialist workflows could lose manual recovery paths;
- expensive CI could multiply rather than consolidate;
- a syntax error in one mass rewrite could disable a large surface at once.

Therefore V1 uses an append-compatible strategy:

`inventory → one human entrypoint → reusable core → wrappers → evidence → family-by-family migration`.

## Evidence boundary

The following remain intentionally distinct:

`workflow_discovered != workflow_executed != build_proof != apk_proof != device_proof != release_certification`.

A GitHub Actions PASS must not close `device_runtime_proof`. Physical Android evidence remains `TOKEN_VAZIO` until a post-build APK is installed and a new device receipt is exported.

## Validation state at authoring

- branch created from current `master`: observed
- static compare: observed
- control-plane contract test: committed, execution pending PR CI
- GitHub Actions syntax/runtime: `TOKEN_VAZIO` until PR event
- ARM32 artifact from new reusable pillar: `TOKEN_VAZIO`
- physical Android bootstrap receipt: `TOKEN_VAZIO`
- `claim_allowed=false`

## Next gate

Open draft PR, observe all PR-triggered Actions, isolate any failure by job/step, then manually run the new control plane with:

`mission=bootstrap-arm32`, `ndk_lane=canonical`, `strict_governance=false`.

Only after the ARM32 artifact is produced should the device installation/bootstrap receipt gate be attempted.
