# RAFAELIA Roadmap

## Fase 1 (concluída)
- JNI zero-copy com DirectByteBuffer
- Commit-gate Java (LOAD->PROCESS->VERIFY->COMMIT)
- Telemetria inicial de ciclo/phi

## Fase 2 (em andamento)
- Integração em worker real (`RafaeliaPipelineWorker`)
- Métrica de commit/rollback por janela
- Teste instrumentado Android para runtime JNI real

## Fase 3 (próxima)
- Painel de auditoria de convergência período 42
- Export de séries (phi/coerência/entropia) para JSON/SVG
- Política de release e assinatura de artefatos
