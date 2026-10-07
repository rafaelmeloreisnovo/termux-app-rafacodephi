# Receipt — CRC32C Gate FAIL 01 — Missing Include Route — 2026-09-26

```text
μID=MU-RAFCODE-CRC32C-GATE-FAIL-01-20260926
parent=MU-RAFCODE-CRC32C-FIXED-PURE-V1-20260926
source/ref=rafaelmeloreisnovo/termux-app-rafacodephi#465@d0162ceff71cb329c0b3b139f4063e50af351a74
kind=FAIL_RECEIPT+BUILD_ROUTE_GAP
Δsummary=freestanding gate stopped before CRC vector execution because host harness compiled authoritative rafz.c without its include directory; CRC pure leaf itself not implicated by observed failure
evidence=control-plane run36273981423 policy_job108493086791 FAIL; tests/pure_crc32c_fixed_vectors.c host compile: rafz.c:1:10 fatal error 'rafz.h' file not found
gap=TOKEN_VAZIO_DEPENDENCY_BLOCKED(host include route); CRC runtime vector remains TOKEN_VAZIO_NOT_RUN for this head
next=add explicit host_vector_include_dirs to auditor/config and rerun exact-head gate; do not alter CRC algorithm source
claim_allowed=false
```
