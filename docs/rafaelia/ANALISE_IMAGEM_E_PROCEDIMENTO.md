# Análise dos textos da imagem e procedimento adequado

## Escopo
Este documento transforma os textos das capturas em ações objetivas de engenharia de baixo nível (C/ARM), com foco em:
- sem abstrações desnecessárias,
- sem garbage collector,
- mínimo overhead,
- comandos básicos reproduzíveis.

## 1) Textos identificados na imagem (resumo objetivo)

### Bloco A — Diagnóstico técnico
1. `malloc` é usado extensivamente em `baremetal.c` (`mx_create`, `arena_create`).
2. `pthread_once` é usado para inicialização de capacidades.
3. Existe ASM NEON para ARM32/ARM64 (dot, vadd, memcpy).
4. `hw_profile_t` lê de `/proc/self/auxv` e `/sys/devices/system/cpu/`.
5. Não há suporte GPU ativo no módulo atual; caminho sugerido: OpenCL/Vulkan via `dlopen`.

### Bloco B — Perfil de hardware alvo da captura
1. 8x Cortex-A53 (sem big.LITTLE heterogêneo).
2. GPU PowerVR GE8320.
3. Sem CRC32 por instrução no modo ARM32.
4. NEON disponível.
5. OpenCL 2.0 disponível (PowerVR).

### Bloco C — Arquivo proposto na captura
Proposta de criar `rafaelia_gpu_orchestrator.c` com:
- probe de GPU via `dlopen(libOpenCL.so)`;
- mapeamento de 8 vCPUs em zonas geométricas (toroidais);
- escalonamento harmônico por frequência (Hz);
- hierarquia de memória L1/L2/buffer/RAM/storage;
- integridade CRC32 em software;
- evitar `malloc` (arena estática).

## 2) Verificação no código atual (repositório)

A verificação do estado atual confirma o diagnóstico da imagem:
- O módulo low-level existe em `app/src/main/cpp/lowlevel/`.
- `baremetal.c` contém `malloc` em rotinas de criação de arena/matriz.
- `pthread_once` é usado para detectar caps de arquitetura.
- Há kernels ASM em `baremetal_asm.S`.
- O build NDK já inclui assembly condicional por ABI em `Android.mk`.

## 3) Procedimento adequado (passo a passo operacional)

## 3.1 Fase 0 — Requisitos de implementação
- Linguagem: C11.
- Dependências: libc, `libdl`, `libm`, `liblog`.
- Política de memória: buffers estáticos fixos e arenas pré-alocadas.
- Compatibilidade: ARM32/ARM64 + fallback seguro.

## 3.2 Fase 1 — Estrutura mínima do orquestrador
Criar novo arquivo:
- `app/src/main/cpp/lowlevel/rafaelia_gpu_orchestrator.c`

Interface mínima:
- `int rgpu_probe_opencl(void);`
- `int rgpu_probe_vulkan(void);`
- `void rcpu_map_toroidal(uint32_t cpu_count, uint32_t* zones, uint32_t zone_cap);`
- `uint32_t rcrc32_sw(const uint8_t* data, uint32_t len);`
- `uint32_t rscheduler_pick_core(uint32_t task_hz_q16);`

## 3.3 Fase 2 — Probe GPU via `dlopen`
Fluxo:
1. `dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL)`.
2. Se falhar, tentar `libOpenCL.so.1`.
3. Resolver apenas símbolos mínimos com `dlsym`.
4. Fechar handle no shutdown.

Regras:
- não usar wrappers C++;
- sem alocação dinâmica para metadata de probe;
- retornar códigos inteiros estáveis (0 ok, negativo erro).

## 3.4 Fase 3 — Mapeamento toroidal de CPUs
Modelo enxuto:
- para 8 cores, usar grade `R x C` com `R*C >= 8`;
- índice `i` mapeado para coordenadas `(r=i/C, c=i%C)`;
- rotação cíclica de zona com passo coprimo ao perímetro para cobertura uniforme.

Condição de cobertura:
- `gcd(Δr, R)=1` e `gcd(Δc, C)=1`.

## 3.5 Fase 4 — Escalonamento por harmônicos (Hz)
Definir frequência base por core em Q16.16:
- `f_core[i] = f0 * ratio[i]`.

Seleção de core:
- calcular erro absoluto `|task_hz - f_core[i]|`;
- escolher menor erro (argmin);
- empate: menor carga acumulada.

## 3.6 Fase 5 — Integridade sem CRC32 HW
Implementar CRC32 software tabelado (256 entradas) com:
- init lazy por `pthread_once`;
- tabela estática;
- sem heap.

## 3.7 Fase 6 — Integração no build
Atualizar `app/src/main/cpp/Android.mk`:
- incluir `lowlevel/rafaelia_gpu_orchestrator.c` em `LOCAL_SRC_FILES` da lib `termux-baremetal`;
- garantir `LOCAL_LDLIBS += -ldl`.

## 3.8 Fase 7 — Teste mínimo de bancada
Comandos básicos sugeridos:
1. Build NDK.
2. Teste unitário de CRC32 (`123456789` => `0xCBF43926`).
3. Teste de probe OpenCL com retorno determinístico.
4. Teste de distribuição de 8 cores (sem colisão inválida de zona).

## 4) Relação com os blocos matemáticos enviados
Os 50 itens matemáticos podem ser usados como especificação conceitual do orquestrador:
- `T^7` e `s in [0,1)^7`: estado normalizado de agendamento.
- EMA de `C` e `H` (`alpha=0.25`): suavização de carga/entropia.
- `x_{n+42}=x_n`: janela cíclica de planejamento.
- `gcd` em grade: percurso toroidal sem repetição curta.
- `bits_geom = log2(M*N)`: limite de codificação de zonas.

Na prática, isso deve virar constantes e funções C simples, não uma camada abstrata pesada.

## 5) Critérios de aceitação (objetivos)
- Sem uso novo de `malloc/free` no orquestrador.
- Probe OpenCL via `dlopen` funcional com fallback limpo.
- Scheduler retorna core válido em O(N) com N pequeno (8).
- CRC32 software validado por vetor conhecido.
- Build do módulo nativo sem warnings adicionais.

## 6) Comandos básicos (LOW BASIC COMMANDS)
```bash
# localizar pontos atuais
rg -n "malloc|pthread_once|dlopen|OpenCL|CRC|NEON" app/src/main/cpp/lowlevel

# editar arquivos-alvo
$EDITOR app/src/main/cpp/lowlevel/rafaelia_gpu_orchestrator.c
$EDITOR app/src/main/cpp/Android.mk

# build (exemplo)
./gradlew :app:assembleDebug

# inspeção de símbolos
nm -D app/build/intermediates/**/libtermux-baremetal.so | rg "rgpu_|rcrc32"
```

---

Este é o procedimento adequado para converter o conteúdo textual da imagem em implementação real, de baixo nível e com mínimo overhead.
