# R3: Estratégia de Rebuild Prefix-Safe para RAFCODEΦ

**Data**: 2026-08-06  
**Status**: STAGED (documentação apenas; rebuild requer termux-packages externo)  
**Prioridade**: P0 — Bloqueador principal  

---

## Problema Raiz

### Condição Atual

A distribuição Termux padrão codifica caminhos absolutos nos ELFs durante compilação:

```
Termux upstream (com.termux):
  RUNPATH/RPATH = /data/data/com.termux/files/usr/lib
  NEEDED: libc.so (em /data/data/com.termux/files/usr/lib)
```

RAFCODEΦ usa namespace separado:

```
RAFCODEΦ (com.termux.rafacodephi):
  PREFIX = /data/data/com.termux.rafacodephi/files/usr
  RPATH/RUNPATH deve apontar para: /data/data/com.termux.rafacodephi/files/usr/lib
```

### Por Que Não É Possível Patchear In-Place

A string `"/data/data/com.termux/files/usr"` (36 bytes) é **fisicamente maior** que substituições imediatas:

- `strlen("/data/data/com.termux/files/usr") = 36`
- `strlen("/data/data/com.termux.rafacodephi/files/usr") = 45`

Patchear ELFs após compilação:
- Quebra assinaturas (se presentes)
- Desloca seções e quebra offsets
- Invalida checksums de debug info
- Não é reproduzível

**Solução obrigatória**: Recompilação com `--prefix` correto.

---

## Solução: Recompilação Prefix-Safe

### 1. Arquitetura da Build Chain

```
├── termux-packages.git (upstream ou fork)
│   └── packages/
│       ├── libc/
│       ├── libstdc++/
│       ├── apt/               ← CRÍTICO: gerenciador de pacotes
│       ├── apt-get/           ← wrapper/symlink
│       ├── dpkg/              ← CRÍTICO: gestor de .debs
│       ├── ca-certificates/   ← DNS/TLS
│       └── proot/ (opcional)  ← sandbox
│
├── NDK r26+ (clang, aarch64-linux-android28)
├── Build flags: --prefix=/data/data/com.termux.rafacodephi/files/usr
└── Output:
    └── debs/ (ARM32 + ARM64)
        ├── libstdc++_*.deb
        ├── libc_*.deb
        ├── apt_*.deb
        ├── dpkg_*.deb
        ├── ca-certificates_*.deb
        └── proot_*.deb (optional)
```

### 2. Configuração de Build Flags

**CFLAGS & LDFLAGS mínimas**:

```bash
export ANDROID_NDK="${NDK_HOME:-/opt/android-ndk-r26}"
export CC="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android28-clang"
export CXX="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android28-clang++"

export CFLAGS="-O2 -fno-stack-protector -fno-strict-aliasing"
export CXXFLAGS="$CFLAGS"
export LDFLAGS="-Wl,-z,max-page-size=16384 -Wl,-z,relro -Wl,-z,now"

CONFIGURE_FLAGS="
  --prefix=/data/data/com.termux.rafacodephi/files/usr
  --host=aarch64-linux-android
  --with-libc
  --disable-nls
  --disable-rpath
"
```

### 3. Sequência de Build (Ordem Crítica)

```
R3.1 — Setup: Clonar termux-packages, verificar NDK
R3.2 — libc: Compilar glibc/musl com prefix-safe
R3.3 — libstdc++: Compilar libstdc++ com libc prefix-safe
R3.4 — ca-certificates: Compilar ou incorporar ca-bundle
R3.5 — dpkg: Compilar dpkg (gestor de .deb)
R3.6 — apt: Compilar apt (gestor de pacotes)
R3.7 — proot: [OPCIONAL] Compilar proot (sandbox)
R3.8 — Validação: Rodar validate_deb_prefix_safe.sh em todos
R3.9 — Empacotamento: Gerar bootstrap real-pkg.zip com debs
R3.10 — Assinatura: BLAKE3-sign todos artefatos
```

### 4. Validação Pós-Build

Cada arquivo `.deb` DEVE passar em:

```bash
scripts/validate_deb_prefix_safe.sh libc_1.2.3_arm64.deb
scripts/validate_deb_prefix_safe.sh apt_2.6.1_arm64.deb
scripts/validate_deb_prefix_safe.sh dpkg_1.21.x_arm64.deb
```

**Critério de aceitação**:
- Nenhum ELF contém `/data/data/com.termux/files/usr`
- Todos os RUNPATH/RPATH apontam para `/data/data/com.termux.rafacodephi/files/usr`
- SHA-256 reproduzível (mesmo commit = mesmo hash)

---

## Artefatos Esperados

```
build-output/debs/
├── libc_*.deb                              # Versão ARM32 + ARM64
├── libstdc++_*.deb
├── ca-certificates_*.deb
├── dpkg_1.21.x_arm64.deb                   # CRÍTICO
├── dpkg_1.21.x_armv7a.deb                  # CRÍTICO
├── apt_2.6.1_arm64.deb                     # CRÍTICO
├── apt-get_2.6.1_arm64.deb
├── proot_5.4.x_arm64.deb                   # OPCIONAL
│
└── SHA256SUMS                              # BLAKE3-signed
```

**Entrega esperada para R4**:
- `bootstrap-real-pkg-arm32.zip` (contém debs ARM32)
- `bootstrap-real-pkg-arm64.zip` (contém debs ARM64)
- Cada com `SHA256SUMS` + assinatura

---

## Decision Points (Bloqueadores)

| Decisão | Opções | Impacto | Recomendação |
|---------|--------|--------|--------------|
| **Termux fork vs. upstream** | `rafaelmeloreisnovo/termux-packages` vs. `termux/termux-packages` | Rastreabilidade, sincronismo | Usar fork próprio com commit pinned |
| **NDK versão** | r21, r23c, r26 | Compatibilidade clang, flags | Verificar em `app/build.gradle`; usar r26+ |
| **musl vs. glibc** | Termux usa musl | Tamanho, compatibilidade | Usar musl (padrão Termux) |
| **ca-certificates** | Embutido vs. Mozilla store | TLS/DNS funcional | Embutir ca-bundle mínimo |
| **ARM32 multilib** | Sim/não | Compatibilidade E7 | Sim, tanto ARM32 quanto ARM64 |
| **proot** | Compilar ou skip | Sandbox container | Skip (R3.x) — adicionar em R4 se needed |

---

## Possíveis Armadilhas & Mitigações

### Armadilha 1: Rpath Hardcoded vs. RUNPATH

**Problema**: Se `configure` gera RPATH fixo, não respeita `--prefix`

**Mitigação**:
```bash
./configure --prefix=/data/data/com.termux.rafacodephi/files/usr \
  --disable-rpath  # força RUNPATH dinâmica
```

Validar com:
```bash
readelf -d libc.so.6 | grep -E 'RUNPATH|RPATH'
```

### Armadilha 2: Cross-Compilation Path Issues

**Problema**: Configure pode usar paths do host (Linux x86_64) em vez de target (Android ARM)

**Mitigação**:
```bash
./configure --host=aarch64-linux-android \
  --build=x86_64-linux-gnu \
  --target=aarch64-linux-android
```

### Armadilha 3: Symbol Visibility & Stripping

**Problema**: `strip` pode danificar RUNPATH ou relocations

**Mitigação**:
```bash
# Não fazer strip; deixar símbolos para debug
# Se precisar reduzir tamanho: usar --strip-all com cautela
objcopy --only-keep-debug libc.so.6 libc.so.6.debug
```

### Armadilha 4: Reprodutibilidade

**Problema**: Timestamps em .a e .so quebram determinismo

**Mitigação**:
```bash
SOURCE_DATE_EPOCH="$(git log -1 --format='%ct' <commit>)"
export SOURCE_DATE_EPOCH
# Recompila com timestamps fixos
```

---

## Referência: Comandos de Validação

### Verificar se ELF tem prefix legado

```bash
#!/bin/sh
file="${1:?usage: $0 <elf_file>}"
readelf -d "$file" 2>/dev/null | grep -q "/data/data/com.termux/files/usr" && {
  echo "FAIL: contains legacy prefix"
  exit 1
} || {
  echo "OK: no legacy prefix detected"
}
```

### Verificar RUNPATH/RPATH

```bash
readelf -d app.elf | grep -E 'RUNPATH|RPATH|NEEDED'
# Esperado: RUNPATH=/data/data/com.termux.rafacodephi/files/usr/lib
```

### Calcular SHA-256 reproduzível

```bash
git log -1 --format='%ct' | xargs -I {} date -d @{} -u +%Y-%m-%dT%H:%M:%SZ
SOURCE_DATE_EPOCH="$(git log -1 --format='%ct')"
sha256sum libc.so.6
```

---

## Próximas Fases (R4+)

- **R4**: Bootstrap real-pkg (inserir debs em APK)
- **R5**: Rebuild APK + testes locais
- **R6-R8**: Testes ARM32/ARM64 físicos
- **R9+**: CI, assinatura, release

---

## Referências Internas

- `app/build.gradle` — NDK version, page-size flags
- `rmr/Rrr/Android_nomalloc.mk` — NDK build rules pattern
- `scripts/build_apk_matrix.sh` — Build invocation reference
- `docs/RAFAELIA_METHODOLOGY.md` — Determinism principles
- `runtime-lock.json` — Commit pinning structure
