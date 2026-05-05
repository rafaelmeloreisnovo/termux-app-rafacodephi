# RAFAELIA MissHit Cache

- `cache_hit`: acesso resolvido no nível esperado.
- `cache_miss`: evento normal da hierarquia de memória.
- `misshit_cache`: incoerência entre expected/actual layer+crc+hz+jitter+commit gate.
- `noise_delta`: divergência mensurável sem quebra de contrato.
- `operational_error`: quebra de contrato funcional.
- `soft_error`: recuperável (rollback/retry).
- `hard_error`: não recuperável no contrato vigente.
- `cosmic/noise event`: hipótese física, não claim comprovada.

## Regra operacional
`misshit_cache` só pode ser afirmado quando houver evidência objetiva correlacionando:
- expected_layer vs actual_layer;
- expected_crc vs actual_crc;
- expected_hz_q16 vs actual_hz_q16;
- jitter_ns;
- status commit gate.

Sem essas medições: classificar como HIPÓTESE/PRECISA_MEDIÇÃO.
