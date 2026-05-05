# RAFAELIA Code-Doc Sync

## Regras
- CODE_AHEAD_DOC: código sem doc correspondente.
- DOC_AHEAD_CODE: doc sem código implementado.
- CLAIM_WITHOUT_ARTIFACT: claim sem artifact reproduzível.
- BENCH_WITHOUT_ARTIFACT: benchmark sem artifact CI/device.
- SYNCED: alinhado.

## Itens críticos
- `raf_bitraf.c` <-> `docs/RAFAELIA_BIT_CHAIN.md`: SYNCED parcial.
- `raf_clock.c` <-> `docs/RAFAELIA_HZ_AS_MEMORY.md`: SYNCED parcial (precisa medição device).
- `raf_memory_layers.c` <-> `docs/RAFAELIA_TOTAL_MATRIX.md`: CODE_AHEAD_DOC parcial para thresholds completos.
- `rmr/Rrr/*` claims avançados <-> docs: DOC_AHEAD_CODE no oficial.
- TOP42 docs/scripts sem device artifact: CLAIM_WITHOUT_ARTIFACT/BENCH_WITHOUT_ARTIFACT.
