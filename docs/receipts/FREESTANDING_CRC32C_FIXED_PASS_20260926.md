# Receipt — Fixed-Block CRC32C Gate PASS — 2026-09-26

```text
μID=MU-RAFCODE-CRC32C-FIXED-PASS-20260926
parent=MU-RAFCODE-CRC32C-GATE-FAIL-01-20260926
supersedes=TOKEN_VAZIO_DEPENDENCY_BLOCKED(host include route)
source/ref=rafaelmeloreisnovo/termux-app-rafacodephi#465@9023b9f1542e27ab63eabd9107c54995d54652e4
kind=SEMANTIC_EQUIVALENCE+EXACT_HEAD_CI_EVIDENCE
Δsummary=host include route fixed without changing CRC algorithm; fixed8/fixed9/fixed40/fixed64 leaf executed against authoritative rafz_crc32c and canonical 123456789 CRC32C vector; ARMv7/AArch64 probe remained control-transfer free
evidence=control-plane run36274066348 policy_job108494115794 PASS; host-native-pure_crc32c_fixed_vectors compile_exit=0 run_exit=0; armv7a-none-eabi compile_exit=0 branches=[]; aarch64-none-elf compile_exit=0 branches=[]; pure_core_source_policy=PASS
inventory=363 sources; 320 unique blobs; 43 duplicate excess
gap=production consumer migration; exact pure-core ELF/object artifact; physical ARM32/ARM64
next=N070_BITRAF_FIXED_WIDTH from authoritative app/src/main/cpp/lowlevel/raf_bitraf.c
claim_allowed=false
```
