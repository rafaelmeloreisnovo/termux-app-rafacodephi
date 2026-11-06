# RAFBOX Ω v1 — authorial bounded multicall core

STATUS: SOURCE_IN_BRANCH / HOST_PASS_SCOPED / DUAL_ABI_PENDING / PHYSICAL_ANDROID=TOKEN_VAZIO.
claim_allowed=false; release_allowed=false; NO_AUTO_INSTALL.

## Motivation and boundaries

RAFBOX is a new RAFAELIA-specific implementation of a **strict subset** of
Unix applet interfaces. It is influenced by the principle of independent
compatibility implementations (analogous to Wine's approach to interfaces),
but does **not** contain, import, rename or replace BusyBox, Toybox or Wine.
No compatibility with a full shell, POSIX runtime or BusyBox as a whole is claimed.

Two separated layers:

- PURE_CORE: `rafbox_core.h` — C11 applet selection and pathname spans;
  no libc, heap, syscalls, JNI or platform headers.
- PLATFORM_GATE: `rafbox_linux.c` — direct Linux ARMv7/AArch64 syscalls
  through the repository-owned `bootstrap/proot_syscall_bridge.h`;
  fixed 4096-byte buffer; no C runtime, malloc, JVM or dynamic loader.

**Implemented applets:** `true`, `false`, basic `echo` (no options
or escape handling), `cat` (stdin and read-only file inputs), `basename`
and `dirname` (one nonempty operand, normalized repeated slash behavior).
`rafbox --list` and `rafbox --help` report the subset honestly.

**Not implemented:** `sh`, `ash`, `cp`, `rm`, `find`, `tar`,
`gzip`, package managers, POSIX shell language, BusyBox full argument
semantics, service management and installation/linker alias behavior.
Unsupported applets return 64 and print a typed error; they cannot
silently return success. Read/print failure exits are nonzero.

## How to test

The scoped workflow `.github/workflows/rafbox-authorial-v1.yml` builds
and tests the pure core on GCC/Clang with UBSan; cross-links static ELF
for ARMv7 and AArch64 with no undefined externals or dynamic dependencies;
then attempts bounded QEMU-user runtime and negative fixtures.

`tests/rafbox_core_v1_selftest.c`: 44 local source assertions observed
PASS on GCC and Clang. This is **not** ARM execution proof.

## Deployment HOLD

Do not install RAFBOX as `bin/busybox`, `bin/ash`, `bin/sh` or as
a package replacement. Do not change package dependency graphs, rootfs
links, official bootstrap ZIPs or Android release signing.
A future *optional*, uniquely named `libexec/rafbox` requires:
(1) exact source/build/hash/ABI receipt, (2) applet parity fixture,
(3) provider + consumer integration test, (4) physical device E2E,
(5) explicit reviewed activation and rollback.

Rollback today: close the draft PR, leaving master/bootstraps unchanged.
After a reviewed merge, revert only the additive commits.

See `docs/contracts/RAFBOX_BUSYBOX_BOUNDARY_V1.md` and repository
`COPYING` (GPL-3.0-only). No upstream third-party authorship is claimed.
