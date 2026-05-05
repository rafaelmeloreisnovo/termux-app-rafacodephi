# BETA — Lowlevel Performance and Hardware Use Audit

## Matriz de rotinas

| Rotina | Arquivo | Linguagem | Está no hot path? | Usa hardware melhor? | Evidência | Teste | Benchmark | Status |
|---|---|---|---|---|---|---|---|---|
| `processNative` (JNI zero-copy) | `app/src/main/cpp/lowlevel/rafaelia_jni_direct.c` | C/JNI | Sim (runtime RAF) | Sim (DirectBuffer + zero malloc + CRC inline) | arena 256KB, `GetDirectBufferAddress`, sem `NewByteArray` | unit JNI | obrigatório | POSSIBLE_LOWLEVEL_GAIN |
| `stepNative` | `app/src/main/cpp/lowlevel/rafaelia_jni_direct.c` | C/JNI | Sim (math runtime) | Sim (Q16 inteiro, sem heap) | update 7D + fase 42 em estado nativo | unit JNI | obrigatório | POSSIBLE_LOWLEVEL_GAIN |
| `stepAllVcpusNative` | `app/src/main/cpp/lowlevel/rafaelia_jni_direct.c` + `raf_vcpu.c` | C/JNI | Sim (math runtime) | Sim (8 vCPU estáticos) | `RAF_VCPU=8`, estado alinhado 64B | unit JNI | obrigatório | POSSIBLE_LOWLEVEL_GAIN |
| Clock RAF | `raf_clock.c/.h` | C | Sim (scheduler/telemetria RAF) | Sim (tempo monotônico + jitter/miss) | `actual_delta_ns`, `jitter_ns`, `missed_ticks` | unit C | obrigatório | POSSIBLE_LOWLEVEL_GAIN |
| Scheduler por custo core/GPU | `rafaelia_gpu_orchestrator.c/.h` | C | Parcial (dependente de integração) | Sim (probe OpenCL/Vulkan + seleção por custo/freq) | `remk_cost_fn`, `remk_pick_core`, probe runtime | unit C | obrigatório | NEEDS_BENCHMARK |
| Commit gate branchless | `rafaelia_commit_gate_ll.c/.h` | C | Sim (pipeline RAF) | Sim (sem branches críticos) | `(p>=theta)&(h<=eta)` | unit C | obrigatório | POSSIBLE_LOWLEVEL_GAIN |
| GP dimension | `raf_gp_dimension.c` | C | Não (análise) | Incerto | função de relatório, sem integração hot path terminal | unit C | necessário | EXPERIMENTAL_NOT_IN_HOT_PATH |
| BitRAF encode/decode | `raf_bitraf.c` | C | Parcial | Sim (compactação 42-bit) | packing bit a bit fixo | unit C | obrigatório | MICROBENCH_ONLY |
| BitRAF core (CRC16/ECC/paridade/atrator) | `rafaelia_bitraf_core.c` | C | Não confirmado no caminho terminal | Potencial | estruturas de ruído/ECC/paridade implementadas | unit C | necessário | EXPERIMENTAL_NOT_IN_HOT_PATH |
| Buffers estáticos Java bridge | `RafaeliaCore.java` | Java | Sim | Sim (reduz GC/cópia JNI extra) | `allocateDirect` estático único | unit Java | obrigatório | POSSIBLE_LOWLEVEL_GAIN |
| BareMetal math/mem ops | `BareMetal.java` (+ native lib) | Java+Native | Não confirmado no hot path terminal | Potencial (SIMD where available) | API declara fastMemCopy/Set, rsqrt | unit Java/native | necessário | NO_EVIDENCE |

## Hot path terminal vs hot path RAF
- **Terminal hot path real**: PTY/session I/O do app Termux base (não evidenciado nos arquivos obrigatórios desta auditoria).
- **RAFAELIA math runtime hot path**: `RafaeliaCore.process/step` → JNI direct buffers → `processNative/stepNative/stepAllVcpusNative`.

Conclusão operacional: ganho do RAF provado apenas para *runtime matemático nativo isolado*; ganho *end-to-end terminal* exige benchmark integrado de sessão PTY.

## Benchmarks mínimos (plano executável)
### Grupo A — RAF microbench
1. `stepNative` 10.000 ciclos.
2. `stepAllVcpusNative` 10.000 ciclos.
3. `processNative` payload 64KB.
4. `crc32Native` payload 64KB.
5. `remk_pick_core` 100.000 iterações.
6. `rfg_u` (commit gate) 100.000 iterações.
7. `raf_gp_dimension_report` com n∈{64,128,256,512,1024}.
8. Java equivalente vs native (onde existir).
9. `DirectByteBuffer` vs `byte[]`.
10. jitter clock em 10/60/120/1000 Hz.

### Grupo B — End-to-end terminal
1. abertura de sessão.
2. fechamento de sessão.
3. 100 comandos `echo`.
4. throughput stdout.
5. latência stdin→stdout.
6. cleanup após 100 sessões.
7. BitRAF encode/decode em fluxo real.
8. CRC/hash em fluxo real.
9. buffer copy em fluxo real.
10. native vs Java equivalent, quando existir.

## Matriz comparativa obrigatória
Executar todos os testes nos 5 perfis:
1. Termux base.
2. RafCodePhi lowlevel OFF.
3. RafCodePhi lowlevel ON.
4. RafCodePhi experimental OFF.
5. RafCodePhi experimental ON.

Regras de interpretação:
- Não declarar ganho sem número.
- Não extrapolar microbenchmark para ganho global.
- Reportar separado: micro nativo isolado vs end-to-end terminal.
