# Inventário RAFAELIA (humano + IA)

## Núcleo nativo
- `app/src/main/cpp/lowlevel/rafaelia_jni_direct.c`
- `app/src/main/cpp/lowlevel/baremetal_nomalloc.c`
- `app/src/main/cpp/lowlevel/baremetal_nomalloc.h`

## Build
- `app/src/main/cpp/Android.mk` (módulo `termux_rafaelia_direct`)

## API Java
- `rafaelia/src/main/java/com/termux/rafaelia/RafaeliaCore.java`
- `rafaelia/src/main/java/com/termux/rafaelia/RafaeliaUtils.java`
- `rafaelia/src/main/java/com/termux/rafaelia/runtime/RafaeliaPipelineWorker.java`
- `rafaelia/src/main/java/com/termux/rafaelia/runtime/RafaeliaBatchWorker.java`
- `rafaelia/src/main/java/com/termux/rafaelia/runtime/RafaeliaBatchScheduler.java`
- `rafaelia/src/main/java/com/termux/rafaelia/runtime/RafaeliaAuditStore.java`
- `rafaelia/src/main/java/com/termux/rafaelia/runtime/RafaeliaPromotionGate.java`

## Testes
- Unit: `RafaeliaCoreTest`, `RafaeliaUtilsDirectPipelineTest`
- Unit (runtime): `RafaeliaPipelineWorkerTest`
- Instrumentado: `RafaeliaDirectInstrumentedTest`, `RafaeliaLoadInstrumentedTest`

## Documentação operacional
- `docs/RELEASE_NOTES_RAFAELIA.md`
- `docs/ROADMAP_RAFAELIA.md`
- `docs/RAFAELIA_SEMANTIC_LAYERS.md`
- `docs/RAFAELIA_GAP_MATRIX.md`
