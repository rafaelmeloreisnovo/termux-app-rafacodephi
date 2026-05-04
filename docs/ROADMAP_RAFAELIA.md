# RAFAELIA Roadmap

## Fase 1 (concluída)
- JNI zero-copy com DirectByteBuffer
- Commit-gate Java (LOAD->PROCESS->VERIFY->COMMIT)
- Telemetria inicial de ciclo/phi

## Fase 2 (em andamento)
- Integração em worker real (`RafaeliaPipelineWorker`)
- Métrica de commit/rollback por janela
- Teste instrumentado Android para runtime JNI real

## Fase 3 (em andamento)
- Painel de auditoria de convergência período 42 (SVG MVP adicionado)
- Export de séries (phi/coerência/entropia) para JSON/SVG (phi→SVG entregue)
- Política de release e assinatura de artefatos (manifesto SHA-256 + raiz merkle-like entregue)
