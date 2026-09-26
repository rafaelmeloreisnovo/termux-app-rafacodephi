# Receipt — CRC32C Fixed Gate PASS — 2026-09-26

```text
μID=MU-RAFCODE-CRC32C-GATE-PASS-20260926
parent=MU-RAFCODE-CRC32C-GATE-FAIL-01-20260926
source/ref=rafaelmeloreisnovo/termux-app-rafacodephi#465@9023b9f1542e27ab63eabd9107c54995d54652e4
kind=CI_SCOPE_PASS+SEMANTIC_EQUIVALENCE_GATE
Δsummary=explicit host include route restored the intended CRC vector gate; fixed8/fixed9/fixed40/fixed64 vectors executed successfully; ARMv7/AArch64 pure-core assembly probes remained without prohibited nonterminal transfers
evidence=run36274066348 job108494115794: pure_crc32c_fixed_vectors compile_exit=0 run_exit=0; armv7a-none-eabi compile_exit=0 branches=[]; aarch64-none-elf compile_exit=0 branches=[]; pure_core_source_policy=PASS
gap=consumer migration; exact final ELF audit; physical ARM32/ARM64
next=N070_BITRAF_FIXED_WIDTH
claim_allowed=false
```
