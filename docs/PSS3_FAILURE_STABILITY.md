# PSS3 Failure Stability
PSS3 (Problem Stability System 3) usa três variáveis centrais: recorrência, severidade e estabilidade pós-fix.

- `delta_failure = p(fail|recurring) - p(fail|non_recurring)`
- `beta_risk = severity * recurrence_rate * (1 - fix_stability)`
- Classe: STABLE_OK, WATCH, RECURRENT, CRITICAL_RECURRENT, BLOCKER_BETA, FIXED_LIKELY, NOISE_LIKELY

`PSS3_DELTA_Q16 = 11796` representa 0.18 em Q16.
