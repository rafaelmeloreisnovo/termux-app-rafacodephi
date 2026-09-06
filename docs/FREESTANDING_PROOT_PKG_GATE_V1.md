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

## Cold-start real bootstrap

A fresh prefix must not need an already-installed Clang merely to create the control gate. The host-side cold-start builder first produces the real Termux ARM package core (`apt`, `dpkg`, `pkg`, PRoot and dependency closure), then cross-compiles the freestanding gate and embeds it as `libexec/rafproot-fs` inside the rewritten bootstrap ZIP.

```sh
python3 scripts/build_freestanding_real_arm_bootstrap.py --arch all
```

Per architecture:

```sh
python3 scripts/build_freestanding_real_arm_bootstrap.py --arch aarch64
python3 scripts/build_freestanding_real_arm_bootstrap.py --arch arm
```

The builder:

1. calls `build_real_arm_bootstrap_core.py`;
2. compiles the gate as static AArch64/ARM ELF;
3. rejects a dynamic interpreter or `DT_NEEDED` when `readelf` is available;
4. injects `libexec/rafproot-fs` into `rewritten-bootstrap-{aarch64,arm}.zip`;
5. adds gate SHA-256 and `DEVICE=TOKEN_VAZIO` metadata to `BOOTSTRAP_INFO`;
6. recalculates the bootstrap SHA-256 in the existing real-core manifest;
7. writes `freestanding_gate_receipt.json`;
8. runs the existing real ARM bootstrap validator unless `--skip-validator` is explicitly supplied.

A host artifact reaching this point is `BUILD_PROVEN`. It is not `DEVICE_PROVEN` until installed and executed on Android.

## Device install for an already-built standalone gate

When testing a standalone gate outside the cold-start bootstrap, install the binary matching the device ABI into the private prefix:

```sh
mkdir -p "$PREFIX/libexec"
cp ./rafproot-fs-* "$PREFIX/libexec/rafproot-fs"
chmod 700 "$PREFIX/libexec/rafproot-fs"
```

The local helper can also build and install it when a usable Clang is already present:

```sh
sh scripts/build_install_freestanding_gate.sh
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

The first `--probe` is allowed to return non-zero when required tools are absent. That is evidence, not a bootstrap failure to hide.

## Evidence ladder

`SOURCE_OBSERVED -> WIRED -> BUILD_PROVEN -> RUNTIME_PROVEN -> DEVICE_PROVEN -> REPRODUCED`

The CI workflow can prove static build properties and the cold-start builder contract. It intentionally keeps `device_runtime_state=TOKEN_VAZIO` and `claim_allowed=false`.

## Boundary clarification

Making the orchestration gate freestanding does not make stock PRoot, Ninja, `pkg`, apt/dpkg or QEMU freestanding. Reimplementing those complete programs without their runtime dependencies is a separate project. This gate creates the low-level, auditable boundary needed to test and progressively replace components without confusing an executed payload with the freestanding control core.
