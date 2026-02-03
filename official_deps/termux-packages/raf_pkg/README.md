# raf_pkg (RAFAELIA) — low-level packaging kernel

`rafpkg` is a small C tool that can **scan**, **plan**, and **audit** Termux packages with
**deterministic, static parsing** (it does **not** execute `build.sh`).

It was added to this repo to support your "no-zombie/low-overhead" pipeline: parse → graph → plan → build.

## Commands

- `rafpkg scan [--roots a:b:c] [--outdir out] [--arch aarch64]`
  - Writes an index JSON (default: `out/index.json`).

- `rafpkg plan [--roots a:b:c] [--arch aarch64]`
  - Prints a deterministic build order in the same format used by `build-all.sh`:
    `pkg_name pkg_dir`

- `rafpkg audit [--roots a:b:c] [--arch aarch64]`
  - Prints basic counts: total pkgs, excluded, deps, internal/external refs.

- `rafpkg cpu`
  - Probes the current host CPU/ABI using `/proc/self/auxv` (best-effort) and prints:
    - feature bits (ASIMD/NEON, AES, SHA2, CRC32, ATOMICS, SVE...)
    - a conservative `recommend_cflags` string you can reuse to compile tools/kernels.

- `rafpkg fileaudit [--root path] [--depth n] [--cache path|--no-cache]`
  - Deterministically walks a subtree and reports potentially problematic artifacts:
    - ELF files (binaries/objects/libraries)
    - scripts (shebang)
    - files with no extension
    - files with unusual extensions
  - Useful to catch "soltos"/unknown blobs before they become zombie deps.

### RafStore cache (fileaudit)

`fileaudit` can store per-file metadata (mtime/size/kind) in a tiny binary cache to reduce IO
when auditing the same tree repeatedly. Default cache file:

- `.rafstore/fileaudit.bin`

You can disable it with `--no-cache` or point to a different location with `--cache path`.

To inspect a cache file quickly:

- `rafpkg store-dump [--cache path] [--limit n]`

## Integration

`build-all.sh` now supports an optional mode:

```bash
RAFPKG=1 ./build-all.sh
```

This uses `scripts/rafpkg.sh plan` instead of the Python `scripts/buildorder.py`.

### Fast resume (no-grep)

`build-all.sh` was also patched to load `buildstatus.txt` once into an in-memory set
(avoids per-package `grep`). On large buildorders this removes a pure-zombie overhead layer.

## Scope / Constraints

- Parses only common variables like:
  `TERMUX_PKG_DEPENDS`, `TERMUX_PKG_BUILD_DEPENDS`, `TERMUX_PKG_ANTI_BUILD_DEPENDS`,
  `TERMUX_PKG_EXCLUDED_ARCHES`.
- Unknown deps are treated as external (no edge).
- The goal is **determinism + speed**, not perfect shell emulation.

