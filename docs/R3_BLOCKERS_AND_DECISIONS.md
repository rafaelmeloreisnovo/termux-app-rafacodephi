# R3: Bloqueadores & Pontos de Decisão

**Data**: 2026-08-06  
**Etapa**: R3 Preparação (antes de rebuild físico)  
**Responsabilidade**: Documented antes de clonar termux-packages  

---

## Bloqueadores Críticos (P0)

### B1: Repositório termux-packages — Qual versão clonar?

**Estado**: ✅ RESOLVIDO (R3.1, 2026-08-06)
**Impacto**: Determina quais fontes serão compiladas  

**Opções**:

| Opção | URL | Vantagem | Risco |
|-------|-----|---------|-------|
| Upstream | `https://github.com/termux/termux-packages.git` | Comunidade mantida, patches recentes | Não garantida reprodutibilidade RAFCODEΦ |
| Fork próprio | `https://github.com/rafaelmeloreisnovo/termux-packages.git` | Controle total, reproducível | **Repositório vazio** (verificado via `git ls-remote`) |

**Decisão real (R3.1)**: O fork `rafaelmeloreisnovo/termux-packages` existe mas está **vazio** — `git ls-remote --heads` não retorna nenhuma branch. Como não há nada para clonar do fork, foi usado o **upstream** como fonte, com o commit pinado em `runtime-lock.json`:

```
url:    https://github.com/termux/termux-packages.git
branch: master
commit: eb124b51a949c7a0943275a18f94371e69756449
```

Se reprodutibilidade estrita exigir um fork privado no futuro, popular o fork a partir deste commit exato e atualizar `runtime-lock.json.repositories[].fork_url`/`fork_status`.

**Verificação (já executada, sem clone completo)**:
```bash
git ls-remote https://github.com/rafaelmeloreisnovo/termux-packages.git HEAD  # vazio
git ls-remote https://github.com/termux/termux-packages.git HEAD             # eb124b51a949c7a0943275a18f94371e69756449
```

---

### B2: NDK Versão & Toolchain Path

**Estado**: ⚠️ VERIFICADO E BLOQUEADO (R3.2, 2026-08-06) — versão confirmada, mas NDK **não está instalado** no host de build usado nesta sessão.

**Impacto**: Compatibilidade clang, flags disponíveis, size binários  

**Correção importante**: A versão documentada anteriormente aqui (`26.2.11900637`, citada de `app/build.gradle`) estava **desatualizada**. A fonte de verdade real é `gradle.properties`:

```properties
ndkVersion=26.3.11579264
compileSdkVersion=35
targetSdkVersion=28
minSdkVersion=21
```

`app/build.gradle` apenas lê `ndkVersion` de `gradle.properties` via `project.properties.ndkVersion` — não a declara diretamente. `runtime-lock.json.toolchain.ndk_version` foi corrigido de `"r26+"` para `"26.3.11579264"`.

**Script de verificação criado**: `scripts/verify_ndk_r26_installation.sh` (read-only, não instala nada)
- Lê `ndkVersion` de `gradle.properties`
- Resolve `$ANDROID_NDK` / `$ANDROID_NDK_HOME` / `$ANDROID_NDK_ROOT` / `$ANDROID_SDK_ROOT/ndk/<version>`
- Confirma versão real via `source.properties` dentro do NDK (não confia só no nome do path)
- Verifica presença dos compiladores cross exigidos pela matriz ABI (`armeabi-v7a` + `arm64-v8a`):
  - `aarch64-linux-android${targetSdkVersion}-clang`
  - `armv7a-linux-androideabi${targetSdkVersion}-clang`

**Resultado real desta execução** (host Linux x86_64 desta sessão):
```
status=NDK_NOT_FOUND
expected_ndk_version=26.3.11579264
r3_gate=R3.2_BLOCKED
```

Nenhum SDK/NDK Android está instalado neste ambiente de build. Isso é um achado honesto, não uma falha do script — não existe simulação de "PASS" quando o pré-requisito real não está presente.

**Remediação**: Rodar `scripts/setup_android_toolchain.sh` (já existente no repositório) em um host com acesso ao Android SDK/`sdkmanager`, que instala `ndk;26.3.11579264` a partir de `gradle.properties`. Depois, re-rodar `scripts/verify_ndk_r26_installation.sh` para confirmar `r3_gate=R3.2_COMPLETE` antes de iniciar R3.4 (build real).

**Decisão**: Usar NDK `26.3.11579264` (não `r26+` genérico) para reprodutibilidade exata, conforme `gradle.properties`.

---

### B3: Configuração Exata de Flags (--prefix, LDFLAGS, etc.)

**Estado**: Draft na estratégia R3  
**Impacto**: Reprodutibilidade, tamanho, performance  

**Mínimo obrigatório**:
```bash
CONFIGURE="--prefix=/data/data/com.termux.rafacodephi/files/usr"
LDFLAGS="-Wl,-z,max-page-size=16384 -Wl,-z,relro -Wl,-z,now"
CC="clang-18" CXX="clang++-18"
```

**Verificação**: Cada compilação deve gerar:
```bash
git log -1 --format='%ct' > SOURCE_DATE_EPOCH
./configure --prefix=... --host=aarch64-linux-android
make install DESTDIR=$TMPDIR
readelf -d $TMPDIR/.../*.so | grep RUNPATH  # Deve ser /data/data/com.termux.rafacodephi/...
```

---

### B4: ARM32 vs ARM64 vs Universal

**Estado**: Ambas necessárias  
**Impacto**: Coverage de devices, tamanho bootstrap  

**Decisão**:
- **ARM32** (armeabi-v7a): Moto E7 (Helio G25) — `armv7a-linux-android`
- **ARM64** (arm64-v8a): Realme Note 50 — `aarch64-linux-android28`
- **Ambas**: Em bootstrap.zip, separadas por ABI

**Build Matrix**:
```
R3.Build {
  for abi in armv7a aarch64; do
    configure --prefix=... --host=$abi-linux-android
    make -j$(nproc)
    make install DESTDIR=debs-$abi
  done
}
```

---

### B5: Qual Package Closure Mínimo?

**Estado**: Definido em estratégia R3  
**Impacto**: Funcionalidade pkg real  

**Obrigatório**:
- `libc` (musl ou glibc)
- `libstdc++`
- `dpkg` (gestor de .deb)
- `apt` (gestor de pacotes)
- `ca-certificates` (TLS/DNS)

**Opcional** (R3.x):
- `proot` (sandbox)
- `curl` (downloads)
- `grep`, `sed` (utils)

**Decisão**: Compilar obrigatório em R3; adicionar opcional em R4 se needed.

---

## Bloqueadores Secundários (P1)

### B6: Reprodutibilidade — Como pinnar timestamps?

**Estado**: Documentado em R3_PREFIX_SAFE_REBUILD_STRATEGY.md  
**Impacto**: Mesma source → mesma saída (verificação de integridade)  

**Solução**:
```bash
export SOURCE_DATE_EPOCH="$(git log -1 --format='%ct' <commit>)"
# Recompila com timestamps fixos
find . -name "*.a" -exec touch -d @$SOURCE_DATE_EPOCH {} \;
```

**Validação**:
```bash
sha256sum libstdc++.so.6
# Rodar novamente com mesmo SOURCE_DATE_EPOCH
sha256sum libstdc++.so.6
# Deve ser idêntico
```

---

### B7: Assinatura de Artefatos (BLAKE3)

**Estado**: TOKEN_VAZIO  
**Impacto**: Integridade chain-of-custody  

**Workflow**:
```bash
# Após compilar debs
for deb in debs/*.deb; do
  sha256sum "$deb"
done > SHA256SUMS

# Assinar
gpg --detach-sign SHA256SUMS  # ou BLAKE3 se tooling existir
```

**Verificação**:
```bash
gpg --verify SHA256SUMS.sig
cat SHA256SUMS | while read sum file; do
  sha256sum -c <<< "$sum  $file" || exit 1
done
```

---

### B8: Clone Seguro (não usar submodules perigosos)

**Estado**: Requer cuidado em checkout  
**Impacto**: Supply chain risk  

**Verificações**:
```bash
git clone --depth 1 --branch master https://github.com/rafaelmeloreisnovo/termux-packages.git
cd termux-packages
git log --oneline -1
# Sem --recursive; não clonar submodules
```

---

## Armadilhas Conhecidas

### A1: Rpath vs RUNPATH (Resolved)

**Problema**: `configure` gera RPATH fixo  
**Solução**: `--disable-rpath` força RUNPATH dinâmica  
**Validação**: `readelf -d libc.so | grep RUNPATH`

### A2: Cross-Compilation Paths (Resolved)

**Problema**: Configure usa paths do host  
**Solução**: `--host=aarch64-linux-android --build=x86_64-linux-gnu`  
**Validação**: `file libc.so` deve dizer "ELF 64-bit ARM"

### A3: Strip Danifica Relocations (Resolved)

**Problema**: `strip` quebra RUNPATH  
**Solução**: Não fazer strip; manter símbolos  
**Se precisar**: `objcopy --only-keep-debug` em vez de `strip`

### A4: Versão de Build Tools (Resolved)

**Problema**: ar/tar pode ser incompatível  
**Solução**: Usar ar/tar do host (Linux x86_64)  
**Validação**: `ar -V` deve mostrar GNU binutils

---

## Checklist Pré-R4

Antes de prosseguir para **R4 (Bootstrap real-pkg)**:

- [x] termux-packages commit pinned em runtime-lock.json (R3.1 — upstream eb124b51a949c7a0943275a18f94371e69756449; fork vazio)
- [ ] NDK 26.3.11579264 instalado e verificado (`scripts/verify_ndk_r26_installation.sh` → `r3_gate=R3.2_COMPLETE`); bloqueado nesta sessão por ausência de SDK/NDK no host de build
- [ ] Todas flags de configuração (--prefix, LDFLAGS) documentadas
- [ ] ARM32 + ARM64 builds testados localmente (ou em CI)
- [ ] Cada .deb passou `validate_deb_prefix_safe.sh`
- [ ] SHA256SUMS gerado e assinado
- [ ] SOURCE_DATE_EPOCH reprodutibilidade validada
- [ ] Package closure (libc, dpkg, apt, ca-certs) presente

---

## Sequência de Ação

```
R3_BLOCKER_1: Decidir termux-packages (fork vs. upstream)
              └→ Commit: git clone + git log -1 --format='%ct'
                
R3_BLOCKER_2: Verificar NDK (ls $ANDROID_NDK/toolchains/...)
              └→ Set ANDROID_NDK env var
              
R3_BLOCKER_3: Documentar flags finais (--prefix, CFLAGS, LDFLAGS)
              └→ Criar build.env
              
R3_BLOCKER_4: Build ARM32 + ARM64 (local ou CI)
              └→ Validar cada .deb com validate_deb_prefix_safe.sh
              
R3_BLOCKER_5: Assinatura (BLAKE3/GPG)
              └→ Gerar SHA256SUMS.sig
              
→ PASSE para R4: Incorporation em bootstrap + APK rebuild
```

---

## Referências

- `app/build.gradle` — NDK version (r26.2.11900637)
- `docs/R3_PREFIX_SAFE_REBUILD_STRATEGY.md` — Estratégia técnica completa
- `scripts/validate_deb_prefix_safe.sh` — Validação post-build
- `runtime-lock.json` — Pinning de commits (será atualizado após R3)
