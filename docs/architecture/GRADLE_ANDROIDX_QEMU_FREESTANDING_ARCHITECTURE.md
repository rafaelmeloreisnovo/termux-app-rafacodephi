# Gradle + AndroidX + QEMU — Arquitetura Freestanding Portável

> Documento canônico de arquitetura e execução para separar o núcleo RAFAELIA portátil das camadas Android, AndroidX, Gradle, JNI, Linux e QEMU.
>
> Baseline auditada: `termux-app-rafacodephi@b47b46564737ec1c19d83569dde7cc5da9d647da`.

## 1. Objetivo

Construir um núcleo computacional que possa executar a mesma transformação bit a bit em:

- Android/Termux;
- teste host;
- QEMU user-mode;
- QEMU system-mode sem sistema operacional;
- firmware ou hardware futuro;
- outro sistema operacional, sem reescrever a matemática.

A invariante é:

```text
mesmo estado inicial
+ mesmos bytes de entrada
+ mesmo perfil de arquitetura
= mesmo estado final e mesma assinatura
```

A portabilidade não será obtida tentando tornar o APK inteiro freestanding. Ela será obtida por uma fronteira rígida:

```text
PURE CORE                    PLATFORM ADAPTER
------------------------     ------------------------------
sem Android                  Android/JNI/AndroidX
sem Linux                    Linux/QEMU-user
sem QEMU                     QEMU-system/placa/MMIO
sem libc                     logging/arquivo/thread/relógio
sem syscall                  instalação/bootstrap/UI
sem heap                     transporte e ciclo de vida
```

## 2. Verdade atual do repositório

### 2.1 Gradle e Android

Arquivos canônicos:

- `settings.gradle`
- `build.gradle`
- `gradle/wrapper/gradle-wrapper.properties`
- `gradle.properties`
- `app/build.gradle`

Estado observado:

```text
Gradle wrapper        = 8.14.3
Android Gradle Plugin = 8.13.2
compileSdk            = 35
targetSdk             = 28
minSdk                = 21
NDK                    = 26.3.11579264
ABIs oficiais          = armeabi-v7a, arm64-v8a
universal APK          = true
Java source/target     = 8
```

Módulos Gradle atuais:

```text
:app
:termux-shared
:terminal-emulator
:terminal-view
:rafaelia
:rmr
```

O Gradle é **fábrica e orquestrador**. Nenhuma propriedade do Gradle existe no binário em execução, exceto valores deliberadamente convertidos em `BuildConfig`, manifesto, recursos ou flags do compilador.

### 2.2 AndroidX usado pelo app

O app consome AndroidX oficial por Maven, entre outros:

```text
androidx.annotation:annotation:1.9.0
androidx.core:core:1.13.1
androidx.drawerlayout:drawerlayout:1.2.0
androidx.preference:preference:1.2.1
androidx.viewpager:viewpager:1.0.0
androidx.appcompat:appcompat:1.6.1
androidx.window:window:1.1.0
androidx.work:work-runtime:2.9.1
```

O repositório externo `rafaelmeloreisnovo/androidx_RmR` **não é dependência real do app hoje**. Ele é um fork grande do AndroidX, usa seu próprio sistema de build e registra AGP `9.0.0-beta05` quando não há override. Não deve ser importado inteiro como `includedBuild` no app.

O fork já contém uma direção útil: extensões RmR isoladas em módulos e namespace próprios. O consumo aceitável é seletivo, versionado e testado como AAR/JAR, nunca por mistura da árvore completa AndroidX com este repositório.

### 2.3 QEMU usado hoje

`qemu_rafaelia` é externo e já possui dois papéis distintos:

1. produtor de artefatos `qemu-system-*` para Vectras;
2. provador QEMU user-mode do contrato ARM32 Q16.

O harness atual `tests/arm32/rafaelia_q16/run_qemu_user.sh` compila com:

```text
-nostdlib
-ffreestanding
-fno-builtin
-fno-stack-protector
-fno-unwind-tables
-fno-asynchronous-unwind-tables
-ffunction-sections
-fdata-sections
-Wl,--gc-sections
-Wl,--build-id=none
-Wl,-static
```

Porém ele usa syscalls Linux ARM EABI. Logo, prova **portabilidade de ISA + ABI Linux**, não ausência de sistema operacional.

## 3. Definições obrigatórias

### 3.1 Freestanding de fonte

Um translation unit é freestanding quando:

- não inclui APIs de Android, JNI, POSIX ou libc;
- não chama `malloc`, `free`, `open`, `read`, `write`, `pthread_*`, `clock_*`, logging ou runtime Java;
- recebe toda memória e configuração pelo chamador ou usa armazenamento estático explicitamente limitado;
- não presume processo, filesystem, usuário, thread, descriptor ou kernel.

### 3.2 Freestanding de artefato

Um ELF final só recebe o selo `FREESTANDING_ARTIFACT_PROVED` quando:

```text
PT_INTERP ausente
PT_DYNAMIC ausente
DT_NEEDED = 0
UND inesperados = 0
relocações dinâmicas = 0
exports = allowlist exata
heap symbols = 0
syscall instructions no core = 0
```

Uma biblioteca `.so` JNI não satisfaz esse contrato, mesmo que parte de seu C seja escrita em estilo freestanding.

### 3.3 Sem função nativa

Neste projeto, “sem função nativa” deve significar:

```text
sem API nativa de plataforma dentro do core
```

Não significa que C não possa ter funções internas. Um núcleo precisa de ao menos um ponto de entrada. Para perfil estrito:

- 1 a 4 entrypoints públicos;
- helpers `static inline` ou macros;
- LTO opcional para eliminar chamadas internas;
- nenhuma chamada indireta;
- nenhuma tabela de ponteiros no hot path;
- nenhuma função de Android/Linux/QEMU.

### 3.4 Sem nome de variável no runtime

C e assembly precisam de identificadores no fonte. O objetivo executável é:

- nomes locais não exportados;
- sem strings de debug;
- sem RTTI/reflection;
- sem `.symtab` no artefato distribuído, quando aplicável;
- exports definidos por allowlist;
- offsets e contratos de registrador formalizados;
- arquivo de símbolos/debug mantido separadamente para auditoria.

Apagar nomes do fonte não melhora o runtime e reduz auditabilidade. O que precisa ser eliminado é o nome residual no artefato final, não a legibilidade da fonte canônica.

### 3.5 Sem branch

“Branchless” só pode ser declarado para uma região delimitada.

Perfis:

```text
BRANCHLESS_KERNEL
- bloco de tamanho fixo
- sem branch dependente de dado
- csel/csinc/máscaras aritméticas
- unroll em tempo de compilação

BOUNDED_LOOP
- loop permitido
- limite superior fixo
- prova de término
- sem alocação

CONTROL_ADAPTER
- branches permitidos
- fora do hot path
- valida entrada, capacidades e erro
```

Não declarar um programa inteiro branchless se ele contém loops, dispatch indireto, `if`, `b`, `bl`, `blr`, `cbz` ou `b.ne`.

## 4. Arquitetura alvo

```text
native/
├── core/
│   ├── include/
│   │   ├── raf_types.h
│   │   ├── raf_core.h
│   │   ├── raf_contract.h
│   │   └── raf_offsets.h
│   ├── src/
│   │   ├── raf_core.c
│   │   ├── raf_crc32c.c
│   │   ├── raf_state.c
│   │   └── raf_vectors.c
│   ├── arch/
│   │   ├── arm32/
│   │   │   ├── raf_hot_arm32.S
│   │   │   └── raf_arm32_caps_adapter.c
│   │   └── arm64/
│   │       ├── raf_hot_arm64.S
│   │       ├── raf_crc_arm64.S
│   │       └── raf_arm64_caps_adapter.c
│   ├── linker/
│   │   ├── arm32-qemu-virt.ld
│   │   └── arm64-qemu-virt.ld
│   └── tests/
│       ├── vectors/
│       ├── host/
│       └── compile_fail/
├── adapters/
│   ├── android-jni/
│   ├── qemu-user-linux/
│   ├── qemu-system-arm32/
│   ├── qemu-system-arm64/
│   └── host-posix/
└── audit/
    ├── allowed_exports.txt
    ├── forbidden_symbols.txt
    └── critical_ranges.json
```

### 4.1 `native/core`

Contrato:

```text
entrada  = ponteiro + comprimento + estado + configuração imutável
saída    = estado + código numérico + assinatura
I/O      = nenhum
clock    = nenhum
thread   = nenhum
syscall  = nenhum
heap     = nenhum
```

API proposta:

```c
typedef struct RafCoreState RafCoreState;
typedef struct RafCoreConfig RafCoreConfig;
typedef struct RafCoreResult RafCoreResult;

uint32_t raf_core_init(
    RafCoreState *state,
    const RafCoreConfig *config);

uint32_t raf_core_step(
    RafCoreState *state,
    const uint8_t *input,
    uint32_t input_len,
    RafCoreResult *result);

uint32_t raf_core_hash(
    const RafCoreState *state,
    uint8_t out32[32]);
```

Nenhum callback de I/O pertence a essa API. Callback é abstração de adapter.

### 4.2 `android-jni`

Pode usar:

- JNI;
- Android logging, somente fora de release estrito;
- mutex/atomic;
- `GetDirectBufferAddress` e `GetDirectBufferCapacity`;
- AndroidX na camada Java;
- ciclo de vida Android.

Deve:

- validar `offset`, `length` e capacidade;
- possuir erro numérico estável;
- não duplicar CRC/estado/matemática do core;
- nunca manter um segundo kernel global concorrente;
- carregar a biblioteca com fallback controlado.

### 4.3 `qemu-user-linux`

Serve para:

- testar ARM32/ARM64 em CI sem device;
- detectar instrução ilegal;
- comparar original/stripped;
- validar Linux EABI;
- testar syscalls apenas no harness.

Não serve como prova de ausência de OS.

### 4.4 `qemu-system-arm*`

Serve para a prova realmente sem OS:

```text
reset vector
→ startup mínimo
→ stack estática
→ zero de BSS
→ core
→ assinatura em RAM reservada ou UART MMIO
→ halt determinístico
```

O core não acessa UART. O adapter da placa QEMU escreve em MMIO.

Para eliminar até semihosting e syscall do teste:

1. reservar uma página de resultado no linker script;
2. o adapter grava `status`, `hash`, `cycles` e `sentinel` nessa página;
3. o runner QEMU usa monitor/QMP/GDB para extrair a RAM;
4. o host compara bytes com o vetor ouro.

Isso produz máquina-a-máquina sem sistema operacional dentro do guest.

## 5. Estratégia Gradle

### 5.1 Gradle não entra no runtime

Use Gradle para:

- selecionar perfil;
- chamar NDK/Clang/LLD;
- gerar manifestos;
- executar auditorias de símbolo;
- empacotar APK/AAR;
- chamar harness QEMU externo;
- publicar relatórios.

Não codificar semântica do core em Groovy.

### 5.2 Perfis de build

Criar propriedades explícitas:

```text
-Praf.profile=android-host
-Praf.profile=pure-audit
-Praf.profile=qemu-user-arm32
-Praf.profile=qemu-system-arm32
-Praf.profile=qemu-system-arm64
```

Tarefas alvo:

```text
:app:assembleRafDebug
:app:assembleRafRelease
:rafaelia:buildPureCoreArm32
:rafaelia:buildPureCoreArm64
:rafaelia:auditPureCoreSymbols
:rafaelia:testQemuUserArm32
:rafaelia:testQemuSystemArm32
:rafaelia:testQemuSystemArm64
:rafaelia:compareGoldenVectors
```

### 5.3 Não misturar flags globais

Hoje existem flags globais `-Os` e módulos que adicionam `-O3`. Isso deixa o vencedor dependente da ordem final da linha de compilação.

Cada módulo deve escolher um único objetivo:

```text
core-size       = -Oz ou -Os
hot-kernel      = -O3
adapter-debug   = -Og -g
adapter-release = -O2
```

O `compile_commands.json` deve ser artifact de CI para provar a linha efetiva.

### 5.4 Gradle e `printVersionName`

O runbook e o workflow chamam `:app:printVersionName`; a task precisa existir e falhar de forma observável. Toda pipeline shell que usa `| tail` deve habilitar `set -euo pipefail` antes da extração.

## 6. Estratégia AndroidX

### 6.1 Uso permitido

AndroidX permanece na camada hospedada:

```text
Activity / Service / WorkManager / lifecycle / UI / preference
```

Não entra em:

```text
native/core
arch/*.S
ELF freestanding
QEMU system guest
```

### 6.2 Uso de `androidx_RmR`

O fork RmR pode ser útil em três pontos:

1. estado Java determinístico;
2. estruturas contíguas para passagem a `DirectByteBuffer`;
3. lifecycle/navigation experimental isolado.

Contrato de adoção:

- selecionar um único módulo RmR;
- publicar AAR/JAR com versão e commit pinados;
- namespace próprio `rmr.*`;
- teste de API/ABI;
- benchmark antes/depois;
- nenhuma substituição silenciosa de `androidx.*`;
- nenhuma dependência do megarepo inteiro no build do app;
- claims de performance permanecem `EXPERIMENTAL` até medição reproduzível.

### 6.3 Opções de integração

Ordem recomendada:

```text
1. Maven local/CI com AAR pinado
2. submodule/repo de módulo RmR isolado
3. source dependency pequena e auditada
4. nunca: includeBuild da árvore AndroidX inteira
```

## 7. Estratégia QEMU

### 7.1 QEMU user-mode

Usar para o contrato Linux:

```text
qemu-arm
qemu-aarch64
```

Matriz mínima:

```text
ARM32 original
ARM32 stripped
ARM64 original
ARM64 stripped
```

Comparar:

- stdout byte a byte;
- exit status;
- hash do estado;
- símbolo/relocação;
- comportamento de caudas `len=0..257`.

### 7.2 QEMU system-mode

Usar para o contrato sem OS:

```text
qemu-system-arm
qemu-system-aarch64
```

Perfis sugeridos:

```text
ARM32: machine virt + Cortex-A15
ARM64: machine virt + Cortex-A53
```

O runner deve desabilitar semihosting. O guest não usa filesystem, Linux ABI ou syscall.

### 7.3 QEMU como produto externo

Manter o contrato já existente:

```text
QEMU compila fora
→ artifact possui source commit + SHA256SUMS + BUILD_INFO + qemu-exec
→ consumidor valida
→ execução acontece por processo controlado
```

Não incorporar a árvore QEMU no APK nem no build Gradle normal.

## 8. Flags por módulo

### 8.1 Core C estrito

```text
-std=c11
-ffreestanding
-fno-builtin
-fno-stack-protector
-fno-unwind-tables
-fno-asynchronous-unwind-tables
-fno-ident
-fvisibility=hidden
-ffunction-sections
-fdata-sections
-fno-common
-fno-strict-aliasing
-Wall
-Wextra
-Werror
-Wpedantic
-Wundef
-Wconversion
-Wsign-conversion
-Wshadow
-Wcast-align
-Wcast-qual
-Wstrict-prototypes
-Wmissing-prototypes
-Wmissing-declarations
-Wformat=2
-Wimplicit-fallthrough
-Wdouble-promotion
-Wfloat-conversion
```

Warnings incompatíveis com assembly ou código gerado devem ser desabilitados somente no arquivo específico, com justificativa ao lado.

### 8.2 Link do ELF sem OS

```text
-nostdlib
-nodefaultlibs
-nostartfiles
-Wl,--gc-sections
-Wl,--icf=all
-Wl,--fatal-warnings
-Wl,--warn-common
-Wl,-z,defs
-Wl,--build-id=none
-Wl,-Map,<artifact>.map
-Wl,--cref
-Wl,-e,<entry>
-T <linker-script>
```

### 8.3 ARM32

Baseline:

```text
-march=armv7-a
-mfloat-abi=softfp
```

NEON deve ficar em objeto separado:

```text
-march=armv7-a
-mfpu=neon-vfpv4
-mfloat-abi=softfp
```

Não compilar todo o core assumindo NEON quando a política diz runtime dispatch.

### 8.4 ARM64

Baseline:

```text
-march=armv8-a
```

CRC e extensões opcionais em objeto separado:

```text
-march=armv8-a+crc
```

Nunca declarar capacidade pelo fato de o binário ter sido compilado com a extensão. O adapter detecta HWCAP e seleciona a implementação.

### 8.5 Assembly

Cada função em sua própria seção:

```asm
.section .text.raf_hot_xor,"ax",%progbits
.global raf_hot_xor
.hidden raf_hot_xor
.type raf_hot_xor,%function
...
.size raf_hot_xor,.-raf_hot_xor
```

Adicionar `.note.GNU-stack` quando aplicável. Isso permite que `--gc-sections` elimine uma rotina realmente não usada.

## 9. Redução de símbolos

### 9.1 Allowlist

O core deve exportar somente:

```text
raf_core_init
raf_core_step
raf_core_hash
raf_core_contract
```

Ou uma lista ainda menor definida em `native/audit/allowed_exports.txt`.

### 9.2 Auditoria obrigatória

```bash
llvm-nm -u artifact
llvm-nm -D --defined-only artifact
llvm-readelf -Ws artifact
llvm-readelf -dW artifact
llvm-readelf -lW artifact
llvm-readelf -rW artifact
llvm-objdump -d artifact
llvm-size -A artifact
strings -a artifact
```

Gates:

```text
malloc/calloc/realloc/free = proibidos
memcpy/memset inesperados  = proibidos
open/read/write/pthread    = proibidos no core
JNI/Android/log            = proibidos no core
UND inesperado             = proibido
export fora da allowlist   = proibido
```

### 9.3 Comentários e strings

Comentários de fonte não entram no ELF. Strings literais entram.

Logo:

- não remover comentários úteis para “reduzir binário”;
- remover logging e mensagens literais do perfil puro;
- gerar códigos numéricos de erro;
- manter tabela de tradução de erro no adapter hospedado.

## 10. Memória

Contrato do core:

- sem `malloc/free`;
- sem VLA;
- sem recursão;
- sem objetos de tamanho não limitado;
- sem arena global compartilhada entre sessões;
- estado e scratch fornecidos pelo chamador;
- alinhamento expresso em contrato;
- tamanho verificável por `_Static_assert`.

Modelo:

```c
#define RAF_CORE_STATE_BYTES   256u
#define RAF_CORE_SCRATCH_BYTES 4096u
#define RAF_CORE_ALIGN         64u
```

A memória pode ser:

- estática em firmware;
- BSS no guest QEMU;
- `DirectByteBuffer` no Android adapter;
- array local no teste host.

## 11. Plano de migração

### Fase 0 — congelar a verdade

- gerar mapa de símbolos dos `.so` atuais;
- guardar `compile_commands.json`;
- guardar APK e libs por ABI;
- registrar vetores ouro;
- marcar claims como `PROVADO`, `PARCIAL` ou `TOKEN_VAZIO`.

### Fase 1 — extrair matemática única

- escolher entre `termux_rafaelia_direct`, `api_lowlevel`, `rafaelia` e `rmr` qual é o estado canônico;
- mover CRC/FNV/transição para `native/core`;
- proibir duplicação de algoritmo nos adapters;
- criar testes C puros.

### Fase 2 — criar adapters

- Android JNI;
- host POSIX;
- QEMU user Linux;
- QEMU system ARM32;
- QEMU system ARM64.

### Fase 3 — fechar símbolos e branches

- uma seção ASM por função;
- `.hidden`;
- allowlist;
- mapa do linker;
- auditoria de disassembly das regiões críticas;
- caudas e alinhamentos testados.

### Fase 4 — Gradle especializado

- tasks por perfil;
- nenhum `-Os` + `-O3` concorrente;
- relatório de flags efetivas;
- CI por ABI;
- artifact de auditoria sempre enviado, inclusive em falha.

### Fase 5 — QEMU sem OS

- startup e linker scripts;
- RAM mailbox;
- vetores ouro;
- ARM32/ARM64;
- sem semihosting;
- zero syscall no guest.

### Fase 6 — AndroidX/RmR seletivo

- publicar módulo isolado;
- integrar somente na camada Java;
- medir memória, GC, tempo e método count;
- manter AndroidX oficial como fallback até prova.

## 12. Critério de pronto

O selo final exige:

```text
CORE_SOURCE_PURE
AND CORE_NO_PLATFORM_INCLUDE
AND CORE_NO_HEAP
AND CORE_NO_SYSCALL
AND CORE_NO_UNEXPECTED_UNDEFINED
AND EXPORT_ALLOWLIST_EXACT
AND C_ASM_BIT_EQUIVALENCE
AND ARM32_QEMU_USER_PASS
AND ARM64_QEMU_USER_PASS
AND ARM32_QEMU_SYSTEM_PASS
AND ARM64_QEMU_SYSTEM_PASS
AND ANDROID_DEVICE_PASS
AND GOLDEN_VECTOR_IDENTICAL
```

## 13. Antimetas

Não fazer:

- reescrever todo o app em C/ASM;
- chamar `.so` JNI de firmware freestanding;
- colocar syscalls no core;
- importar QEMU inteiro no APK;
- importar o AndroidX megarepo no Gradle do app;
- apagar nomes do fonte para fingir redução de símbolos;
- usar `-ffast-math` em matemática que exige determinismo bit a bit;
- usar uma flag `-DNO_MALLOC` que não é consumida pelo fonte;
- desabilitar warning globalmente para esconder falha;
- declarar branchless sem auditar o objeto final;
- criar outro estado/CRC/kernel paralelo.

## 14. Síntese

```text
Gradle constrói.
AndroidX hospeda.
JNI adapta.
O core transforma.
QEMU-user testa ABI.
QEMU-system prova ausência de OS.
ELF e vetores provam o bit final.
```
