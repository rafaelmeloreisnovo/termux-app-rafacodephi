# RMR Cold Path Boundaries

## Objetivo
Definir fronteiras explícitas entre rotinas de medição/descoberta de ambiente e o loop crítico de execução (vCPU/BitRAF/CRC), evitando chamadas de sistema no hot path.

## Classificação de rotinas

### 1) `raf_clock_now_ns` (`clock_gettime`)
- Arquivo: `app/src/main/cpp/lowlevel/raf_clock.c`
- Chamada de sistema: `clock_gettime(CLOCK_MONOTONIC, ...)`
- Classificação: **MEASURE_PATH / COLD_PATH**
- Regra: usar para telemetria, timestamp de borda, benchmarking e coleta de métricas; **não usar no inner-loop crítico**.

### 2) `raf_memory_layers_get` (`sysconf`)
- Arquivo: `app/src/main/cpp/lowlevel/raf_memory_layers.c`
- Chamadas de sistema: `sysconf(_SC_LEVEL1_DCACHE_LINESIZE)` e `sysconf(_SC_PAGESIZE)`
- Classificação: **COLD_PATH**
- Regra: executar em bootstrap/init, warmup, diagnóstico, snapshot de hardware; **não usar no inner-loop crítico**.

## Regra explícita de loop crítico

**É proibido chamar `raf_clock_now_ns` e `raf_memory_layers_get` no loop crítico de vCPU/BitRAF/CRC.**

Loop crítico inclui, sem limitar:
- passo contínuo de vCPU (ex.: `raf_vcpu_step` em sequência);
- encode/decode/processamento BitRAF por bloco;
- atualização/validação CRC por bloco no caminho de processamento principal.

## Mapeamento de chamadas críticas

### Caminhos críticos (hot path)
- `Java_com_termux_x11_LowLevelRaf_processNative` → processamento por buffer + CRC por bloco.
- `Java_com_termux_x11_LowLevelRaf_stepAllVcpusNative` → passo contínuo de vCPUs.
- `raf_vcpu_step` / rotinas de encode-decode BitRAF / CRC incremental no pipeline principal.

## Pontos de invocação permitidos (cold/measure)

### Permitido para `raf_clock_now_ns`
- início/fim de APIs JNI para medir latência de borda (entry/exit);
- benchmark/suite de medição;
- telemetria agregada fora do loop interno.

### Permitido para `raf_memory_layers_get`
- init nativo/JNI (`init`);
- consulta de capacidade/página/cache em setup;
- endpoints de diagnóstico/status.

## Diretriz operacional
- Se precisar tempo/cache/page-size no processamento crítico, **capturar antes** e reutilizar valores em estruturas de contexto.
- Evitar `clock_gettime`/`sysconf` por iteração de bloco/frame/instrução.
- Toda nova API crítica deve declarar explicitamente se executa em `HOT_PATH` ou `COLD_PATH`.
