# μRECEIPT — RAFBOX autoral v1 candidate — 2026-11-03

parent=termux-app-rafacodephi/master@5fa40c2ab4e82142267b8d8d6a21168488fcb7c8
authority=termux-app-rafacodephi producer; Vectras consumer unchanged
kind=ADDITIVE_AUTHORIAL_FREESTANDING_COMPATIBILITY_CANDIDATE
intent=implement minimal independent multicall BusyBox-like applet subset
paths=bootstrap/rafbox_v1/{rafbox_core.h,rafbox_linux.c,README.md,LICENSE.md}
tests=tests/rafbox_core_v1_selftest.c
ci=.github/workflows/rafbox-authorial-v1.yml
contract=docs/contracts/RAFBOX_BUSYBOX_BOUNDARY_V1.md
source=local C11 new code, no third-party BusyBox/Wine source
license=GPL-3.0-only / repository COPYING; BusyBox upstream remains GPL-2.0
PURE_CORE=no libc, heap, syscalls; PLATFORM_GATE=Linux ARM syscalls via repo-owned bridge
implemented={true,false,echo-basic,cat-readonly,basename,dirname}
unsupported={sh,ash,cp,rm,tar,gzip,apt,proot,package_manager}
baseline_stub=bootstrap_src/common/bin/busybox is legacy stdout + exit 0, NOT RUNTIME_PROVEN
host_evidence=local GCC+Clang C11 -O2 -Werror -pedantic UBSan 44/44 assertions PASS_SCOPED
arm_cross_compile=TOKEN_VAZIO_PENDING_CI
qemu_emulation=TOKEN_VAZIO_PENDING_CI
physical_android=TOKEN_VAZIO_NOT_RUN
apk_sha=TOKEN_VAZIO_NOT_BUILT
installed_runtime=TOKEN_VAZIO_NOT_RUN
guest_boot=TOKEN_VAZIO_NOT_RUN
license_reconciliation=source-level GPLv3 under fork; distribution review pending
writable_scope=additive source/test/CI/docs/receipt only
main_or_bootstrap_change=NONE
rollback=close draft PR / reviewed revert; no default symlink or busybox override
claim_allowed=false; release_allowed=false
F_ok=pure core 44 local host assertions, independent scoped platform adapter and CI defined
F_gap=exact GitHub SHA CI, full BusyBox parity, signed APT, physical runtime and Vectra guest
F_next=execute exact-head QEMU gate; independently audit BusyBox requirements before explicit integration
