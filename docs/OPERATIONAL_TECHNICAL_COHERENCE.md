# RAFCODE-Φ — Coerência Operacional Técnica

**Parte:** 1 de uma auditoria incremental de fechamento de lacunas  
**Repositório:** `rafaelmeloreisnovo/termux-app-rafacodephi`  
**Matriz canônica:** `configs/operational-technical-coherence.json`  
**Gate:** `python3 scripts/validate_operational_technical_coherence.py`

## 1. Invariante

```text
arquivo presente
!= fonte integrada
!= unidade compilada
!= artefato produzido
!= execução comprovada
!= conformidade certificada
```

Consequentemente, nomes como `browser`, `kernel`, `TLS`, `ELF`, `DEX`, `compiler`
ou `loader` não promovem automaticamente a implementação ao estado funcional.

Cada corpo precisa declarar:

1. fonte canônica;
2. produtor;
3. consumidor;
4. artefato gerado;
5. verificador independente;
6. evidência;
7. limitações;
8. próxima ação.

## 2. Cadeias que não devem ser confundidas

### 2.1 APK Android real

```text
Java/Kotlin
-> javac/kotlinc
-> D8/R8
-> classes.dex

C/ASM
-> NDK clang/assembler
-> linker ELF
-> lib/*.so

manifesto + resources + DEX + ELF
-> APK
-> assinatura
```

O pipeline Android/NDK comprovado no PR #282 valida APKs ARM32, ARM64 e universal,
bibliotecas ELF presentes e assinatura de desenvolvimento. Isso não prova um emissor ELF
autoral dentro de `apkc/` e não prova execução em aparelho físico.

### 2.2 Gerador DEX estrutural do APKC

`apkc/fmt_dex.h` produz um contêiner DEX 035 mínimo:

```text
header
+ map_list
+ SHA-1
+ Adler-32
```

O artefato não contém:

- strings;
- tipos;
- protótipos;
- campos;
- métodos;
- classes;
- bytecode executável.

Portanto:

```text
DEX estrutural válido
!= compilador Java
!= compilador Kotlin
!= APK funcional
```

Nesta parte foram corrigidos:

- comprimento original da mensagem no padding SHA-1;
- `data_size`;
- `data_off`;
- verificação de ponteiro/capacidade;
- adaptação explícita para teste hospedado, sem mudar o runtime ARM de produção.

A prova é independente:

```text
C emitter
-> arquivo classes.dex
-> Python hashlib/zlib/struct
-> assinatura, checksum, header e map_list
```

## 3. Navegador ASM

`Browser.sh` contém componentes reais de protótipo:

- syscalls por arquitetura;
- socket TCP;
- DNS autoral simplificado;
- construtor HTTP/1.1;
- parser parcial de resposta;
- renderer textual HTML;
- arena estática;
- geração freestanding para ARM32/ARM64/x86-64.

Isso permite classificá-lo como:

```text
HTTP text-mode source prototype
```

Ainda não permite classificá-lo como navegador HTTPS funcional.

## 4. Estado verdadeiro de TLS

### 4.1 O que existe

O código gera parte de um `ClientHello` com:

- `legacy_version = 0x0303`;
- extensão `supported_versions = 0x0304`;
- SNI;
- lista de grupos;
- lista de algoritmos de assinatura;
- cipher suites TLS 1.3;
- parser do cabeçalho de record;
- máquina de estados demonstrativa.

### 4.2 O que não existe

Não foram encontrados, nesse corpo:

- aleatoriedade criptograficamente segura;
- `key_share` X25519 completo;
- cálculo do segredo compartilhado;
- transcript hash;
- HKDF Extract/Expand;
- derivação de handshake/application traffic secrets;
- AES-GCM ou ChaCha20-Poly1305 operacional;
- autenticação do record;
- parse e validação da cadeia X.509;
- hostname verification;
- trust store;
- verificação de `CertificateVerify`;
- verificação de `Finished`;
- proteção contra downgrade;
- fechamento correto de alertas;
- testes de interoperabilidade com servidores reais.

O próprio código registra que a criptografia não foi implementada e possui fallback
para HTTP demonstrativo. Portanto:

```text
TLS 1.3 = ClientHello/record prototype
TLS 1.2 = legacy-version constants only
TLS certification = TOKEN_VAZIO
```

Nenhum banner, comentário, compilação ou teste interno equivale a certificação externa.

## 5. Regra de segurança para a próxima etapa do navegador

A próxima alteração no runtime deve substituir:

```text
HTTPS solicitado
-> handshake incompleto
-> HTTP silencioso
```

por:

```text
HTTPS solicitado
AND TLS completo ausente
-> FAIL_CLOSED
-> erro explícito
-> nenhum request plaintext
```

Depois, a implementação TLS deve ser dividida em módulos verificáveis:

```text
entropy
x25519
sha256/sha384
hkdf
record-aead
handshake-transcript
x509/path-validation
hostname-verification
interoperability-tests
```

## 6. ELF

Há duas classes diferentes:

### A. ELF produzido pelo NDK

Estado: comprovado em CI para as bibliotecas nativas empacotadas nos APKs ARM32/ARM64.

### B. Emissor ELF autoral do APKC

Estado: `TOKEN_VAZIO` nesta matriz.

A existência de `.so` produzido pelo linker Android não demonstra que `apkc/` possua um
writer ELF completo. Para promover esse corpo serão necessários:

- fonte canônica identificada;
- ELF32 e/ou ELF64 explicitamente delimitados;
- headers e program headers válidos;
- máquina/ABI corretas;
- alinhamento;
- segmentos carregáveis;
- relocação ou declaração explícita de ausência;
- `readelf -h -l -S -s -r`;
- execução controlada;
- hashes e fixtures negativas.

## 7. Perfis de linguagens

O repositório contém perfis, rotas, tabelas e emissores mínimos associados a várias
linguagens. Isso não significa que cada nome possua:

```text
lexer
+ parser
+ AST
+ análise semântica
+ IR
+ otimização
+ backend
+ linker/package
+ runtime tests
```

A classificação canônica é:

```text
language profile/router = STRUCTURAL_PRIMITIVE
complete compiler = TOKEN_VAZIO por linguagem, salvo prova específica
```

## 8. Documentos e arquivos soltos

Os caminhos a seguir contêm memória útil, mas não são automaticamente build ativo:

```text
Arme/
BugOrAdd/
rafaelia/old/
root-level *.txt
root-level documentos históricos
```

Cada item precisa entrar em um índice futuro com:

```yaml
artifact_id:
path:
object_type:
origin:
author:
license:
status: CANONICAL | CANDIDATE | DUPLICATE | HISTORICAL | QUARANTINE
build_consumer:
integration_target:
evidence:
next_action:
```

Até esse índice existir:

```text
presença = memória
presença != integração
presença != runtime
```

## 9. Estados desta parte

| Corpo | Estado |
|---|---|
| APK ARM32/ARM64/universal | `PROVEN_CI` |
| ELF empacotado pelo NDK | `PROVEN_CI` |
| DEX estrutural `apkc/fmt_dex.h` | `VERIFIED_HOST` após gate |
| compiladores completos por linguagem | `TOKEN_VAZIO` |
| emissor ELF autoral APKC | `TOKEN_VAZIO` |
| browser HTTP textual | `IMPLEMENTED_UNPROVEN_RUNTIME` |
| TLS 1.3 | `PROTOTYPE_FAIL_CLOSED_REQUIRED` |
| TLS 1.2 | `DOCUMENT_ONLY` |
| certificação TLS | `TOKEN_VAZIO` |
| loader APK | `STUB_NO_BOOTSTRAP_PAYLOAD` |
| arquivos soltos | `HISTORICAL_OR_LOOSE` |

## 10. Gate canônico

```bash
bash scripts/test_raf_native_compile_contract.sh
```

Esse gate passa a:

1. testar o núcleo numérico;
2. testar os dois perfis ECC32;
3. compilar o emissor DEX em modo de contrato hospedado;
4. validar o DEX por implementação independente;
5. validar a matriz de coerência operacional;
6. preservar os testes de warnings e linker GC.

## 11. Limites

Esta parte não declara concluídos:

- navegador TLS;
- certificação TLS;
- emissor ELF autoral;
- compilador geral de múltiplas linguagens;
- instalação funcional pelo loader;
- execução física ARM32/ARM64;
- migração automática dos documentos soltos.

Esses itens agora possuem estado, limite e próxima ação explícitos; não permanecem mais
misturados como uma única alegação.

---

```text
F_ok   = DEX corrigido + mapa operacional + gate independente
F_gap  = TLS completo, ELF autoral, device runtime e indexação dos arquivos soltos
F_next = remover fallback plaintext e construir o índice de artefatos soltos
```
