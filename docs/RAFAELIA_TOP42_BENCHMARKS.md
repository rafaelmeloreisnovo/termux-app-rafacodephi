# RAFAELIA TOP42 Benchmarks

Fonte principal: `scripts/run_top42_bench.sh`, `top42.csv`, `top42.json`, workflow `.github/workflows/top42_bench.yml`.

## Classificação de estado
- CI atual gera placeholders e schema base.
- Métricas dependentes de hardware real: `NEEDS_DEVICE` / `PRECISA_ARTIFACT`.

## Regras de interpretação
- Números muito altos de ops/sec devem entrar em `logical_instruction` ou `memory_compute`.
- Não classificar como storage IOPS sem evidência de I/O físico real.

## Campos centrais monitorados
- scheduler: `target_hz`, `actual_hz_q16`, `jitter_ns`, `missed_ticks`.
- memória: L1/L2 bytes, arena used, page size, cache line.
- compute: crc32c, memcpy/memset, BitRAF encode/decode, vCPU ops.
