# RAFAELIA Gap Matrix (Sementes vs Repositório)

## Objetivo
Mapear, de forma operacional, o que está implementado, parcial e ausente para orientar execução incremental.

## Matriz
| Tema | Estado | Evidência no repositório | Próximo passo técnico |
|---|---|---|---|
| Pipeline JNI zero-copy | Implementado | `rafaelia_jni_direct.c`, `RafaeliaCore` | Expandir cobertura de testes em device farm |
| Commit gate LOAD→PROCESS→VERIFY→COMMIT | Implementado (nível app) | `RafaeliaCore.processWithCommitGate`, `RafaeliaPipelineWorker` | Levar gate para CI de release |
| Janela periódica 42 | Implementado | `phiWindow[42]`, export JSON | Adicionar teste de estabilidade estatística por build |
| Persistência de auditoria | Implementado | `RafaeliaAuditStore`, `RafaeliaBatchWorker` | Criar comparador entre builds |
| Gate de promoção por rollback | Implementado | `RafaeliaPromotionGate` | Parametrizar por perfil de dispositivo |
| raf_bench_suite.c (microbench C) | **Ausente** | não encontrado | Criar suíte bare-metal de métricas (latência, energia proxy, throughput) |
| Medições multilíngues (entropia 10 línguas) | **Ausente** | não encontrado | Definir dataset + pipeline de coleta |
| Grassberger-Procaccia / D_H | **Ausente** | não encontrado | Implementar módulo analítico offline |
| CLIMEX/PLIMEX | **Ausente** | não encontrado | Especificar API e experimento reprodutível |
| Compilador domain-aware | **Ausente** | não encontrado | Desenhar RFC e MVP parser |

## Critério de prontidão v0.x
1. Estrutural: build + testes unitários verdes.
2. Operacional: batch periódico + `audit_json` persistido.
3. Governança: `promotable` por gate definido.
4. Científico: benchmark mínimo replicável com dataset controlado.
