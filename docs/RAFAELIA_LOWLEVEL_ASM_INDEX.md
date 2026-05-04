# RAFAELIA Low-Level / ASM Index

## Escopo
Inventário dos arquivos `.s/.S` e fontes com low-level/inline asm.

## Itens
### 1) `app/src/main/cpp/termux-bootstrap-zip.S`
- Build oficial: **SIM** (`libtermux-bootstrap`, `Android.mk`).
- ABI: multi-ABI (assembler de bootstrap zip).
- Instruções: macros/rotinas de leitura de blob zip (sem claim de SIMD dedicado).
- Export: símbolo de payload bootstrap usado por `termux-bootstrap.c`.
- Cache/L1/L2: hot-path de cópia bootstrap, sem política explícita de layer.
- Hz/tick/phase/TTL: ausente.
- Branchless: não declarado.
- CRC/seal: ausente.
- Benchmark: não há benchmark dedicado no CI.
- Pontos S: S0=`termux-bootstrap-zip.S`; S1=OFICIAL+IMPLEMENTADO; S2=payload bootstrap; S3=CODE_AHEAD_DOC; S4=page-size em link; S5=sem selo dedicado; S6=N/A; S7=cópia de memória bootstrap; S8=PRECISA_MEDIÇÃO; S9=erro se payload inválido; S10=blob binário; S11=código; S12=PRECISA_BENCHMARK.

### 2) `app/src/main/cpp/lowlevel/baremetal_asm.S`
- Build oficial: **SIM condicional** (`termux-baremetal` em `arm64-v8a`/`armeabi-v7a`).
- ABI: ARM32/ARM64 com NEON habilitado por `-DHAS_BM_NEON_ASM=1`.
- Instruções: rotinas asm para otimização baremetal (NEON/ARM).
- Export: chamadas por `baremetal*.c`.
- Cache/L1/L2: otimização de throughput/memcpy low-level.
- Hz/tick/phase/TTL: sem scheduler explícito.
- Branchless: parcial (uso de instruções vetoriais reduz branch).
- CRC/seal: indireto (pipeline baremetal usa validações em C).
- Benchmark: coberto por suite lowlevel (`raf_bench_suite.c`) de forma indireta.
- Pontos S: S0=`baremetal_asm.S`; S1=OFICIAL+PARCIAL; S2=otimização ISA; S3=SYNCED parcial; S4=sem malloc no hot path; S5=sem selo próprio; S6=N/A; S7=hot path memória; S8=PRECISA_ARTIFACT por ABI; S9=risco ABI mismatch; S10=ABI asm; S11=código+build; S12=PRECISA_MEDIÇÃO.

### 3) `rmr/Rrr/rafaelia_b1.S`
- Build oficial: **NÃO** (SOLTO/Rrr).
- ABI: ARM assembly experimental.
- Instruções/export: bloco B1 Rrr, fora do pipeline oficial.
- Cache/Hz/TTL/branchless/CRC/benchmark: sem integração oficial.
- Pontos S: S0=`rmr/Rrr/rafaelia_b1.S`; S1=SOLTO/Rrr+EXPERIMENTAL; S2=prova de conceito; S3=DOC_AHEAD_CODE (oficial); S4=sem contrato oficial; S5=sem artifact oficial; S6=conceitual; S7=conceitual; S8=ruído alto; S9=INVÁLIDO para release; S10=asm puro; S11=arquivo solto; S12=HIPÓTESE.

### 4) `mvp/rafaelia_mvp_puro.s`
- Build oficial: **NÃO** (MVP).
- ABI: ASM MVP (fora do Android.mk).
- Instruções/export: MVP puro, sem ligação JNI/Gradle.
- Cache/Hz/TTL/branchless/CRC/benchmark: conceito/MVP.
- Pontos S: S0=`mvp/rafaelia_mvp_puro.s`; S1=MVP; S2=baseline conceitual; S3=DOC_AHEAD_CODE; S4=sem hardening; S5=sem selo; S6=conceitual; S7=conceitual; S8=PRECISA_BENCHMARK; S9=não aplicável ao release; S10=asm MVP; S11=arquivo MVP; S12=EXPERIMENTAL.

### 5) `rmr/src/main/cpp/rmr.c` (inline asm)
- Build oficial: **SIM** (módulo `rmr`).
- ABI: `rev` para ARM/AArch64 com fallback C.
- Instruções: `__asm__ __volatile__("rev ...")`.
- Export: JNI `RmrCore_*`.
- Cache/L1/L2: não explícito.
- Hz/tick/phase/TTL: ausente.
- Branchless: classificação `ARCH_SELECTED` (asm por arquitetura + fallback).
- CRC/seal: hash FNV-like local, não selo de release.
- Benchmark: sem artifact dedicado.
- Pontos S: S0=`rmr.c`; S1=OFICIAL+IMPLEMENTADO; S2=utilitário JNI; S3=SYNCED parcial; S4=bounds JNI básicos; S5=hash local; S6=N/A; S7=heap malloc em trecho não hot-path crítico; S8=PRECISA_MEDIÇÃO; S9=erro se JNI null/malloc fail tratado; S10=JNI API; S11=código; S12=PRECISA_ARTIFACT.
