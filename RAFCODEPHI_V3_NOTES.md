# RAFCODEPHI V3 — build determinístico + lowlevel kernel

Este ZIP V3 junta **correções de build/runtime** com uma **refatoração low-level** do pipeline de `termux-packages`.

## ✅ Correções aplicadas (build)

- `dependencies {}` foram movidos para fora de `android {}` onde estavam no nível errado.
- `compileSdkVersion` foi removido de `defaultConfig {}` (o lugar correto é `android { compileSdkVersion ... }`).
- Runner de testes antigo foi migrado para `androidx.test.runner.AndroidJUnitRunner`.
- Flags de linker foram separadas de `cFlags` para reduzir `-Werror`/"argument unused" (linker flags ficam em `Android.mk` via `LOCAL_LDFLAGS`).

## ✅ Correções aplicadas (runtime / segurança)

- JNI `vectorAdd`: adicionadas validações de tamanho antes de acessar buffers (evita escrita fora de limite).
- `System.loadLibrary("rmr")`: adicionado fallback com `try/catch` e flag `nativeAvailable` (evita crash imediato quando a `.so` não está disponível).

## 🧱 Refatoração low-level (termux-packages)

Foi adicionado:

- `official_deps/termux-packages/raf_pkg/` — núcleo em C (`rafpkg`) para:
  - `scan` (indexação estática)
  - `plan` (ordem determinística/toposort)
  - `audit` (sanidade/invariantes básicas)
- `official_deps/termux-packages/scripts/rafpkg.sh` — wrapper que compila/roda o `rafpkg`.
- `official_deps/termux-packages/build-all.sh` — toggle:
  - padrão: `buildorder.py`
  - alternativo: `RAFPKG=1 ./build-all.sh aarch64`

### V3.2 — processos pesados (Δ→Σ)

- `build-all.sh` agora carrega `buildstatus.txt` uma vez em memória (set) e evita `grep` por pacote.
  - remove overhead puro em listas grandes (zombie loop)
- `rafpkg` ganhou:
  - `cpu` (ABI/features → recomenda flags)
  - `fileaudit` (varredura determinística de "arquivos soltos" / blobs desconhecidos)

## ▶️ Como validar

### Build do app (PC/proot)

```
./gradlew :app:assembleDebug --stacktrace
```

### Planejamento determinístico de pacotes

```
cd official_deps/termux-packages
./scripts/rafpkg.sh plan --arch aarch64 | head -n 40
RAFPKG=1 ./build-all.sh aarch64
```

---

## V3.3 :: RafStore + IO/footprint (Δ→Σ)

### What changed

- **rafstore**: added a tiny binary cache used by `rafpkg fileaudit` to avoid re-reading file headers when nothing changed.
  - default cache: `.rafstore/fileaudit.bin`
  - flags: `--cache <path>` or `--no-cache`
  - debug: `rafpkg store-dump <cachefile> [limit]`

- **build-all.sh** now **keeps** `buildstatus.txt` and stores snapshots in `.rafstore/` for deterministic resumes.

### Why this matters (bandwidth/IOPS/footprint)

- repeated audits become **stat-only** (no extra reads) for unchanged files
- fewer syscalls + less disk churn ⇒ better battery + lower thermal drift

