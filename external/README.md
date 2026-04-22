# External Kernel Source Integration

This repository integrates the external kernel source from:

- `https://github.com/wojcikiewicz17/linuxkernel`

To keep this project lightweight, the kernel source is **not vendored** directly.
Instead, we pin a commit in `external/linuxkernel.lock` and provide a sparse sync script.

## Sync command

```bash
./scripts/sync_linuxkernel.sh
```

Optional target directory:

```bash
./scripts/sync_linuxkernel.sh /path/to/linuxkernel-src
```

## Pin update flow

1. Update `COMMIT` (and optional `SPARSE_PATHS`) in `external/linuxkernel.lock`.
2. Run `./scripts/sync_linuxkernel.sh`.
3. Validate any dependent native/CI workflows.
