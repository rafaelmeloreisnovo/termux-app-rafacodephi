# Build local APK matrix (arm32 + arm64, signed + unsigned)

```bash
./scripts/build_apk_matrix.sh
```

## Output

Artifacts are generated at `dist/apk-matrix/`:

- `*arm64-v8a*.apk` (debug/release unsigned)
- `*armeabi-v7a*.apk` (debug/release unsigned)
- `*release*-signed.apk` (signed local release variants)
- `SHA256SUMS.txt`

## Notes

- Uses repository toolchain source of truth (`gradle.properties`).
- Bootstraps SDK/NDK/CMake via `scripts/ci_android_preflight.sh`.
- Creates a local keystore at `dist/local-release.keystore` if missing.
