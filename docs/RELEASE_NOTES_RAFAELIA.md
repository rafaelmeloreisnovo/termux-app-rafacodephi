# Release Notes (Rafaelia Integration)

## v0.11.0-unreleased
- `app/src/main/cpp/Android.mk` atualizado com bloco `RMR_PURE_CORE` para modo puro do core low-level.
- Modo puro aplica define/flags: `RAFAELIA_NO_MALLOC`, `RMR_NO_HEAP`, `RMR_NO_STDIO`, `RMR_NO_LIBM`, `RMR_NO_DEBUG_STRING`, `RMR_USE_Q16`, `RMR_ENABLE_ASM`, `RMR_ENABLE_BRANCHLESS`, `-fvisibility=hidden`, `-fno-unwind-tables`, `-fno-asynchronous-unwind-tables`, `-fno-ident`.
- `termux-baremetal` força `lowlevel/baremetal_nomalloc.c` quando `RMR_PURE_CORE=1` e mantém compatibilidade JNI/Android no modo não puro.
- `-lm` passou a ser condicional por `RMR_NO_LIBM` em `termux-baremetal` e `termux_rafaelia_direct` (hot path).

## v0.9.0-unreleased
- `RafaeliaAuditManifest` adicionado para gerar hashes SHA-256 de `audit_json`/`audit_svg` e raiz merkle-like.
- `RafaeliaAuditStore.saveManifest(...)` adicionado para persistência de manifesto.
- `RafaeliaBatchWorker` agora retorna `audit_manifest_path`.
- Teste `RafaeliaAuditManifestTest` adicionado.

## v0.8.0-unreleased
- `RafaeliaAuditSvg` adicionado para exportar visualização SVG da série `phiWindow(42)`.
- `RafaeliaBatchWorker` passa a persistir e retornar `audit_svg_path` além de `audit_json`.
- `RafaeliaAuditStore` ampliado com persistência `saveAuditSvg(...)`.
- `ROADMAP_RAFAELIA.md` atualizado com Fase 3 em andamento.

## v0.7.0-unreleased
- Adicionado `app/src/main/cpp/lowlevel/raf_bench_suite.c` (MVP) com métricas de CRC32C throughput, EMA Q16.16 e entropia milli.
- Atualizada matriz de gaps para refletir `raf_bench_suite.c` como implementado (MVP).

## v0.6.0-unreleased
- Novo `RafaeliaAuditComparator` para comparar auditorias entre builds consecutivos (`commitRate`, `rollbackRate`, `currentCycle`).
- Teste unitário `RafaeliaAuditComparatorTest` cobrindo cálculo de deltas.
- Novo `docs/RAFAELIA_GAP_MATRIX.md` com mapeamento objetivo de lacunas (implementado/parcial/ausente).
- Atualização de inventário para incluir runtime completo, testes e documentação operacional.

## v0.5.0-unreleased
- `RafaeliaPipelineWorker` agora exporta `schemaVersion` e `rollbackRate` no JSON de auditoria.
- `phiWindow` exportado em ordem temporal (mais antigo -> mais novo) para comparação consistente entre builds.
- Teste unitário novo `RafaeliaPipelineWorkerTest` validando contrato mínimo do JSON.
- Novo documento `RAFAELIA_SEMANTIC_LAYERS.md` com mapa semântico entre matemática, runtime e release.

## v0.4.0-unreleased
- Agendador periódico `RafaeliaBatchScheduler` para coleta histórica via WorkManager.
- Persistência local de auditoria `audit_json` por build em `files/rafaelia-audit/`.
- Gate formal de promoção por rollback rate (`RafaeliaPromotionGate`, default <= 10%).
- `RafaeliaBatchWorker` agora retorna `audit_path` e `promotable` além de `audit_json`.

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


## v0.10.0-unreleased
- seed alignment
- loose files map
- vCPU/HZ official MVP
- memory layers
- BitRAF encode/decode MVP
- GP dimension MVP


## vNext / Unreleased
- código alterado: correção AVX2 preprocessor, locking de perfil de clock, validação de capacidade em debugStep, guardas crc32 Java, pipeline TOP42 CI artifacts.
- motivo: coerência code+docs+artifact.
- teste: validate_code_doc_sync + geração top42.*
- status: IMPLEMENTADO/PARCIAL (device benchmark pendente).
- gap remanescente: medições em arm32/arm64 real.
- próximos passos: anexar artifacts por ABI.
