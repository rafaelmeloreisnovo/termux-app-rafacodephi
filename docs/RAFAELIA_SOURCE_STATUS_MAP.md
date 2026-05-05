# RAFAELIA Source Status Map

## OFICIAL_BUILD
- `app/src/main/cpp/Android.mk` + `app/build.gradle` + `rafaelia/src/main/cpp/Android.mk`.
- Inclui bootstrap, baremetal, JNI direct, vCPU/clock/memory layers/bitraf/gp dimension.
- S1=OFICIAL/IMPLEMENTADO.

## OFICIAL_JAVA_API
- `rafaelia/src/main/java/com/termux/rafaelia/RafaeliaCore.java` e `runtime/*`.
- Wrappers JNI + scheduler/worker.
- S1=OFICIAL/IMPLEMENTADO.

## SOLTO_Rrr
- `rmr/Rrr/*` (ex.: `rafaelia_core.c`, `rafaelia_b1.S`, `rafaelia_arena.h`, etc.).
- S1=SOLTO/Rrr, em geral EXPERIMENTAL/PARCIAL.

## MVP
- `mvp/rafaelia_mvp_puro.s`, `mvp/rafaelia_opcodes.hex`.
- S1=MVP/EXPERIMENTAL.

## OLD_LEGACY
- `rafaelia/old/*`.
- S1=LEGADO.

## DOC_ONLY
- Conceitos sem símbolo/código oficial confirmado: ZipRAF (quando citado sem código), BitOmega (idem), TTL literal no core oficial.

## BENCH_ONLY
- `scripts/run_top42_bench.sh`, `.github/workflows/top42_bench.yml`, `top42.csv/json` placeholders.
- S1=PRECISA_ARTIFACT/NEEDS_DEVICE para métricas device-dependent.
