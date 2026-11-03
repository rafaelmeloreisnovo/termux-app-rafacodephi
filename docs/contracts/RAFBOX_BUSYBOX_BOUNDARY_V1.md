# RAFBOX ↔ BusyBox ↔ Vectra/Termux boundary v1

STATE = SCOPED_CANDIDATE; SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM.
claim_allowed=false; release_allowed=false.

## Dependency graph (verified from source inventories)

| Role | Source authority | What remains | Current gap |
| --- | --- | --- | --- |
| Termux RAFCODEPHI provider | termux-app-rafacodephi `master` | Real bootstrap/proot/pkg/APT and BusyBox payload from package producer; legacy compatibility wrappers | signed binary provenance, APK/ABI/device receipt |
| Termux upstream package producer | termux-packages `packages/busybox/build.sh` | BusyBox 1.38.0 package GPL-2.0; full applets and libs remain external | cannot relabel as authorial |
| Vectra VM consumer | Vectras-VM-Android `master` | Alpine rootfs `distro/bin/busybox`, dependent shell/env symlinks and guest boot | no safe substitution without guest parity tests |
| RAFBOX v1 | termux-app-rafacodephi `bootstrap/rafbox_v1/` | Six new applets only; separate file and code | CI/exact APK/device evidence TOKEN_VAZIO |

## Important legacy risk

`bootstrap_src/common/bin/busybox` is a historical script stub that
prints text and returns code zero. Another developer bridge delegates to
Android toybox/toolbox. Neither proves native BusyBox correctness.
Keep as historical compatibility, classify `BUILD_ONLY/BRIDGE_ONLY`;
**never** use these as runtime execution evidence or release gate success.
Avoid changing the legacy stubs in this patch, to preserve rollback.

## Authorial compatibility policy (Wine-style, not Wine code)

Implement behavior from documented CLI contracts/independent fixtures,
not by copying BusyBox applet source or upstream patches. Keep
`PURE_CORE` separate from syscalls/Android/ELF. For every applet:
- enumerate recognized argv/options; reject unsupported combinations;
- verify stdout/stderr/exit and exact file effects with adversarial tests;
- build ARMv7/AArch64 static ELF and check symbols/DT_NEEDED/PT_INTERP;
- require source→commit→artifact SHA→IPC/installed APK→device witness;
- register owner/compatible license and reviewed activation/rollback.

## Non-negotiable activation gate

RAFBOX MUST NOT be named/installed as `busybox` or replace `sh`, `ash`
or any Alpine rootfs symlink until parity for every consumer/command has
been independently demonstrated, with physical Android + guest proof.
New core = six applets, NOT general BusyBox or shell compatibility.
Historical snippets with `busybox --help` succeeding are not validation.

Next test targets: run exact-head CI, identify full bootstrap consumed
BusyBox applet usage and symlink graph (no recursive corpus sweep), then
select one small additional applet only if needed. TOKEN_VAZIO for
unbound semantics, not 0. Preserve GPL notices and existing package recipe.
