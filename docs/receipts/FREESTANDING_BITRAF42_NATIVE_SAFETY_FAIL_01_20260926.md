# Receipt — BITRAF42 Native Safety FAIL 01 — Base ABI Hash Drift — 2026-09-26

```text
μID=MU-RAFCODE-BITRAF42-NATIVE-SAFETY-FAIL-01-20260926
parent=MU-RAFCODE-BITRAF42-FIXED-PURE-V1-20260926
source/ref=rafaelmeloreisnovo/termux-app-rafacodephi#468@9d86c6d710a815982f1a81703659db00d79b8b40
kind=FAIL_RECEIPT+ABI_CUSTODY_GATE
Δsummary=Native Safety rejected an additive rafz_u64 typedef because rafz.h is exact-blob protected; BITRAF semantic/vector evidence was not implicated by this observed failure
evidence=run36277312669 job108502444769: rafaelia-zero-runtime-contract FAIL; expected rafz.h blob 019254937a4d7d50c3a862baa72966722faa38e2, observed dfbb93c5d4b0b8cf21b137f1759a48c31d346311
gap=leaf-local 64-bit type required without widening the canonical RAFZ base ABI
next=restore rafz.h exactly and move the compiler-builtin u64 alias into rafz_pure_bitraf42.h; rerun gates
claim_allowed=false
```
