# μRECEIPT Vectra-Termux basename protection (2026-11-03)

parent=termux-app-rafacodephi@2d1d0c5b7b20e9240611515d7c51e90304a6b2e8
producer_authority=rafaelmeloreisnovo/termux-app-rafacodephi
consumer_reference=rafaelmeloreisnovo/Vectras-VM-Android@9468ace1ca7a851377ba71565f2fd6f05c47c01c
risk=P0 executable path namespace ambiguity via traversal
source=bootstrap/proot_freestanding.c + raf_guard_v1 + tests
host_evidence=GCC+Clang Werror UBSan pass, freestanding nm -u empty
ci_exact_head=TOKEN_VAZIO_PENDING
device_abi_apt_proot_ninja_qemu=TOKEN_VAZIO_NOT_RUN
privacy=no user/device data copied; data=source-only
claim_allowed=false; release_allowed=false
rollback=close draft PR or reviewed revert; preserve provider master
F_ok=validator and hosted test added; F_gap=real binaries, exact CI,
symlink containment, PREFIX authority, APK provenance, physical Android
F_next=exact-head terminal CI + independent review then provider/consumer
transaction and signed package provenance receipts
