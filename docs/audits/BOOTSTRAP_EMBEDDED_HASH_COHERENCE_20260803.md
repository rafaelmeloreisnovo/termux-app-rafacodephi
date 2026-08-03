# Bootstrap embedded hash coherence — 2026-08-03

## Fact

`app/src/main/cpp/termux-bootstrap-zip.S` embeds:

```text
rewritten-bootstrap-aarch64.zip
rewritten-bootstrap-arm.zip
rewritten-bootstrap-i686.zip
rewritten-bootstrap-x86_64.zip
```

Those exact bytes are returned by `TermuxInstaller.getZip()` and verified by `BootstrapIntegrityVerifier` against `BuildConfig.BOOTSTRAP_BLAKE3_*`.

## Gap found

`prepare_bootstrap_env.sh` previously calculated SHA-256 and BLAKE3 from the pre-rewrite files:

```text
bootstrap-aarch64.zip
bootstrap-arm.zip
bootstrap-i686.zip
bootstrap-x86_64.zip
```

After profile materialization, the rewritten archive differs because it contains `BOOTSTRAP_PROFILE.json` and corrected `BOOTSTRAP_INFO`. Hashing the source archive would therefore not bind the build value to the payload embedded in the APK.

## Correction

The environment preparation now hashes the exact rewritten archives used by `.incbin`.

```text
hash input == embedded input == getZip() output
```

The invariant is covered by a focused test that cross-checks all four archive names between:

```text
termux-bootstrap-zip.S
prepare_bootstrap_env.sh
```

## Validation

```text
focused bootstrap-profile tests = 11/11 PASS
bash syntax = PASS
POSIX device-audit syntax = PASS
```

The three exact corrected blobs used in the local validation match the branch blobs:

```text
prepare_bootstrap_env.sh               a372a03c8b5421789f8fd9d4dab4e699deb8915e
test_bootstrap_profile_runtime_guard.py 41ea66a551d8821f736c077b0116aa102ed4c836
termux-bootstrap-zip.S                  c8e62f4f112ea0fe875d8181f59a53f8b7fe9885
```

## Boundary

This proves build-time hash-source coherence. It does not prove APK compilation, installation or physical-device BLAKE3 execution.

```text
claim_allowed=false
release_allowed=false
physical_device_receipt=TOKEN_VAZIO
```
