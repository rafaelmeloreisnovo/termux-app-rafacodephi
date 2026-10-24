# AGENTS.md — S15 Android hosted boundary

Scope: this directory and its descendants, including `vendor_pinned/**` and `tests/**`. This narrows root `AGENTS.md`; it cannot weaken repository, Termux, S12/S14, source-provenance or human-review gates.

## Security and implementation invariants
- `vendor_pinned/raf_mw.c` and `.h` must remain **byte-identical** to RafPolimata S14 producer commit `68195995718b9f0653ca4ce4575d375ce60ac756`, Git blobs `e46b1e24191f901edf91d6ce4c5003fa5d74438f` and `ad97c24a2c9c3ea291e26ce3d74322f89d84f9d5`; any change requires an explicit successor and new receipts.
- `raf_mw_jni.c` is **HOSTED_BOUNDARY**, not PURE_CORE: JNI API may be used only here, with caller-owned exact 16-word arrays, alias/bounds rejection, no heap or logging in new native code. The JVM may allocate; never state the Android app is malloc/Java free.
- No GPU, NUMA, NEON computation, process management, ELF/DEX/ART loading, actual watchdog, actual failover or rollback is performed by a route-plan bit. `result[14] == 0` must remain invariant.
- `RafaeliaMiddlewareBridge.java` exists because the Java JNI calling convention needs a Java surface; no broad Java refactor, mock-based success or new UI component is authorized. Invalid and unavailable states fail closed.
- `RAFAELIA_S15_MIDDLEWARE=0` is the **default** in `Android.mk`. ARM32/ARM64 native linking with NDK does not authorize APK installation or production deployment.
- Preserve Termux package identity, bootstrap, signing, keyboard, session data, existing JNI modules, existing Java and historical upstream licenses. No delete/merge by agent.
- Every gate binds exact HEAD, compiler/toolchain, object ELF machine, JNI export, synthetic vs physical state and rollback. Run host, NDK, APK packaging and physical tests as **distinct gates**. Host simulation is never an Android PASS.
- Prior to source redistribution or claiming exclusive authorship, review `LICENSE-STATUS.md` with the rights holder. `claim_allowed=false`, `TOKEN_VAZIO != 0`.

## Required file sequence
`README.md → LICENSE-STATUS.md → vendor_pinned/* → raf_mw_jni.c → tests/verify_host_s15.sh → tests/verify_ndk_s15.sh → Android.mk → Java facade → actual device receipt`.
