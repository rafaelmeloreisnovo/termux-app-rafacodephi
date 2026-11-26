# Low-level selection: Java / DEX / ELF

## Contract

This repository owns the Android runtime boundary. The implementation is selected per routine, not by branding the whole APK as freestanding.

- **PURE_CORE**: authorial Rust/C/ASM, deterministic and bounded; no libc, heap, GC, JNI, Android API, network or syscall.
- **PLATFORM_GATE**: explicit ELF/ABI/entry/syscall boundary. It is low-level, but not pure core.
- **HOSTED_BOUNDARY**: Java and DEX for Android lifecycle, UI, compatibility and IPC. It is hosted code and must remain labelled as such.

Critical behavior must be implemented in the smallest auditable native layer available. Java is an adapter and must not silently replace a selected native routine.

## Unified artifact directory

The producer must emit one versioned directory per ABI:

`artifacts/rafcodephi-lowlevel/<version>/<abi>/`

It contains the selection manifest, provenance, DEX, native ELF/shared object and append-only SHA-256 receipts. Publication is atomic and partial directories are rejected.

## Minimum proof

A build receipt is not an install or runtime receipt. Promotion requires source provenance, DEX consistency, ELF symbol/ABI inspection, ARMv7 and AArch64 compilation, and consumer validation. Physical device evidence remains `TOKEN_VAZIO_DEVICE_RUNTIME` until an exact APK/device receipt exists.

## License boundary

The accompanying study/research license is a controlled permission notice, not a substitute for legal advice or registration. Commercial use is disallowed unless a separately signed formal agreement expressly authorizes it and satisfies the requested notarization/registration requirements where legally applicable. Enforcement, damages and recovery of investigation/audit costs depend on the agreement and applicable law; they are not guaranteed by repository text alone.
