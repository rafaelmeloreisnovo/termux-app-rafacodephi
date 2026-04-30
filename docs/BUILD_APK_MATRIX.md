# Build local APK matrix (arm32 + arm64, signed + unsigned)

```bash
./scripts/build_apk_matrix.sh
```

## Output

Artifacts are generated at `dist/apk-matrix/`:

- `unsigned/*.apk` (debug/release unsigned artifacts)
- `signed/*-signed.apk` (locally signed release artifacts)
- `SHA256SUMS.txt` (checksums for signed and unsigned artifacts)

## Guarantees

- Fails if unsigned `arm64-v8a` or `armeabi-v7a` APK is missing.
- Fails if signed release `arm64-v8a` or `armeabi-v7a` APK is missing.
- Verifies signatures with `apksigner verify`.

## Notes

- Uses repository toolchain source of truth (`gradle.properties`).
- Bootstraps SDK/NDK/CMake via `scripts/ci_android_preflight.sh`.
- Creates a local keystore at `dist/local-release.keystore` if missing.
