# GitHub Copilot — Playbook de Issues para Migração Freestanding

## Uso

Este documento deve ser copiado para a descrição de uma Epic ou dividido em issues sequenciais. O Copilot deve trabalhar em uma issue por vez, com escopo de arquivos explícito.

Documento canônico:

- `docs/architecture/GRADLE_ANDROIDX_QEMU_FREESTANDING_ARCHITECTURE.md`

## Regra operacional

```text
uma issue
→ um contrato
→ poucos arquivos
→ teste local
→ relatório objetivo
→ nenhum claim novo sem evidência
```

## Epic

**Título:** `EPIC: separar pure core de Android/AndroidX/JNI/QEMU e fechar símbolos`

**Objetivo:** extrair um único núcleo determinístico, sem plataforma, e transformar Android, QEMU user e QEMU system em adapters especializados.

## Issue 1 — Baseline de flags e símbolos

**Título:** `P0: registrar linha efetiva de compilação e superfície ELF por ABI`

### Arquivos

- `app/build.gradle`
- `app/src/main/cpp/Android.mk`
- `rafaelia/build.gradle`
- `rafaelia/src/main/cpp/Android.mk`
- `rmr/build.gradle`
- `rmr/src/main/cpp/Android.mk`
- `terminal-emulator/build.gradle`
- scripts novos em `scripts/native_audit/`

### Fazer

- exportar `compile_commands.json`;
- guardar comandos ARM32/ARM64;
- gerar `nm`, `readelf`, `objdump`, `size` e `strings` para cada `.so`;
- registrar exports, UND, `DT_NEEDED`, relocações e tamanhos;
- enviar artifacts mesmo quando o gate falhar.

### Não fazer

- não alterar algoritmo;
- não remover símbolo antes de registrar baseline;
- não chamar warning silenciado de warning resolvido.

### Pronto quando

```text
reports/native/<abi>/<module>/compile-command.txt
reports/native/<abi>/<module>/symbols.txt
reports/native/<abi>/<module>/dynamic.txt
reports/native/<abi>/<module>/relocations.txt
reports/native/<abi>/<module>/disassembly.txt
reports/native/<abi>/<module>/strings.txt
```

## Issue 2 — Corrigir Gradle e pipeline de versão

**Título:** `P0: restaurar printVersionName e falha shell com pipefail`

### Fazer

- criar task `:app:printVersionName`;
- retornar somente a versão efetiva;
- adicionar `set -euo pipefail` antes de pipelines shell;
- validar string vazia;
- testar SemVer com metadata de commit;
- não mascarar erro do Gradle com `tail`.

### Testes

```bash
./gradlew -q :app:printVersionName
TERMUX_APP_VERSION_NAME=0.118.0 ./gradlew -q :app:printVersionName
TERMUX_APP_VERSION_NAME=invalid ./gradlew :app:tasks
```

## Issue 3 — Definir propriedade única de perfil

**Título:** `P1: adicionar perfis native android-host pure-audit qemu-user qemu-system`

### Fazer

Criar uma fonte única:

```text
raf.profile=android-host|pure-audit|qemu-user-arm32|qemu-system-arm32|qemu-system-arm64
```

Cada perfil deve selecionar flags e tarefas sem condicional duplicada entre workflows.

### Pronto quando

- o perfil aparece no manifesto de build;
- flags efetivas são artifact;
- perfil inválido falha;
- `-Os` e `-O3` não aparecem juntos no mesmo compile command.

## Issue 4 — Inventário de propriedade do algoritmo

**Título:** `P0: escolher estado CRC hash e transição canônicos`

### Inspecionar

- `app/src/main/cpp/lowlevel/api_lowlevel.*`
- `app/src/main/cpp/lowlevel/rafaelia_jni_direct.c`
- `app/src/main/cpp/lowlevel/raf_*.c`
- `rafaelia/src/main/cpp/*.c`
- `rmr/src/main/cpp/*`

### Produzir

Tabela:

```text
algoritmo | implementações | diferenças | consumidor | autoridade escolhida
```

### Proibido

- não criar nova implementação;
- não fundir matemáticas diferentes sem vetor ouro;
- não manter dois estados globais com o mesmo significado.

## Issue 5 — Criar pure core

**Título:** `P0: extrair core C11 sem plataforma e sem heap`

### Estrutura

```text
native/core/include/
native/core/src/
native/core/tests/
native/audit/
```

### Restrições

- somente tipos próprios;
- sem `jni.h`, Android, POSIX, `stdio`, `stdlib`, `string`, `pthread`, `unistd`, `fcntl`;
- sem callback de I/O;
- sem global mutável compartilhado;
- sem VLA;
- sem recursão;
- scratch fornecido pelo chamador;
- `_Static_assert` para tamanho e offset.

### API máxima

```text
raf_core_init
raf_core_step
raf_core_hash
raf_core_contract
```

## Issue 6 — Flags estritas do core

**Título:** `P0: aplicar warnings estritos e contrato freestanding por translation unit`

### Flags

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
-Wall -Wextra -Werror -Wpedantic -Wundef
-Wconversion -Wsign-conversion -Wshadow
-Wcast-align -Wcast-qual
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
-Wformat=2 -Wimplicit-fallthrough
-Wdouble-promotion -Wfloat-conversion
```

### Regra

Uma exceção de warning deve ser local ao arquivo e acompanhada de comentário com motivo e teste.

## Issue 7 — ASM coletável e oculto

**Título:** `P0: separar funções ASM em seções e corrigir caudas`

### Fazer

- uma `.section .text.<fn>` por rotina;
- `.hidden` em rotinas internas;
- `.type` e `.size`;
- eliminar FNV prime incorreto;
- implementar caudas ARM32/ARM64;
- comparar C e ASM para `len=0..257`;
- não declarar branchless sem disassembly.

### Pronto quando

- rotina não usada desaparece com `--gc-sections`;
- `nm -D` não exporta helper interno;
- vetores C/ASM são idênticos.

## Issue 8 — Android JNI adapter

**Título:** `P0: reduzir JNI a validação e chamada do core`

### Fazer

- `offset`, `length`, `capacity` validados;
- sem CRC/estado duplicados no JNI;
- erro numérico estável;
- fallback de `System.loadLibrary`;
- sincronização definida;
- nenhuma arena global compartilhada sem owner.

### Testes

- buffer nulo;
- heap buffer;
- direct buffer;
- offset negativo;
- length maior que capacity;
- length zero;
- chamadas concorrentes;
- 100 mil eventos.

## Issue 9 — Símbolos e allowlist

**Título:** `P0: bloquear exports e dependências proibidas`

### Arquivos

```text
native/audit/allowed_exports.txt
native/audit/forbidden_symbols.txt
scripts/native_audit/check_symbols.sh
```

### Gate

Falhar para:

```text
malloc calloc realloc free
open read write close
pthread
JNI
android_log
DT_NEEDED no ELF sem OS
UND inesperado
export fora da allowlist
```

## Issue 10 — QEMU user-mode

**Título:** `P1: ampliar harness QEMU user para ARM32 e ARM64`

### Fazer

- manter syscalls somente no harness;
- executar core original/stripped;
- comparar estado, hash, stdout e exit status;
- gerar manifesto com compiler/linker/QEMU/hash;
- incluir caudas e alinhamentos.

### Declaração correta

```text
QEMU_USER_LINUX_ABI_PROVED
```

Não declarar `NO_OS_PROVED`.

## Issue 11 — QEMU system sem OS

**Título:** `P1: provar core ARM32 ARM64 sem syscall e sem semihosting`

### Estrutura

```text
native/adapters/qemu-system-arm32/start.S
native/adapters/qemu-system-arm32/platform.c
native/core/linker/arm32-qemu-virt.ld
native/adapters/qemu-system-arm64/start.S
native/adapters/qemu-system-arm64/platform.c
native/core/linker/arm64-qemu-virt.ld
```

### Saída

- mailbox em RAM reservada;
- sentinel final;
- hash de estado;
- status numérico;
- extração via monitor/QMP/GDB no host.

### Proibido

- syscall;
- semihosting;
- libc;
- filesystem;
- JNI;
- Android.

## Issue 12 — AndroidX/RmR seletivo

**Título:** `P2: avaliar artifact isolado de androidx_RmR`

### Fazer

- identificar módulo mínimo;
- publicar AAR/JAR em repositório de teste;
- pin de commit;
- namespace `rmr.*`;
- medir APK, method count, memória, GC e latência;
- manter fallback AndroidX oficial.

### Não fazer

- não adicionar `androidx_RmR` inteiro em `settings.gradle`;
- não substituir `androidx.*` por shadowing;
- não aceitar claims documentais sem benchmark.

## Issue 13 — Documentação e matriz de prova

**Título:** `P1: atualizar status com prova por artifact`

### Atualizar

- `docs/STATUS.md`
- `docs/RUNTIME_TRUTH_TABLE.md`
- `docs/ENGINEERING_SYSTEM_RUNBOOK.md`

### Estados

```text
PROVADO
PROVADO ESTRUTURAL
PARCIAL
TOKEN_VAZIO
EXPERIMENTAL
FUTURO
```

## Modelo de resposta do Copilot por issue

```text
Resumo
Arquivos alterados
Contrato preservado
Warnings corrigidos
Warnings silenciados e justificativa
Símbolos antes/depois
Testes executados
Artifacts gerados
Lacunas TOKEN_VAZIO
Próxima issue desbloqueada
```

## Critério de encerramento da Epic

```text
core único
+ zero plataforma
+ zero heap
+ zero syscall
+ allowlist exata
+ C/ASM equivalentes
+ QEMU user ARM32/ARM64
+ QEMU system ARM32/ARM64
+ device Android
+ vetores ouro idênticos
```
