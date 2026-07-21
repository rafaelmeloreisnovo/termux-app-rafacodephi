# APKC — Contrato ELF executável mínimo

## Objetivo

Transformar o emissor ELF autoral de um contêiner apenas relocável para uma prova
estrutural executável pequena e verificável, sem declarar um linker geral.

## Artefatos produzidos

`apkc/fmt_elf.h` possui quatro emissores limitados por capacidade:

```text
apkc_elf32_arm_build_checked          -> ELF32 ARM ET_REL
apkc_elf64_aarch64_build_checked      -> ELF64 AArch64 ET_REL
apkc_elf32_arm_exec_build_checked     -> ELF32 ARM ET_EXEC
apkc_elf64_aarch64_exec_build_checked -> ELF64 AArch64 ET_EXEC
```

Os dois executáveis contêm:

- header ELF little-endian;
- máquina correta (`EM_ARM` ou `EM_AARCH64`);
- tipo `ET_EXEC`;
- um program header;
- um segmento `PT_LOAD` com permissões `PF_R | PF_X`;
- alinhamento de 4096 bytes;
- entrypoint dentro do segmento;
- nenhum section header;
- stub fixo `exit(0)`.

## Stub ARM32

```asm
mov r7, #1
mov r0, #0
svc #0
```

Palavras little-endian registradas:

```text
E3A07001 E3A00000 EF000000
```

## Stub AArch64

```asm
mov x8, #93
mov x0, #0
svc #0
```

Palavras little-endian registradas:

```text
D2800BA8 D2800000 D4000001
```

## Prova independente

O emissor hospedado é:

```text
tests/native/apkc_emit_exec_elf.c
```

O parser independente é:

```bash
python3 scripts/validate_apkc_elf_contract.py FILE \
  --expect arm32 \
  --kind exec \
  --pretty
```

ou:

```bash
python3 scripts/validate_apkc_elf_contract.py FILE \
  --expect arm64 \
  --kind exec \
  --pretty
```

O validador rejeita divergências em:

- magic, classe, endianness e versão;
- tipo e máquina;
- tamanho total;
- entrypoint;
- offset, quantidade e tamanho dos program headers;
- presença indevida de section table;
- ABI flags;
- tipo e permissões do segmento;
- endereços e tamanhos carregáveis;
- alinhamento e congruência de página;
- entrypoint fora do segmento;
- padding não nulo;
- opcodes diferentes do stub registrado.

## Gate canônico

```bash
bash scripts/test_raf_native_compile_contract.sh
```

O gate compila o emissor hospedado, grava os dois executáveis e executa os dois
validadores independentes.

## Estado honesto

```text
ELF32_ARM_FIXED_EXIT_STUB     = VERIFIED_HOST após gate
ELF64_AARCH64_FIXED_EXIT_STUB = VERIFIED_HOST após gate
GENERAL_OBJECT_LINKER         = TOKEN_VAZIO
ARBITRARY_CODE_EMISSION       = TOKEN_VAZIO
SYMBOL_TABLE                  = TOKEN_VAZIO
RELOCATIONS                   = TOKEN_VAZIO
DYNAMIC_LINKING               = TOKEN_VAZIO
SHARED_OBJECT_WRITER          = TOKEN_VAZIO
PHYSICAL_DEVICE_RUNTIME       = TOKEN_VAZIO
```

## O que a prova não significa

Um arquivo com `ET_EXEC`, `PT_LOAD` e instruções válidas é um avanço técnico real,
mas ainda não demonstra:

- compilação de uma linguagem;
- ligação de objetos;
- resolução de símbolos;
- aplicação de relocações;
- construção de `.so`;
- compatibilidade com qualquer kernel ou política SELinux;
- execução no Android físico;
- instalador APK funcional.

```text
EXECUTABLE_STRUCTURE != GENERAL_LINKER != DEVICE_RUNTIME
```

## Próxima fronteira

```text
stub fixo
-> seção .text e .shstrtab
-> tabela de símbolos local
-> relocações ARM/AArch64 delimitadas
-> entrada de código controlada
-> linker estático mínimo
-> fixtures negativas
-> execução QEMU controlada
-> coleta física Android não destrutiva
```

A promoção de cada etapa exige fonte, teste independente e evidência específica.
