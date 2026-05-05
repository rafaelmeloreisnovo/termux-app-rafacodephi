# PSS3 Failure Stability
PSS3 (Problem Stability System 3) usa três variáveis centrais: recorrência, severidade e estabilidade pós-fix.

- `delta_failure = p(fail|recurring) - p(fail|non_recurring)`
- `beta_risk = severity * recurrence_rate * (1 - fix_stability)`
- Classe: STABLE_OK, WATCH, RECURRENT, CRITICAL_RECURRENT, BLOCKER_BETA, FIXED_LIKELY, NOISE_LIKELY

`PSS3_DELTA_Q16 = 11796` representa 0.18 em Q16.

## Futuroro token invarite (token invariance)
- `token_invariant_q16 = max(PSS3_DELTA_Q16 - delta_q16, 0)`
- Quanto maior, mais margem estável restante até o limite beta.


## AArch64 ASM path
- Núcleo low-level em `rmr/Rrr/rafaelia_pss3_aarch64.S` com rotinas `umull/lsr/sub/csel` para delta e token invariance.
- Em `__aarch64__`, `rafaelia_pss3_step` usa o caminho ASM; demais arquiteturas usam fallback C equivalente.
