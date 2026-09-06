# RAFCODEPHI Freestanding PRoot / pkg / Ninja Gate v1

## Purpose

Provide a dependency-free execution boundary for testing Termux package payloads used by RAFCODEPHI and Vectras. The gate itself is freestanding; `pkg`, PRoot, Ninja, Clang, CMake and QEMU remain package payloads and are **not** relabeled as freestanding binaries.

## Invariants

- no libc, malloc, heap, GC, TLS, CRT, fork, threads or stdio in `bootstrap/proot_freestanding.c`;
- direct Linux syscalls, with distinct AArch64 and ARM EABI syscall tables;
- static ELF, no `PT_INTERP`, no `DT_NEEDED`, no undefined external symbols;
- package absence is `TOKEN_VAZIO`, never success;
- executable presence is `OBSERVED`, not `RUNTIME_PROVEN`;
- Android device execution is required before `DEVICE_PROVEN`;
- compatibility stubs must fail closed.

## Build

AArch64:

```sh
clang --target=aarch64-linux-android21 \
  -std=c11 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector -fomit-frame-pointer \
  -nostdlib -static -Wl,-no-pie,-e,_start,--gc-sections \
  bootstrap/proot_freestanding.c -o rafproot-fs-aarch64
```

ARMv7:

```sh
clang --target=armv7a-linux-androideabi21 -marm \
  -std=c11 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector -fomit-frame-pointer \
  -nostdlib -static -Wl,-no-pie,-e,_start,--gc-sections \
  bootstrap/proot_freestanding.c -o rafproot-fs-armv7
```

## Device install for test

Install the binary matching the device ABI into the private Termux prefix:

```sh
mkdir -p "$PREFIX/libexec"
cp ./rafproot-fs-* "$PREFIX/libexec/rafproot-fs"
chmod 700 "$PREFIX/libexec/rafproot-fs"
```

## Deterministic test sequence

1. Observe required executables without promoting runtime claims:

```sh
"$PREFIX/libexec/rafproot-fs" --probe
```

2. Install the bootstrap/toolchain package set through Termux `pkg`:

```sh
"$PREFIX/libexec/rafproot-fs" --pkg-bootstrap
```

This requests: `x11-repo`, `proot`, `proot-distro`, `ninja`, `clang`, `lld`, `cmake`, `make`, `binutils`, `file`, `patchelf`.

3. Run the probe again. Any required missing executable remains `TOKEN_VAZIO`.

4. Install the Vectras/QEMU package set after the repository package is present:

```sh
"$PREFIX/libexec/rafproot-fs" --pkg-vectras
```

This requests: `qemu-common`, `qemu-system-x86-64-headless`, `qemu-utils`.

5. Exercise package payloads through the same freestanding `execve` boundary:

```sh
"$PREFIX/libexec/rafproot-fs" --run ninja --version
"$PREFIX/libexec/rafproot-fs" --run proot --version
"$PREFIX/libexec/rafproot-fs" --run clang --version
"$PREFIX/libexec/rafproot-fs" --run qemu-system-x86_64 --version
```

## Evidence ladder

`SOURCE_OBSERVED -> WIRED -> BUILD_PROVEN -> RUNTIME_PROVEN -> DEVICE_PROVEN -> REPRODUCED`

The CI workflow can prove only the static build properties. It intentionally writes `device_runtime_state=TOKEN_VAZIO` and `claim_allowed=false`.

## Boundary clarification

Making the orchestration gate freestanding does not make stock PRoot, Ninja, `pkg`, apt/dpkg or QEMU freestanding. Reimplementing those complete programs without their runtime dependencies is a separate project. This gate creates the low-level, auditable boundary needed to test and progressively replace components without confusing an executed payload with the freestanding control core.
