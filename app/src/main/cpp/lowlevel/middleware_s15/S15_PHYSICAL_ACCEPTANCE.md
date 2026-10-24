# S15 — physical test and rollback acceptance (NOT_RUN)

No physical Android device was connected or controlled in this work. These are **future acceptance conditions**, not a completed receipt.

1. **Authority and custody:** require explicit permission to replace/test a specific package, record exact HEAD, source blob IDs, rights/license approval, package signature and APK SHA-256. No backup means no install.
2. **Capabilities:** capture only essential, redacted `ro.product.cpu.abi`, API level, page-size requirement and package version. A probe reporting NEON/SIMD/GPU/NUMA is a **capability hint**, not independent work execution.
3. **Build:** confirm both ABI libraries exist inside the signed APK, JNI symbol exported, `DT_NEEDED` permitted, source `raf_mw.c/.h` byte-identical, `EXECUTED=0` after returned plan, and default-off build variant still reproducible.
4. **Runtime:** check installed APK launches, bootstrap remains intact, keyboard and session work, JNI facade returns plan deterministically for 16 words and fails closed on invalid state. Collect no private terminal history.
5. **Watchdogs:** use real **host-supplied** monotonic ticks and observe how the outer executor handles recommended state changes; never assert that the passive C comparisons run an OS watchdog.
6. **Failover and rollback:** independently trigger a controlled device test transaction and record before/after hashes and service states; do not automatically attempt destructive rollback in production.
7. **Stop conditions:** wrong ABI, mismatched SHA, unsigned/unreviewed APK, untrusted permission, source/provenance conflict, JNI bridge leakage, bootstrap regression or absent recovery path. Record `FAIL`/typed `TOKEN_VAZIO`, not “success”.
8. **Promotion:** same-HEAD required checks, independent review, human authorization and a reversible deployment. No automatic merge or production claim.

States: `SOURCE_IMPLEMENTED`; `HOST_CI` and `NDK_CI` require actual named run; `APK_SIGNED=TOKEN_VAZIO`; `PHYSICAL_DEVICE=TOKEN_VAZIO`; `claim_allowed=false`.
