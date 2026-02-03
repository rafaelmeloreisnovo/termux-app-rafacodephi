# RAFAELIA PERF NOTES — proot-distro

## Practical performance wins (Android)
- Keep rootfs + tmp on **internal storage** (avoid slow SD).
- Set `PROOT_TMP_DIR=$PREFIX/tmp` to reduce cross-filesystem overhead.
- Use `--shared-tmp` when launching to avoid excessive tmp rebinds.
- Consider `--link2symlink` for fewer heavy link operations.

## Wrapper
Use:
```sh
rafaelia/tools/rafaelia_proot_wrap.sh debian -- bash
```
