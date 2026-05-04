# RAFAELIA MissHit Cache

- `cache_hit`: acesso atendido pela camada esperada.
- `cache_miss`: evento normal de hierarquia.
- `misshit_cache`: incoerência entre layer/CRC/Hz esperados vs observados + estado commit gate.
- `noise_delta`: diferença mensurável sem quebra de contrato.
- `operational_error`: quebra de contrato.
- `soft_error`: degradou métrica sem invalidar estado.
- `hard_error`: invalidação objetiva (CRC, buffer, ABI, gate).
- `cosmic/noise event`: hipótese física, não claim comprovada.

## Contrato mínimo
Comparar `expected_layer`, `actual_layer`, `expected_crc`, `actual_crc`, `expected_hz_q16`, `actual_hz_q16`, `jitter_ns`, `commit_gate_status`.

## Pontos S
S1=DOC_ONLY/PRECISA_TESTE até módulo formal.
S3=DOC_AHEAD_CODE.
S12=HIPÓTESE/PRECISA_MEDIÇÃO.
