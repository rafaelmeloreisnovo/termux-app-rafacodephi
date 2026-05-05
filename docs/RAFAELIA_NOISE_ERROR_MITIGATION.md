# RAFAELIA Noise/Error Mitigation

## Ruído
- jitter, variação de Hz, latência, rollback rate, aproximação Q16, delta de benchmark, cache miss.
- Mitigar com: janelas de medição, percentis, repetição N>=30, isolamento de afinidade CPU.

## Erro
- CRC inválido, overflow, buffer insuficiente, build quebrado, teste falho, race condition, claim sem fonte, doc divergente, benchmark mal classificado.
- Mitigar com: CRC32C gate, asserts de limites, CI de build por ABI, suíte regressiva, doc-sync automático.

## MissHit
- `expected_layer != actual_layer`
- `expected_crc != actual_crc`
- `target_hz != actual_hz`
- `expected_phase != observed_phase`

Mitigação direta:
- teste determinístico,
- seal de artifact,
- benchmark com metadata,
- rollback controlado,
- sincronização documentação/código,
- registrar `silence useful` como estado de incerteza e não como sucesso.
