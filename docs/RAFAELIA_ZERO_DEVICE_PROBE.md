# RAFAELIA ZERO — physical Android device probe

## Purpose

This probe closes the **measurement path**, not the measurement itself:

```text
debug APK
-> adb shell guarded Activity
-> native RFZ1 ingest
-> process ABI comparison
-> state transition checks
-> app-private JSON receipt
-> run-as capture
-> fail-closed host validation
-> SHA-256 custody
```

Until a receipt is captured from a named device and associated with an APK hash,
physical execution remains `TOKEN_VAZIO`.

## Security boundary

The component exists only in:

```text
app/src/debug/AndroidManifest.xml
app/src/debug/java/com/termux/app/rafaelia/RafaeliaZeroProbeActivity.java
```

It does **not** exist in the main/release source set. The activity is explicit,
has no UI, exits immediately and requires:

```text
android.permission.DUMP
```

That permission allows `adb shell` diagnostics while preventing ordinary apps
from invoking the exported debug component.

## What the probe executes

One direct native payload is ingested through:

```text
RafaeliaZeroRuntime.ingestDirect(...)
```

The receipt is promoted to `PASS` only when all conditions hold:

1. the installed application is debuggable;
2. `libtermux_rafaelia_zero_runtime.so` loads;
3. the native self-check/init returns `0`;
4. process architecture equals `rafz_get_build_info().arch_id`;
5. `RAFZ_MAX_PAYLOAD == 1024`;
6. one valid RFZ1 ingest returns `0`;
7. accepted count increases exactly once;
8. rejected count does not change;
9. state digest changes and remains non-zero;
10. null input fails with `RAFZ_E_NULL`;
11. oversized input fails with `RAFZ_E_RANGE`;
12. the observed OS page size is a positive power of two.

The payload is the fixed 20-byte sequence:

```text
RFZ1-DEVICE-PROBE-V1
```

## Build and run

Build or provide a debug APK, enable USB debugging and execute from the repository root:

```bash
bash scripts/run_rafaelia_zero_device_probe.sh path/to/termux-rafcodephi-debug-<abi>.apk
```

When the debug APK is already installed:

```bash
bash scripts/run_rafaelia_zero_device_probe.sh
```

Optional environment variables:

```text
ADB                       adb executable
RAFAELIA_ZERO_PACKAGE     application id; default com.termux.rafacodephi
RAFAELIA_ZERO_RECEIPT     local output path
```

The runner:

- waits for the device;
- optionally installs the APK with `-r -t`;
- verifies `run-as` availability;
- removes any stale receipt;
- starts the guarded probe activity;
- captures `files/rafaelia-zero/latest.json` with `run-as`;
- runs the fail-closed Python validator;
- prints receipt SHA-256, device serial, fingerprint and installed APK path.

## Receipt schema

```text
rafaelia.zero.device.probe.v1
```

Principal sections:

```text
device    manufacturer/model/fingerprint/SDK/process ABI/page size
native    init, availability, architecture, guards and ingest status
observed  counters and state digests before/after
checks    non-compensatory boolean gates
```

A failed or incomplete probe writes:

```json
{
  "result": "FAIL",
  "claim_allowed_device": false
}
```

No average or partial score can compensate for a failed required check.

## Static contract gate

```bash
python3 scripts/validate_rafaelia_zero_device_probe_contract.py
python3 scripts/validate_rafaelia_zero_device_receipt.py --self-test
python3 scripts/validate_rafaelia_zero_runtime.py
```

The canonical runtime validator invokes the probe contract automatically.
It still reports:

```text
claim_allowed_device_execution = false
physical_device_receipt = TOKEN_VAZIO
```

because static validation is not physical execution.

## Evidence promotion

A defensible device evidence bundle contains at least:

```text
debug APK
APK SHA-256
receipt JSON
receipt SHA-256
device serial or privacy-preserving identifier
build fingerprint
adb command transcript
UTC capture time
```

The receipt proves a runtime transition in the installed debug process. It does
not independently prove APK provenance; therefore APK and receipt hashes must be
preserved together.

## Current boundary

```text
probe implementation       INTEGRATED_SOURCE after merge
static contract            LOCALLY_EXECUTABLE
physical receipt           TOKEN_VAZIO until device capture
release component          ABSENT BY DESIGN
claim_allowed_device       false until valid captured receipt
```
