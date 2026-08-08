# RAFCODEPHI E2E Product Proof v1

## Objective

Turn the RAFCODEPHI Android/Termux stack into one falsifiable product path:

`termux-packages source → bootstrap ZIP → APK build → physical device → workload → receipt → reproduction`

The chain is split into three evidence envelopes so no layer can silently promote the next one.

## Gate model

1. **Bootstrap source manifest — `termux-packages`**
   - validates a `real-pkg` bootstrap profile;
   - records `termux-packages` commit, ABI, package/prefix and bootstrap SHA-256;
   - keeps `device_runtime=TOKEN_VAZIO` and `claim_allowed=false`.

2. **Build receipt — `termux-app-rafacodephi`**
   - verifies the local bootstrap ZIP SHA-256 equals the source manifest;
   - hashes the built APK;
   - records both repositories/commits in one build envelope;
   - keeps physical runtime `TOKEN_VAZIO`.

3. **Physical-device receipt**
   - consumes the build receipt;
   - verifies the installed `base.apk` SHA-256 equals the build receipt;
   - checks the canonical RAFCODEPHI prefix and required `sh/bash/pkg/apt/dpkg/proot` surface;
   - executes and hashes a workload;
   - leaves `reproduction=TOKEN_VAZIO`.

A fourth step compares two independent physical receipts. Only that validator may return `claim_allowed=true`.

## 1. Produce bootstrap source manifest

On `rafaelmeloreisnovo/termux-packages` after producing a canonical `real-pkg` bootstrap ZIP:

```bash
python3 scripts/emit_rafcodephi_bootstrap_source_manifest.py \
  --artifact <bootstrap.zip> \
  --arch aarch64 \
  --out reports/bootstrap-source-arm64.json
```

Use `--arch arm` for the 32-bit ARM artifact.

This step proves artifact identity and profile compatibility only. It does not prove DEB repository reachability, Android installation, runtime, or performance.

## 2. Bind bootstrap to APK build

In `rafaelmeloreisnovo/termux-app-rafacodephi`, with the exact source manifest and bootstrap ZIP used by the build:

```bash
python3 scripts/emit_e2e_build_receipt.py \
  --bootstrap-source-manifest reports/bootstrap-source-arm64.json \
  --bootstrap-zip <exact-bootstrap-used-by-build.zip> \
  --apk <built.apk> \
  --out reports/e2e-build-arm64.json
```

The command fails closed if the bootstrap ZIP hash differs from the `termux-packages` source manifest.

## 3. Collect receipt A on physical Android

Copy `reports/e2e-build-arm64.json` onto the installed `com.termux.rafacodephi` environment and run:

```bash
bash scripts/collect_e2e_device_receipt.sh \
  --build-receipt reports/e2e-build-arm64.json \
  --out reports/device-e2e/device-a.json
```

For a deterministic application workload:

```bash
bash scripts/collect_e2e_device_receipt.sh \
  --build-receipt reports/e2e-build-arm64.json \
  --workload 'your deterministic command here' \
  --out reports/device-e2e/device-a.json
```

The collector requires the installed APK hash to match the build receipt. A mismatch is `BLOCKED`.

## 4. Collect receipt B

Perform an independent rerun from the same build provenance and save it as `device-b.json`. A different device is stronger evidence; a fresh independent run on the same device is still useful but must not be described as independent hardware replication.

## 5. Evaluate reproduction

```bash
python3 scripts/validate_e2e_receipt.py \
  reports/device-e2e/device-b.json \
  --reference reports/device-e2e/device-a.json
```

Promotion is `PASS` only when both receipts have the first six stages `PASS` and match on:

- app repository + commit;
- build receipt SHA-256;
- APK SHA-256;
- `termux-packages` repository + commit;
- bootstrap source-manifest SHA-256;
- bootstrap artifact SHA-256;
- bootstrap profile SHA-256;
- workload command;
- workload stdout SHA-256.

Receipt IDs must differ.

## Invariants

`claim_allowed=false` is the default.

Cloud CI verifies only proof machinery and synthetic adverse cases. CI green is not a physical-device receipt.

`TOKEN_VAZIO` is preserved whenever required evidence is absent. It is never converted to `PASS` by inference.

## What this closes

The implementation supplies a machine-checkable custody chain:

`termux-packages commit → source manifest → bootstrap hash → app commit → APK hash → installed APK hash → workload hash → independent reproduction`

## What remains outside the claim

This v1 still does not prove:

- DEB repository/network availability end-to-end;
- package installation of arbitrary packages from a production repository;
- performance superiority;
- long-duration stability;
- security certification;
- broad third-party device coverage;
- market adoption or commercial readiness.

Those remain separate evidence gates.
