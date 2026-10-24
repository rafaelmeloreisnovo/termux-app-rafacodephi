# Termux-App — S12 / Matriz autoral L0 e fronteiras do Android

**Status:** PROPOSED_INTEGRATION_ONLY; source exists in RafPolimata branch; no Android JNI adapter implemented here. Do not claim APK/ART/DEX execution or on-device PASS. `claim_allowed=false`.

## Ownership and invariants
- PURE_CORE producer: `rafaelmeloreisnovo/RafPolimata:freestanding/service_matrix/raf_fs_probe.h` and `raf_fs_probe.c`. API: `void raf_fs_probe(const unsigned char*, unsigned int length, unsigned int matrix[8]);`.
- Consumer: Termux app; package name remains `com.termux.rafacodephi`; keep ARM32 `armeabi-v7a` and ARM64 `arm64-v8a`. No impact on bootstrap identity, signing, existing AndroidManifest, permissions, update installer or task dispatch in this branch.
- Android framework: Java, SDK, NDK, JNI, ART/JIT, JDK, D8/R8, Gradle and Linux syscalls are **host/build/OS boundary**, never transitively imported into PURE_CORE.
- The 8-cell caller-owned matrix explicitly names state/family/input length/required bytes/ambiguity/validated_format=0/ownership/ABI version. It does not use malloc, libc, GC, native system APIs, callbacks, file I/O, syscalls or external functions.

## Handoff
An Android adapter may be implemented **only as a later independently reviewed change**. It must verify bounded caller-owned byte buffers and Android permission/export boundary, import the proven ARM32/ARM64 object for an exact SHA, avoid copying raw device user data into public logs and ensure that any native-hosted API stays outside L0. `MZ` is not a validated PE file; `ZIP` is not a validated APK; `ART`/DEX prefix does not validate bytecode, signature or execution.

## Physical-runtime gates (open)
1. Pin exact RafPolimata producer commit and C blob.
2. Obtain compiler/linker objects for both ARMv7 and AArch64, `nm -u` empty, `readelf` machine correct.
3. Implement minimal versioned JNI/NativeActivity adapter (when required), not in L0; produce signed APK and reproducible hash.
4. Verify embedded object/ELF/DEX, install/launch on physical Moto E7 ARM32 and ARM64 device, capture bounded redacted receipt. Add rollback and upstream license evidence.
5. Review by human before merge/promotion; CI green cannot substitute for step 4.

This is a routing contract and an integration backlog, **not a tested Termux runtime repair**. No files were deleted. `SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`. `TOKEN_VAZIO_DEVICE_EXECUTION`.