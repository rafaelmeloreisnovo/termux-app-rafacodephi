# RAFAELIA Semantic Layers & Concept Map

## Objetivo
Este documento conecta matemática, implementação e operação para humanos e IA, com foco em rastreabilidade de decisões.

## Camadas Semânticas
1. **Camada Matemática**  
   Estado toroidal em `T^7`, atualização EMA (`alpha=0.25`), e periodicidade-alvo de 42 ciclos.
2. **Camada de Integridade**  
   Commit gate `LOAD -> PROCESS -> VERIFY -> COMMIT` e validação por checksum/CRC.
3. **Camada de Runtime**  
   `RafaeliaPipelineWorker` agrega commit/rollback, `phiWindow[42]`, taxas e snapshot.
4. **Camada de Operação Android**  
   `RafaeliaBatchWorker` + `RafaeliaBatchScheduler` executam lotes e periodicidade.
5. **Camada de Governança de Release**  
   `RafaeliaAuditStore` persiste auditoria por build e `RafaeliaPromotionGate` decide promoção.

## Relação entre Elementos
- **Entrada de dados** -> `RafaeliaBatchWorker.KEY_PAYLOAD`
- **Transformação de estado** -> `RafaeliaCore`/JNI direto
- **Métrica de qualidade** -> `commitRate` e `rollbackRate`
- **Memória temporal** -> `phiWindow` (42 elementos ordenados no JSON)
- **Decisão de release** -> `promotable` por limiar de rollback

## Material para análise no GitHub
- Usar `docs/RELEASE_NOTES_RAFAELIA.md` como trilha incremental por versão.
- Usar `docs/INVENTORY_RAFAELIA.md` como inventário de artefatos.
- Usar auditorias persistidas (`audit_*.json`) para comparação entre builds.

## Próximos passos sugeridos
1. Adicionar diffs automáticos entre `audit_*.json` consecutivos.
2. Publicar baseline de métricas por dispositivo (emulador vs hardware real).
3. Criar checklist de promoção por versão (gate técnico + gate semântico).
