# Termux RAFCODEΦ runtime receipt v2

## Purpose

This adapter records a bounded, non-destructive execution receipt for the exact Android device, APK and producer commit.

```text
producer commit + APK SHA-256 + device identity + Termux identity + command probes
-> canonical receipt digest
-> validation
-> DEVICE_RECEIPT_COMPLETE or explicit incomplete state
```

## States

- `HOST_SIMULATION`: the contract ran outside an Android target; useful only for validating the collector.
- `DEVICE_OBSERVED_INCOMPLETE`: Android was observed, but at least one mandatory proof is absent.
- `DEVICE_RECEIPT_COMPLETE`: Android, producer commit, APK digest, prefix, `termux-info` and bounded shell probes are present.

`DEVICE_RECEIPT_COMPLETE` proves only the bounded environment and command observations recorded in the receipt. It does not prove every feature of the application, performance, security, installation provenance outside the recorded artifact, or scientific claims.

## Collection

```sh
PRODUCER_COMMIT=<40-hex-git-sha> \
APK_PATH=/path/to/exact.apk \
TERMUX_PACKAGE_NAME=com.termux.rafacodephi \
sh scripts/federation/collect_runtime_evidence.sh artifacts/device-receipt.json

python3 tools/validate_runtime_evidence.py \
  artifacts/device-receipt.json \
  --require-device-complete
```

The collector does not install packages, mutate application data, request root or send evidence over the network.

## Fail-closed boundaries

- missing commit or APK digest cannot become complete;
- host execution cannot be labeled device evidence;
- receipt tampering invalidates `receipt_sha256`;
- `claim_allowed` is always false;
- absence remains `TOKEN_VAZIO` or an explicit incomplete state.
