# RAFCODEPHI E2E Product Proof v1

## Objective

Turn the RAFCODEPHI Android/Termux stack into one falsifiable product path:

`packages → bootstrap → APK → physical device → workload → receipt → reproduction`

This contract does **not** convert CI success into a physical-device claim. Cloud CI validates only the proof machinery. A product claim is allowed only after two independent physical receipts reproduce the same source/artifact/workload observation.

## Invariant

`claim_allowed=false` is the default.

A single device run always records:

- `packages=PASS` only when `pkg` and `apt` are present and `apt --version` executes;
- `bootstrap=PASS` only when the canonical RAFCODEPHI prefix contains the required shell/package/proot surface;
- `apk=PASS` only when Android resolves the installed package and the installed `base.apk` SHA-256 equals the expected build SHA-256;
- `device=PASS` only on a supported canonical ABI and the RAFCODEPHI prefix;
- `workload=PASS` only when the workload exits zero and its stdout is hashed;
- `receipt=PASS` only after a canonical JSON receipt is emitted;
- `reproduction=TOKEN_VAZIO` because one observation cannot prove its own reproducibility.

The validator derives promotion. It never trusts a receipt that sets `claim_allowed=true`.

## Collect receipt A on the physical Android device

From the installed `com.termux.rafacodephi` shell:

```bash
bash scripts/collect_e2e_device_receipt.sh \
  --git-commit <40-hex-commit-used-to-build> \
  --apk-sha256 <sha256-of-built-apk> \
  --bootstrap-sha256 <sha256-of-embedded-bootstrap> \
  --out reports/device-e2e/device-a.json
```

For a real workload, replace the default deterministic smoke workload:

```bash
bash scripts/collect_e2e_device_receipt.sh \
  --git-commit <40hex> \
  --apk-sha256 <64hex> \
  --bootstrap-sha256 <64hex> \
  --workload 'your deterministic command here' \
  --out reports/device-e2e/device-a.json
```

## Collect receipt B

Reinstall/restart from the same build inputs and collect a second independent run as `device-b.json`. A different device is stronger evidence; a fresh independent run on the same device is still useful but should be described honestly.

## Evaluate promotion

```bash
python3 scripts/validate_e2e_receipt.py \
  reports/device-e2e/device-b.json \
  --reference reports/device-e2e/device-a.json
```

Promotion is `PASS` only if:

1. both receipts are structurally valid;
2. their first six stages are `PASS`;
3. installed APK hash equals build provenance;
4. Git commit, APK hash and bootstrap hash match;
5. workload command and stdout hash match;
6. receipt IDs differ.

Any mismatch produces `BLOCKED`, not an inferred success.

## What this closes

This adds the missing product-level evidence bridge between build CI and actual Android execution. It makes the proof boundary machine-checkable and turns `TOKEN_VAZIO` for physical reproduction into a concrete, fillable artifact.

## What remains outside the claim

This v1 does not by itself prove:

- performance superiority;
- security certification;
- long-duration stability;
- package repository/network availability;
- third-party device coverage;
- market adoption or commercial readiness.

Those require separate receipts and gates.
