# RAFAELIA ZERO — operational evidence gate

## Purpose

This gate converts a debug-device execution into a bounded, auditable evidence
bundle without confusing static validation with physical proof.

```text
source contract
→ debug APK
→ guarded adb probe
→ native RFZ1 ingest
→ atomic receipt
→ APK capture
→ command transcript
→ SHA-256 bindings
→ atomic evidence bundle
→ single-device validation
→ ARM32/ARM64 matrix
```

The source tree starts with both required device roles in `TOKEN_VAZIO`.
Only captured bundles may change the generated matrix state.

## Authority

```text
native runtime
  app/src/main/java/com/termux/app/rafaelia/RafaeliaZeroRuntime.java
  rafaelia/src/main/cpp/zero/rafz.c

physical probe
  app/src/debug/java/com/termux/app/rafaelia/RafaeliaZeroProbeActivity.java

operational contract
  configs/rafaelia-zero-operational-evidence-contract.json

capture runner
  scripts/run_rafaelia_zero_device_probe.sh

validators
  scripts/validate_rafaelia_zero_device_receipt.py
  scripts/create_rafaelia_zero_device_bundle.py
  scripts/validate_rafaelia_zero_device_bundle.py
  scripts/validate_rafaelia_zero_device_matrix.py
```

## Required physical targets

| Role | Native architecture ID | Expected Android ABI |
|---|---:|---|
| `arm32-legacy` | `1` | `armeabi-v7a` |
| `arm64-modern` | `2` | `arm64-v8a` |

The intended project devices are:

```text
Moto E7 / Android 10       → arm32-legacy
Realme Note 50 / Android   → arm64-modern
```

The device names are operational intentions, not pre-recorded evidence. Until a
bundle is captured from each device, both entries remain `TOKEN_VAZIO`.

## Running the probe

With a local debug APK:

```bash
bash scripts/run_rafaelia_zero_device_probe.sh \
  path/to/termux-rafcodephi-debug.apk
```

With an already installed debuggable APK:

```bash
bash scripts/run_rafaelia_zero_device_probe.sh
```

When no APK path is supplied, the runner resolves the installed package path
with `pm path` and captures that exact APK through `adb pull`.

The runner fails when any of these conditions is not met:

- `adb` is unavailable;
- the target is not connected;
- `run-as` does not prove a debuggable package;
- the probe component cannot start;
- the receipt does not appear;
- receipt validation fails;
- serial, fingerprint or APK path is empty;
- the APK cannot be captured;
- bundle creation or validation fails;
- matrix reconstruction fails.

## Evidence bundle

Each successful execution creates:

```text
build/reports/rafaelia-zero/evidence/<timestamp>-<serial>/
├── receipt.json
├── capture.json
├── transcript.txt
├── apk.bin
├── manifest.json
└── SHA256SUMS
```

### `receipt.json`

Generated inside the app-private directory by the debug probe. It records:

- package;
- device fingerprint;
- process architecture;
- page size;
- native architecture ID;
- init status;
- ingest status;
- null/range guards;
- accepted/rejected counters;
- state digest before and after.

### `capture.json`

Generated on the host. It records:

- package;
- `adb` device serial;
- build fingerprint;
- installed APK path;
- capture timestamp.

The bundle builder requires:

```text
capture.package == receipt.package
capture.device_fingerprint == receipt.device.fingerprint
```

### `transcript.txt`

Contains the command path and must include:

```text
RAFAELIA_ZERO_DEVICE_PROBE=PASS
receipt_sha256=<exact receipt digest>
apk_sha256=<exact captured APK digest>
```

The builder rejects a transcript that is not cryptographically bound to both
files.

### `manifest.json`

Contains normalized identity, role, runtime summary, file sizes and SHA-256
hashes. A single bundle may set:

```text
claim_allowed_device_single = true
```

It must always retain:

```text
claim_allowed_device_matrix = false
release_claim_allowed = false
```

### `SHA256SUMS`

Covers the four payload files and `manifest.json`. Unknown files, missing files,
empty files, symlinks, digest mismatches and size mismatches are rejected.

## Atomic publication

The builder writes to a sibling staging directory:

```text
.<bundle>.tmp-<pid>
```

After all files, manifest and hashes are complete, the directory is synchronized
and promoted by an atomic rename. An interrupted build is never treated as a
published bundle.

The matrix file is also written through `temp + fsync + replace`.

## Promotion state machine

```text
0 required roles
→ TOKEN_VAZIO
→ claim_allowed_device_matrix=false

1 required role
→ PARTIAL_DEVICE_PROOF
→ claim_allowed_device_matrix=false

ARM32 + ARM64, distinct receipts and devices
→ DUAL_ARM_DEVICE_PROOF
→ claim_allowed_device_matrix=true
```

Even in `DUAL_ARM_DEVICE_PROOF`:

```text
release_claim_allowed=false
independent_reproduction=TOKEN_VAZIO
```

Debug evidence demonstrates execution of the measured debug artifact. It does
not automatically attest a release artifact or independent third-party
reproduction.

## Anti-replay and collision rules

The canonical matrix rejects:

- duplicate role;
- duplicate receipt SHA-256;
- duplicate device serial;
- duplicate device fingerprint;
- unsupported architecture role;
- a single bundle attempting to promote the matrix;
- a debug bundle attempting to promote a release claim.

The APK SHA-256 may legitimately be equal across ARM32 and ARM64 when a universal
APK is measured. Therefore identical APK hashes are not treated as replay.

## Static gate

The existing native-safety chain remains the only CI entry point:

```text
Rafaelia Native Safety
→ scripts/verify_rafaelia_native_safety.py
→ scripts/validate_rafaelia_zero_runtime.py
→ scripts/validate_rafaelia_zero_device_probe_contract.py
→ receipt self-test
→ bundle builder self-test
→ bundle validator adversarial tests
→ dual-architecture matrix self-test
```

No additional GitHub Actions workflow is introduced.

Static self-tests verify the instrument and rejection rules. They do not produce
physical device receipts and never promote the source-tree matrix.

## Manual verification

Validate one bundle:

```bash
python3 scripts/validate_rafaelia_zero_device_bundle.py \
  build/reports/rafaelia-zero/evidence/<bundle>
```

Reconstruct a deliberate matrix:

```bash
python3 scripts/validate_rafaelia_zero_device_matrix.py \
  build/reports/rafaelia-zero/evidence/<arm32-bundle> \
  build/reports/rafaelia-zero/evidence/<arm64-bundle> \
  --output build/reports/rafaelia-zero/evidence/matrix.json
```

Run static/adversarial validation:

```bash
python3 scripts/validate_rafaelia_zero_device_probe_contract.py
```

## Epistemic boundary

```text
instrument implemented                FATO
static adversarial path implemented   FATO
physical ARM32 bundle                 TOKEN_VAZIO until captured
physical ARM64 bundle                 TOKEN_VAZIO until captured
dual-device matrix                    TOKEN_VAZIO until both validate
release artifact proof                TOKEN_VAZIO
independent reproduction              TOKEN_VAZIO
```

This boundary is intentional: operational excellence is not the appearance of a
green state. It is the ability to identify exactly which artifact ran, where it
ran, what was measured, how the evidence is sealed and what remains unproved.
