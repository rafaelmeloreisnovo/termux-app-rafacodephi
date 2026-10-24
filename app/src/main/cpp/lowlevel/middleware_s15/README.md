# S15 — Android hosted middleware with an unchanged authorial L0

**What exists:** a disabled-by-default `Android.mk` module for `armeabi-v7a` and `arm64-v8a`; byte-pinned S14 C plan logic; JNI transport with exact 16-word input/output; a minimal Java call surface; host and NDK build checks. This does not repair Termux bootstrap or keyboard.

## Control plane, work plane, and evidence
```text
request/consent [HOSTED]
   → measured capability mask/quality/ticks [HOSTED, NOT SELF-ATTESTED]
   → JNI fixed-size transfer [HOSTED]
   → raf_mw_route 16×uint32 pure C [L0]
   → plan in result[0..15], executed=result[14]=0
   → separate hardware/Android executor [NOT IMPLEMENTED]
   → physical receipt + independent review [NOT_RUN]
```

`JNI nativePlan` returns transport status 0 on successful frame transfer. **Do not mistake that return for service success, NEON processing, GPU dispatch or hardware failover.** The plan's state is in `result[0]`. Java refuses missing library, null buffers, same-array alias, wrong lengths. Native JNI copies through stack buffers, checks pending exceptions and does not retain pointers.

## Build boundaries
By default `RAFAELIA_S15_MIDDLEWARE ?= 0`; nothing is linked into existing native modules. To test the optional module in an **isolated build job** with the correct NDK: set `RAFAELIA_S15_MIDDLEWARE=1` for ndk-build; do not change the default or publish/install an APK without signing/privacy review. It is valid that existing x86 ABIs do not receive S15; the facade will fail closed.

- Host: `sh app/src/main/cpp/lowlevel/middleware_s15/tests/verify_host_s15.sh` — 14 route scenarios and four memory guards, six Java negative cases, standalone L0 object symbol checks, native JNI host syntax.
- NDK: `sh app/src/main/cpp/lowlevel/middleware_s15/tests/verify_ndk_s15.sh` with Android NDK `26.3.11579264` — compile and link JNI shared libraries for ARMv7 and AArch64. The resulting `.so` is a hosted Android library with platform link dependencies, **not** a freestanding executable.
- Not yet done: signed APK `assembleDebug/Release`, physical installation, Termux bootstrap, keyboard, runtime/JNI instrumentation, measured hardware dispatch, dual active watchdog and rollback transaction.

## Reversibility and evidence
Disable `RAFAELIA_S15_MIDDLEWARE`, keep all original packages and branches, do not delete phone data, and do not auto-upgrade a running app. Run a fresh physical canary on a user-authorized test device only after an exact SHA/signed APK/backup/rollback plan exists.

`SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`; `TOKEN_VAZIO != 0`; `claim_allowed=false`.
