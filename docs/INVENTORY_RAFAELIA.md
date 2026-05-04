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

## Testes
- Unit: `RafaeliaCoreTest`, `RafaeliaUtilsDirectPipelineTest`
- Instrumentado: `RafaeliaDirectInstrumentedTest`
