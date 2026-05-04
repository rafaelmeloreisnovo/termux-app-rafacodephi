# Release Notes (Rafaelia Integration)

## v0.3.0-unreleased
- `RafaeliaPipelineWorker` com métricas: `total`, `committed`, `rolledBack`, `commitRate`.
- Export JSON de auditoria (`phiWindow[42]` + estado de ciclo).
- `RafaeliaBatchWorker` (WorkManager) para execução batch real em ambiente Android.
- Teste instrumentado de carga `RafaeliaLoadInstrumentedTest` (84 ciclos / 2 períodos).

## v0.2.0-unreleased
- Integração de `processCommitGate(...)` no `RafaeliaUtils`.
- Testes unitários `RafaeliaCoreTest` e `RafaeliaUtilsDirectPipelineTest`.

## v0.1.0-unreleased
- JNI zero-copy (`termux_rafaelia_direct`) com DirectByteBuffer.
- Base `baremetal_nomalloc` e ponte `RafaeliaCore`.
