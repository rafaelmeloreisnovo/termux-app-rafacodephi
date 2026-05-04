# RAFAELIA CODE DOC SYNC REPORT

## F_ok
- Correção de pré-processador AVX2 inválido em `baremetal_nomalloc.c`.
- `getClockProfileNative` com lock (`g_sched_mutex`) + `ensure_scheduler_locked(10)`.
- `debugStepNative` validando capacidade real de `outDebug` e truncamento de `snprintf`.
- Script e workflow para gerar/publicar artifacts TOP42.

## F_gap
- Métricas TOP42 majoritariamente com `NEEDS_ARTIFACT` em ambiente sem device dedicado.
- Testes instrumentados Android/JNI ainda não consolidados para todas as 42 métricas.

## F_noise
- Divergência esperada entre `target_hz` e `actual_hz_q16` em CI virtualizado.

## F_error
- Erro sintático em bloco `#if defined(HAS_AVX2);` corrigido.
- Risco de overflow/truncamento JSON em `debugStepNative` e `getClockProfileNative` mitigado.

## F_next
- Executar benchmark em device ARM32/ARM64 real e atualizar status para `DEVICE_ARTIFACT`.
- Expandir validações automáticas de schema e contratos JNI em testes CI.
