# Beta Failure Stability Audit

| signature_hash | domain | event_type | count | recurrence | severity | fix_stability | delta_failure | beta_risk | status |
|---|---|---|---:|---:|---:|---:|---:|---:|---|
| _pending_from_out/failure_trace.csv_ | _pending_ | _pending_ | 0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | WATCH |

## Regras de conclusão do relatório
- BLOCKER_BETA: gate=3 ou beta_risk>=0.75 com recorrência.
- RECURRENT/CRITICAL_RECURRENT: recorrência acima do baseline com risco progressivo.
- NOISE_LIKELY: ocorrência isolada e baixo risco.
- FIXED_LIKELY: alta estabilidade pós-fix com baixa recorrência.

## Checklist operacional
- Falhas que bloqueiam beta.
- Falhas recorrentes com impacto.
- Falhas que parecem ruído.
- Falhas mitigadas por fix.
- Falhas que precisam teste manual.
- Falhas que precisam benchmark.
- Falhas que precisam correção de código.
