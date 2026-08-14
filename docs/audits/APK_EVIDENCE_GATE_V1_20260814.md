# RAFCODEΦ APK Evidence Gate V1.1 — 2026-08-14

Status: `IMPLEMENTED_BRANCH / EVIDENCE_GATED / REAL_BUILD_PENDING`

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
  - independently verifies build provenance instead of trusting a contract declaration;
  - recomputes the build-receipt SHA-256;
  - compares receipt `source_commit` with the contract pin;
  - requires the exact APK SHA-256 to occur in the build receipt;
  - requires clean-tree evidence before and after the canonical build;
  - emits a machine-readable receipt;
  - never promotes runtime or scientific claims.

- `scripts/build_apk_matrix_with_evidence.sh`
  - pins `git rev-parse HEAD`;
  - requires a clean tracked source tree for canonical provenance;
  - executes the existing `scripts/build_apk_matrix.sh` through an explicit `bash` invocation, leaving the old build route unchanged;
  - rejects tracked-source mutation during build;
  - locates Android Build Tools `apksigner`;
  - writes `dist/apk-matrix/evidence/BUILD_PROVENANCE.json` and its SHA-256;
  - pins that SHA-256 and source commit in the generated run contract;
  - pins the expected RAFCODEΦ versionName for the run;
  - runs the APK evidence verifier over every signed APK with `--build-receipt`;
  - writes per-APK evidence receipts and `RECEIPT_SHA256SUMS.txt`;
  - refuses the final index if any signed APK fails identity/signature/provenance gates;
  - writes `APK_EVIDENCE_INDEX.json`;
  - preserves `physical_runtime=TOKEN_VAZIO`.

- `data/contracts/apk_rafcodephi_release.v1.json`
  - expected package `com.termux.rafacodephi`;
  - v1 + v2 signature requirements for signed release artifacts;
  - dynamic run version and provenance pins;
  - ARM32/ARM64 runtime targets;
  - runtime remains TOKEN_VAZIO until physical execution.

- `data/contracts/apk_baseline_com.termux_0.118.3.v1.json`
  - freezes the uploaded historical baseline by package, version and SHA-256;
  - explicitly states that it is not the current default RAFCODEΦ build.

- `tests/test_verify_apk_evidence.py`
  - standard-library `unittest` runner, so direct invocation really executes tests;
  - synthetic Android binary XML fixture;
  - manifest identity extraction;
  - invalid-root fail-closed;
  - positive build-receipt linkage;
  - negative receipt-hash mismatch;
  - negative unlisted-artifact case;
  - negative dirty-build case.

- `.github/workflows/apk-evidence-gate.yml`
  - Python syntax check;
  - direct `unittest` execution;
  - shell syntax check;
  - contract sanity check;
  - isolated from Gradle/Android build cost so this code path cannot hide behind unrelated green CI.

## Reference validation already performed

Uploaded historical baseline:

- package: `com.termux`
- versionName: `0.118.3`
- versionCode: `1002`
- SHA-256: `e6265a57eb5ca363808488e3b01955958bed93bc0c8a0d281849b363b11027ec`
- full ZIP CRC: PASS
- JAR/v1 verification: PASS
- baseline contract checks: 5/5 PASS
- original parser reference tests: 2/2 PASS
- `apksigner` in reference container: unavailable, therefore v2/v3/v4 remain `TOKEN_VAZIO_TOOL_MISSING` there.

The later provenance-negative tests are implemented in-repository and require the isolated CI observation before being classified as repository-executed PASS.

This reference execution is not a physical Android runtime test and is not a current RAFCODEΦ build.

## Execution

Canonical evidence build:

```bash
bash scripts/build_apk_matrix_with_evidence.sh
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
2. `TV-CURRENT-BUILD-EVIDENCE-RUN`: execute V1.1 against a clean current RAFCODEΦ checkout with Android Build Tools; the linkage mechanism is implemented but its real build receipt is not yet observed.
3. `TV-PHYSICAL-INSTALL-BOOT-ARM32`: install/boot test on a physical ARM32 Android device.
4. `TV-PHYSICAL-INSTALL-BOOT-ARM64`: install/boot test on a physical ARM64 Android device.
5. `TV-REPRODUCIBLE-SECOND-BUILD`: second clean checkout build and byte/hash comparison.
6. `TV-OFFICIAL-SIGNER-CERT-PIN`: official release signer certificate digest must be pinned separately from internal/local validation keys.
7. `TV-ISOLATED-CI-V1.1`: observe the exact V1.1 head running the isolated workflow; do not inherit the status of older heads.

## Operational route

`SOURCE_COMMIT -> CLEAN_TREE -> BUILD -> SIGN -> SHA256 -> MANIFEST_PARSE -> CRC -> APKSIGNER -> RECEIPT_HASH -> SOURCE_PIN -> ARTIFACT_MEMBERSHIP -> RECEIPT -> SECOND_BUILD -> PHYSICAL_INSTALL -> BOOT -> RUNTIME_RECEIPT -> CLAIM_GATE`

No missing gate is silently converted into PASS.
