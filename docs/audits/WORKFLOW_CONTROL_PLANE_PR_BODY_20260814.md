# PR summary — workflow control plane V1

## Problem

The repository has a large, capable GitHub Actions surface, but the human operator must choose among many specialist YAML workflows. ARM32 canonical and NDK29 compatibility workflows also duplicate most heavy implementation.

## Change

- Add one human entrypoint: `🧭 RAFAELIA — Executar / Diagnosticar`.
- Expose four plain missions: `diagnostico`, `arm32-v7`, `bootstrap-arm32`, `completo-seguro`.
- Add whole-tree workflow inventory/governance with SHA-256 and TOKEN_VAZIO preservation.
- Extract the duplicated ARM32 implementation into `_reusable-arm32-compat.yml`.
- Keep the existing ARM32 workflow names as thin compatibility wrappers.
- Make unit tests, bootstrap contract and APK evidence contract callable from the control plane.
- Add operator/runbook docs, static contract tests and append-only governance checkpoint.

## Safety

No release/signing workflow is silently rerouted. No physical-device claim is inferred from CI. `device_runtime_proof=TOKEN_VAZIO` and `claim_allowed=false` remain explicit until a post-build APK is installed on Android and a new physical receipt is observed.

## Migration strategy

This is intentionally not a destructive 50-file big-bang rewrite. V1 governs the whole workflow tree and establishes the single operator surface; remaining specialist families can be migrated behind reusable pillars incrementally with equivalence evidence.

## Requested validation

1. GitHub accepts all workflow syntax.
2. Automatic control-plane inventory lane passes.
3. Existing PR checks show no material regression.
4. Then manually run `bootstrap-arm32 / canonical` and observe the ARM32 artifact before returning to the device gate.
