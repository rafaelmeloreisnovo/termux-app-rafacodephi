# RAFCODE-Φ — Coerência Operacional Técnica

**Parte:** 2 da auditoria incremental de fechamento de lacunas  
**Repositório:** `rafaelmeloreisnovo/termux-app-rafacodephi`  
**Matriz canônica:** `configs/operational-technical-coherence.json`  
**Gate principal:** `bash scripts/test_raf_native_compile_contract.sh`

## 1. Invariante

```text
arquivo presente
!= fonte integrada
!= unidade compilada
!= artefato produzido
!= execução comprovada
!= conformidade certificada
```

Cada alegação operacional precisa declarar fonte, produtor, consumidor, artefato,
verificador independente, evidência, limitações e próxima ação.

## 2. O que esta parte realmente acrescenta

Esta etapa não transforma protótipos em produtos completos. Ela fecha quatro lacunas
estruturais que antes estavam misturadas:

1. emissor ELF autoral mínimo, separado do ELF produzido pelo NDK;
2. divergência explícita entre DEX estrutural e DEX executável;
3. matriz por linguagem que impede chamar perfil/rota de compilador completo;
4. promoção governada de documentos e fontes soltas, sem movimentação automática.

Também cria percentuais calculados por critérios de evidência, em vez de percentuais
subjetivos de prontidão.

## 3. Cadeias Android que não devem ser confundidas

### 3.1 APK Android produzido pela toolchain oficial

```text
Java/Kotlin
-> javac/kotlinc
-> D8/R8
-> classes.dex

C/ASM
-> NDK clang/assembler
-> linker ELF
-> lib/<abi>/*.so

manifesto + resources + DEX + ELF
-> APK
-> assinatura
```

Essa cadeia está comprovada em CI para ARM32, ARM64 e APK universal. Ela prova a
integração com toolchains externas. Não prova compiladores Java, Kotlin, C ou ASM
implementados dentro de `apkc/`.

## 4. DEX

### 4.1 Estado comprovado

`apkc/fmt_dex.h` produz um contêiner DEX 035 mínimo com:

- header;
- `map_list`;
- assinatura SHA-1;
- checksum Adler-32;
- verificação de ponteiro e capacidade.

A prova independente é:

```text
C emitter
-> classes.dex
-> scripts/validate_apkc_dex_contract.py
-> magic/header/map/data/SHA-1/Adler-32
```

### 4.2 Divergência ainda aberta

O contêiner deliberadamente não possui:

- strings;
- tipos;
- protótipos;
- campos;
- métodos;
- classes;
- `code_item`;
- bytecode executável;
- multidex;
- prova ART/Dalvik.

Portanto:

```text
DEX_STRUCTURAL_CONTAINER = VERIFIED_HOST
DEX_EXECUTABLE_CONTENT   = TOKEN_VAZIO
JAVA_COMPILER            = TOKEN_VAZIO dentro de APKC
KOTLIN_COMPILER          = TOKEN_VAZIO dentro de APKC
```

## 5. ELF

Existem agora três corpos diferentes.

### 5.1 ELF produzido pelo Android NDK

```text
state = PROVEN_CI
scope = bibliotecas nativas ARM32/ARM64 empacotadas no APK
```

### 5.2 Emissor ELF estrutural autoral do APKC

Fonte: `apkc/fmt_elf.h`.

O emissor produz, com limites de capacidade:

- ELF32 little-endian;
- `ET_REL`;
- `EM_ARM`;
- EABI5;
- ELF64 little-endian;
- `ET_REL`;
- `EM_AARCH64`;
- header ELF;
- uma única section header nula obrigatória.

A prova independente é:

```text
tests/native/apkc_emit_minimal_elf.c
-> apkc-arm32.o
-> apkc-arm64.o
-> scripts/validate_apkc_elf_contract.py
```

O validador confere classe, endianness, ABI, máquina, tipo, offsets, tamanhos,
ausência de program headers e section header nula.

```text
APKC_STRUCTURAL_ELF32_ARM     = VERIFIED_HOST após gate
APKC_STRUCTURAL_ELF64_AARCH64 = VERIFIED_HOST após gate
```

### 5.3 ELF executável, shared object e linker

Ainda não existem no writer autoral:

- seções nomeadas;
- tabela de strings;
- símbolos;
- relocações;
- segmentos `PT_LOAD`;
- entrypoint;
- dynamic section;
- resolução de símbolos;
- linker;
- execução controlada.

```text
APKC_EXECUTABLE_ELF_WRITER = TOKEN_VAZIO
APKC_LINKER                = TOKEN_VAZIO
APKC_DYNAMIC_LOADER        = TOKEN_VAZIO
```

O avanço desta parte é real, mas delimitado: o estado passou de ausência total para
um emissor estrutural verificável, não para executável funcional.

## 6. Compiladores por linguagem

A matriz canônica é:

```text
configs/compiler-capability-matrix.json
```

O verificador é:

```text
python3 scripts/validate_compiler_capability_matrix.py --pretty
```

Cada linguagem possui dez fases obrigatórias:

1. source reader;
2. lexer;
3. parser;
4. AST;
5. análise semântica;
6. IR;
7. optimizer;
8. backend;
9. link/package;
10. runtime tests.

A regra fail-closed é:

```text
complete_compiler=true
ONLY IF
cada fase possui estado aceito
AND evidência não vazia
```

A matriz acompanha C, C++, Assembly, Java, Kotlin, Rust, Go, Python, Lua,
JavaScript, TypeScript, Ruby, PHP e Swift. Nenhum nome de linguagem é promovido
por extensão, comentário, tabela ou rota.

Estado atual:

```text
APKC-owned complete compilers = 0
Gradle/D8/R8                   = external toolchain proven in CI
NDK clang/assembler/linker     = external toolchain proven in CI
```

## 7. Navegador ASM e TLS

`Browser.sh` permanece um protótipo freestanding orientado a syscalls com elementos
reais de DNS, TCP, HTTP/1.1 e renderização textual. O caminho canônico remove o
downgrade silencioso HTTPS -> HTTP:

```bash
python3 scripts/materialize_browser_fail_closed.py \
  --output /tmp/raf-browser-build-safe.sh
python3 scripts/validate_browser_fail_closed.py
```

Estado correto:

```text
HTTP_TEXTMODE_SOURCE          = IMPLEMENTED_UNPROVEN_RUNTIME
HTTPS_FAIL_CLOSED_GATE        = VERIFIED_HOST
TLS_1_2_FUNCTIONAL            = TOKEN_VAZIO
TLS_1_3_CLIENTHELLO_PROTOTYPE = PARTIAL
TLS_1_3_CRYPTOGRAPHY          = TOKEN_VAZIO
X509_PATH_VALIDATION          = TOKEN_VAZIO
TLS_CERTIFICATION             = TOKEN_VAZIO
```

Para TLS 1.3 funcional ainda são necessários entropy, X25519, transcript hash,
HKDF, AEAD, parsing de handshake, X.509, hostname verification, CertificateVerify,
Finished e interoperabilidade. TLS 1.2 exige uma implementação própria equivalente
ou integração explícita com uma biblioteca auditada. Certificação exige evidência
externa; comentários e testes internos não certificam protocolo.

## 8. Arquivos e documentos soltos

Política:

```text
configs/loose-artifact-policy.json
```

Indexador:

```bash
python3 scripts/index_loose_operational_artifacts.py \
  --validate \
  --output build/reports/loose-artifacts.json
```

Cada registro v2 contém:

```yaml
artifact_id:
path:
object_type:
content_sha256:
size_bytes:
status:
origin:
author:
license:
references:
review_flags:
promotion_blockers:
promotion_ready:
build_consumer:
integration_target:
evidence_state:
claim_allowed:
next_action:
```

O indexador detecta referências textuais e conteúdo duplicado, mas mantém como
`TOKEN_VAZIO` tudo que não puder ser provado. Para promoção canônica são exigidos:

- origem;
- autoria;
- licença;
- referências revisadas;
- destino aprovado;
- consumidor identificado;
- testes identificados.

```text
INDEXING_DOES_NOT_PROMOTE_TO_BUILD_OR_RUNTIME
AUTOMATIC_MOVE            = false
AUTOMATIC_DELETE          = false
AUTOMATIC_CLAIM_PROMOTION = false
```

Assim, os documentos podem completar módulos futuros, mas somente depois de revisão
de proveniência, licença, referência, destino, consumidor e teste.

## 9. Percentuais auditáveis

Mapa:

```text
configs/first-part-gap-map.json
```

Validação e relatório:

```bash
python3 scripts/validate_first_part_gap_map.py --pretty
python3 scripts/validate_first_part_gap_map.py --format markdown
```

A fórmula é:

```text
coverage = soma dos pesos comprovados / número de critérios
PROVEN      = 1.0
PARTIAL     = 0.5
TOKEN_VAZIO = 0.0
BLOCKED     = 0.0
```

Esse percentual mede cobertura de evidência registrada. Ele não significa prontidão
comercial, execução física, segurança total ou certificação.

## 10. Gate canônico

```bash
bash scripts/test_raf_native_compile_contract.sh
```

O gate executa:

1. núcleo numérico;
2. ECC32 compacto;
3. ECC32 desenrolado;
4. emissor e validador DEX;
5. emissor ELF32 ARM e validador independente;
6. emissor ELF64 AArch64 e validador independente;
7. matriz dos compiladores;
8. mapa de cobertura da primeira parte;
9. coerência operacional;
10. HTTPS fail-closed;
11. índice de arquivos soltos;
12. linker GC;
13. contrato de warnings.

## 11. Estado consolidado

| Corpo | Estado correto |
|---|---|
| APK ARM32/ARM64/universal | `PROVEN_CI` |
| ELF produzido pelo NDK | `PROVEN_CI` |
| ELF32/ELF64 estrutural APKC | `VERIFIED_HOST` após gate |
| ELF executável/linker APKC | `TOKEN_VAZIO` |
| DEX estrutural vazio | `VERIFIED_HOST` |
| DEX com classes e bytecode | `TOKEN_VAZIO` |
| compiladores completos APKC | `0`, `TOKEN_VAZIO` por linguagem |
| HTTP textual ASM | `IMPLEMENTED_UNPROVEN_RUNTIME` |
| HTTPS fail-closed | `VERIFIED_HOST` |
| TLS 1.2 funcional | `TOKEN_VAZIO` |
| TLS 1.3 funcional | `TOKEN_VAZIO` |
| certificação TLS | `TOKEN_VAZIO` |
| documentos soltos indexados | `VERIFIED_HOST` para inventário |
| documentos soltos promovidos | `TOKEN_VAZIO` até revisão |
| device runtime | `TOKEN_VAZIO` |
| release de produção | `BLOCKED` |

## 12. Próxima fronteira técnica

```text
ELF:
null ET_REL
-> sections/string table
-> symbols
-> relocations ARM/AArch64
-> code section
-> PT_LOAD executable
-> controlled runtime

DEX:
empty container
-> string/type/proto pools
-> method/class definitions
-> code_item
-> verifier fixtures
-> ART runtime

TLS:
fail-closed
-> cryptographic modules
-> handshake
-> certificate validation
-> interoperability
-> external conformance evidence

COMPILERS:
per-language map
-> one language selected
-> complete frontend
-> IR
-> one proven backend
-> APK end-to-end
```

```text
release_allowed = false
claim_allowed only inside each proven scope
```
