# Claude/Cloud — Contrato de Execução Freestanding

## Prompt canônico

Você está trabalhando no repositório `rafaelmeloreisnovo/termux-app-rafacodephi`.

Seu papel é atuar como **arquiteto e revisor sistêmico**, decompondo a migração do núcleo RAFAELIA para uma arquitetura realmente portátil e verificável, sem misturar Gradle, AndroidX, JNI, Linux ou QEMU no core.

Leia, nesta ordem:

1. `docs/architecture/GRADLE_ANDROIDX_QEMU_FREESTANDING_ARCHITECTURE.md`
2. `AGENTS.md`
3. `docs/STATUS.md`
4. `docs/RUNTIME_TRUTH_TABLE.md`
5. `docs/ENGINEERING_SYSTEM_RUNBOOK.md`
6. `app/build.gradle`
7. `gradle.properties`
8. `app/src/main/cpp/Android.mk`
9. `rafaelia/src/main/cpp/Android.mk`
10. `rmr/src/main/cpp/Android.mk`

## Missão

Produzir uma refatoração em fases que resulte em:

```text
PURE CORE
- C11 freestanding
- sem libc
- sem heap
- sem syscall
- sem JNI
- sem Android
- sem QEMU
- estado fornecido pelo chamador
- código numérico de erro
- matemática única

ADAPTERS
- Android/JNI
- QEMU user/Linux
- QEMU system/ARM32
- QEMU system/ARM64
- host tests
```

## Princípios não negociáveis

### 1. Não criar outro kernel

O repositório já possui caminhos sobrepostos:

- `termux-baremetal`;
- `termux_rafaelia_direct`;
- `api_lowlevel`;
- módulo `rafaelia`;
- módulo `rmr`.

Mapeie responsabilidades e escolha uma única fonte canônica para:

- estado;
- CRC32C;
- hash;
- transição;
- classificação;
- assinatura.

Adapters não podem reimplementar esses algoritmos.

### 2. “Sem função nativa” significa sem plataforma no core

Não tente eliminar toda função C do fonte. Em vez disso:

- limite a ABI pública;
- transforme helpers em `static inline` quando comprovado;
- proíba chamadas indiretas no hot path;
- use LTO somente com relatório reproduzível;
- mantenha legibilidade e auditabilidade.

### 3. “Sem nome de variável” significa sem nomes residuais no artifact

Não ofusque o fonte. Use:

- `-fvisibility=hidden`;
- version script/allowlist;
- `.hidden` no ASM;
- strip do artifact distribuído;
- arquivo de debug separado;
- auditoria `nm/readelf/strings`.

### 4. Branchless precisa ser local e provado

Classifique cada função como:

```text
BRANCHLESS_KERNEL
BOUNDED_LOOP
CONTROL_ADAPTER
```

Não use a palavra branchless para o módulo inteiro sem disassembly.

### 5. QEMU possui dois contratos

- QEMU user-mode: testa ISA + ABI Linux, aceita syscall no harness.
- QEMU system-mode: prova guest sem OS, sem syscall e sem semihosting.

O core é idêntico nos dois. Apenas startup e saída pertencem ao adapter.

## Trabalho obrigatório antes de modificar código

Entregue um inventário com:

| Item | Arquivo atual | Símbolo/ABI | Dependências | Estado | Destino |
|---|---|---|---|---|---|
| Estado canônico | | | | | |
| CRC32C | | | | | |
| FNV/hash | | | | | |
| arena/scratch | | | | | |
| JNI | | | | | |
| ASM ARM32 | | | | | |
| ASM ARM64 | | | | | |
| QEMU harness | | | | | |

Use os estados:

```text
PROVADO
PROVADO ESTRUTURAL
PARCIAL
TOKEN_VAZIO
EXPERIMENTAL
FUTURO
```

## Plano de commits

Não faça um commit monolítico. Use esta sequência:

1. `docs(audit): record native ownership and symbol baseline`
2. `refactor(core): introduce platform-free state contract`
3. `refactor(core): move deterministic transforms into pure core`
4. `refactor(android): reduce JNI to validated adapter`
5. `refactor(asm): split ARM routines into collectible hidden sections`
6. `build(native): add strict flags and export allowlist`
7. `test(core): add golden vectors and tail-length coverage`
8. `test(qemu): add user-mode ARM32/ARM64 contracts`
9. `test(qemu): add system-mode no-OS RAM mailbox contracts`
10. `ci(native): publish symbol maps and compile commands`

Cada commit deve compilar ou ser explicitamente documental.

## Gradle

Revise:

- task ausente `:app:printVersionName`;
- extrações shell sem `pipefail`;
- conflito potencial `-Os`/`-O3`;
- flags repetidas entre `app`, `rafaelia`, `rmr` e `terminal-emulator`;
- warnings desligados por módulo;
- `compile_commands.json` como evidence artifact;
- tasks específicas para `pure-audit`, `qemu-user` e `qemu-system`.

Não migre para Kotlin DSL ou CMake apenas por estética. A mudança de ferramenta precisa fechar uma lacuna concreta.

## AndroidX

O app usa AndroidX oficial. O fork `androidx_RmR` é externo e muito maior que o app.

Proponha integração somente quando houver:

- módulo RmR isolado;
- artifact AAR/JAR versionado;
- commit pinado;
- namespace próprio;
- API/ABI test;
- benchmark reproduzível;
- fallback oficial.

Não substitua classes `androidx.*` silenciosamente.

## Símbolos

Adicione gates que falham quando houver:

```text
UND inesperado
DT_NEEDED no ELF sem OS
export fora da allowlist
malloc/free/calloc/realloc
open/read/write no core
pthread/JNI/android_log no core
memcpy/memset gerado sem contrato
string de debug no artifact puro
```

Para ASM:

- seção por função;
- `.global` somente quando necessário;
- `.hidden` para rotinas internas;
- `.type` e `.size`;
- caudas completas;
- mesma matemática do C;
- teste `len=0..257`.

## Resultado que você deve entregar

1. mapa arquitetural antes/depois;
2. matriz de arquivos movidos/mantidos/removidos;
3. riscos e falsificadores;
4. commits pequenos;
5. testes;
6. relatório de símbolos;
7. relatório QEMU user/system;
8. PR com checklist;
9. nenhuma alegação não provada.

## Condição de parada

Pare e marque `TOKEN_VAZIO` quando faltar:

- artifact;
- device;
- toolchain;
- significado de estrutura;
- autorização de mudança cross-repo;
- evidência de equivalência.

Não substitua lacuna por implementação inventada.
