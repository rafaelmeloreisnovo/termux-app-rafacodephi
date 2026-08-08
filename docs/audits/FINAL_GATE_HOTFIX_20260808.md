# Final Gate Hotfix — 2026-08-08

## Scope

Minimal convergence hotfix for the two concrete CI failures observed after PR #333.

## Closed defects

1. `rafaelia_pipeline.yml` calls `:app:printVersionName`, but the task was absent.
   - The root Gradle contract now registers `:app:printVersionName` alongside the Android application plugin.
   - The task prints the configured base `versionName` and fails closed if it cannot be resolved.

2. Legacy/general GitHub Actions paths invoked `scripts/setup_android_toolchain.sh` and then Gradle/NDK without materializing `rewritten-bootstrap-*.zip`.
   - On GitHub Actions, the toolchain helper now invokes `prepare_bootstrap_env.sh --github-env --skip-android-preflight` after SDK/NDK setup.
   - Local developer behavior is preserved; this automatic step is gated by `GITHUB_ACTIONS=true`.
   - `TERMUX_SKIP_BOOTSTRAP_PREPARE=1` remains an explicit escape hatch only for jobs that provably do not build the app/native bootstrap target.

## Regression gate

`validate_release_pipeline_contract.sh` now asserts:

- `:app:printVersionName` is registered;
- the RAFAELIA workflow consumes it;
- CI toolchain setup prepares rewritten bootstraps;
- recursive Android preflight is explicitly skipped during that preparation.

## Claim boundary

This hotfix closes wiring failures only. It does not by itself prove Android physical-device installation/runtime, performance, or release readiness.

`claim_allowed=false` remains appropriate until the mandatory CI matrix and device receipts pass.
