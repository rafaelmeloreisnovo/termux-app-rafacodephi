# RAFAELIA — Seven Knowledge/Work Guards — Termux App V1

Repository role: **Android runtime provider**.

Canonical source: `rafaelmeloreisnovo/Mapa@3a2821d44d555c28cd041ba029cfa940fe3d4c0a`,
`docs/canonical/2026-09-13/CASA_CONHECIMENTO_TRABALHO_V1.md`.

This adapter makes seven guards explicit in this repository:

`provenance -> context -> evidence -> contradiction -> uncertainty -> reproduction -> rollback`.

Reconstructibility remains transverse through `reconstruction_pointer`; it is not duplicated as an eighth local adapter guard.

## Termux-specific boundary

- CI/build evidence is not physical Android runtime evidence.
- APK presence is not installation or launch.
- provider dispatch is not terminal execution.
- discovery is not QEMU/guest execution.
- package identity is `com.termux.rafacodephi`.
- `armeabi-v7a` must not be silently dropped.
- `TOKEN_VAZIO != 0 != PASS`.
- structural readiness never promotes a domain claim; `claim_allowed=false`.

The validator returns only `BLOCKED` or `READY_FOR_DOMAIN_REVIEW`.
A material mutation requires rollback state `READY` or `EXECUTED`.
