# RAFCODEPHI — attestation of installed APK, CI 42811514174 (V1)

**Status:** source/config implemented on isolated branch; 9/9 host mock tests passed locally. Android ADB execution NOT_RUN. Not a release.

This route is the **producer-owned installed-byte identity gate** for the three uploaded debug APKs. It does not replace the existing APT/Ninja, physical PA, bootstrap, PRoot, or release-signing tests.

## Authority and files

- Producer: `rafaelmeloreisnovo/termux-app-rafacodephi`, base `master@0653ac63538b333460a1c739c593440911cff496`.
- Immutable user-supplied CI evidence: CI run `42811514174`, see `configs/device-apk-attestation-42811514174.v1.json`.
- Host-only probe: `scripts/attest_installed_apk.py`.
- Synthetic regression: `tests/test_attest_installed_apk.py`.
- CI guard: `.github/workflows/rafcodephi-device-apk-identity-contract.yml`.
- Preexisting E2E / package probes are independent: `scripts/collect_e2e_device_receipt.sh`, `scripts/device_signed_apt_smoke.sh`, `scripts/capture_installed_apk_receipt.sh`.

## How to run (with an ADB-connected physical device)

The probe is **read-only**. It never installs, uninstalls, updates, clears app data or enables the unsigned APT channel.

```sh
python3 scripts/attest_installed_apk.py \
  --variant armeabi-v7a \
  --candidate-apk ./termux-app_real-pkg-ci-42811514174_armeabi-v7a.apk \
  --receipt ./local-private-receipts/installed-$(date -u +%Y%m%dT%H%M%SZ).json
```

For an installed ARM64 or universal APK change `--variant` and the candidate path. An exact match means only **installed base.apk bytes match the declared APK**, not that the reported PA benchmark ran on this exact package. On Android 10 a separate ADB host is required for this particular route. Existing in-app capture can be used separately, but the in-app result must also be reconciled to an exact manifest hash before promotion.

## Fail-closed rules

- Require exactly one authorized ADB-connected device; do not record its serial.
- Reject wrong ABI, absent package, split-APK layouts, unreadable/invalid `sha256sum`, and differing hashes.
- Optional candidate must match declared byte count and SHA-256 before touching device.
- Local receipts are exclusive-create (append-only by filename), sanitized, and not automatically published.
- A missing `source_commit` or `build_receipt_sha256` remains `TOKEN_VAZIO`.
- No device or release claim inherits PASS from the host tests.
- `claim_allowed_release=false`, `claim_allowed_benchmark_binding=false`, `apt_pkg_proot_e2e=TOKEN_VAZIO` regardless of identity-match status.

## Next gate

Physical device attestation for the exact uploaded APK → independent signature/build receipt gate → `pkg/apt/proot` E2E → PA receipt tied to exact installation. Keep CI debug track separate from a signed production release.

Rollback: close the DRAFT PR without merging, or revert only the scoped files if already merged; preserve original build and historical receipts.

SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM. `PASS_SCOPED ≠ RELEASE_READY`.
