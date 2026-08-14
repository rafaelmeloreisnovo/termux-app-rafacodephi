# Upload Artifact Audit — 2026-08-14

Status: `VERIFIED_LIMITED_STATIC`  
Claim gate: `claim_allowed=false`  
Mode: append-only / evidence-first / no APK execution.

## Invariant

`IDENTITY != PROVENANCE != EXECUTION != EVIDENCE != CLAIM`

Raw personal corpus was **not** committed. Historical filename/size continuity is not promoted to byte identity without a historical digest. Declared Android permissions are not treated as proof of runtime grants or behavior.

## APK baseline

Input `com.termux_1002 (1).apk`:

- bytes: `113880067`
- SHA-256: `e6265a57eb5ca363808488e3b01955958bed93bc0c8a0d281849b363b11027ec`
- package string: `com.termux`
- versionName string: `0.118.3`
- 738 ZIP entries
- ABIs: arm64-v8a, armeabi-v7a, x86, x86_64
- `libtermux-bootstrap.so` and `libtermux.so` present for all four ABIs
- native ELF markers: Android 24 / NDK r22b (7171670)
- JAR/v1 verification: `jar verified`
- signer certificate: FDroid, valid 2015-10-26 through 2043-03-13
- APK was inspected statically only; it was not installed or executed.

The repository's current default `app/build.gradle` uses `applicationId=com.termux.rafacodephi`, default version `0.118.0` plus suffix `-rafacodephi`. Therefore the uploaded APK is classified as:

`BASELINE_REFERENCE_NOT_CURRENT_DEFAULT_RAFCODEPHI_BUILD`

A historical build using overrides is not excluded; exact producing source commit remains `TOKEN_VAZIO_SOURCE_COMMIT`.

Observed permission declarations include INTERNET, ACCESS_NETWORK_STATE, FOREGROUND_SERVICE, DUMP, MANAGE_DOCUMENTS, MANAGE_EXTERNAL_STORAGE, PACKAGE_USAGE_STATS, READ_LOGS, REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, REQUEST_INSTALL_PACKAGES, SYSTEM_ALERT_WINDOW, VIBRATE, WAKE_LOCK, WRITE_EXTERNAL_STORAGE and WRITE_SECURE_SETTINGS. These are declarations only.

## Historical ChatGPT export

Input archive:

`d6e9db90fbd5ffb1158e68518e6f46005d1c78dbaef56427bb8a934c8f466c0a-2025-10-06-21-49-49-edd15254776a4346b05570a99e6d418f.zip`

- bytes: `305843744`
- SHA-256: `53fbfbc52d110d5815024ca851868555d23c3180cd73bb5433dd5f5bade9d93f`
- filename prefix `d6e9db...` is **not** the observed file SHA-256; semantics remain `TOKEN_VAZIO_FILENAME_HASH_SEMANTICS`
- 5 entries / `1607970090` uncompressed bytes
- entries: `user.json`, `conversations.json`, `message_feedback.json`, `shared_conversations.json`, `chat.html`
- `conversations.json`: `792693581` bytes
- `chat.html`: `815263242` bytes
- `shared_conversations.json`: 72 records
- `message_feedback.json`: 2 records
- ZIP-slip candidates: 0; symlinks: 0; encrypted entries: 0; duplicate names: 0
- full decompression/CRC gate: `ZipFile.testzip() => None` (PASS)

The archive is time-scoped historical material from 2025-10-06 and contains personal data, so only sanitized metadata/hashes belong in GitHub evidence.

### Longitudinal cross-links

Observed substring counts in `conversations.json`:

- `com.termux_1002.apk`: 88
- exact attachment size `113880067`: 3
- current APK SHA-256: 0
- `fractal_unificado_visual.png`: 9
- six current `RAFAELIA_FRACTAL_Ω_0001..0006` filenames: 0

Interpretation: the current APK has strong historical continuity by filename+exact size, but byte identity is not proven. `fractal_unificado_visual.png` has direct filename anteriority. The six numbered Ω images require independent provenance.

## Image manifest

| File | Dimensions | SHA-256 | Metadata |
|---|---:|---|---|
| fractal_unificado_visual.png | 956×900 | `34b34fb6db90eb47ff206c013ea1264055af205296f380766749f7c835e14264` | no PNG software text |
| RAFAELIA_FRACTAL_Ω_0001_hybrid.png | 1024×1024 | `09ce438287465ed76f3b4cc607dac5a5d915096b07a7c1cec5dac419b5d6e185` | Matplotlib 3.10.3 |
| RAFAELIA_FRACTAL_Ω_0002_voynich.png | 900×900 | `15e9752115e79967066e6409a0b8e0f51265feac963f8970829ac54b8c6778a8` | Matplotlib 3.10.3 |
| RAFAELIA_FRACTAL_Ω_0003_fibonacci.png | 840×840 | `ab9af9253e1985a02b7731e775c6ea83891c79470a336efca8976eb88939beee` | Matplotlib 3.10.3 |
| RAFAELIA_FRACTAL_Ω_0004_delta.png | 1100×1100 | `b3cabd090f54da28dab569951238d99013eeb1ca5a6d328726c24333a32ceb5d` | Matplotlib 3.10.3 |
| RAFAELIA_FRACTAL_Ω_0005_tag14.png | 950×950 | `c6c34c72ecd7cb78e2b3217d1320e98a46f34e039b4b59e067db2dbecb1f5f3a` | Matplotlib 3.10.3 |
| RAFAELIA_FRACTAL_Ω_0006_cheio.png | 1024×1024 | `b4bbb92f475870d3a673bc45ffa97acc1eb02842e18610a55f2fdb3d9465c033` | Matplotlib 3.10.3 |

Visual inspection places the six numbered images in a Mandelbrot-like family with different framing/color transforms. This is a visual classification, not proof of generator parameters.

## TOKEN_VAZIO ledger

- `TV-APK-01`: historical SHA-256 for the 2025 APK attachment(s)
- `TV-APK-02`: exact source commit/build recipe that produced the uploaded APK
- `TV-APK-03`: v2/v3 signing-block verification with Android `apksigner`
- `TV-APK-04`: physical Android runtime behavior; intentionally not executed here
- `TV-ZIP-01`: meaning of SHA-like ZIP filename prefix
- `TV-IMG-01`: exact generator source/coordinates/iterations/palette for numbered fractals
- `TV-IMG-02`: historical byte lineage of `fractal_unificado_visual.png`

## Next verifiable gates

1. Hash any historical local `com.termux_1002.apk` and compare to the current artifact.
2. Reproduce RAFCODEΦ APK from a pinned Git commit and compare package/version/signer/bootstrap hashes.
3. Run `apksigner verify --verbose --print-certs` for v2/v3/v4 coverage.
4. Bind each fractal image to source + parameters + environment and generate deterministic receipts.
5. Ingest the 2025 export only as a time-scoped historical corpus using streaming/sharded parsing.
