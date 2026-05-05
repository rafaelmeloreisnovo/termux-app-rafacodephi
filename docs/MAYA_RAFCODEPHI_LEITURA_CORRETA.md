# MAYA — Leitura Correta do RafCodePhi Termux

Documento único para leitura humana, IA assistente, Codex, Maya ou qualquer agente técnico que precise entender o projeto **sem reduzir o RafCodePhi a um Termux comum com módulos nativos soltos**.

Este arquivo consolida os vetores técnicos levantados na sessão de análise e corrige a leitura incompleta anterior: o núcleo importante não é apenas `TerminalSession` nem apenas `baremetal.c`. O projeto possui uma segunda via operacional: **Java como casca de controle, DirectByteBuffer como barramento, JNI/C/ASM como execução, buffer/cache como alimentação de vCPU, frequência como memória operacional e ruído/latência como parte do custo de decisão**.

---

## 1. Resumo executivo

O `termux-app-rafacodephi` é um fork do Termux com identidade própria, instalação lado a lado e módulos nativos adicionais. Porém, a leitura correta não é apenas:

> “Termux + C/ASM + boosters”.

A leitura correta é:

> **RafCodePhi é uma variante experimental do Termux que mantém o terminal como veículo Android, mas adiciona um runtime RAFAELIA/RMR orientado a cache, frequência, estado e baixo nível. Java atua como casca de controle; os dados quentes são movidos por buffers diretos; a execução crítica passa por JNI/C/ASM; vCPUs recebem estado alimentado por buffer/cache; Hz vira memória operacional; ruído, latência, carga, térmica e intensidade entram no scheduler/commit gate.**

A primeira beta deve ser avaliada em duas camadas separadas:

1. **Termux Runtime Path**: abertura do app, sessão terminal, PTY, stdout/stderr, fechamento, processo/zumbi, Android lifecycle.
2. **RAFAELIA Math/Cache Runtime Path**: DirectByteBuffer, arena nativa, estado 7D, fase 42, vCPU, clock Hz/Q16, CRC, commit gate, scheduler por custo, barreiras de memória, BitRAF/append-only.

Erro de leitura a evitar:

> Não comparar apenas `ByteQueue` do terminal com Termux oficial e concluir que o projeto inteiro é “igual ao Termux”. Esse é só um caminho. O núcleo RAFAELIA direto opera por outro canal.

---

## 2. Status atual do projeto

### 2.1 Status por evidência

| Área | Status | Evidência / leitura |
|---|---:|---|
| Fork Termux com package próprio | PROVADO_NO_CÓDIGO | `com.termux.rafacodephi`, app side-by-side. |
| Módulos extras `:rafaelia` e `:rmr` | PROVADO_NO_CÓDIGO | `settings.gradle` inclui módulos além do Termux oficial. |
| Build matrix APK | PROVADO_NO_CI | GitHub Actions run `25354676970` gerou artifact `6797725229`. |
| Artifact digest | PROVADO_NO_CI | `sha256:cb5831988989b81bc66a8abe54d3021bdb733ac1fc5984dde08fa5b28c9ea900`. |
| APKs por ABI | PROVADO_NO_CI | `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`, `universal`. |
| Java como casca low-level | PROVADO_NO_CÓDIGO | `RafaeliaCore.java` usa buffers diretos estáticos e JNI fino. |
| Buffer/cache alimentando núcleo | PROVADO_NO_CÓDIGO | `IN_BUF/OUT_BUF=65536`, `STATE_BUF=64`, arena JNI 256KB, memory layers. |
| Hz como memória operacional | PROVADO_NO_CÓDIGO | `raf_clock_t`: target, period, delta, jitter, missed ticks, total ticks, Q16. |
| VCPU estado 7D/fase 42 | PROVADO_NO_CÓDIGO | `RAF_STATE_DIM=7`, `RAF_PERIOD=42`, `RAF_VCPU=8`. |
| Commit gate/coerência/entropia | PROVADO_NO_CÓDIGO | `rafaelia_commit_gate_ll.c`. |
| Scheduler por custo | PROVADO_NO_CÓDIGO | latência, load, intensity, thermal, gpu_bias, core frequency error. |
| Barreiras ASM | PROVADO_NO_CÓDIGO | `dmb`, `dmb ish`, `mfence`. |
| Senoides explícitas via `sin()` | NÃO_ENCONTRADO | Não foi visto `sinf()` como hot path principal. |
| Senoides/fases discretas Q16 | IMPLEMENTADO_COM_NOME_DIFERENTE | seeds/constantes Q16, estado 7D, fase 42, √3/2 representado por `56755`. |
| Triângulo isósceles explícito | NÃO_ENCONTRADO_COM_ESSE_NOME | A geometria aparece codificada por constantes/fase/Q16; falta doc formal. |
| Ganho global contra Termux oficial | PRECISA_BENCHMARK | CI prova build, não runtime/performance real. |
| Terminal sem zumbi em aparelho | PRECISA_RUNTIME_TEST | Código melhorou kill/cleanup, mas precisa teste ADB. |

---

## 3. Arquitetura correta: dois hot paths

### 3.1 Caminho A — Termux Runtime Path

Esse caminho é herdado do Termux e precisa continuar funcionando:

```text
Android Activity / Service
→ TerminalSession
→ JNI.createSubprocess
→ PTY
→ FileInputStream / FileOutputStream
→ ByteQueue
→ Handler main thread
→ TerminalEmulator
→ TerminalView
```

Melhorias já observadas no RafCodePhi:

- tratamento de erro em `createSubprocess`;
- validação de PID/FD;
- fechamento defensivo de recursos;
- proteção contra cleanup duplo;
- `mShellPid > 0` como critério mais correto de sessão ativa;
- kill de grupo de processo via `Os.kill(-shellPid, SIGKILL)`;
- `waitpid` com tratamento de `EINTR`;
- correção de liberação de string JNI em `cwd`;
- validação de geometria do terminal;
- fechamento de `/dev/ptmx` em falhas.

Risco ainda existente:

- `ByteQueue` de saída processo→terminal foi reduzido para `4096` bytes no caminho Java do terminal. Isso pode ser bom para latência/interatividade, mas pode ser pior em stdout pesado. Precisa benchmark com `seq`, `yes`, `apt`, compilação e logs longos.

### 3.2 Caminho B — RAFAELIA Math/Cache Runtime Path

Esse é o caminho que não deve ser confundido com o terminal comum:

```text
Java controle
→ DirectByteBuffer estático
→ JNI direto
→ arena nativa alinhada
→ estado 7D
→ fase 42
→ vCPU
→ clock Hz/Q16
→ CRC/ECC/paridade
→ scheduler por custo
→ commit gate
→ telemetria
```

Esse caminho está muito mais perto de baixo nível:

- `RafaeliaCore.java` aloca `IN_BUF`, `OUT_BUF` e `STATE_BUF` apenas uma vez.
- `IN_BUF` e `OUT_BUF` possuem 64KB.
- `STATE_BUF` possui 64 bytes.
- `rafaelia_jni_direct.c` usa `GetDirectBufferAddress`.
- A arena JNI é estática, 256KB, alinhada a 64 bytes.
- O estado é atualizado em C, com Q16, CRC e mutexes explícitos.
- O scheduler usa `pthread_mutex`, `atomic_uint`, `memory_order_acquire/release` e barreiras de memória por arquitetura.

---

## 4. Java como casca de baixo nível

A leitura correta do Java do RafCodePhi não é “Java idiomático comum”.

No núcleo RAFAELIA, Java atua como:

- carregador de biblioteca nativa;
- proprietário de buffers diretos fixos;
- camada de despacho JNI;
- leitor de resultados compactos;
- interface Android/usuário.

### 4.1 Evidência principal

`rafaelia/src/main/java/com/termux/rafaelia/RafaeliaCore.java`:

```java
private static final int IN_CAP    = 65536;
private static final int OUT_CAP   = 65536;
private static final int STATE_CAP = 64;

public static final ByteBuffer IN_BUF;
public static final ByteBuffer OUT_BUF;
public static final ByteBuffer STATE_BUF;

static {
    IN_BUF    = ByteBuffer.allocateDirect(IN_CAP).order(ByteOrder.nativeOrder());
    OUT_BUF   = ByteBuffer.allocateDirect(OUT_CAP).order(ByteOrder.nativeOrder());
    STATE_BUF = ByteBuffer.allocateDirect(STATE_CAP).order(ByteOrder.nativeOrder());
    System.loadLibrary("termux_rafaelia_direct");
}
```

Leitura:

- não cria `ByteBuffer.wrap()` no hot path;
- não cria novo buffer a cada chamada;
- usa `nativeOrder()`;
- chama JNI com `DirectByteBuffer`;
- Java não é o dono do estado matemático quente.

### 4.2 Onde Java ainda aloca

Telemetria e relatório ainda criam `byte[]` e `String`, por exemplo em `getVcpuTelemetry()` e `getClockProfile()`.

Isso não invalida o desenho, mas separa:

| Caminho | Tipo | Leitura |
|---|---|---|
| `process()` | hot path | baixo nível / buffer direto |
| `step()` | hot path | estado nativo / fase |
| `crc32()` | hot path | CRC nativo |
| `getClockProfile()` | reporting path | pode alocar String |
| `getVcpuTelemetry()` | reporting path | pode alocar String |
| `getHwProfile()` | diagnóstico | pode alocar String |

Regra documental:

> Não afirmar “zero alocação no Java inteiro”. Afirmar “hot path RAFAELIA evita alocação por chamada; reporting path ainda aloca para legibilidade”.

---

## 5. Buffer/cache como alimentação de vCPU

### 5.1 Memory layers

`raf_memory_layers.c` define:

```text
L1_STATE_CAP = 64
L2_IN_BUF    = 65536
L2_OUT_BUF   = 65536
L2_JNI_ARENA = 256KB
L2_BM_ARENA  = 512KB
RAM_NOTE     = 1
CACHE_LINE   = 64
PAGE_SIZE    = sysconf(_SC_PAGESIZE)
```

Leitura correta:

- L1 não é “cache real garantida”, mas estado pequeno e alinhado para favorecer cache residency.
- L2 é área de trabalho com buffers/arenas maiores.
- RAM/page layer mede página do sistema e ambiente de execução.
- `64` aparece como estado e cache line; `65536` aparece como buffers diretos e camada L2.

### 5.2 VCPU

`raf_vcpu.h` define:

```c
#define RAF_STATE_DIM 7
#define RAF_PERIOD 42
#define RAF_VCPU 8
```

`raf_vcpu_t` é alinhado a 64 bytes:

```c
typedef struct __attribute__((aligned(64))) {
    raf_state_t state;
    uint32_t id;
    uint32_t target_hz;
    uint32_t actual_hz_q16;
    uint64_t ticks;
    uint64_t last_ns;
    uint64_t jitter_ns;
    uint32_t flags;
    uint32_t crc;
} raf_vcpu_t;
```

Leitura:

- vCPU não é metáfora solta;
- é unidade de estado/ciclo;
- tem estado 7D;
- possui Hz alvo/real;
- possui ticks, jitter e CRC;
- é alinhada a 64 bytes.

---

## 6. Frequência como memória operacional

`raf_clock_t` não é apenas temporizador. Ele registra:

```text
target_hz
period_ns
last_tick_ns
actual_delta_ns
jitter_ns
missed_ticks
total_ticks
```

O sistema calcula:

- período esperado por Hz;
- delta real;
- jitter;
- ticks perdidos;
- frequência real em Q16.

Leitura correta:

> Hz vira estado operacional. A frequência serve como memória compacta de comportamento temporal, não apenas como relógio externo.

Fluxo:

```text
target_hz
→ period_ns
→ now_ns
→ actual_delta_ns
→ jitter_ns
→ missed_ticks
→ actual_hz_q16
→ scheduler/vCPU telemetry
```

---

## 7. Matemática: 7D, fase 42, Q16 e geometria

### 7.1 Sete dimensões

O runtime usa `RAF_STATE_DIM=7`. O estado contém:

```c
uint32_t s[7];
uint32_t coherence;
uint32_t entropy;
uint32_t phase;
uint32_t step;
uint32_t crc;
```

### 7.2 Fase 42

`RAF_PERIOD=42`. A fase é atualizada por módulo 42:

```c
st->phase = ((uint32_t)cycle) % RAF_PERIOD;
```

### 7.3 Constantes Q16 / geometria

Em `rafaelia_jni_direct.c`, os seeds incluem:

```c
56755u
46341u
```

Leitura:

- `56755 / 65536 ≈ 0.8660`, representação Q16 de `√3/2`.
- `46341 / 65536 ≈ 0.7071`, representação Q16 de `√2/2`.

Isso sustenta a leitura de geometria convertida para inteiro/fase:

```text
geometria → constante irracional → Q16 inteiro → estado/cache/ciclo
```

### 7.4 Sobre as sete senoides

Não foi encontrada chamada explícita `sin()`/`sinf()` no hot path principal. O que aparece é uma implementação equivalente/alternativa em baixo nível:

- sete dimensões de estado;
- seeds geométricos Q16;
- fase circular;
- período 42;
- acoplamento toroidal;
- atualização por multiplicação/shift/máscara.

Status:

```text
sete senoides explícitas por função trigonométrica: NÃO_ENCONTRADO
sete fases/senoides discretizadas por Q16/estado/fase: IMPLEMENTADO_COM_NOME_DIFERENTE
```

Falta documentar formalmente essa equivalência.

---

## 8. Ruído, latência, inferência e commit gate

### 8.1 Ruído como dado, não como erro descartado

`rafaelia_bitraf_core.c` usa:

- `noise`;
- `noise_acc`;
- `attract_acc`;
- `crc16`;
- pontos append-only;
- áreas e matriz de transição.

Leitura:

> O ruído é acumulado e rastreado. Ele não é apenas exceção; vira telemetria de estado.

### 8.2 Commit gate

`rafaelia_commit_gate_ll.c` usa:

- coerência `c`;
- entropia `h`;
- `p = (1 - h) * c` em Q16;
- mix/hash;
- CRC32C;
- validação por máscaras branchless.

Leitura:

> O commit gate filtra atualização de estado antes de aceitar resultado. Isso é uma forma de reduzir inferência espúria, ruído e estado inconsistente.

### 8.3 Scheduler por custo

`rafaelia_gpu_orchestrator.c` usa função de custo com:

```text
latency
load
intensity
thermal_penalty
gpu_bias
```

A escolha de core usa erro entre `task_hz_q16` e frequência de core:

```text
freq_error = abs(task_hz_q16 - core_freq_q16)
cost = latency + load + intensity + thermal - gpu_bias
```

Leitura:

> O sistema tenta servir a tarefa pelo core/caminho com menor custo operacional, considerando frequência, carga, intensidade, térmica e possibilidade de GPU.

---

## 9. Low-level real: atomics, barriers, ASM e ABI flags

### 9.1 Barreiras de memória

O orquestrador usa:

```text
ARM64: dmb ish
ARM32: dmb
x86/x86_64: mfence
fallback: __sync_synchronize()
```

Leitura:

> Há controle explícito de ordem de memória. Isso não é Java app comum.

### 9.2 Fila atômica

A fila do orquestrador usa:

- `atomic_uint head`;
- `atomic_uint tail`;
- `memory_order_acquire`;
- `memory_order_release`;
- ring buffer de tarefas.

### 9.3 Flags por ABI

O build nativo aplica flags como:

- ARM32: `-march=armv7-a`, `-mfpu=neon`, `-mfloat-abi=softfp`, `-ftree-vectorize`;
- ARM64: `-march=armv8-a`, `-ftree-vectorize`;
- x86/x86_64: SSE/SSE4.2/AVX quando aplicável;
- linker com 16KB page alignment.

Leitura:

> Java/Android serve como embalagem; o núcleo busca hardware via C/ASM/JNI/flags.

---

## 10. BitRAF e append-only

`raf_bitraf.c` define uma instrução de 42 bits:

```text
opcode: 6 bits
dir:    3 bits
layer:  10 bits
imm:    12 bits
flags:  11 bits
```

Total: 42 bits.

`rafaelia_bitraf_core.c` implementa:

- `RAF_D/I/P/R`;
- `slot10`;
- `base20`;
- paridade dupla;
- ECC;
- noise;
- CRC16;
- ring de pontos append-only;
- atrator 42;
- áreas 8x8;
- manifesto determinístico.

Leitura:

> BitRAF não deve ser documentado apenas como “formato”. Ele é um barramento de instrução/estado append-only, com ruído, ECC, paridade, transição e telemetria.

---

## 11. Comparação correta com Termux oficial

### 11.1 Onde o Termux oficial ainda vence

| Área | Motivo |
|---|---|
| maturidade | comunidade, uso real, tempo de produção |
| estabilidade provada | muitos dispositivos e anos de uso |
| ecossistema/plugin | compatibilidade oficial com apps Termux |
| terminal clássico | baseline consolidado |
| bootstrap atual | upstream pode estar mais recente |

### 11.2 Onde RafCodePhi já muda a máquina

| Área | Motivo |
|---|---|
| package próprio | instalação lado a lado |
| terminal cleanup | kill de grupo, cleanup defensivo, waitpid robusto |
| build matrix | artifact por ABI, hash, debug/release |
| RMR/RAFAELIA | módulos extras reais |
| Java low-level | DirectByteBuffer e JNI fino |
| cache runtime | L1/L2/arena/page/cache_line |
| Hz como estado | clock Q16, jitter, missed ticks |
| vCPU | estado 7D, fase 42, CRC |
| scheduler | custo por frequência/carga/térmica/intensidade |
| ASM/barriers | dmb/mfence, NEON/SIMD/flags |

### 11.3 Claim correto

Usar:

> RafCodePhi é uma variante low-level experimental do Termux, com runtime RAFAELIA/RMR orientado por buffer/cache/frequência, build matrix reproduzível, cleanup de terminal reforçado e núcleo nativo para execução cache-oriented.

Não usar ainda:

> RafCodePhi é globalmente mais rápido que Termux oficial.

Esse claim precisa benchmark runtime em aparelho real.

---

## 12. O que está faltando agora

### 12.1 Documentação faltante

Criar ou completar:

1. `docs/RAFCODEPHI_MATH_RUNTIME_AUDIT.md`
   - explicar estado 7D, fase 42, Q16, √3/2, vCPU, scheduler, commit gate.

2. `docs/RAFCODEPHI_JAVA_LOWLEVEL_AUDIT.md`
   - separar Java casca low-level de Java normal/reporting path.

3. `docs/RAFCODEPHI_CACHE_CLOCK_ARCHITECTURE.md`
   - explicar L1/L2/RAM, buffers 64KB, arena, cache line, page size, Hz como memória.

4. `docs/RAFCODEPHI_SENOIDS_TRIANGLE_Q16.md`
   - formalizar sete senoides/fases discretas, triângulo isósceles, √3/2, 56755 Q16.

5. `docs/BETA_RUNTIME_TEST_PLAN.md`
   - instalar APK, abrir terminal, rodar comandos, fechar sessão, checar zumbi/logcat.

6. `docs/RAFCODEPHI_VS_TERMUX_BENCHMARK_PLAN.md`
   - comparação justa com Termux oficial.

7. `docs/BETA_RELEASE_NOTES.md`
   - release notes humana para Beta 0.1.

8. `docs/ROADMAP.md`
   - roadmap por fases.

### 12.2 Implementação/teste faltante

1. Benchmark ADB real de terminal.
2. Benchmark nativo de `processNative`.
3. Benchmark nativo de `stepNative`.
4. Benchmark de `stepAllVcpusNative`.
5. Benchmark de `scheduler_pick_core`.
6. Benchmark de `commit_gate`.
7. Benchmark de `crc32Native`.
8. Medição de `arena_used` antes/depois de 10.000 chamadas.
9. Medição de GC count antes/depois.
10. Medição de jitter em 10/60/120/1000Hz.
11. Teste de 100 sessões terminal: abre → comando → fecha → checa zumbi.
12. Teste Android 12+ phantom process behavior.
13. Comparação 4KB vs 16KB vs 64KB no `ByteQueue` do terminal.
14. Validação de page size 16KB em aparelho real.
15. Separar benchmarks simulados/documentais de benchmarks reproduzidos em CI/aparelho.

---

## 13. Release notes humanas — Beta 0.1 proposta

# Termux RAFCODEΦ Beta 0.1 — Build/Runtime Foundation

## O que esta beta entrega

- App Termux com identidade própria `com.termux.rafacodephi`.
- Instalação lado a lado com Termux oficial.
- Build matrix por ABI: `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`, `universal`.
- APKs debug/release e signed/unsigned internos.
- SHA256 de artifact e APKs.
- Compatibilidade nativa preparada para page size 16KB.
- Módulos extras `rmr` e `rafaelia` compilando.
- Núcleo RAFAELIA direto com `DirectByteBuffer`, arena nativa, vCPU, clock e CRC.
- Cleanup de terminal reforçado em relação ao caminho tradicional.
- Base para benchmarks nativos de baixo nível.

## O que esta beta ainda não promete

- Não promete ser globalmente mais rápida que o Termux oficial.
- Não promete ausência total de zumbis sem teste em aparelho.
- Não promete benchmark final sem relatório reproduzível.
- Não promete que todo o terminal usa o caminho RAFAELIA direto.
- Não promete que todos os claims matemáticos estejam formalizados.

## Como testar manualmente

1. Baixar o APK `arm64-v8a-signed` ou `universal-signed` do artifact.
2. Instalar via ADB.
3. Abrir o app.
4. Abrir uma sessão terminal.
5. Executar:

```bash
echo rafcodephi_beta_ok
pwd
id
uname -a
exit 0
```

6. Abrir/fechar 20 sessões.
7. Abrir/fechar 100 sessões.
8. Coletar `logcat`.
9. Verificar processos órfãos/zumbis.
10. Registrar Android version, ABI, page size e modelo do aparelho.

## Status de release

```text
CI_BETA_BUILD_READY: sim
BETA_RUNTIME_READY: pendente de aparelho real
PUBLIC_RELEASE_READY: não
```

---

## 14. Roadmap

### Fase 0 — Documentação de leitura correta

Objetivo: impedir leitura errada do projeto.

Entregas:

- Este arquivo `MAYA_RAFCODEPHI_LEITURA_CORRETA.md`.
- Documento de matemática runtime.
- Documento Java low-level.
- Documento cache/clock.
- Documento senoides/triângulo/Q16.

Status: em andamento.

### Fase 1 — Beta build fechada

Objetivo: manter CI gerando artifact por ABI.

Entregas:

- Build matrix funcionando.
- SHA256 em artifact.
- APK signed/unsigned interno.
- Remover keystore local do artifact final, se aplicável.
- Corrigir erro não bloqueante no summary do GitHub Actions.

Status: build já passou; higiene de artifact ainda recomendada.

### Fase 2 — Beta runtime mínima

Objetivo: provar que o app funciona em aparelho real.

Entregas:

- App abre.
- Terminal abre.
- Comando simples executa.
- Terminal fecha.
- Sem crash crítico.
- Sem processo zumbi crítico observado.
- Logcat coletado.
- Relatório runtime salvo.

Status: pendente.

### Fase 3 — Benchmark RAFAELIA direto

Objetivo: provar o núcleo cache-oriented.

Entregas:

- `processNative` 1KB/4KB/64KB.
- `stepNative` 10.000 ciclos.
- `stepAllVcpusNative` 10.000 ciclos.
- `crc32Native` 64KB.
- `commit_gate` 100.000 iterações.
- `scheduler_pick_core` 100.000 iterações.
- `arena_used` antes/depois.
- GC count antes/depois.
- jitter em 10/60/120/1000Hz.

Status: pendente.

### Fase 4 — Comparação justa com Termux oficial

Objetivo: comparar caminhos equivalentes.

Regras:

- mesmo aparelho;
- mesma bateria/modo térmico;
- mesmo Android;
- Termux oficial e RafCodePhi instalados lado a lado;
- separar terminal clássico de RAFAELIA direto;
- não comparar microbenchmark nativo com uso geral do terminal.

Status: pendente.

### Fase 5 — Integração maior do RAFAELIA hot path

Objetivo: decidir se RAFAELIA direto deve alimentar partes reais do terminal ou permanecer como runtime paralelo.

Entregas:

- mapa de integração;
- riscos de regressão;
- flag para ativar/desativar;
- rollback seguro;
- benchmark antes/depois.

Status: futuro.

---

## 15. Próximos passos objetivos

### Próximo 1 — Documento matemático formal

Criar `docs/RAFCODEPHI_MATH_RUNTIME_AUDIT.md` com:

- 7D state;
- fase 42;
- Q16;
- `56755 = √3/2 em Q16`;
- `46341 = √2/2 em Q16`;
- Hz como memória;
- commit gate;
- scheduler por custo;
- ruído/latência/térmica;
- GP/correlation dimension;
- BitRAF append-only.

### Próximo 2 — Benchmark real

Criar scripts:

```text
scripts/benchmark_rafaelia_direct_adb.sh
scripts/beta_runtime_smoke_adb.sh
scripts/beta_process_cleanup_probe.sh
```

### Próximo 3 — Corrigir documentação antiga

A documentação atual contém bons blocos, mas mistura:

- claims já provados;
- claims plausíveis;
- claims de benchmark ainda sem artifact runtime;
- analogias matemáticas;
- implementação real.

A correção é adicionar status em cada claim:

```text
PROVADO_NO_CÓDIGO
PROVADO_NO_BUILD
PROVADO_NO_CI
PROVADO_NO_RUNTIME
BENCHMARKADO
PRECISA_BENCHMARK
DOCUMENTADO_NÃO_PROVADO
```

---

## 16. Prompt curto para Maya/Codex usar este arquivo

```text
Leia docs/MAYA_RAFCODEPHI_LEITURA_CORRETA.md antes de qualquer inferência.

Não trate RafCodePhi como apenas Termux + C/ASM.
Separe dois caminhos:
1. Terminal Runtime Path.
2. RAFAELIA Math/Cache Runtime Path.

Audite claims por evidência:
- código;
- build;
- CI;
- runtime;
- benchmark.

Não aceite claim de performance global sem benchmark reproduzível.
Não ignore que o núcleo RAFAELIA usa DirectByteBuffer, arena nativa, vCPU, clock Hz/Q16, estado 7D, fase 42, scheduler por custo, commit gate, CRC/ECC/paridade e barreiras de memória.

Próxima tarefa: gerar RAFCODEPHI_MATH_RUNTIME_AUDIT.md e benchmarks ADB/runtime.
```

---

## 17. Conclusão

O RafCodePhi deve ser lido como uma arquitetura em transição:

```text
Termux compatível
+ identidade side-by-side
+ terminal cleanup reforçado
+ Java como casca low-level
+ buffers diretos fixos
+ runtime nativo RAFAELIA/RMR
+ vCPU/cache/frequência/ruído/custo
+ build matrix reproduzível
```

A beta correta não deve tentar vender tudo como pronto. Deve afirmar com precisão:

- build está forte;
- runtime terminal ainda precisa teste em aparelho;
- núcleo RAFAELIA está implementado no código;
- benchmark ainda precisa transformar potencial em número;
- documentação precisa separar prova, hipótese e roadmap.

**Leitura final:**

> O projeto não é lógica antiga de app. Também ainda não é prova final de performance global. Ele é um runtime experimental de baixo nível acoplado ao Termux, com base real em cache, buffer, vCPU, frequência e commit de estado. A próxima entrega é provar isso em runtime e benchmark reproduzível.
