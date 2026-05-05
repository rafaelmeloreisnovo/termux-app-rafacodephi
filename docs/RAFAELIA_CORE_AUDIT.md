# RAFAELIA CORE AUDIT

## Fato implementado
- Núcleo JNI direto com DirectByteBuffer e validações de capacidade.
- Estado lógico: RAF_STATE_DIM=7, RAF_PERIOD=42, RAF_VCPU=8.
- Buffers Java IN/OUT = 64KB, STATE_CAP=64.
- JNI_ARENA_SZ=256KB em `rafaelia_jni_direct.c`.
- BM_ARENA_SZ=512KB no baremetal nomalloc.
- CRC32C, coherence, entropy, phase, step ativos.
- Flag `RAFAELIA_NO_MALLOC` ativa seleção `baremetal_nomalloc.c`.
- Flag `RMR_PURE_CORE=1` força `baremetal_nomalloc.c` + modo puro sem heap/stdio/libm com CFLAGS dedicadas.

## Constante declarada/hipótese
- Hz de motor é lógico e não Hz físico de CPU.
- telemetria de clock/vCPU é medição operacional, não benchmark de performance.

## Ativo no build
- `termux_rafaelia_direct` compila com `rafaelia_jni_direct.c`, `raf_vcpu.c`, `raf_clock.c`.
- `termux-baremetal` compila `baremetal.c` ou `baremetal_nomalloc.c` conforme flag (modo puro prioriza `RMR_PURE_CORE`).
- `-lm` é condicional por `RMR_NO_LIBM`: quando `RMR_NO_LIBM=1`, `termux-baremetal` e `termux_rafaelia_direct` não linkam libm.

## Legado/experimental
- partes toroidais e bitraf avançado permanecem experimentais sem claim quântico real.

## Precisa prova executável
- assembleDebug/externalNativeBuildDebug em ambiente com Android SDK instalado.
- testes JNI instrumentados para erro controlado com buffers pequenos.


## Certification and audit claim notice

This repository does not claim ISO certification, formal ISO compliance, or accredited external audit status. Any ISO/IEC references are internal checklist references or methodological alignment notes only. Certification requires an external accredited audit process and is outside the scope of this repository.

Este repositório não declara certificação formal baseada em ISO, conformidade ISO formal nem auditoria externa acreditada. Qualquer referência a ISO/IEC é apenas checklist interno, referência metodológica ou alinhamento preliminar de boas práticas. Certificação exige processo externo acreditado e está fora do escopo deste repositório.

### Audit/benchmark/runtime trail
- `docs/AUDIT_CLAIMS_POLICY.md`
- `reports/vectra_grade_benchmarks.md`
- `reports/device_runtime_smoke.md`
- `reports/rmr_equivalence.md`
- CI validação não equivale a validação em device real.
- Benchmark definido não equivale a benchmark medido.
