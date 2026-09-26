# Freestanding Pure Core — Inventário e Fronteira V2

> Baseline auditado: `master@988d3c24cf3a4a676ff0cab0512a2436a8d1c704` (PR #462).  
> Escopo: C/C++/ASM do repositório, com aprofundamento no caminho low-level ativo.  
> Regra: `SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`.

## 1. Resultado do inventário

A árvore observada contém **355 arquivos C/C++/ASM** e **312 blobs de fonte únicos**. Existem **29 grupos de duplicação exata**, correspondendo a **43 cópias excedentes**. A maior parte da redundância ocorre entre `Arme/Add/`, `BugOrAdd/` e `rmr/Rrr/`.

A política desta versão não apaga os rascunhos nem reescreve história. Ela separa autoridade:

| camada | função | regra |
|---|---|---|
| `PURE_CORE` | transformação determinística de valores/memória fornecida pelo caller | zero libc, heap, syscall, JNI, I/O e decisão de plataforma |
| `PLATFORM_GATE` | entrada ELF, syscall, acesso a processo/arquivo e ABI do SO | pode conter syscall explícita; nunca é chamado de pure core |
| `HOSTED_ADAPTER` | Android/JNI/dlopen/POSIX e integração de app | depende da plataforma por definição |
| `TOOLING_TEST` | Python, shell, CI, compilador, linker e auditoria | pode depender do host; não entra no runtime core |
| `STAGING_HISTORY` | cópias, experimentos e versões preservadas | não define autoridade de produção |

## 2. O que a auditoria encontrou

O recorte aprofundado cobre **83 arquivos** em:

- `app/src/main/cpp/lowlevel/`
- `app/src/main/cpp/freestanding/`
- `src/bootstrap/`
- `bootstrap/`
- `bootstrap_rafaelia/`
- `rafaelia/src/main/cpp/zero/`

Nesse recorte, a varredura não encontrou chamadas explícitas a
`malloc/calloc/realloc/free`. Portanto o principal delta não é remover heap:
é separar syscall/hosted/control-flow do núcleo aritmético.

Exemplos observados:

- `src/bootstrap/freestanding_syscalls.h` é `PLATFORM_GATE`, não `PURE_CORE`;
- `src/bootstrap/start_arm.S` contém entrada ELF + `exit_group` e é `PLATFORM_GATE`;
- `src/bootstrap/freestanding_log.h` escreve por syscall e é `PLATFORM_GATE`;
- `bootstrap/proot_freestanding.c` é gate syscall-only e permanece fora do core;
- `app/src/main/cpp/lowlevel/rafaelia_gpu_orchestrator.c` inclui `dlfcn.h` e `unistd.h`, portanto é `HOSTED_ADAPTER`;
- `rafaelia/src/main/cpp/zero/rafz.c` já respeita zero libc/heap/I/O/syscall, mas contém loops e validações; ele é o produtor correto para extrair folhas branchless sem duplicar arquitetura.

## 3. Primeiro delta de código

As primitivas Bagua/Q16 foram extraídas para:

`rafaelia/src/main/cpp/zero/include/rafz_pure_primitives.h`

Essa folha é propositalmente limitada a operações escalares sem dependência
externa, heap, syscall, JNI ou sintaxe de decisão/iteração em runtime.
`rafz.c` mantém a ABI pública e delega essas primitivas à nova folha.

A sonda `tests/pure_core_probe.c` é compilada separadamente para ARMv7 e
AArch64 com `-ffreestanding -nostdlib -nostdinc -fno-builtin`. O auditor
rejeita transferência de controle não terminal na assembly gerada. O retorno
ABI terminal (`bx lr`/equivalente ou `ret`) é permitido porque uma função
chamável precisa retornar; ele não representa uma decisão de algoritmo.

## 4. Flags e linker

O contrato do core usa, no mínimo:

```text
-std=c11
-O2
-ffreestanding
-nostdlib
-nostdinc
-fno-builtin
-fno-stack-protector
-fno-unwind-tables
-fno-asynchronous-unwind-tables
```

No ELF final, quando houver link de uma imagem executável, os gates devem
continuar verificando separadamente `PT_INTERP`, `DT_NEEDED`, símbolos
indefinidos, relocations runtime e seções descartáveis. `--gc-sections` é
redução por alcançabilidade, não autorização para remover sem prova semântica.

## 5. Sobre “sem branch”

Há três estados distintos:

1. **source-control-free**: nenhum `if/for/while/switch/goto/?:` na folha;
2. **assembly-control-free**: nenhuma decisão/call/backedge no código gerado;
3. **whole-binary-control-free**: propriedade do ELF inteiro.

Esta mudança mede 1 e 2 somente para a sonda pure-core. O estado 3 permanece
`TOKEN_VAZIO` até existir auditoria do artefato ELF exato. Não se infere a
propriedade do binário inteiro a partir de uma função.

## 6. Comando canônico

```bash
python3 tools/audit_freestanding_boundaries.py \
  --strict --compile-probe --write-report
```

Saídas:

- `reports/freestanding-boundary-inventory.json`
- `reports/freestanding-boundary-inventory.md`

## 7. Evidência CI observada — PR #463

Commit auditado pelo gate específico: `38e02d288b24a48bbbc0fe0909af541442f05bd0`.

Control plane: run `36245846563`; job `108414818810` concluiu `success`.

```text
pure_core_source_policy=PASS
source_files_total=357
source_unique_blobs=314
exact_duplicate_excess=43
focus_files=84
focus_heap_call_files=0
armv7a-none-eabi: compile_exit=0, non_terminal_transfers=0
aarch64-none-elf: compile_exit=0, non_terminal_transfers=0
rafaelia.zero.android-runtime-validation.v3: PASS
runtime_receipt.v2: PASS / HOST_SIMULATION
claim_allowed=false
physical_android=TOKEN_VAZIO
whole_binary_no_branch=TOKEN_VAZIO
```

O incremento 355→357 e 312→314 em relação ao baseline é explicado pelos dois
novos artefatos de fonte do próprio gate: a folha pure-core e a sonda de teste.

O workflow `Provider Protection Gate` permanece FAIL por configuração do ruleset
do `master` (ausência de `required_status_checks` e requisito de resolução de
threads esperado pelo gate). Esse estado é governança externa ao delta C/ASM e
não é usado para promover nem rebaixar a propriedade técnica da folha pure-core.

## 8. Próximas extrações

A ordem de menor risco é:

1. folhas escalares do RAFAELIA ZERO;
2. BITRAF encode/extract sem ponteiros opcionais;
3. CRC com blocos fixos/unroll gerado em compile-time;
4. geometria Q16 de tamanho conhecido;
5. somente depois, adaptadores de memória variável.

Operações de comprimento variável, parsing e I/O não devem ser artificialmente
chamadas de branchless. Elas ficam em camadas superiores ou ganham variantes
especializadas por tamanho fixo.

## R3

`F_ok`: autoridade do core separada; inventário reproduzível; primeira folha
pure-core definida; source policy PASS; probes ARMv7/AArch64 compilados com
zero transferências não terminais detectadas; compatibilidade RAFAELIA ZERO PASS.

`F_gap`: whole-binary no-branch; auditoria do ELF final; execução física Android;
deduplicação semântica dos 43 excedentes; ruleset provider-protection fora do delta.

`F_next`: extrair a próxima folha de tamanho/semântica fixa, adicionar gate ELF
(PT_INTERP/DT_NEEDED/undefined/relocations) e gerar receipt físico quando houver aparelho.
