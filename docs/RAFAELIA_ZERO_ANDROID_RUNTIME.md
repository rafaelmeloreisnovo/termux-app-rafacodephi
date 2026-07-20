# RAFAELIA ZERO no Termux RAFCODEΦ

## Cadeia executável

```text
conversation chunk
→ byte[] reutilizado ou DirectByteBuffer
→ frame RFZ1 little-endian
→ CRC32C de header e payload
→ anel nativo fixo de 42 slots
→ 8 lanes inteiras
→ 7 fases
→ recorrência Q16
→ digest de estado
```

## Autoridade única

O núcleo C não foi reescrito no aplicativo. `app` e `:rafaelia` compilam os mesmos arquivos:

```text
rafaelia/src/main/cpp/zero/include/rafz.h
rafaelia/src/main/cpp/zero/rafz.c
```

Esses dois blobs são byte a byte iguais aos blobs integrados no `Rafaelia_Private` pelo PR #186. O arquivo `PROVENANCE.json` fixa repositório, merge commit, caminhos e Git blob SHAs.

## Memória

O núcleo não usa:

```text
malloc/calloc/realloc/free
libc
arquivo
syscall
GC
RTTI
reflexão
plugin
função virtual
```

O estado e o frame temporário são estáticos na casca JNI. A API canônica recebe `DirectByteBuffer`; a rota `byte[]` usa um único staging buffer direto criado durante a inicialização da classe, sem nova alocação por ingestão.

Isso não afirma que ART, JVM, linker Android ou driver do sistema não usem heap. A ausência de heap é uma propriedade do core e da memória criada pela casca nativa.

## Bibliotecas

| Camada | Biblioteca | Entrada Java |
|---|---|---|
| módulo `:rafaelia` | `libtermux_rafaelia_zero.so` | `com.termux.rafaelia.RafaeliaZero` |
| APK principal | `libtermux_rafaelia_zero_runtime.so` | `com.termux.app.rafaelia.RafaeliaZeroRuntime` |

O APK principal compila a fonte canônica diretamente pelo `app/src/main/cpp/Android.mk`; ele não mantém uma segunda cópia do código C.

## Inicialização

`TermuxApplication` chama:

```java
RafaeliaZeroRuntime.init();
```

A inicialização executa:

1. carregamento da `.so`;
2. autoteste Bagua `ROL3/ROR3`;
3. vetor CRC32C conhecido `123456789 → 0xE3069283`;
4. inicialização do anel e das lanes;
5. registro da arquitetura compilada.

Falha do núcleo é registrada, mas não derruba automaticamente o bootstrap Termux.

## ABIs Android

A casca está preparada para:

```text
armeabi-v7a → ARMv7-A + NEON-VFPv4
arm64-v8a  → ARMv8-A + SIMD
x86        → i686 + SSE2
x86_64     → x86-64 + SSE2
```

As flags permanecem baseline dentro de cada ABI; AVX e extensões opcionais não são exigidos no runtime RAFAELIA ZERO.

## Uso

### DirectByteBuffer

```java
ByteBuffer chunk = ByteBuffer.allocateDirect(1024);
chunk.put(data);
chunk.flip();
int status = RafaeliaZeroRuntime.ingestDirect(
    chunk,
    chunk.remaining(),
    sourceId,
    sequence,
    flags);
```

### Buffer reutilizado de bytes

```java
int status = RafaeliaZeroRuntime.ingest(
    chunkBytes,
    chunkLength,
    sourceId,
    sequence,
    flags);
```

## Estado da prova

```text
core privado executado em host x86_64       PASS
blobs C preservados no repositório Termux   PASS
sintaxe C + JNI no host                     PASS
sintaxe Java no host                        PASS
wiring Android.mk                           PASS estático
inicialização TermuxApplication             PASS estático
build real pelo Android NDK                  TOKEN_VAZIO
APK instalado                               TOKEN_VAZIO
ingestão em aparelho                        TOKEN_VAZIO
benchmark NEON/SSE2                         TOKEN_VAZIO
```

A cross-compilação anterior para sete ISAs continua sendo evidência da portabilidade do core. Este documento delimita apenas a integração Android real nas quatro ABIs do APK.
