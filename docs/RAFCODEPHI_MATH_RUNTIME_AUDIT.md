# RAFCODEPHI / RAFAELIA — Math Runtime Audit

## Escopo auditado
Arquivos obrigatórios auditados:
- `rafaelia/src/main/java/com/termux/rafaelia/RafaeliaCore.java`
- `app/src/main/cpp/lowlevel/rafaelia_jni_direct.c`
- `app/src/main/cpp/lowlevel/raf_clock.c`
- `app/src/main/cpp/lowlevel/raf_clock.h`
- `app/src/main/cpp/lowlevel/raf_vcpu.c`
- `app/src/main/cpp/lowlevel/raf_vcpu.h`
- `app/src/main/cpp/lowlevel/raf_memory_layers.c`
- `app/src/main/cpp/lowlevel/raf_memory_layers.h`
- `app/src/main/cpp/lowlevel/rafaelia_gpu_orchestrator.c`
- `app/src/main/cpp/lowlevel/rafaelia_gpu_orchestrator.h`
- `app/src/main/cpp/lowlevel/rafaelia_commit_gate_ll.c`
- `app/src/main/cpp/lowlevel/rafaelia_commit_gate_ll.h`
- `app/src/main/cpp/lowlevel/raf_gp_dimension.c`
- `app/src/main/cpp/lowlevel/raf_bitraf.c`
- `rafaelia/src/main/cpp/rafaelia_bitraf_core.c`
- `app/src/main/cpp/Android.mk`
- `app/src/main/java/com/termux/lowlevel/BareMetal.java`

## Mapa matemático e de execução
1. **Buffer/cache → estado 7D**: `RafaeliaCore` fixa `IN_BUF/OUT_BUF/STATE_BUF` como `DirectByteBuffer` estático; JNI acessa via `GetDirectBufferAddress`; `raf_state_t` tem vetor `s[7]` + coerência/entropia/fase/step. **Status: PROVADO_NO_CÓDIGO**.
2. **Fase 42**: `RAF_PERIOD=42`; fase avança modulo 42 em `stepNative`, `processNative` e `raf_vcpu_step`. **Status: PROVADO_NO_CÓDIGO**.
3. **Constantes geométricas Q16 / frequência como memória operacional**: uso de Q16.16 em EMA e `phi=(1-H)*C`; clock materializa memória temporal (`period_ns`, `actual_delta_ns`, `jitter_ns`, `missed_ticks`, `total_ticks`, `actual_hz_q16`). **Status: PROVADO_NO_CÓDIGO**.
4. **VCPU e scheduler por custo**: `RAF_VCPU=8`; vCPU guarda clock local/crc/estado; orquestrador calcula custo por latência+carga+intensidade+térmica−bias GPU e escolhe core por erro de frequência Q16. **Status: PROVADO_NO_CÓDIGO**.
5. **Ruído/latência/térmica/intensidade**: latência+intensidade+térmica entram em `remk_cost_fn`; `noise_acc` e `noise` existem no BitRAF core; gate atual usa coerência/entropia/hash. **Status: IMPLEMENTADO_COM_NOME_DIFERENTE**.
6. **Commit gate / integridade**: `rfg_u` branchless (`commit = (p>=theta)&(h<=eta)`), hash evolutivo e integração com `crc32Native` no pipeline Java. **Status: PROVADO_NO_CÓDIGO**.
7. **CRC/ECC/paridade**: CRC32C no JNI; CRC16 e dual parity/ECC no `rafaelia_bitraf_core.c`. **Status: PROVADO_NO_CÓDIGO**.
8. **Atrator 42**: descrito e implementado no núcleo BitRAF (`atrator 42`, slot/base). **Status: PROVADO_NO_CÓDIGO**.
9. **GP/correlation dimension**: `raf_gp_dimension.c` expõe cálculo/report JSON simplificado de correlação/dimensão. **Status: PROVADO_NO_CÓDIGO**.

## Claims obrigatórios (classificação)
- RAF_STATE_DIM=7 → **PROVADO_NO_CÓDIGO**.
- RAF_PERIOD=42 → **PROVADO_NO_CÓDIGO**.
- RAF_VCPU=8 → **PROVADO_NO_CÓDIGO**.
- Estado alinhado 64B → **PROVADO_NO_CÓDIGO**.
- L1_STATE=64 → **PROVADO_NO_CÓDIGO**.
- L2_IN/L2_OUT=65536 → **PROVADO_NO_CÓDIGO**.
- Arena JNI=256KB → **PROVADO_NO_CÓDIGO**.
- Arena BareMetal=512KB → **PROVADO_NO_CÓDIGO**.
- DirectByteBuffer estático → **PROVADO_NO_CÓDIGO**.
- Q16 fixed point → **PROVADO_NO_CÓDIGO**.
- Constantes geométricas Q16 (incl. 56755 ~ √3/2) → **PROVADO_NO_CÓDIGO**.
- Constantes irracionais/seeds → **PROVADO_NO_CÓDIGO**.
- Clock como memória (target/period/delta/jitter/missed/total) → **PROVADO_NO_CÓDIGO**.
- actual_hz_q16 → **PROVADO_NO_CÓDIGO**.
- EMA coerência/entropia → **PROVADO_NO_CÓDIGO**.
- Commit gate branchless → **PROVADO_NO_CÓDIGO**.
- CRC32C/CRC16 → **PROVADO_NO_CÓDIGO**.
- ECC/paridade dupla → **PROVADO_NO_CÓDIGO**.
- Noise accumulation → **IMPLEMENTADO_COM_NOME_DIFERENTE**.
- Atrator 42 → **PROVADO_NO_CÓDIGO**.
- GP/correlation dimension → **PROVADO_NO_CÓDIGO**.
- Scheduler por custo (latência/carga/intensidade/térmica/GPU bias) → **PROVADO_NO_CÓDIGO**.
- Memory barriers dmb/mfence → **PROVADO_NO_CÓDIGO**.
- Queue atomics acquire/release → **PROVADO_NO_CÓDIGO**.
- GPU/OpenCL/Vulkan probe → **PROVADO_NO_CÓDIGO**.
- Core selection por erro de frequência → **PROVADO_NO_CÓDIGO**.
- Diferença Terminal Hot Path vs RAFAELIA Math Runtime Hot Path → **PRECISA_RUNTIME_TEST**.

## Pontos especiais
1. **Sete senoides**: não há sete chamadas explícitas `sin/cos` nos arquivos obrigatórios; dinâmica usa mistura Q16/seeds/EMA/phase-step. **Status: NÃO_ENCONTRADO (sin/cos explícito)** e **IMPLEMENTADO_COM_NOME_DIFERENTE (oscilação discreta Q16)**.
2. **Triângulo isósceles na triangulação**: não localizado explicitamente nos arquivos obrigatórios. **Status: NÃO_ENCONTRADO**.
3. **√3/2 = 56755 Q16**: constante 56755 aparece em seeds e evolução de estado; compatível com claim geométrico Q16. **Status: PROVADO_NO_CÓDIGO**.
4. **Latência+ruído no scheduler/commit gate**: latência entra no scheduler; ruído explícito está no core BitRAF, não no gate lowlevel atual. **Status: IMPLEMENTADO_COM_NOME_DIFERENTE**.
5. **Buffer/cache alimenta vCPU direto**: JNI step all vcpus invoca `raf_vcpu_step` sem alocação por chamada. **Status: PROVADO_NO_CÓDIGO**.
6. **Estado no hot path sem alloc**: caminho JNI usa buffers/arênas estáticas. **Status: PROVADO_NO_CÓDIGO**.
7. **Arredondamento float vs Q16**: runtime RAF principal é Q16 inteiro; float aparece em APIs BareMetal genéricas. **Status: IMPLEMENTADO_COM_NOME_DIFERENTE**.
8. **Redução de ciclos/instruções**: não há contadores de instruções nos arquivos obrigatórios. **Status: PRECISA_BENCHMARK**.
9. **ASM/inline/barriers**: barriers inline ASM (`dmb/mfence`) presentes; não há bloco extenso ASM dedicado no fluxo obrigatório. **Status: PROVADO_NO_CÓDIGO**.

## Mapa de cache/buffer
- L1 estado: 64B.
- L2 stream: IN/OUT 64KB cada.
- Arena JNI: bump allocator estático 256KB alinhado 64B.
- Arena BareMetal no módulo nomalloc: 512KB estático alinhado 64B.
- Queue work-stealing: atomics acquire/release e barreira arquitetural dedicada.

## Mapa de frequência
- `raf_clock_init(target_hz)` define `period_ns`.
- `raf_clock_mark_tick(now)` atualiza `actual_delta_ns`, `jitter_ns`, `missed_ticks`, `total_ticks`.
- `raf_clock_actual_hz_q16` converte delta em Hz Q16.
- Scheduler usa erro de frequência por core (`abs(task_hz_q16-core_freq_q16)`).

## Mapa de ruído
- Entropia EMA em `processNative/stepNative` alimenta `phi` e gate.
- `noise` / `noise_acc` no core BitRAF com metadados de instrução.
- Ruído não aparece como exception; aparece como variável operacional e assinatura.

## Gaps
- Ausência de evidência de “sete senoides” em forma trigonométrica explícita.
- Ausência de medição end-to-end terminal (sessão real PTY) nos arquivos obrigatórios.
- Ausência de benchmark formal comparando OFF/ON experimental no mesmo harness.

## Plano de benchmark obrigatório (proposto)
1. `stepNative` 10.000 ciclos.
2. `stepAllVcpusNative` 10.000 ciclos.
3. `processNative` 64KB.
4. `crc32Native` 64KB.
5. `remk_pick_core` 100.000 chamadas.
6. `rfg_u` (commit gate) 100.000 chamadas.
7. `raf_gp_dimension_report` com n variável.
8. Equivalente Java vs native.
9. `DirectByteBuffer` vs `byte[]`.
10. jitter em 10/60/120/1000 Hz.

Métricas mínimas: p50/p95/p99, ns/op, MiB/s, CPU%, erro de frequência Q16, jitter acumulado, misses de tick, alocação GC, e separação estrita microbenchmark vs end-to-end.
