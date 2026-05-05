# PSS3 Failure Stability

PSS3 (Problem Stability System 3) mede estabilidade beta com três variáveis normalizadas em Q16: recorrência, severidade e estabilidade pós-fix.

- `delta_failure = p(fail|recurring) - p(fail|non_recurring)`
- `beta_risk = severity * recurrence_rate * (1 - fix_stability)`
- `PSS3_DELTA_Q16 = 11796` (0.18 em Q16)

Classificação operacional:
- `STABLE_OK`
- `WATCH`
- `RECURRENT`
- `CRITICAL_RECURRENT`
- `BLOCKER_BETA`
- `FIXED_LIKELY`
- `NOISE_LIKELY`

Ruído = ocorrência com baixa recorrência, severidade baixa e gate 0.
Blocker = gate 3, delta acima do limite ou CRC inválido em modo beta.
