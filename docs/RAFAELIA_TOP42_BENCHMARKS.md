# RAFAELIA TOP42 Benchmarks

## Estado atual
- Suite low-level em `app/src/main/cpp/lowlevel/raf_bench_suite.c` (MEASURED_LOCAL).
- Comparativos em `BENCHMARKS_COMPARISON.md` (DOC_ONLY/SCRIPT_ONLY conforme item).
- Sem evidência completa de artifact CI/device para todas as métricas TOP42.

## Classificação obrigatória
- storage_io: NEEDS_DEVICE na ausência de artifact físico.
- memory_compute: MEASURED_LOCAL quando extraído localmente.
- logical_instruction: MEASURED_LOCAL/DOC_ONLY sem artifact CI.
- scheduler: NEEDS_BENCHMARK se não publicar jitter/missed_ticks reproduzíveis.
- binary/system: CI_ARTIFACT quando workflow exporta APK/.so size, senão DOC_ONLY.

## Claims sem artifact
Todo claim de throughput/IOPS/latency sem log+hash+artifact permanece `CLAIM_WITHOUT_ARTIFACT`.
