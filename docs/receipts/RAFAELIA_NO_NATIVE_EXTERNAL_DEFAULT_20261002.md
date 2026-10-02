# RAFAELIA No-Native/No-External Default Receipt — 2026-10-02

event: `RAFAELIA-NO-NATIVE-EXTERNAL-DEFAULT-20261002`
parent_ref: `rafaelmeloreisnovo/termux-app-rafacodephi@c30ef43c8126bc85b72e75e7d6ca330ddb00b7ae`
local_read_ref: `70ef505aa7d31193ab80ee9d6bfb3205b45ece42`
branch_scope: `codex/rafaelia-no-native-default-20261002`
authority: Termux RAFCODEPHI local Android/provider implementation
write_scope: `gradle.properties`, `rafaelia/build.gradle`, `RafaeliaUtils.java`, tests, `app/src/main/cpp/Android.mk`, this receipt

## Goal

Make the RAFAELIA module default to a low-friction package profile:

- no NDK/native build unless explicitly enabled;
- no AndroidX WorkManager dependency unless explicitly enabled;
- no required public JNI surface in `RafaeliaUtils`;
- preserve hosted/native paths as opt-in gates, not default package dependencies.

## Delta

- Added `rafaelia.nativeBridge.enabled=false`.
- Added `rafaelia.workRuntime.enabled=false`.
- Gated `ndkVersion`, `externalNativeBuild`, and `Android.mk` behind `rafaelia.nativeBridge.enabled`.
- Gated WorkManager/annotation dependencies behind `rafaelia.workRuntime.enabled`.
- Excluded `RafaeliaBatchScheduler` and `RafaeliaBatchWorker` from the default source set when Work runtime is disabled.
- Replaced public `native` utility methods in `RafaeliaUtils` with Java fallbacks for memory, vector, ANOVA, sequence, radix, and zero-curve helpers.
- Added `tests/test_rafaelia_dependency_profile_contract.py`.
- Updated `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md` with the real ARM validator gate, the `LEGACY_PREFIX_BINARY_RISK` promotion stop, and the device-bound `pkg install` promotion sequence.
- Declared the bridge's canonical real-pkg promotion package/ABI set with `termux-api` and ARM/AArch64 markers, while preserving cheaper profile resolution for structural lanes.
- Aligned `docs/RUNTIME_TRUTH_TABLE.md` so `pkg update`/`pkg install` remain `FUTURO` until `device pkg smoke` reaches `DEVICE_REAL_PKG_VALIDATED`.
- Added `lowlevel/gpu_compute_evidence_gate.c` to the existing `termux-baremetal` source list. The implementation already exists in the repository; the build manifest was the missing edge.

## Evidence

PASS:

```text
python3 tests/test_rafaelia_dependency_profile_contract.py
```

The test verifies default Gradle flags, native/Work gates, worker source exclusion, and absence of required public JNI in `RafaeliaUtils`.

Observed CI failure before the manifest correction:

```text
undefined symbol: rgpu_compute_dispatch_proven
```

Observed in ARM32 canonical, ARM32 NDK29, loader APK, RAFAELIA ARM64, and Vectra benchmark jobs. The missing symbol is now linked by the one-line source-list correction in `app/src/main/cpp/Android.mk`.

NOT_RUN / TOKEN_VAZIO:

```text
GRADLE_USER_HOME=.gradle-home ./gradlew :rafaelia:testDebugUnitTest --no-daemon --stacktrace
```

Result: `TOKEN_VAZIO_TOOLCHAIN_UNAVAILABLE`.

Reason: this executor has no Android SDK configured: `SDK location not found. Define ANDROID_HOME or sdk.dir`.

Additional local limitation: `javac` is not installed in this executor.

## Boundary

`SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`.

This receipt proves the source/config contract check and records the link-gap correction. It does not yet prove a post-correction APK, `.so` absence inside a produced APK, device execution, physical ARM32/ARM64 behavior, or release-signing reproducibility.

claim_allowed: `false` for Android package/build/device claims until a fresh SDK-backed CI rerun and artifact inspection complete.

## R3

F_ok: default source profile removes required RAFAELIA NDK/Work dependency path; public utility JNI dependency removed; contract test passes; missing GPU evidence-gate object is now declared in the existing native target.

F_gap: fresh post-correction CI results, Android SDK-backed unit test, APK artifact inspection, device install, and provider protection remain open.

F_next: rerun the affected CI workflows from the corrected head; promote only if linker, artifact, and device gates pass independently.
