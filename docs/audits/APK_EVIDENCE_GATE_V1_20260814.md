# RAFCODEΦ APK Evidence Gate V1 — 2026-08-14

Status: `IMPLEMENTED_BRANCH / EVIDENCE_GATED`

## Invariant

`SOURCE != BUILD != APK_IDENTITY != SIGNATURE != PHYSICAL_RUNTIME != CLAIM`

`claim_allowed=false` until the relevant evidence gates are closed.

## Why this exists

The repository already has an APK build matrix, bootstrap integrity checks, signing, SHA-256 output and release contracts. The missing layer was a deterministic final-artifact verifier that can independently inspect the generated APK rather than trusting the build script's filename or configuration.

## Implemented artifacts

- `scripts/verify_apk_evidence.py`
  - streams SHA-256;
  - runs full ZIP CRC/decompression verification;
  - parses Android binary XML (`AndroidManifest.xml`) without `aapt`;
  - extracts package, versionName and versionCode;
  - verifies JAR/v1 with `jarsigner` when available;
  - verifies APK Signature Scheme v1/v2/v3/v3.1/v4 with Android `apksigner` when available;
  - evaluates an explicit JSON contract;
  - emits a machine-readable receipt;
  - never promotes runtime or scientific claims.

- `scripts/build_apk_matrix_with_evidence.sh`
  - pins `git rev-parse HEAD`;
  - requires a clean tracked source tree unless `ALLOW_DIRTY_BUILD=1` is explicit;
  - executes the existing `scripts/build_apk_matrix.sh` unchanged;
  - rejects tracked-source mutation during build;
  - locates the same Android Build Tools `apksigner` used by the matrix;
  - writes `dist/apk-matrix/evidence/BUILD_PROVENANCE.json` and its SHA-256;
  - runs the APK evidence verifier over every signed APK;
  - writes per-APK evidence receipts and `RECEIPT_SHA256SUMS.txt`;
  - writes `APK_EVIDENCE_INDEX.json`;
  - preserves `physical_runtime=TOKEN_VAZIO`.

- `data/contracts/apk_rafcodephi_release.v1.json`
  - expected package `com.termux.rafacodephi`;
  - v1 + v2 signature requirements for signed release artifacts;
  - ARM32/ARM64 runtime targets;
  - runtime remains TOKEN_VAZIO until physical execution.

- `data/contracts/apk_baseline_com.termux_0.118.3.v1.json`
  - freezes the uploaded historical baseline by package, version and SHA-256;
  - explicitly states that it is not the current default RAFCODEΦ build.

- `tests/test_verify_apk_evidence.py`
  - synthetic Android binary XML fixture;
  - manifest identity extraction test;
  - invalid-root fail-closed test.

## Reference validation performed before repository write

Uploaded baseline:

- package: `com.termux`
- versionName: `0.118.3`
- versionCode: `1002`
- SHA-256: `e6265a57eb5ca363808488e3b01955958bed93bc0c8a0d281849b363b11027ec`
- full ZIP CRC: PASS
- JAR/v1 verification: PASS
- baseline contract checks: 5/5 PASS
- parser unit tests: 2/2 PASS
- `apksigner` in reference container: unavailable, therefore v2/v3/v4 remain `TOKEN_VAZIO_TOOL_MISSING` there.

This reference execution is not a physical Android runtime test and is not repository CI.

## Execution

Canonical evidence build:

```bash
./scripts/build_apk_matrix_with_evidence.sh
```

Historical baseline verification when the APK is locally present:

```bash
python3 scripts/verify_apk_evidence.py \
  /path/to/com.termux_1002.apk \
  --contract data/contracts/apk_baseline_com.termux_0.118.3.v1.json \
  --out dist/baseline-apk-evidence.json
```

## Current gaps / TOKEN_VAZIO

1. `TV-APK-V2V3V4-BASELINE`: verify the uploaded historical APK with Android `apksigner` in an environment that has Build Tools.
2. `TV-BUILD-RECEIPT-LINK`: V1 materializes and hashes `BUILD_PROVENANCE.json`, but deliberately does not yet promote it to `provenance_claim_allowed`; receipt-to-contract linkage needs an independent pin verification step.
3. `TV-PHYSICAL-INSTALL-BOOT-ARM32`: install/boot test on a physical ARM32 Android device.
4. `TV-PHYSICAL-INSTALL-BOOT-ARM64`: install/boot test on a physical ARM64 Android device.
5. `TV-REPRODUCIBLE-SECOND-BUILD`: second clean checkout build and byte/hash comparison.
6. `TV-OFFICIAL-SIGNER-CERT-PIN`: official release signer certificate digest must be pinned separately from internal/local validation keys.

## Operational route

`SOURCE_COMMIT -> CLEAN_TREE -> BUILD -> SIGN -> SHA256 -> MANIFEST_PARSE -> CRC -> APKSIGNER -> RECEIPT -> SECOND_BUILD -> PHYSICAL_INSTALL -> BOOT -> RUNTIME_RECEIPT -> CLAIM_GATE`

No missing gate is silently converted into PASS.
