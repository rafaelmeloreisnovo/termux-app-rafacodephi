# RAFAELIA PERF NOTES — termux-packages

This folder adds *non-invasive* performance tuning helpers (no upstream modification required).

## Build speed knobs
- Parallel builds: `MAKEFLAGS=-j$(nproc)`
- `ccache` (massive wins for iterative builds)
- Prefer `clang` on Android (often faster + better diagnostics)

## Package size & install speed
- Enable stripping at package time (smaller .deb)
- Prefer `zstd` for `.deb` payload compression when available

## Usage
```sh
source rafaelia/tools/rafaelia_build_env.sh
# then run your normal build workflow
```
