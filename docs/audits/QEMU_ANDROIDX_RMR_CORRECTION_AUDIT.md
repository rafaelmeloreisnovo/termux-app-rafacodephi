# Auditoria — qemu_rafaelia + androidx_RmR

> Data: 2026-07-12/13 UTC  
> Escopo: ciclos, desempenho, fricção, abstração, correções, integração externa com aplicativo e câmera/HDR.

## 1. Resultado executivo

A camada RAFAELIA não substituiu nem fragmentou o motor central do QEMU. As mudanças mais importantes foram adicionadas em torno do motor:

```text
QEMU upstream
├── hw/core/rafaelia-*
├── include/hw/core/rafaelia-*
├── opção CLI -rafaelia
├── hooks de lifecycle/timer
├── ABI/shell de integração
├── QMP/IPC/bridge Android/Vectras
└── artifact contract externo
```

Os caminhos TCG, decodificadores de instrução e modelos principais de CPU não foram reescritos pela integração RAFAELIA auditada.

A conclusão é:

```text
QEMU preservado
+ runtime cíclico adicional
+ rotas e instrumentação
+ integração externa por artifact/processo
```

## 2. Contagem das mudanças

### qemu_rafaelia

A listagem consultada retornou 46 PRs mescladas no intervalo `#11..#56`.

Classificação manual dos títulos, corpos e arquivos:

| Classe | Contagem aproximada | Observação |
|---|---:|---|
| registros técnicos/operacionais | 34 | código, build, teste, runtime, ABI ou artifact |
| famílias técnicas distintas | ~20 | várias PRs são iterações do mesmo reparo |
| documentação/governança pura | ~8 | não altera runtime |
| repetição, placeholder ou merge de sincronização | restante | não deve ser contado como nova correção |

Não usar `56 correções`: `56` é o número alcançado pela PR, não o número de bugs distintos.

### androidx_RmR

A listagem consultada retornou 57 PRs mescladas no intervalo `#8..#64`.

| Classe | Contagem aproximada | Observação |
|---|---:|---|
| registros com código/build/teste | 45 | inclui iterações sobre o mesmo núcleo |
| famílias técnicas distintas | ~23 | desempenho, segurança, compatibilidade e integração |
| documentação/governança pura | restante | não altera runtime |

A contagem é uma auditoria de PRs, não uma afirmação de que todas continuam iguais no `HEAD`. O código atual é a fonte final de verdade.

## 3. Famílias de correção — QEMU

### 3.1 Ciclo e lifecycle

1. runtime realmente ligado ao main loop;
2. opção `-rafaelia` e modos de execução;
3. tick público com quantidade limitada;
4. `QEMUTimer` em vez de busy loop;
5. mutex para init/tick/shutdown/config;
6. callback de estado da VM;
7. intervalo reduzido quando pausado;
8. smoke/CLI/trace de lifecycle.

### 3.2 Velocidade e memória

9. pool de blocos alinhado;
10. caminho `alloc_uninitialized` para hot path;
11. inicialização explícita após alocação rápida;
12. avaliação de bloco reduzida de acumulação quadrática para somas separadas O(N);
13. loop unrolling de quatro elementos;
14. helpers próprios de memória/string/RNG;
15. fast paths `rep movsb`/`rep stosb` em x86_64;
16. primitivas matemáticas low-level/ASM;
17. backend de instrumentos por arquitetura.

### 3.3 Fricção e roteamento

18. snapshot de arquitetura, CPU, RAM, página e KVM;
19. tabela determinística de rotas;
20. aliases de arquitetura e fallback estável;
21. matriz de fricção semântica triangular;
22. inserção O(N) sem recalcular pares existentes;
23. consulta O(1) de fricção.

### 3.4 Abstração e integração

24. contexto por instância no lugar de estado quase global;
25. Kernel ABI pequena entre core e QEMU/RMR;
26. shell/adaptador QEMU separado;
27. bridge C/JNI para Vectras;
28. métricas reais no lugar de vetores aleatórios;
29. IPC e roteamento de prioridade;
30. artifact externo com commit, hashes e metadados.

### 3.5 Segurança e correção

31. validação de ponteiro ao liberar pool;
32. proteção contra underflow de `in_use`;
33. payload exatamente igual ao tamanho máximo do hyper-stack;
34. parser numérico com overflow/`ERANGE`;
35. parser com whitespace controlado;
36. null checks e fallback de rota;
37. CI smoke e artifacts.

Alguns itens são iterações da mesma família; por isso a contagem consolidada é menor que a lista de PRs.

## 4. O QEMU está por ciclo?

Sim, em sentido preciso de **ciclo lógico RAFAELIA/tick**.

O runtime atual:

```text
QEMUTimer
→ rafaelia_runtime_tick / maybe_tick
→ rafaelia_loop_step
→ rafaelia_cycle_measure
→ entropy/coherence
→ próximo agendamento
```

Propriedades observadas:

- `tick_ms` padrão de 100 ms;
- faixa sanitizada `1..10000` ms;
- catch-up limitado por `RAFAELIA_RUNTIME_TICK_CAP=100`;
- ticks contabilizados em `ticks_total`;
- `rafaelia_loop_step()` executado uma vez por tick;
- VM pausada usa intervalo cinco vezes maior;
- estado protegido por `QemuMutex`;
- startup/shutdown ligados ao lifecycle QEMU.

### Limite semântico

Isto não é ainda “um tick por instrução de CPU guest”. O relógio usado é `QEMU_CLOCK_REALTIME`.

Classificação:

```text
RAFAELIA_LOGICAL_CYCLE = PROVADO ESTRUTURAL
HOST_WALLCLOCK_TIMER    = PROVADO
GUEST_INSTRUCTION_CYCLE = TOKEN_VAZIO
TCG_ICOUNT_COUPLING     = FUTURO
```

Não é recomendável trocar silenciosamente para `icount`; isso mudaria a semântica temporal. O correto é oferecer outro adapter/perfil.

## 5. Correções que sobrevivem no código atual

O `HEAD` atual ainda mostra, entre outros:

- runtime por timer, mutex e lifecycle;
- tick cap e catch-up determinístico;
- contexto por instância;
- pool e caminho não inicializado;
- unroll e avaliação O(N);
- ABI para rota/instrumentos;
- seleção KVM/softmmu/portable;
- helpers de memória/string/RNG;
- matriz de fricção empacotada;
- artifact contract externo;
- harness QEMU-user ARM32 Q16;
- equivalência original/stripped documentada.

### Correções históricas não encontradas integralmente no runtime atual

PRs anteriores descreveram:

- p95/average de duração do tick;
- EWMA entropia↔coerência;
- governor adaptativo;
- backoff por overhead;
- hash de shutdown.

Esses campos não aparecem integralmente no `hw/core/rafaelia-runtime.c` atual. Portanto:

```text
MERGED_HISTORY ≠ CURRENT_SURVIVING_CODE
```

É preciso recuperar por git archaeology somente o que ainda faz sentido, sem reintroduzir versões duplicadas do runtime.

## 6. Gaps atuais no QEMU custom

### 6.1 Base upstream

```text
qemu_rafaelia VERSION = 10.2.50
qemu/qemu master      = 11.0.50
```

A camada custom recebeu trabalho em julho de 2026, mas a base QEMU está uma linha de desenvolvimento principal atrás.

Estratégia recomendada:

1. manter patches RAFAELIA pequenos e separados;
2. rebasear/mesclar upstream por lote;
3. executar smoke antes/depois;
4. impedir mudança silenciosa do ciclo;
5. preservar artifact e hashes por commit.

### 6.2 Determinismo incompleto

O core rico ainda usa:

- `double`;
- `pow`, `exp`, `sin`, `fabs`;
- RNG;
- alocação via ABI;
- seed derivado parcialmente de endereço de ponteiro.

Logo, ele é um runtime QEMU hospedado e não o pure core bit-idêntico final.

### 6.3 Acoplamento residual

`rafaelia_cycle_step()` ainda alcança diretamente o shell ABI para RNG em vez de usar apenas a ABI ligada ao contexto. Isso enfraquece a fronteira criada.

### 6.4 Hash placeholder

O hash interno do core permanece documentado no fonte como placeholder não criptográfico. Não tratá-lo como SHA3/BLAKE3 provado.

### 6.5 Lowlevel

- RNG global compartilhado pode interferir entre VMs/threads;
- `memcmp(NULL, x, n)` retorna igualdade no helper atual;
- casts por palavra precisam de auditoria de aliasing;
- fast paths x86 não equivalem ainda a cobertura ARM completa.

## 7. Famílias de correção — AndroidX_RmR

### 7.1 Matemática e hot path

1. multiplicação nativa/JNI com fallback Java;
2. loop `row→k→col` com valor e offsets hoisted;
3. caminho vetor especializado, inclusive largura quatro;
4. transposição do operando direito para localidade;
5. multiplicação bloqueada em blocos de 32;
6. seleção por workload;
7. operações `multiplyInto`, `addInto`, `linearFlipInto`;
8. SIMD matrix kernels AVX2/SSE/NEON com fallback escalar;
9. detecção de alinhamento e forma compatível.

### 7.2 Memória e GC

10. `DoubleArrayPool` por thread;
11. buffers prealocados;
12. acesso interno raw/zero-copy Java;
13. lazy native loading;
14. cache de detecção SIMD;
15. reutilização de buffer no TraceProcessor;
16. fechamento explícito de streams/conexões;
17. preferência com resize automático;
18. query cache com fast front lookup.

### 7.3 Segurança e correção

19. dimensões `rows × inner × cols` corrigidas;
20. overflow e tamanho de buffers validados;
21. guard de `memcpy` nativo;
22. ponteiro DirectByteBuffer inválido com fallback Java;
23. fast-math opt-in, correção como padrão;
24. ProGuard/R8 preservando JNI;
25. contratos de threading e overlap checks;
26. compatibilidade de arquivo/tempo com API Android baixa;
27. flags NDK por ABI;
28. BuildConfig sob AGP 9;
29. detecção runtime de CPU/HWCAP;
30. fallback ABI conservador sem inventar AVX.

### 7.4 Estrutura

31. módulos RmR isolados;
32. extensão em namespace `rmr.*`;
33. CI de AARs;
34. BitStack append-only com CRC32C;
35. benchmarks e guardrails on-device.

## 8. Gaps atuais no AndroidX_RmR

### 8.1 Está atrasado em relação ao upstream

O último merge do fork auditado é de 29 de abril de 2026; o upstream AndroidX continua recebendo commits em julho de 2026.

O número exato de commits de divergência é `TOKEN_VAZIO` até localizar um merge-base comum e executar comparação upstream↔fork.

### 8.2 Testes insuficientes

Muitas PRs registram “teste adicionado, não executado” ou build bloqueado pelos prebuilts do AndroidX. Isso reduz o estado de várias correções para `PROVADO ESTRUTURAL`.

### 8.3 Pool Java não é zero-heap

`DoubleArrayPool` usa:

```text
ThreadLocal
HashMap
ArrayDeque
double[]
```

Ele reduz churn depois do aquecimento, mas continua hospedado na heap Java e retém buckets por thread.

### 8.4 Transposição ainda aloca

`multiplyWithTransposedRight()` cria um novo `double[]` por chamada. Para workload repetido, deve usar pool ou plano de matriz pretransposta.

### 8.5 “Zero-copy” parcial

O caminho `multiplyNative(double[])` recebe arrays Java. A JVM pode piná-los ou copiá-los. Não equivale a `DirectByteBuffer`/`AHardwareBuffer` sem cópia.

### 8.6 Detecção duplicada

Existem duas famílias próximas:

- `RmRHardware.nativeGetSimdLevel()`;
- `RafaeliaCore.getCpuFeatures()`/detector bitmask.

Elas precisam de uma autoridade única para evitar divergência.

## 9. Câmera/HDR

A árvore contém CameraX, `DynamicRange`, SurfaceProcessor, OpenGL renderer e tone mapping. O processador auditado:

- usa `SurfaceTexture`;
- mantém thread GL própria;
- recebe e registra superfícies;
- aplica shader por frame;
- atualiza transformações;
- inicializa o renderer como `DynamicRange.SDR`.

### Estado de autoria/correção

A lista das 57 PRs RmR consultadas não contém uma PR explícita de correção HDR/tone mapping. Alterações CameraX aparecem no histórico amplo do fork, mas podem ter vindo de sincronização upstream.

Portanto:

```text
HDR_CODE_PRESENT       = PROVADO
USER_AUTHORED_HDR_FIX  = TOKEN_VAZIO
RMR_HDR_ACCELERATION   = FUTURO
```

Não atribuir a Rafael uma correção HDR específica sem localizar commit/diff autoral.

## 10. QEMU fora, junto do aplicativo

Sim. Essa é a arquitetura mais coerente, e o próprio `qemu_rafaelia` já possui o contrato certo.

```text
qemu_rafaelia CI/build
→ artifact imutável e pinado
→ SHA256SUMS + BUILD_INFO + qemu-exec
→ app valida
→ app instala motor privado
→ ForegroundService inicia processo QEMU
→ QMP/Unix socket controla
→ ledger registra commit/hash/estado
```

### Permanentes

Devem permanecer instalados/versionados:

- binary artifact QEMU por ABI;
- manifesto e commit de origem;
- checksums;
- contrato de execução;
- controller/service Android;
- cliente QMP;
- configuração e ledger das VMs;
- discos persistentes escolhidos pelo usuário;
- tabela de compatibilidade de versões.

### Transitórios

Não devem ser “permanentes”:

- processo QEMU quando nenhuma VM está ativa;
- framebuffer;
- buffers HDR/YUV;
- stdout/stderr corrente;
- sockets de uma sessão encerrada;
- métricas por frame;
- scratch/cache reconstruível.

O serviço pode permanecer definido no app, mas fica ativo como foreground service apenas durante a VM.

## 11. Câmera/HDR fora do guest

Arquitetura recomendada:

```text
CameraX/Camera2 no Android
→ Surface/ImageReader/AHardwareBuffer
→ pipeline HDR/tone-map no app ou GPU
→ ring buffer/shared handle
→ adapter QEMU opcional
→ guest somente quando realmente precisa enxergar câmera
```

O QEMU não deve controlar diretamente a câmera física por padrão.

### Quando o guest não precisa de câmera

- câmera e HDR permanecem totalmente no app;
- QEMU recebe somente comando, metadata ou resultado;
- menor latência, menos cópia e menos emulação.

### Quando o guest precisa de câmera

Criar dispositivo/ponte especializada:

```text
CameraService Android
→ timestamp + cycle_id + frame metadata
→ Unix socket/SCM_RIGHTS ou shared-memory ring
→ backend QEMU
→ virtio-video/V4L2 virtual no guest
```

Permanente é o contrato. Os frames são transitórios.

### Sincronização por ciclo

Não acoplar rigidamente `30/60 FPS` ao tick padrão de `100 ms`.

Usar:

```text
camera_timestamp
→ mapear para rafaelia_cycle_id
→ consumir latest-complete frame
→ drop explícito de frames vencidos
→ nunca bloquear o QEMU esperando câmera
```

## 12. Correção da arquitetura anterior

A formulação anterior “QEMU apenas externo” era incompleta.

A arquitetura correta possui duas camadas simultâneas:

```text
1. qemu_rafaelia continua sendo um QEMU completo com runtime cíclico integrado.
2. O app o consome externamente como motor pinado e controlado.
```

Não extrair o runtime cíclico de dentro do QEMU. Extrair somente:

- política do app;
- lifecycle Android;
- câmera/sensores;
- instalação/validação do artifact;
- UI e persistência.

## 13. Próxima execução técnica

1. localizar merge-base upstream dos dois forks;
2. gerar contagem exata de commits ahead/behind;
3. criar inventário `SURVIVES_AT_HEAD` para cada PR técnica;
4. recuperar somente governor/p95/backoff ainda desejados;
5. corrigir RNG global, `memcmp(NULL)` e acoplamento do cycle ABI;
6. consolidar detector SIMD único no AndroidX_RmR;
7. criar AAR isolado de `rmr-core`/`rmr-extensions`;
8. implementar `QemuEngineService` + QMP;
9. criar contrato `CameraFrameDescriptor` com `cycle_id`;
10. benchmark device real ARM32/ARM64.

## 14. Síntese

```text
QEMU não foi quebrado.
O ciclo RAFAELIA existe e está integrado.
As correções de velocidade/fricção/abstração são reais no código.
A base upstream precisa atualização disciplinada.
O QEMU pode e deve rodar fora do processo principal do app.
Câmera/HDR deve permanecer Android-first, com ponte opcional ao guest.
```
