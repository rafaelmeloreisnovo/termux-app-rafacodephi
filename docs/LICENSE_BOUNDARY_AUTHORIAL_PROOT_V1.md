# RAFCODEPHI authorial core / Termux / PRoot license boundary V1

This register separates authorship and license scopes. It does not relicense
upstream code and does not claim that the whole Android application is
freestanding.

## Distribution boundary

| Unit | Source identity | License / copyright boundary | Integration decision |
|---|---|---|---|
| Termux Android application | root `COPYING`, blob `f288702d2fa16d3cdf0035b15a9fcbc552cd88e7` | GNU GPL-3.0-only; inherited notices remain | Combined APK remains GPL-3.0-only |
| RAFCODEPHI independent microcore | `raf_authorial_v1/`, license blob `fbb27447536397e157f931fb598eb4ef7543c4a6` | MIT; copyright notice is retained | May remain independently reusable; a combined APK still follows GPLv3 obligations |
| PRoot package payload | `termux-packages@380af61c75c0902bc27b80cc4d48b5fb20e855ee`, recipe blob `0bf124c3299e5dc1ce0307b46af56aef20d61227` | Recipe declares GPL-2.0 and upstream `termux/proot` source | Separate executable/package payload; never relabel as RAFCODEPHI authorial core |
| RAFCODEPHI freestanding PRoot gate | `bootstrap/proot_freestanding.c` and syscall bridge | Repository modification under root GPLv3 unless a narrower valid file notice applies | Control gate only; it is not PRoot and does not prove PRoot compatibility |

The repository therefore contains multiple component licenses, while the
combined derivative application keeps its controlling GPLv3 distribution
obligations. “Two licenses” means separate licensed units and preserved
notices; it does not mean that inherited PRoot or Termux code becomes
authorial by modification, renaming or recompilation.

## Cross-repository source decision

Observed default-branch trees on 2026-11-11:

| Repository | Tree | C/H/ASM files | Exact blobs already present in this app | Decision |
|---|---|---:|---:|---|
| `Vectras-VM-Android` | `bb2807a252bcfbd09c9eb8c95b79a013e17c0249` | 548 | 31 | GPLv2 root; do not recopy duplicates or combine unresolved spans into GPLv3 app |
| `RafCode-Magick` | `6e15628b8382f832de08f3f08b2cab216b81ccaf` | 32 | 0 | GPLv3 root, but no startup/freestanding source selected in this bounded read |
| `RafPolimata` | `714be9f4f3058279c6ce50afb581c2bb7ea8e2cb` | 542 | 2 | Root license decision remains `TOKEN_VAZIO_OWNER_DECISION`; no source import |
| `RafGitTools` | `1a06ae32afbd7a8b60b5ca8d7b724598b2088582` | 201 | 37 | GPLv3 root; existing identical blobs are provenance links, not new copies |

Exact-blob intersection proves byte identity only. It does not prove original
authorship, semantic authority, production wiring or runtime execution. The
canonical pure-core implementation for this app remains
`rafaelia/src/main/cpp/zero/`; historical duplicates remain staging until their
consumers and ancestry are resolved.

## Java/native boundary

Java source necessarily uses classes and the Android/JVM runtime. It cannot be
truthfully compiled as a C freestanding core. The supported architecture is:

```text
PURE_CORE: C/ASM, caller-owned memory, no libc/heap/syscall/JNI/Android
PLATFORM_GATE: minimal ABI/syscall leaf when required
HOSTED_BOUNDARY: Java lifecycle + JNI/POSIX adapters
```

The optional Verbativo audit is now scheduled only after a terminal session is
available and outside the UI/bootstrap critical path. This reduces startup
coupling without pretending the Java adapter is freestanding.

## Remaining gates

- Exact-head Android build and startup contract CI.
- ARMv7/AArch64 object, relocation, undefined-symbol and assembly inspection.
- Physical device session/keyboard/bootstrap receipt.
- Human license decision for any additional authorial license or RafPolimata
  source scope.

Until those gates complete: `IMPLEMENTED_UNTESTED`, `claim_allowed=false`.

