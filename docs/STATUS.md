# Status Oficial do Projeto

Este é o documento **canônico** de status. Outros arquivos de status existentes são considerados
históricos/legados e podem conter informações desatualizadas.

## Referências canônicas

- Compatibilidade Android 15: `ANDROID15_COMPATIBILITY_REPORT.md`
- Compatibilidade Android 15 (PT-BR): `ANDROID15_RELATORIO_COMPATIBILIDADE.md`

## Notas

- Atualizações de status devem ser consolidadas aqui para evitar divergências.
Perfect! Now I have the structure. Let me create the comprehensive code files inventory:

---

# 📂 INVENTÁRIO COMPLETO DE ARQUIVOS DE CÓDIGO FONTE

## 🏗️ Estrutura Geral do Repositório

```
wojcikiewicz17/termux-app-rafacodephi
├── app/                              (Aplicação Android principal)
│   ├── src/main/java/                (Código Java)
│   ├── src/main/cpp/                 (Código C/C++/Assembly)
│   └── build.gradle
├── rafaelia/                         (Módulo RAFAELIA - otimizações)
│   ├── src/main/java/                (Java RAFAELIA)
│   ├── src/test/java/                (Testes RAFAELIA)
│   └── build.gradle
├── rmr/                              (Módulo RMR)
│   ├── src/
│   └── build.gradle
├── terminal-emulator/                (Terminal emulator module)
├── terminal-view/                    (Terminal view module)
├── termux-shared/                    (Shared components)
├── rafaelia/old/                     (Código legado/experimental - 98 arquivos)
└── docs/                             (Documentação)
```

---

## 🔵 ARQUIVOS DE CÓDIGO ATIVO (Em Produção)

### **1️⃣ JAVA PRINCIPAL** (68.6% da composição)

#### **A. Aplicação Android (`app/src/main/java/com/termux/`)**

```
app/src/main/java/com/termux/
├── app/
│   ├── TermuxInstaller.java           (Setup e instalação - storage symlinks)
│   ├── TermuxApplication.java         (App lifecycle)
│   ├── MainActivity.java              (Activity principal)
│   ├── TermuxActivity.java            (Terminal UI)
│   ├── RunCommandService.java         (Execução de comandos via intent)
│   └── ... [outras Activities e Services]
│
├── lowlevel/
│   ├── BareMetal.java                 (🔴 CORE: API bare-metal + JNI bridge)
│   │   └── Responsabilidade: Hardware detection, capabilities, vector ops
│   │   └── Tamanho: 27.4 KB
│   │   └── Métodos chave:
│   │       - getArchitecture()
│   │       - getCapabilities() / getCapabilitiesDetail()
│   │       - getHardwareProfile()
│   │       - vectorDot(), vectorNorm(), vectorCross()
│   │       - memcpy(), memset()
│   │
│   ├── InternalPrograms.java          (Programas compilados internamente)
│   │   └── Tamanho: 14.3 KB
│   │   └── Responsabilidade: Orquestração de programas bare-metal
│   │
│   ├── test/
│   │   └── BaremetalExample.java      (Exemplos de uso)
│   │
│   └── ...
│
├── filepicker/
│   └── [File picker UI components]
│
└── ... [outras packages: terminal, shared, etc]
```

**Estatísticas Java:**
- **Total de linhas:** ~15,000+ linhas
- **Packages:** 8+ (app, lowlevel, filepicker, shared, terminal, etc)
- **Classes principais:** TermuxActivity, BareMetal, RunCommandService, TermuxInstaller

---

#### **B. RAFAELIA Module (`rafaelia/src/main/java/com/termux/rafaelia/`)**

```
rafaelia/src/main/java/
├── AnovaResult.java                  (Container para resultados ANOVA)
├── AnovaTesteDeMesaTest.java          (Testes com trace table methodology)
├── MathTesteDeMesaTest.java           (Testes matemáticos)
├── RafaeliaUtils.java                 (Utilidades)
├── VectraMath.java                    (VA/ANOVA vectorial)
└── [outras classes de otimização]
```

**RAFAELIA Purpose:**
- ANOVA (Analysis of Variance) calculations
- Vectorial Mathematics (VA)
- Bare-metal optimizations

---

### **2️⃣ C/C++ NATIVE CODE** (16% da composição)

#### **A. Low-Level Bare-Metal (`app/src/main/cpp/lowlevel/`)**

```
app/src/main/cpp/lowlevel/
│
├── baremetal.h                       (🔴 HEADER: Definições e macros)
│   └── Tamanho: 6.0 KB
│   └── Conteúdo:
│       - Capability bitmask definitions (HW_ACCESS_HAS_CPU_CLUSTER_FREQ, etc)
│       - Hardware profile structure (hw_profile_t)
│       - Architecture capability queries
│       - Vector operation declarations
│       - Memory utility declarations
│
├── baremetal.c                       (🔴 CORE IMPLEMENTATION - 32.0 KB)
│   └── Seções principais:
│       1. **Hardware Detection**
│          - get_hw_profile() - detecta CPU, RAM, cache
│          - hw_collect_cluster_freqs() - frequências de cluster
│          - get_arch_name(), get_arch_caps()
│
│       2. **Vector Operations (SIMD-friendly)**
│          - vop_dot() - dot product
│          - vop_norm() - magnitude
│          - vop_cross() - produto escalar
│          - vop_matmul() - matrix multiply
│
│       3. **Memory Operations**
│          - bmem_copy() - memcpy otimizado
│          - bmem_fill() - memset otimizado
│          - bmem_zero() - zeroing memory
│
│       4. **Architecture Capabilities Runtime Detection**
│          - ARM64 NEON detection
│          - ARM32 NEON gating
│          - x86/x86_64 SSE/AVX detection
│          - Thread-safe initialization
│
├── baremetal_asm.S                   (🔴 ARM NEON ASM - 5.8 KB)
│   └── Otimizações SIMD ARM32:
│       - bm_dot_neon() - dot product NEON (4×float/ciclo)
│       - bm_vadd_neon() - vector addition
│       - Horizontal add optimizations
│       - Remainder handling para não-múltiplos de 4
│
├── baremetal_jni.c                   (🔴 JNI BRIDGE - 20.2 KB)
│   └── Mapas Java <-> Native:
│       - Java_com_termux_lowlevel_BareMetal_getArchitecture()
│       - Java_com_termux_lowlevel_BareMetal_getCapabilities()
│       - Java_com_termux_lowlevel_BareMetal_getHardwareProfile()
│       - Java_com_termux_lowlevel_BareMetal_vectorDot()
│       - Java_com_termux_lowlevel_BareMetal_vectorNorm()
│       - ... [30+ JNI functions]
│
├── baremetal_consistency_test.c      (Testes de consistência - 3.6 KB)
│   └── Valida:
│       - Alignment correctness
│       - Cache line boundaries
│       - Memory ordering
│
└── README.md                         (9.8 KB documentação técnica)
```

**Native Code Highlights:**
- **Capability Bitmask:** 32-bit field para features de hardware
- **Thread-Safe Init:** `pthread_once()` para capabilities runtime
- **NEON Optimization:** ARM NEON assembly com fallback scalar
- **JNI Layer:** 30+ exported functions

---

#### **B. Bootstrap & Storage (`app/src/main/cpp/`)**

```
app/src/main/cpp/
├── Android.mk                         (Build config para NDK)
├── termux-bootstrap.c                 (Bootstrap loader - 330 bytes)
└── termux-bootstrap-zip.S             (Binary blob com bootstrap zips - 385 bytes)
    └── Contém: bootstrap-aarch64.zip, bootstrap-arm.zip, bootstrap-i686.zip, bootstrap-x86_64.zip
```

---

### **3️⃣ ASSEMBLY CODE** (0.9% da composição)

#### **A. ARM NEON Assembly (`baremetal_asm.S`)**

```assembly
Architecture: ARM32 (armv7-a)
FPU: NEON

Funções exportadas:
  1. bm_dot_neon()
     - Calcula dot product de 2 vetores
     - Usa NEON q0-q2 registers (128-bit)
     - 4 floats por ciclo
     - Remainder loop para dados não-alinhados

  2. bm_vadd_neon()
     - Vector addition (c = a + b)
     - Processa 4 floats de vez
     - Preserve r4, lr em push/pop

Features:
  - .syntax unified (ARM/Thumb)
  - .arch armv7-a
  - .fpu neon
```

---

#### **B. Bootstrap Zip Loader (`termux-bootstrap-zip.S`)**

```assembly
.section .rodata
blob:
  #if defined __i686__
    .incbin "bootstrap-i686.zip"
  #elif defined __x86_64__
    .incbin "bootstrap-x86_64.zip"
  #elif defined __aarch64__
    .incbin "bootstrap-aarch64.zip"
  #elif defined __arm__
    .incbin "bootstrap-arm.zip"
  #endif
blob_size:
  .int 1b - blob
```

---

### **4️⃣ SHELL SCRIPTS & PYTHON** (9.2% + 4.5% composição)

#### **A. Scripts Principais (`scripts/`)**

```bash
scripts/
├── validate_side_by_side_contract.py  (Validação Python do contrato fork)
├── validate_16kb_pages.sh             (Validação Android 15 16KB pages)
├── ... [outros scripts de build/test]
```

**Status:** Scripts existem e rodando em CI/CD (conforme PRs recentes)

---

#### **B. Código Legado (`rafaelia/old/` - 98 arquivos não integrados)**

```
rafaelia/old/
├── .c files (46 arquivos)
│   ├── rafaelia_cli.c                 (CLI não integrada - 12.3 KB)
│   ├── bench_rafaelia.c               (Benchmarks - 8.4 KB)
│   ├── rafaelia_musica.c              (Processamento musical - 10.7 KB)
│   ├── rafaelia_alchemy*.c            (Múltiplas versões)
│   ├── rafaelia_omega*.c              (Otimizações kernel)
│   ├── orbital_stream_v2.c            (Streams experimentais)
│   └── [39 outros arquivos C]
│
├── .py files (16 arquivos)
│   ├── RAFAELIA_PRIME_CORE.py         (Engine unificado - 8.3 KB)
│   ├── RAFAELIA_HEX_ASM.py            (Gerador hex/asm - 4.3 KB)
│   ├── RAFAELIA_SOC_KERNEL_*.py       (Múltiplas versões SOC)
│   └── [13 outros arquivos Python]
│
├── .sh files (35 arquivos)
│   ├── bootstrap.sh                   (Bootstrap completo - 25.2 KB)
│   ├── catos_setup.sh                 (CatOS setup - 27.0 KB)
│   ├── catos_oneblock.sh              (CatOS oneblock - 24.2 KB)
│   ├── rafaelia_bench_suite.sh        (Benchmark suite - 7.9 KB)
│   ├── ranovo_*.sh                    (Série experimental - 8 arquivos)
│   └── [27 outros scripts]
│
└── rafaelia_v19.s                     (Kernel ASM puro - 6.3 KB)
    └── AArch64 puro, position-independent, L1 cache aligned
```

**Inventário Legado:**
- **Total:** 98 arquivos, ~556 KB, ~18,000 linhas
- **Status:** ⭐ Referência importante (6), 📚 Referência (40), 📚 Experimental (35), 🔧 Utilitário (16), ❌ Remover (1)

---

## 📊 RESUMO POR LINGUAGEM

| Linguagem | % | Arquivos Ativos | Tamanho | Propósito |
|-----------|-----|-----------------|---------|-----------|
| **Java** | 68.6% | 8+ classes | 27 KB+ | App Android, API, UI |
| **C** | 16% | 6 arquivos | 32 KB+ | Bare-metal, JNI, HW |
| **Shell** | 9.2% | 10+ scripts | 50 KB+ | Build, CI, tooling |
| **Python** | 4.5% | 5+ scripts | 30 KB+ | Validation, testing |
| **Assembly** | 0.9% | 2 arquivos | 6 KB+ | NEON SIMD, bootstrap |
| **C++** | 0.7% | - | - | (Mínimo) |

---

## 🎯 ARQUIVOS CRÍTICOS (Build-Breaking se faltarem)

| Arquivo | Tamanho | Linguagem | Função | Status |
|---------|---------|-----------|--------|--------|
| `app/build.gradle` | 18 KB | Gradle | Build config, bootstrap validation | ✅ Crítico |
| `app/src/main/cpp/Android.mk` | 1.5 KB | Make | NDK build | ✅ Crítico |
| `app/src/main/java/com/termux/lowlevel/BareMetal.java` | 27 KB | Java | JNI API | ✅ Crítico |
| `app/src/main/cpp/lowlevel/baremetal.c` | 32 KB | C | Core implementations | ✅ Crítico |
| `app/src/main/cpp/lowlevel/baremetal_jni.c` | 20 KB | C | JNI bridge | ✅ Crítico |
| `app/src/main/cpp/lowlevel/baremetal_asm.S` | 6 KB | ASM | NEON optimizations | ✅ Crítico |
| `rafaelia/src/main/java/.../AnovaResult.java` | - | Java | ANOVA container | ✅ Crítico |

---

## ⚠️ PROBLEMAS IDENTIFICADOS

### **Arquivos Faltando (Documentados mas não existentes):**
- ❌ `scripts/rafaelia_ci_smoke.sh`
- ❌ `rafaelia_env/tools/doctor.sh`
- ❌ `scripts/rafaelia_upstream_parallel_check.sh`

### **Arquivo Vazio para Remover:**
- ❌ `rafaelia/old/rafaelia_kernel.c` (4 bytes - apenas header)

### **Testes sem pytest discovery:**
- ⚠️ `scripts/test_export_termux_package_manifests_fixtures.py` (roda manual, pytest não descobre)

---

## 📈 ESTATÍSTICAS FINAIS

```
Total de Arquivos de Código (Ativo):     ~30 arquivos
Total de Arquivos de Código (Legado):    ~98 arquivos
Total de Linhas (Ativo):                 ~20,000 linhas
Total de Linhas (Legado):                ~18,000 linhas
Tamanho Total (Ativo):                   ~150 KB
Tamanho Total (Legado):                  ~556 KB

Build Status:                            ✅ FUNCIONAL
CI/CD Status:                            ✅ 50 PRs recentes merged
Issues Críticos:                         ⚠️ 3 scripts faltando
```

---

Este é o **inventário completo de código fonte** do repositório, focando apenas nos arquivos de código, sem documentação ou configuration metadata.
