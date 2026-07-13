# OpenAI Codex — Contrato de Execução no Repositório

## Papel

Atue como agente de execução transversal. Você deve transformar o plano freestanding em mudanças auditáveis no GitHub, usando branch, commits pequenos, testes, artifacts e Pull Request.

Documento canônico:

- `docs/architecture/GRADLE_ANDROIDX_QEMU_FREESTANDING_ARCHITECTURE.md`

Não comece alterando código. Primeiro produza baseline verificável.

## Repositório principal

```text
rafaelmeloreisnovo/termux-app-rafacodephi
base: master
```

Repos externos somente por contrato:

```text
rafaelmeloreisnovo/androidx_RmR
rafaelmeloreisnovo/qemu_rafaelia
```

Não escreva nos repos externos durante a primeira PR do app. Registre mudanças cross-repo necessárias em documento ou issues separadas.

## Branch

```text
codex/freestanding-core-boundaries
```

## Objetivo técnico

```text
extrair core único
→ remover dependências de plataforma
→ adapters especializados
→ flags por módulo
→ símbolos por allowlist
→ vetores ouro
→ QEMU user
→ QEMU system sem OS
→ Android device
```

## Regras de segurança arquitetural

1. Não alterar `master` diretamente.
2. Não importar QEMU inteiro no app.
3. Não importar AndroidX inteiro no app.
4. Não criar terceiro/quarto estado global concorrente.
5. Não reescrever algoritmo sem vetor ouro.
6. Não remover comments para reduzir artifact.
7. Não usar `-ffast-math` no core.
8. Não adicionar syscall ao core.
9. Não mascarar warning com cast/attribute sem relatório.
10. Não considerar CI verde se o workflow não audita o módulo alterado.

## Ciclo 0 — Baseline

### Ler

```text
settings.gradle
build.gradle
gradle.properties
gradle/wrapper/gradle-wrapper.properties
app/build.gradle
app/src/main/cpp/Android.mk
rafaelia/build.gradle
rafaelia/src/main/cpp/Android.mk
rmr/build.gradle
rmr/src/main/cpp/Android.mk
terminal-emulator/build.gradle
AGENTS.md
docs/STATUS.md
docs/RUNTIME_TRUTH_TABLE.md
```

### Executar

```bash
./gradlew -q :app:printVersionName
./gradlew :app:assembleDebug
python3 scripts/validate_native_structure.py
```

Se `printVersionName` não existir, registrar falha e corrigi-la em commit separado antes das outras mudanças.

### Coletar

Para cada ABI e `.so`:

```bash
llvm-nm -u
llvm-nm -D --defined-only
llvm-readelf -Ws
llvm-readelf -dW
llvm-readelf -lW
llvm-readelf -rW
llvm-objdump -d
llvm-size -A
strings -a
```

Guardar em `reports/native_baseline/` ou artifact CI, sem versionar binários grandes.

## Ciclo 1 — Corrigir fábrica Gradle

### Obrigatório

- task `printVersionName` determinística;
- `set -euo pipefail` nos shells críticos;
- versão vazia proibida;
- uma única otimização por módulo;
- `compile_commands.json` publicado;
- artifacts enviados com `if: always()`;
- falha de gate imprime esperado e observado.

### Não fazer

- migração total para Kotlin DSL;
- atualização de versão sem necessidade;
- mudança de `targetSdk=28` nesta PR;
- mistura de setup Android em múltiplos workflows.

## Ciclo 2 — Ownership de algoritmo

Crie:

```text
docs/audits/NATIVE_ALGORITHM_OWNERSHIP.md
```

Mapeie:

- state structs;
- CRC32C;
- FNV/hash;
- arena;
- dispatch;
- JNI;
- ARM32 ASM;
- ARM64 ASM;
- benchmark;
- runtime sensor.

Escolha autoridade única. Marque implementações divergentes como:

```text
MIGRATE
ADAPTER_ONLY
DELETE_AFTER_EQUIVALENCE
LEGACY
TOKEN_VAZIO
```

## Ciclo 3 — Pure core

### Estrutura alvo

```text
native/core/include/
native/core/src/
native/core/tests/
native/audit/
```

### Restrições automáticas

Adicionar script que falha se `native/core` contiver:

```text
#include <jni.h>
#include <android/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
malloc(
free(
open(
read(
write(
__android_log
JNIEXPORT
```

A lista pode ser refinada, mas exceções exigem justificativa formal.

### Memória

- estado fornecido pelo chamador;
- scratch fornecido pelo chamador;
- nenhum VLA;
- nenhum singleton mutável;
- `_Static_assert` de tamanho/alinhamento/offset;
- overflow verificado antes de multiplicação/soma de tamanhos.

## Ciclo 4 — Adapters

### Android/JNI

- valida buffer direto;
- valida capacidade, offset e length;
- sincronização documentada;
- nenhuma matemática duplicada;
- erro numérico;
- fallback de library load;
- teste concorrente.

### QEMU user/Linux

- syscalls somente no harness;
- core idêntico;
- ARM32 e ARM64;
- original e stripped;
- manifesto de ferramentas e hashes.

### QEMU system

- startup mínimo;
- linker script;
- stack/BSS;
- sem syscall;
- sem semihosting;
- mailbox em RAM;
- extração pelo host;
- halt determinístico.

## Ciclo 5 — ASM

### Regras

```text
uma função = uma seção
helper interno = .hidden
ABI = .type + .size
cauda = completa
C e ASM = bit idênticos
```

### Auditoria

Use `objdump` para delimitar os símbolos críticos e contar:

- chamadas `bl/blr/blx`;
- branches condicionais;
- loads/stores;
- instruções opcionais;
- prólogo/epílogo;
- tamanho em bytes.

Não bloquear todo branch. Bloquear branch dependente de dado somente em funções marcadas `BRANCHLESS_KERNEL`.

## Ciclo 6 — Símbolos

Criar:

```text
native/audit/allowed_exports.txt
native/audit/forbidden_symbols.txt
scripts/native_audit/check_core_contract.sh
```

Allowlist inicial máxima:

```text
raf_core_init
raf_core_step
raf_core_hash
raf_core_contract
```

O script deve distinguir:

- símbolo exportado;
- símbolo local;
- símbolo indefinido;
- string literal;
- seção de debug;
- função coletada pelo linker.

## Ciclo 7 — AndroidX/RmR

Não integrar no primeiro PR de pure core.

Produzir análise separada:

```text
docs/integrations/ANDROIDX_RMR_SELECTIVE_CONSUMPTION.md
```

Ela deve comparar:

```text
AndroidX Maven atual
vs
AAR RmR isolado
```

Métricas:

- dependency graph;
- method count;
- AAR/APK size;
- heap/GC;
- cold start;
- lifecycle correctness;
- minSdk 21;
- targetSdk 28;
- AGP/Gradle compatibility.

## Commits esperados

```text
fix(ci): restore deterministic app version task
build(native): record compile commands and ELF baseline
refactor(core): add platform-free state and transform contract
refactor(android): make JNI a bounded adapter
refactor(asm): isolate hidden ARM routines per section
test(core): add deterministic golden vectors
test(qemu): add Linux user-mode ARM32 and ARM64
test(qemu): add no-OS system-mode mailbox harness
ci(native): enforce export and forbidden-symbol contracts
docs(native): publish evidence and remaining TOKEN_VAZIO gaps
```

## Pull Request

Título sugerido:

```text
refactor(native): establish portable freestanding core and platform boundaries
```

Corpo obrigatório:

```text
## Baseline
## Arquitetura anterior
## Arquitetura nova
## Flags efetivas
## Símbolos antes/depois
## Warnings corrigidos
## Warnings localmente suprimidos
## Vetores C/ASM
## QEMU user
## QEMU system
## Android
## Tamanho por ABI
## Riscos
## TOKEN_VAZIO
## Rollback
```

## Gates mínimos antes de merge

```text
Gradle config passa
ARM32 compila
ARM64 compila
unit tests passam
core forbidden-include passa
core forbidden-symbol passa
export allowlist passa
C/ASM vectors passam
QEMU user ARM32 passa
QEMU user ARM64 passa
QEMU system ARM32 passa ou fica explicitamente TOKEN_VAZIO
QEMU system ARM64 passa ou fica explicitamente TOKEN_VAZIO
artifact reports publicados
```

Não marcar uma plataforma como provada se o runner ou toolchain não estava disponível.

## Resultado da execução

Ao terminar, responda com:

1. PR criada;
2. commits;
3. arquivos alterados;
4. comandos executados;
5. testes que passaram;
6. testes que falharam;
7. símbolos removidos;
8. símbolos restantes e motivo;
9. lacunas `TOKEN_VAZIO`;
10. próximo passo de menor risco.
