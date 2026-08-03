# Termux Application - RafaCodePhi Fork

**Estado:** `ACTIVE`  
**Proprietário lógico:** `app-maintainer`  
**Repositório:** [`rafaelmeloreisnovo/termux-app-rafacodephi`](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi)

> 📚 **Centro de documentação moderno**: veja [`docs/README.md`](./docs/README.md).  
> 🧭 **Mapa rápido em nível L/L2**: veja [`DOCS_L2_TREE.md`](./DOCS_L2_TREE.md).

## Documentação recomendada

| Objetivo | Documento |
|---|---|
| Entrada principal | [`docs/README.md`](./docs/README.md) |
| Verdade operacional atual | [`docs/STATUS.md`](./docs/STATUS.md) |
| Navegação completa | [`INDICE_DOCUMENTACAO.md`](./INDICE_DOCUMENTACAO.md) |
| Execução de build/release/CI | [`docs/ENGINEERING_SYSTEM_RUNBOOK.md`](./docs/ENGINEERING_SYSTEM_RUNBOOK.md) |
| Excelência operacional | [`docs/EXCELENCIA_OPERACIONAL_MATRIX.md`](./docs/EXCELENCIA_OPERACIONAL_MATRIX.md) |

## Fork Notice and Attribution

**This is a fork of the original [Termux](https://github.com/termux/termux-app) project.**

### Original Project
- **Original Repository**: [termux/termux-app](https://github.com/termux/termux-app)
- **Original Authors**: The Termux team and contributors
- **Original License**: GPLv3 (with exceptions as detailed in LICENSE.md)
- **Website**: [https://termux.com](https://termux.com)

### Fork Information
- **Fork Maintained By**: instituto-Rafael
- **Fork Repository**: [instituto-Rafael/termux-app-rafacodephi](https://github.com/instituto-Rafael/termux-app-rafacodephi)
- **Purpose**: Enhanced version with additional features and customizations

### Legal Notice
This fork complies with the GPLv3 license of the original Termux project. All modifications and additions are also released under GPLv3 (unless otherwise specified). We acknowledge and respect the intellectual property rights of the original Termux developers and all contributors to the upstream project.

---

[![Build status](https://github.com/termux/termux-app/workflows/Build/badge.svg)](https://github.com/termux/termux-app/actions)
[![Testing status](https://github.com/termux/termux-app/workflows/Unit%20tests/badge.svg)](https://github.com/termux/termux-app/actions)
[![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux)
[![Join the Termux discord server](https://img.shields.io/discord/641256914684084234.svg?label=&logo=discord&logoColor=ffffff&color=5865F2)](https://discord.gg/HXpF69X)
[![Termux library releases at Jitpack](https://jitpack.io/v/termux/termux-app.svg)](https://jitpack.io/#termux/termux-app)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/platform-Android%207%2B-brightgreen)](https://developer.android.com)
[![Architecture](https://img.shields.io/badge/arch-ARM32%20%7C%20ARM64-orange)](rmr/Rrr/Android_nomalloc.mk)
[![C11 nomalloc](https://img.shields.io/badge/C-C11%20zero--malloc-blueviolet)](rmr/Rrr/cti_raw_reader.h)
[![CTI BITSTACK](https://img.shields.io/badge/RAFAELIA-CTI%20BITSTACK-critical)](rmr/Rrr/cti_raw_reader.c)
[![ZIPRAF](https://img.shields.io/badge/RAFAELIA-ZIPRAF%20Manifesto-critical)](rmr/Rrr/zipraf_index.c)
[![CRC32C](https://img.shields.io/badge/integrity-CRC32C%20inline-lightgrey)](rmr/Rrr/)

## About Termux

[Termux](https://termux.com) is an Android terminal application and Linux environment.

Note that this repository is for the app itself (the user interface and the terminal emulation). For the packages installable inside the app, see [termux/termux-packages](https://github.com/termux/termux-packages).

Quick how-to about Termux package management is available at [Package Management](https://github.com/termux/termux-packages/wiki/Package-Management). It also has info on how to fix **`repository is under maintenance or down`** errors when running `apt` or `pkg` commands.

**We are looking for Termux Android application maintainers.**

***

**NOTICE: Termux may be unstable on Android 12+.** Android OS will kill any (phantom) processes greater than 32 (limit is for all apps combined) and also kill any processes using excessive CPU. You may get `[Process completed (signal 9) - press Enter]` message in the terminal without actually exiting the shell process yourself. Check the related issue [#2366](https://github.com/termux/termux-app/issues/2366), [issue tracker](https://issuetracker.google.com/u/1/issues/205156966), [phantom cached and empty processes docs](https://github.com/agnostic-apollo/Android-Docs/blob/master/en/docs/apps/processes/phantom-cached-and-empty-processes.md) and [this TL;DR comment](https://github.com/termux/termux-app/issues/2366#issuecomment-1237468220) on how to disable trimming of phantom and excessive CPU usage processes. A proper docs page will be added later. An option to disable the killing should be available in Android 12L or 13, so upgrade at your own risk if you are on Android 11, especially if you are not rooted.

***


## Verdade operacional canônica

- `compileSdkVersion=35`
- `targetSdkVersion=28`
- `minSdkVersion=21`
- ABIs obrigatórias: `armeabi-v7a`, `arm64-v8a`
- `universalApk=true`
- package/applicationId: `com.termux.rafacodephi`

### Estado epistêmico

- **PROVADO**: build/release/contrato validado por comando ou CI.
- **PARCIAL**: existe base funcional, mas falta validação real completa.
- **TOKEN_VAZIO**: wrapper/bridge existe sem backend real. `pkg`, `apt`, `apt-get`, `dpkg`, `libapt` e `proot` permanecem nesta classe até o payload core real e testes `pkg update`/`pkg install`.
- **EXPERIMENTAL**: pesquisa/otimização sem contrato de release.
- **FUTURO**: planejado.

O bootstrap atual fornece uma base mínima guardada para instalação e diagnóstico, mas ainda não equivale a uma distribuição Termux completa com backend apt real. A descrição correta é **Termux-compatible bootstrap shell environment** até pacote real ser provado.

ZIPRAF não comprime fisicamente; cria endereçamento lógico multirresolução sobre bytes existentes. A VCPU atual é uma **RAFAELIA deterministic VCPU state kernel**, não uma VM completa.

## Fork Contract: Upstream vs RAFCODEΦ

### A) Termux Upstream (base)
- Este repositório mantém o app Termux como base upstream (UI, terminal e integração padrão).
- Pacotes do ecossistema continuam referenciando o fluxo `termux-packages`.

### B) Alterações RAFCODEΦ
- Identidade side-by-side própria: `com.termux.rafacodephi`.
- Pipeline RAFAELIA com preparação explícita de bootstrap e validações de contrato.
- Fonte de pacotes/bootstrap RAFCODEΦ: `https://github.com/exacordex-crypto/termux-packagesRafcodephi`, consumida por CI como fonte de código/metadata com commit fixado, nunca como binário versionado neste repo.

### C) Módulo low-level RMR
- Módulo nativo C/ASM com JNI fino, fallback C e dispatch runtime por capacidades.
- Sem promessa de ganho de performance sem benchmark reproduzível.

### D) Compatibilidade Android 15/16
- Binários nativos com alinhamento para page size 16KB via linker flags.
- ABIs oficiais validadas na trilha de build: `armeabi-v7a` (ARM ABI7) e `arm64-v8a` (ARM ABI8), além do APK universal quando gerado.

### E) Bootstrap e Signing
- Bootstraps obrigatórios e hashes BLAKE3 verificados antes de builds críticos.
- Signing oficial é opt-in e separado da trilha unsigned interna de validação.


## Canonical ABI Policy

Fonte única oficial: `gradle.properties`.

- `termux.abi.matrix=armeabi-v7a,arm64-v8a` (ABIs obrigatórias)
- `termux.abi.optional=` (nenhuma ABI opcional; x86/x86_64 não fazem parte da trilha ARM oficial)
- `termux.abi.universal=true` (universal APK quando gerado)

Contratos:
- `app/build.gradle` e `terminal-emulator/build.gradle` consomem essa política via `project.findProperty(...)`.
- Scripts operacionais (`scripts/build_apk_matrix.sh`, `scripts/bootstrap_lowlevel_sync_check.sh`) validam ABIs obrigatórias a partir da mesma fonte.
- CI valida consistência com `scripts/validate_abi_policy_consistency.sh`.

> Histórico: documentos legados em `COMP/` podem conter políticas ABI antigas (ex.: arm64-only). Eles são referência histórica e não definem a política vigente.

## 🚀 Termux RAFCODEΦ - Android 15/16 Ready

**This fork is fully compatible with Android 15/16 and can be installed side-by-side with official Termux.**

### ⚡ Critical Android 16 Fix Applied

**✅ 16KB Page Size Compatibility** - This build includes the critical fix for Android 15/16 devices with 16KB memory pages. The app **will NOT crash** on:
- Android 15 with 16KB pages enabled
- Android 16 Beta (all devices)
- Devices with kernel 5.15.178+ (like RMX3834)

Without this fix, apps crash with SIGSEGV on startup. **This fork includes the compatibility patch; validate in your own environment before production release.**

📖 See [Android 16 Page Size Fix Documentation](./ANDROID16_PAGE_SIZE_FIX.md) for technical details.

### Key Features
- ✅ **Package Name**: `com.termux.rafacodephi` (unique, no conflicts)
- ✅ **App Name**: `Termux RAFCODEΦ` (distinct branding)
- ✅ **Side-by-Side**: Install alongside official Termux without conflicts
- ✅ **Android 15/16**: Configured for 16KB page alignment and Phantom Process Killer handling
- ✅ **Zero Collisions**: Unique authorities, permissions, and data directories
- ✅ **Bare-Metal**: NEON/SIMD optimized native code with pthread support

### RMR Low-Level Module (C/ASM)
- ✅ **Low-level utilities**: Deterministic helpers implemented in C with ASM-backed primitives where possible (RMR module)
- ✅ **No legacy abstractions**: JNI only as a thin bridge to native primitives
- ✅ **Termux packages alignment**: The package ecosystem remains defined by [termux/termux-packages](https://github.com/termux/termux-packages)

---

### CTI BITSTACK — Deterministic Raw File Scanner

> **Module**: `rmr/Rrr/cti_raw_reader.h` + `cti_raw_reader.c`  
> **Status**: ✅ Production — merged in PR #190

CTI BITSTACK scans any file (RAW, JPEG, GIF, PNG, ZIP) at **byte-block granularity** (4 096 bytes/block) and builds a deterministic index of per-block metrics. It operates entirely without heap allocation, using only `write(1,…)` for output and an inline CRC32C table.

**Per-block index entry** (`CtiEntry`, 28 bytes packed):

| Field | Type | Description |
|---|---|---|
| `idx` | `uint32_t` | Physical block index |
| `size` | `uint32_t` | Bytes read from this block |
| `ts` | `uint64_t` | Logical scan counter (monotonic) |
| `fid_crc32` | `uint32_t` | CRC32C (Castagnoli) of block bytes |
| `entropy` | `uint32_t` | Shannon entropy × 1000 (0 = flat; 8000 = max random) |
| `flags` | `uint8_t` | `CTI_FMT_*` detected at file header |
| `xbad` | `uint8_t` | Saturated count of `0x00`/`0xFF` run events |
| `miss_score` | `int16_t` | DELTA_MISS signed deviation from expected chain-CRC |

**5 Scan Modes** (`CtiMode` enum):

| Mode | Constant | Traversal |
|---|---|---|
| Sequential | `CTI_SEQ` | Block 0, 1, 2, … N-1 |
| Spiral | `CTI_SPIRAL` | 2-D counter-clockwise spiral from grid centre |
| Toroidal | `CTI_TOROID` | Coprime-stride walk — `gcd(stride, N) = 1` guarantees full coverage |
| Random mapping | `CTI_RANDOM_PERM` | xorshift64 keyed by `seed` — different order on each seed |
| Delta-miss | `CTI_DELTA_MISS` | SEQ + live miss-score = `f(prev_crc, position, seed)` |

**Format detection** (from first 8 bytes):

| Format | Magic |
|---|---|
| JPEG | `FF D8` |
| PNG | `89 50 4E 47` (4 bytes) |
| GIF | `GIF87a` or `GIF89a` (6 bytes) |
| ZIP | `PK 03 04` (local-file-header signature, 4 bytes) |
| RAW | fallback |

**Public API**:
```c
int      cti_scan_fd(CtiScanner *sc, int fd, CtiMode mode, uint32_t seed);
void     cti_print_report(const CtiScanner *sc);
uint8_t  cti_detect_fmt(const uint8_t *hdr, uint32_t hdr_len);
uint32_t cti_entropy(const uint8_t *buf, uint32_t len);
```

**Build**:
```bash
# Standalone scanner (Linux / Android via NDK):
gcc -std=c11 -O2 -DCTI_BUILD_MAIN -o cti_scan rmr/Rrr/cti_raw_reader.c
./cti_scan /path/to/file [mode 0-4] [seed]

# As part of rafaelia_core (NDK):
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=rmr/Rrr/Android_nomalloc.mk
```

---

### ZIPRAF — Deterministic Manifesto Matrix Index

> **Module**: `rmr/Rrr/zipraf_index.h` + `zipraf_index.c`  
> **Status**: ✅ Production — merged in PR #190

ZIPRAF is a **non-compression, multidimensional deterministic storage model**. Physical bytes in the ZIP container are never modified. Instead, the manifesto overlays **8 reading modes × 33 density levels** of logical addressing structure over the same physical bytes — like DNA codons over atoms: the atoms (bytes) stay fixed while the codon table (manifesto) gives them meaning at 264 different resolutions.

```
Logical capacity = physical_size × 8 × 33 = physical_size × 264
  1 GB ZIP  →  264 GB addressable logical space
  1 TB ZIP  →  264 TB  (→ "1 000 TB+" design target)
```

**Manifesto Matrix: 8 Modes × 33 Density Levels**

| Mode ID | Name | Description |
|---|---|---|
| 0 | `DIRECT` | 1:1 physical byte-offset mapping |
| 1 | `MEMORIA` | In-memory overlay mapping |
| 2 | `LEETRA` | Symbol / character lattice index |
| 3 | `ORBITAL` | Harmonic frequency bins (Q16.16) |
| 4 | `TOROIDAL` | Toroidal stride addressing over `zip_size` |
| 5 | `SIGIL` | Sigil-intent keyed — IA_SIGILS control plane |
| 6 | `FRACTAL` | Fractal subdivision — block → sub-blocks |
| 7 | `ENTROPIC` | Entropy-ordered: highest-entropy blocks first |

**Density Levels**: level 1 = one entry covers the entire ZIP file; level 33 = one entry ≈ 4 KB block. Block size at level `d` = `clamp(zip_size >> (d-1), 4096, zip_size)`.

**Manifesto Entry** (`ZrEntry`, 28 bytes packed+aligned(4)):

| Field | Type | Description |
|---|---|---|
| `mode` | `uint8_t` | `ZrMode` 0–7 |
| `density` | `uint8_t` | Density level 1–33 |
| `mod_id` | `uint16_t` | Module ID (`0x0987 + mode`) |
| `k` | `uint32_t` | Dimension key (`bi × 23 + mode`) |
| `offset` | `uint64_t` | Physical byte offset inside ZIP |
| `len` | `uint32_t` | Logical data length in bytes |
| `policy` | `uint8_t` | `ZrPolicy` access policy |
| `flags` | `uint8_t` | `ZR_FLAG_*` validity/redundancy flags |
| `ext` | `uint16_t` | Extension: sigil ID, orbital freq, etc. |
| `crc32` | `uint32_t` | CRC32C of this entry (excluding `crc32` field) |

**Access Policies**: `DIRETA` (direct), `READONLY`, `OVERLAY` (coherence redundancy), `SIGIL_KEY` (intent-unlocked).

**Geometric Coherence Theorem** (∀ k ∈ n, G/Sₖ is reconstructible):
Removing any one mode leaves the other 7 projections intact. Implemented via `ZR_FLAG_REDUNDANT` + `ZR_POL_OVERLAY` on the ENTROPIC mode.

**Public API**:
```c
void     zr_init(ZrManifest *m, const char *zip_path, uint64_t zip_size);
int      zr_add(ZrManifest *m, ZrMode mode, uint8_t density,
                uint16_t mod_id, uint32_t k,
                uint64_t offset, uint32_t len, ZrPolicy policy);
ZrEntry *zr_lookup(ZrManifest *m, ZrMode mode, uint8_t density,
                   uint16_t mod_id, uint32_t k);
ZrEntry *zr_lookup_by_offset(ZrManifest *m, uint64_t offset, uint32_t len);
int      zr_verify(const ZrManifest *m);          /* returns 1=OK, 0=corrupt */
int      zr_auto_index(ZrManifest *m, uint64_t zip_size, uint32_t block_size);
void     zr_print(const ZrManifest *m);
```

> ⚠️ **Stack warning**: `ZrManifest` is ~59 KB. Declare it `static` or use an arena — **never on a thread stack**.

**Build**:
```bash
# Standalone tool:
gcc -std=c11 -O2 -DZIPRAF_BUILD_MAIN -o zipraf_tool rmr/Rrr/zipraf_index.c
./zipraf_tool /path/to/archive.zip

# As part of rafaelia_core (NDK):
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=rmr/Rrr/Android_nomalloc.mk
```

---

### Quick Start

```bash
# Optional preflight (installs SDK/NDK from gradle.properties and writes local.properties sdk.dir)
./scripts/ci_android_preflight.sh

# Build
./gradlew assembleDebug

# Install
adb install app/build/outputs/apk/debug/termux-app_apt-android-7-debug_universal.apk

# Diagnose
./scripts/diagnose.sh
```

### Build/release pipeline local (bootstrap + BLAKE3)

```bash
# Prepara SDK/NDK, baixa bootstraps e exporta hashes BLAKE3
eval "$(./scripts/prepare_bootstrap_env.sh --print-env)"

# Build debug/release (split APKs habilitado)
./scripts/build_release_artifacts.sh

# Matriz interna completa
RELEASE_TRACK=internal ./scripts/build_apk_matrix.sh

# Matriz oficial
RELEASE_TRACK=official KEYSTORE_PATH=/path/release.jks KEY_ALIAS=... STORE_PASS=... KEY_PASS=... ./scripts/build_apk_matrix.sh
```

### Contrato de trilhas de release (CI)

| Trilha | Assinatura release (`armeabi-v7a`, `arm64-v8a`) | Unsigned permitido | Bloqueios |
|---|---|---|---|
| oficial | Obrigatória em `dist/apk-matrix/signed` | Não | Falha se faltar APK assinado por ABI, se houver release unsigned, se hash/nome divergirem de `SHA256SUMS.txt`, ou se `BOOTSTRAP_BAREMETAL_STRICT!=true`. |
| interna | Obrigatória em `dist/apk-matrix/signed` | Sim, apenas para validação explícita em `dist/apk-matrix/unsigned` | Falha se nomes de signed/unsigned violarem contrato ou se hashes não baterem com `SHA256SUMS.txt`. |

### Documentation
- 🔥 [**Android 16 Page Size Fix**](./ANDROID16_PAGE_SIZE_FIX.md)
- 🚀 [**Boosters de Performance**](./BOOSTERS.md)
- 🚀 [**Performance Boosters Guide**](./BOOSTERS_DOCUMENTACAO.md)
- 📊 [**Benchmarks & Comparison**](./BENCHMARKS_COMPARISON.md)
- 📄 [Android 15 Audit Report](./ANDROID15_AUDIT_REPORT.md)
- 📚 [Android 15 Compatibility Guide](./docs/RAFCODEPHI_ANDROID15_COMPATIBILITY.md)
- 🔧 [Troubleshooting Guide](./TROUBLESHOOTING.md)
- 📝 [Changes and Patch](./docs/MUDANCAS_ANDROID15.md)
- ⚙️ [Bare-Metal Implementation](./IMPLEMENTACAO_BAREMETAL.md)
- 📖 [Complete Documentation](./DOCUMENTACAO.md)
- 🧩 [Total Dependencies Inventory](./docs/DEPENDENCIAS_TOTAIS.md)
- 🗂️ [Loose Files Inventory](./ARQUIVOS_SOLTOS_INVENTARIO.md)
- 🔗 [External Integration Map](./docs/EXTERNAL_INTEGRATION_MAP.md)
- 🔗 [Symbol Encoding Policy](./docs/SYMBOL_ENCODING_POLICY.md)
- 🔍 [CTI BITSTACK — Raw Scanner](./rmr/Rrr/cti_raw_reader.h)
- 📦 [ZIPRAF Manifesto Index](./rmr/Rrr/zipraf_index.h)
- ⚙️ [NDK nomalloc build rules](./rmr/Rrr/Android_nomalloc.mk)
- 📐 [RAFAELIA Math Formulas](./rmr/Rrr/RAFAELIA_MATH_FORMULAS.md)

***

## Contents

**RAFAELIA Modules**
- [CTI BITSTACK — Raw File Scanner](#cti-bitstack--deterministic-raw-file-scanner)
- [ZIPRAF — Manifesto Matrix Index](#zipraf--deterministic-manifesto-matrix-index)
- [ROADMAP](#roadmap)
- [Total Module Inventory](#total-module-inventory--rmrrrr)

**App**
- [Fork Notice and Attribution](#fork-notice-and-attribution)
- [Termux RAFCODEΦ - Android 15/16 Ready](#-termux-rafcodeΦ---android-1516-ready)
- [Termux App and Plugins](#termux-app-and-plugins)
- [Installation](#installation)
- [Uninstallation](#uninstallation)
- [Important Links](#important-links)
- [Debugging](#debugging)
- [For Maintainers and Contributors](#for-maintainers-and-contributors)
- [Forking](#forking)
- [Sponsors and Funders](#sponsors-and-funders)
- [Acknowledgments and Attribution](#acknowledgments-and-attribution)
---




## Auditoria de documentação

- Relatório da raiz: [AUDITORIA.md](./AUDITORIA.md)
- Relatório do módulo MVP: [mvp/AUDITORIA.md](./mvp/AUDITORIA.md)
- Relatório do módulo RMR: [rmr/AUDITORIA.md](./rmr/AUDITORIA.md)
- Relatório de docs RAFAELIA: [docs/rafaelia/AUDITORIA.md](./docs/rafaelia/AUDITORIA.md)
- Relatório do legado RAFAELIA: [rafaelia/old/AUDITORIA.md](./rafaelia/old/AUDITORIA.md)
- Mapa absoluto de markdowns: [docs/MARKDOWN_MAPA_ABSOLUTO.md](./docs/MARKDOWN_MAPA_ABSOLUTO.md)
- Revisão completa de markdowns: [docs/REVISAO_COMPLETA_MARKDOWN.md](./docs/REVISAO_COMPLETA_MARKDOWN.md)
- Top 10 MD (código ↔ documentação): [docs/TOP10_CODE_DOC_GAPS_2026-05.md](./docs/TOP10_CODE_DOC_GAPS_2026-05.md)

***

## Termux App and Plugins

The core [Termux](https://github.com/termux/termux-app) app comes with the following optional plugin apps.

- [Termux:API](https://github.com/termux/termux-api)
- [Termux:Boot](https://github.com/termux/termux-boot)
- [Termux:Float](https://github.com/termux/termux-float)
- [Termux:Styling](https://github.com/termux/termux-styling)
- [Termux:Tasker](https://github.com/termux/termux-tasker)
- [Termux:Widget](https://github.com/termux/termux-widget)
---



## Installation

Upstream reference version cited here is `v0.118.3`; this fork currently declares `0.118.0-rafacodephi` in `app/build.gradle`.

**NOTICE: It is highly recommended that you update to `v0.118.0` or higher ASAP for various bug fixes, including a critical world-readable vulnerability reported [here](https://termux.github.io/general/2022/02/15/termux-apps-vulnerability-disclosures.html).**

Termux can be obtained through various sources listed below for **only** Android `>= 7` with full support for apps and packages.

For local builds in this repository, bootstrap ZIPs under `app/src/main/cpp/bootstrap-*.zip` are **build artifacts** generated by Gradle tasks and are intentionally not versioned in git.

### Hotfix build ("até compilar")

```bash
./scripts/hotfix_ate_compilar.sh
```

### Release build signing (signed or unsigned)

`assembleRelease` now supports two explicit modes:

1. **Unsigned release (default)**: do not set `TERMUX_ENABLE_RELEASE_SIGNING`.
2. **Signed release (explicit opt-in)**: set all variables below and `TERMUX_ENABLE_RELEASE_SIGNING=true`.

Required variables for signed release:

- `TERMUX_RELEASE_KEYSTORE_FILE`
- `TERMUX_RELEASE_KEYSTORE_PASSWORD`
- `TERMUX_RELEASE_KEY_ALIAS`
- `TERMUX_RELEASE_KEY_PASSWORD`

### F-Droid

Termux application can be obtained from `F-Droid` from [here](https://f-droid.org/en/packages/com.termux/).

### GitHub

Termux application can be obtained on `GitHub` either from [`GitHub Releases`](https://github.com/termux/termux-app/releases) for version `>= 0.118.0` or from [`GitHub Build Action`](https://github.com/termux/termux-app/actions/workflows/debug_build.yml?query=branch%3Amaster+event%3Apush) workflows.

## Uninstallation

To uninstall Termux completely, you must uninstall **any and all existing Termux or its plugin app APKs** listed in [Termux App and Plugins](#termux-app-and-plugins).

---

## Important Links

### Community
All community links are available [here](https://wiki.termux.com/wiki/Community).

- [Termux Reddit community](https://reddit.com/r/termux)
- [Termux User Matrix Channel](https://matrix.to/#/#termux_termux:gitter.im)
- [Termux X (Twitter)](https://twitter.com/termuxdevs)

### Wikis

- [Termux Wiki](https://wiki.termux.com/wiki/)
- [Termux App Wiki](https://github.com/termux/termux-app/wiki)
- [Termux Packages Wiki](https://github.com/termux/termux-packages/wiki)

---

## For Maintainers and Contributors

The [termux-shared](termux-shared) library was added in [`v0.109`](https://github.com/termux/termux-app/releases/tag/v0.109). It defines shared constants and utils of the Termux app and its plugins.

### Commit Messages Guidelines

Commit messages **must** use the [Conventional Commits](https://www.conventionalcommits.org) spec.

---

## Forking

- Check [`TermuxConstants`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java) javadocs for instructions on what changes to make in the app to change package name.

---

## Sponsors and Funders

[<img alt="GitHub Accelerator" width="25%" src="site/assets/sponsors/github.png" />](https://github.com)  
*[GitHub Accelerator](https://github.com/accelerator)*

---

## Acknowledgments and Attribution

**For a complete list of contributors and detailed attribution information, please see [CONTRIBUTORS.md](CONTRIBUTORS.md).**

### Upstream Project Acknowledgment

This project is a fork of the **Termux** project, originally created and maintained by the Termux development team.

**Original Termux Project:**
- Repository: [https://github.com/termux/termux-app](https://github.com/termux/termux-app)
- Website: [https://termux.com](https://termux.com)
- License: GPLv3 (with specified exceptions)

---

## Security and release policy (RAFCODEΦ)

- Package name oficial e único: `com.termux.rafacodephi`.
- Keystores/chaves de release não devem ser versionados; use apenas variáveis de ambiente para signing oficial.
- Trilha interna unsigned é somente para validação técnica, nunca para release oficial.

## ROADMAP

> Current iteration: **RMR v1.0** — CTI BITSTACK + ZIPRAF merged.  
> Next iteration: **RMR v1.1** — JNI bridge + Android API surface.

### ✅ Delivered (merged → master)

| Module | Description | PR |
|---|---|---|
| `CTI BITSTACK` | Deterministic raw file scanner — 5 modes, CRC32C, entropy, bad-byte detection | #190 |
| `ZIPRAF Manifesto` | 8×33 logical index over ZIP — non-compression deterministic manifesto | #190 |
| Android 15/16 page-size fix | 16KB page-size alignment via linker flags | earlier |
| BLAKE3 bootstrap hashes | Deterministic bootstrap verification | earlier |

### 🔲 Planned: RMR v1.1

| Target | Description | Priority |
|---|---|---|
| JNI surface for CTI | Expose `cti_scan_fd` / `cti_print_report` via JNI | High |
| JNI surface for ZIPRAF | Expose `zr_auto_index` / `zr_lookup` / `zr_verify` via JNI | High |

---

## Total Module Inventory — `rmr/Rrr/`

### C Source Files

| File | Role | Exported API |
|---|---|---|
| `cti_raw_reader.c` | CTI BITSTACK raw scanner | `cti_scan_fd`, `cti_print_report`, `cti_detect_fmt`, `cti_entropy` |
| `zipraf_index.c` | ZIPRAF deterministic manifesto index | `zr_init`, `zr_add`, `zr_lookup`, `zr_lookup_by_offset`, `zr_verify`, `zr_auto_index`, `zr_print` |
| `rafaelia_bitraf.c` | BITRAF 3D matrix 10×10×10+8 | `raf_bitraf_selftest`, `raf_bitraf_print` |
| `rafaelia_core.c` | RMR core + Q16.16 types | `raf_core_init`, `raf_core_run` |
| `rafaelia_orchestrator.c` | Multi-stage pipeline dispatch | `raf_orchestrator_run` |
| `rafaelia_sigma_omega.c` | Σ/Ω logic module | `raf_sigma_omega_run` |
| `rafaelia_glue.c` | JNI thin bridge | `Java_*` JNI methods |
| `baremetal_nomalloc.c` | Bare-metal helpers, arena allocator | `raf_arena_*`, `raf_bm_*` |

### Build Files

| File | Role |
|---|---|
| `Android_nomalloc.mk` | ndk-build rules: `rafaelia_core.so`, `cti_scan_tool`, `zipraf_tool` |
| `build_all.sh` | Convenience wrapper: native + APK matrix |

---

## Linux/PC user-space contract (Rafaelia)

```bash
./install-rafaelia-linux.sh
./start-rafaelia-linux.sh
```

Validação inicial recomendada:

```bash
./start-rafaelia-linux.sh -lc 'cat /etc/os-release'
./start-rafaelia-linux.sh -lc 'apt update'
./start-rafaelia-linux.sh -lc 'python3 --version'
./start-rafaelia-linux.sh -lc 'gcc --version'
```
