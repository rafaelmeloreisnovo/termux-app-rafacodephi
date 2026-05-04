# RAFAELIA Hz as Memory

1. **Hz físico**: frequência CPU/cpufreq/hardware não equivale automaticamente ao Hz lógico do motor.
2. **Hz lógico**: `target_hz`, `period_ns`, `actual_delta_ns`, `actual_hz_q16`, `jitter_ns`, `missed_ticks`, `total_ticks` (principalmente em `raf_clock.*`).
3. **Hz como memória**: cada tick carrega estado; `phase/step` acumulam histórico; CRC sela snapshots; jitter é ruído; `missed_ticks` pode ser ruído ou erro contratual.
4. **Hz→Layer**: regra `>50000 L1`, `>38000 L2`, `>25000 BUF`, else RAM aparece em `rmr/Rrr/rafaelia_core.c`; no núcleo oficial está parcial e requer teste de equivalência.
5. **TTL**: literal TTL não está formalizado no código oficial; mapear conceitualmente para `tick/cycle/phase/period` sem claim de implementação literal.

## Pontos S
S0: `raf_clock.*`, `raf_memory_layers.*`, `rmr/Rrr/rafaelia_core.c`.
S1: IMPLEMENTADO parcial + HIPÓTESE (TTL).
S2: scheduler determinístico em camadas.
S3: CODE_AHEAD_DOC parcial.
S4: overflow temporal, jitter bounds, missed tick accounting.
S5: selo via CRC/metric artifact pendente.
S6: núcleo do item.
S7: ligação com layers L1/L2/BUF/RAM.
S8: noise = target vs actual.
S9: error = contrato de tick violado.
S10: schema de métricas em benchmark/report.
S11: source = código + benchmark.
S12: PRECISA_MEDIÇÃO device.
