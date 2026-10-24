# S14 — Adapter boundary for deterministic authorial middleware

Source producer: `rafaelmeloreisnovo/RafPolimata` commit `68195995718b9f0653ca4ce4575d375ce60ac756`, path `freestanding/deterministic_middleware`; C blob `e46b1e24191f901edf91d6ce4c5003fa5d74438f` and header blob `ad97c24a2c9c3ea291e26ce3d74322f89d84f9d5`.

The new L0 function `raf_mw_route` **does not integrate** into the Android app at this commit. This document is an immutable handoff contract only, protecting existing Termux bootstrap, package `com.termux.rafacodephi`, keyboard, signing and ARM32/ARM64 support.

## Separation
- **PURE_CORE:** C11, 16×32-bit input/output matrix, bounded scalar decisions, two caller-supplied watchdog counter comparisons, fallback/failover **plans** and rollback **recommendation**. Zero libc, heap, GC, OS syscall, logging, JNI, ART/JIT or loader calls; exported `void raf_mw_route` only. A CPU/NEON/SIMD lane result requires independent hardware evidence and a separately verified executor before physical work.
- **HOSTED_BOUNDARY:** Java/Kotlin/ART/JIT, JNI or NativeActivity only if Android's chosen integration requires them; package permission/API ownership and versioned ABI. No direct Java-class-free Android execution promise while using the Android framework. GPU drivers, NUMA placement, APK/ELF loader, DEX/ART runtime or OS watch operations cannot be replaced with a header prefix or a plan bit.
- **TOOLING_TEST:** Clang, NDK, SDK, Gradle, D8/R8, shell, nm/readelf, CI. Build time dependency is **not** a L0 runtime dependency.
- **No mock success:** neither synthetic watchdog/fallback fixtures nor a successful CI can be reported as phone installation, GPU computation, failover of a live service or kernel watchdog.

## Required physical admission (currently BLOCKED)
1. Independent approval of owner/source/license and `AGENTS.md` scope, privacy and child safeguarding.
2. Pin current producer blob/SHA, recompile ARMv7 and AArch64 freestanding objects; verify `nm -u`, ABI symbols, ELF machine, codegen and size.
3. Implement a separately owned minimal hosted adapter (not in this PR): input framing, capability probing, permissions, bounded logs and error receipts, watchdog monotonic ticks. Never pass unchecked metadata as verified CPU capacity.
4. Rebuild signed APK under existing bootstrap constraints, run Moto E7 ARM32 and ARM64 physical tests, check installer/keyboard/service and SHA receipt; check failover and rollback using real transaction evidence.
5. Human review and provider-required gates before any merge; do not remove branches, source, signing keys or change upstream licensing.

State: `DOCUMENTED_ONLY`, `DEVICE_NOT_RUN`, `JNI_ADAPTER_NOT_IMPLEMENTED`, `claim_allowed=false`.