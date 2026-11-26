# RAFAELIA Vectra-Termux tool-name guard V1

State: REVIEW/HOLD; HOST_PASS_SCOPED; ANDROID=TOKEN_VAZIO; claim_allowed=false.

Producer: rafaelmeloreisnovo/termux-app-rafacodephi.
Consumer: rafaelmeloreisnovo/Vectras-VM-Android.

## Bug and surgical scope

The freestanding rafproot-fs dispatcher appended a caller-supplied tool name
to PREFIX/bin without rejecting slash or parent traversal. The new pure C11
header accepts ASCII executable basenames starting with an alphanumeric or
underscore, followed by ASCII alphanumeric, underscore, hyphen, dot or plus,
with a maximum of 127 bytes. Null, empty, traversal, separators, control
characters, shell punctuation and non-ASCII are rejected.

Existing pkg, apt-get, proot, proot-distro, ninja, clang, cmake, QEMU and
other valid basenames retain their command semantics. The existing path
construction rejection still exits through the caller's failure code 65.

## Gates

1. GCC and Clang Werror + UBSan negative/positive vectors.
2. Freestanding compile (no libc/syscalls/heap) and zero undefined externals.
3. Existing AArch64/ARMv7 static ELF and emulated-runtime gate at exact HEAD.
4. Physical Termux/Android APK/bootstrap, PRoot/Ninja/pkg/QEMU and Vectra
   IPC transaction receipts before any release/guest claim.

Host tests are scoped only. Symlinks, arbitrary PREFIX provenance, APT
repository trust/signatures and physical ART/DEX/guest execution remain
TOKEN_VAZIO, not zero and not PASS.

## Rollback

Close the draft PR before merge; after reviewed merge, revert the isolated
commits. No force push, no app release, no changes to existing package
sets, signing keys, Java classes or Android permissions.
