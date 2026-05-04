# RAFAELIA Code ↔ Doc Sync

| Item crítico | Código | Documento | Status |
|---|---|---|---|
| BitRAF | `raf_bitraf.*` | `docs/RAFAELIA_BIT_CHAIN.md` | SYNCED parcial |
| BitStack | `rmr/Rrr/rafaelia_core.c` | `docs/RAFAELIA_BIT_CHAIN.md` | DOC_AHEAD_CODE (oficial) |
| ZipRAF | não encontrado oficial | docs diversos | DOC_AHEAD_CODE |
| BitOmega | não encontrado oficial | docs diversos | DOC_AHEAD_CODE |
| Hz scheduler | `raf_clock.*` | `docs/RAFAELIA_HZ_AS_MEMORY.md` | CODE_AHEAD_DOC parcial |
| L1/L2/BUF/RAM | `raf_memory_layers.*` + Rrr | `docs/RAFAELIA_TOTAL_MATRIX.md` | BENCH_AHEAD_DOC parcial |
| Commit Gate | `rafaelia_commit_gate_ll.c` | `docs/RAFAELIA_TOTAL_MATRIX.md` | SYNCED parcial |
| ASM index | `.S/.s` + inline asm | `docs/RAFAELIA_LOWLEVEL_ASM_INDEX.md` | SYNCED |
| TOP42 | `raf_bench_suite.c` + docs | `docs/RAFAELIA_TOP42_BENCHMARKS.md` | CLAIM_WITHOUT_ARTIFACT |

## Regras aplicadas
- código sem doc: CODE_AHEAD_DOC
- doc sem código: DOC_AHEAD_CODE
- benchmark sem artifact CI: BENCH_WITHOUT_ARTIFACT
- claim performance sem artifact: CLAIM_WITHOUT_ARTIFACT
