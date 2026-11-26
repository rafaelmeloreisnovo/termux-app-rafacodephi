# RAFCODEPHI Termux runtime

This repository produces the Android runtime for `com.termux.rafacodephi`. The supported APK set is `armeabi-v7a`, `arm64-v8a`, and universal. ARM32 remains a required build and bootstrap path.

## Startup contract

`TermuxActivity` creates the terminal clients, toolbar, soft-keyboard handler, and `TermuxService` before it asks for optional shared-storage access. A denied or deferred storage grant may limit `~/storage/shared`; it must not prevent the private terminal session from starting.

The Verbativo native convergence audit is optional. Missing or ABI-incompatible Verbativo symbols are logged and cannot block terminal startup. Bootstrap installation remains fail-closed through `TermuxInstaller`, `BootstrapBaremetalGuard`, and `BootstrapReadinessGate`.

The first session is created only after the embedded bootstrap passes its startup contract. Shell resolution uses the installed private prefix. The rewritten bootstrap also exposes the compatibility launcher `bin/shellbash`; it does not replace the real `bin/sh` and `bin/bash` payloads.

## Real package manager

The `real-pkg-apk-ci.yml` workflow imports the exact ARM32 and ARM64 source-built bootstrap archives, binds their hashes to the native `.incbin` payload, checks the signed HTTPS APT repository, runs consumer contracts, and builds three debug APKs. The current preview has real `apt`, `dpkg`, and `pkg`; package availability is bounded by the signed repository snapshot consumed by that workflow.

After physical installation, the intended device gate is:

```bash
apt --version
pkg update
pkg install nano ninja python
ninja --version
```

These commands still require an exact Android-device receipt. A successful host build or presence of a package in an archive is not device execution evidence.

## CI toolchain startup

The shared action `.github/actions/rafaelia-android-setup/action.yml` initializes one host toolchain:

- Bash;
- Node.js 24 LTS and a JavaScript smoke;
- Temurin JDK 21 with a compile-and-run smoke;
- Android SDK platform and build tools;
- canonical NDK r26 or the requested NDK29 lane;
- real ARM32 and ARM64 NDK object compilation;
- CMake Ninja with a real graph execution;
- `adb`, `fastboot`, and `apksigner` command startup.

Run the same bounded setup on a compatible Linux host with:

```bash
./scripts/setup_android_toolchain.sh
./scripts/verify_toolchain_startup.sh
./scripts/prepare_bootstrap_env.sh --skip-android-preflight
./gradlew clean :app:assembleDebug --no-daemon --max-workers=2
```

The verifier writes `build/reports/toolchain-startup.json`. `fastboot --version` proves the host CLI starts; it does not prove a connected device, bootloader operation, or flashing. Likewise, host Ninja is separate from Ninja executed inside the Termux bootstrap.

## CI entry point

Use `00 START HERE — RAFCODEΦ Enterprise` route `11_REAL_PKG_TOOLCHAIN` for the real APT APK plus session/keyboard source contracts and toolchain startup gates. The direct `RAFCODEPHI APK with real APT` workflow remains dispatchable and reusable through `workflow_call`.

Until a physical APK/device receipt is attached, these fields remain open:

```text
termux_session_keyboard_device=TOKEN_VAZIO
termux_bootstrap_ninja_device=TOKEN_VAZIO
fastboot_device=TOKEN_VAZIO
physical_android=TOKEN_VAZIO
claim_allowed=false
```
