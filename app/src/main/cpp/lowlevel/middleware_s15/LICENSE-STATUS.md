# S15 license and provenance — pending human decision

This is a **rights/status notice**, not a license grant, waiver or authorial certification.

The low-level C source copied to `vendor_pinned/` is pinned exactly to RafPolimata S14 `raf_mw.c/.h`, source HEAD `68195995718b9f0653ca4ce4575d375ce60ac756`; the two Git blobs are `e46b1e24191f901edf91d6ce4c5003fa5d74438f` and `ad97c24a2c9c3ea291e26ce3d74322f89d84f9d5`. This copy is a project-internal experimental vendor snapshot, **not** permission to distribute/relicense the original. It must not silently supersede the producer.

The new JNI bridge, Java facade, tests and build configuration are independent additions for this project. Original Termux/upstream rights and notices must be preserved. The available source review has not established a single exclusive rights holder for every layer; distribution and APK release require human rights/license review.

Android NDK, Java and GitHub Actions are build/host dependencies under their own licenses. They are not linked into the PURE_CORE object, but the final Android hosted library is not freestanding in the same sense.

`LICENSE_PROMOTION=TOKEN_VAZIO_HUMAN_REVIEW`; `claim_allowed=false`; no automatic release or merge.
