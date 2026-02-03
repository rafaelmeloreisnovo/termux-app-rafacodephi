# RMR Module

RMR is a low-level module focused on deterministic helpers implemented in C with minimal JNI exposure.

## Goals

- Deterministic, auditable helpers
- Minimal dependencies
- Low-level C/ASM orientation

## Current Contents

- `RmrCore`: JNI bridge for normalize, clamp, stable hash, transmutation, and in-place flip operations
- `RmrCore`: JNI bridge for clamp, stable hash, and in-place flip operations
- `nativeTransmuteU32`: deterministic byte-order transmutation with C/ASM fallback

## Reference

- `qemu_rafaelia/termux-packages-README.md`
