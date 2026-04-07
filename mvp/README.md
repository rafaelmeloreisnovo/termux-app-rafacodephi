# 🔧 RAFAELIA MVP PURO - Zero Dependências

## Metadados do Documento

| Campo | Valor |
|-------|-------|
| **Título** | MVP Low-Level RAFAELIA |
| **Versão** | 1.0 |
| **Data** | 2026-04-07 |
| **Autor** | instituto-Rafael |
| **Licença** | GPLv3 |

---

## 📋 Sumário Executivo

Este diretório contém o **MVP (Minimum Viable Product) Puro** do framework RAFAELIA, implementado inteiramente em Assembly (AArch64) e representações hexadecimais, **sem nenhuma dependência externa** - nem mesmo a biblioteca padrão C (libc).

### Características Principais

✅ **Zero Dependências** - Nenhuma biblioteca externa  
✅ **Syscalls Diretas** - Comunicação direta com o kernel Linux  
✅ **Position Independent** - Código relocável  
✅ **Cache Aligned** - Otimizado para cache L1 (64 bytes)  
✅ **Determinístico** - Comportamento previsível e reproduzível  
✅ **Assinatura de Autoria** - Proteção ética da origem do código  

---

## 📁 Estrutura do Diretório

```
mvp/
├── README.md                    # Este documento
├── rafaelia_mvp_puro.s          # Assembly AArch64 completo
└── rafaelia_opcodes.hex         # Representação hexadecimal
```

> Estado atual confirmado em disco: `ASSINATURA_AUTORIA.md` e `Makefile` não existem dentro de `mvp/` nesta revisão.

---

## 🔬 Arquivos do MVP

### 1. `rafaelia_mvp_puro.s` - Assembly AArch64

Assembly puro para ARM64 (AArch64), contendo:

- **Entry Point** (`_start`) - Ponto de entrada sem runtime C
- **Verificação de Autoria** - Checa assinatura embedded
- **Fast SQRT** - Raiz quadrada via Newton-Raphson
- **Vector Dot Product** - Produto escalar de vetores
- **Matrix Determinant** - Determinante 2x2
- **Memory Checksum** - Checksum XOR
- **Print U64** - Conversão e impressão de inteiros

**Tamanho estimado do binário**: ~2KB

### 2. `rafaelia_opcodes.hex` - Hexadecimal

Representação hexadecimal de:

- **Opcodes AArch64** para operações aritméticas
- **Constantes IEEE 754** (PI, E, etc.)
- **Magic Numbers** (Fast RSQRT)
- **Assinatura de Autoria** em bytes

---

### Política de Timestamp em `METADATA`

- **Política adotada**: `build time UTC` (epoch Unix no momento da geração/atualização).
- Atualize o campo de timestamp com:

```bash
./scripts/mvp_metadata_timestamp.sh update
```

- Valide que o timestamp não ficou como placeholder zerado com:

```bash
./scripts/mvp_metadata_timestamp.sh validate
```

> Se `mvp/rafaelia_opcodes.hex` for regenerado por automação, inclua o comando `update` na etapa de geração para evitar commits com `00000000`.

---

## 🔐 Proteção de Autoria

### Assinatura Embedded

Todos os binários gerados contêm uma assinatura de autoria:

```
0x52414641 0x454C4941 0x494E5354 0x52414641
"RAFA"     "ELIA"     "INST"     "RAFA"
```

### Verificação em Runtime

O código verifica sua própria assinatura na inicialização:

```asm
verify_authorship:
    ldr w1, [x0, #0]        /* "RAFA" */
    ldr w2, =AUTHOR_SIG_1
    cmp w1, w2
    b.ne auth_fail          /* Falha se assinatura inválida */
```

### Propósito Ético

Esta proteção garante:

1. **Atribuição** - O código sempre identifica seu autor
2. **Integridade** - Modificações são detectáveis
3. **Rastreabilidade** - Origem do código é verificável
4. **Ética** - Respeito à propriedade intelectual

---

## 🛠️ Como Compilar

### Pré-requisitos

- Toolchain AArch64 (aarch64-linux-gnu-*)
- Linux ou WSL com suporte a cross-compilation

### Compilação Manual

```bash
# Assemblar
aarch64-linux-gnu-as -o rafaelia_mvp.o rafaelia_mvp_puro.s

# Linkar (sem libc)
aarch64-linux-gnu-ld -nostdlib -static -o rafaelia_mvp rafaelia_mvp.o

# Verificar tamanho
ls -la rafaelia_mvp
# Esperado: ~2-3KB
```

### Execução (em dispositivo ARM64)

```bash
# Copiar para dispositivo
adb push rafaelia_mvp /data/local/tmp/

# Executar
adb shell /data/local/tmp/rafaelia_mvp
```

### Saída Esperada

```
╔══════════════════════════════════════════════════════════════╗
║  RAFAELIA MVP PURO - Zero Dependências                       ║
║  Autor: instituto-Rafael | Licença: GPLv3                    ║
║  Framework: RAFAELIA (Φ_ethica)                              ║
╚══════════════════════════════════════════════════════════════╝

[AUTH] Assinatura verificada: instituto-Rafael

[TEST] Iniciando operações low-level...
[MATH] Fast sqrt(16) = 4
[VEC]  Dot product = 70
[MX]   Matrix det 2x2 = 10
[MEM]  Memory checksum = 26

[DONE] Todas operações concluídas com sucesso.
```

---

## 🧮 Operações Implementadas

### 1. Fast Square Root (Newton-Raphson)

```
Algoritmo:
  guess = x / 2
  repeat 4 times:
    guess = (guess + x/guess) / 2
  return guess

Complexidade: O(1) - 4 iterações fixas
Precisão: ~0.01% erro relativo
```

### 2. Vector Dot Product

```
dot(a, b) = Σ(a[i] * b[i])

Exemplo:
  a = [1, 2, 3, 4]
  b = [5, 6, 7, 8]
  dot = 1*5 + 2*6 + 3*7 + 4*8 = 70

Complexidade: O(n)
```

### 3. Matrix Determinant 2x2

```
det([[a,b],[c,d]]) = ad - bc

Exemplo:
  M = [[3, 2], [1, 4]]
  det = 3*4 - 2*1 = 10

Complexidade: O(1)
```

### 4. Memory Checksum

```
checksum = XOR de todos os bytes

Usado para:
  - Verificação de integridade
  - Detecção de corrupção
  - Validação de assinatura
```

---

## 📊 Comparação com Abordagens Tradicionais

| Aspecto | MVP Puro | Com libc | Java |
|---------|----------|----------|------|
| Tamanho | ~2KB | ~50KB | ~5MB |
| Dependências | 0 | libc | JVM + libs |
| Tempo init | ~1μs | ~100μs | ~100ms |
| Relocável | Sim | Parcial | Não |
| Determinístico | Sim | Parcial | Não |

---

## 🔗 Integração com o Projeto

Este MVP serve como:

1. **Prova de Conceito** - Demonstra viabilidade de zero-dependency
2. **Referência** - Base para implementações mais complexas
3. **Educacional** - Material de aprendizado sobre low-level
4. **Benchmark** - Baseline para comparação de performance

### Uso no Termux RAFCODEΦ

O código pode ser integrado via JNI:

```c
// Carregar binário MVP como biblioteca
void* mvp = dlopen("/path/to/rafaelia_mvp", RTLD_NOW);

// Ou incluir diretamente no build nativo
// Via Android.mk ou CMakeLists.txt
```

---

## 📖 Referências Técnicas

### AArch64 ISA

- [ARM Architecture Reference Manual](https://developer.arm.com/documentation)
- Syscalls Linux: `/usr/include/asm-generic/unistd.h`

### IEEE 754 Floating Point

- Single Precision: 32 bits (1 sign + 8 exp + 23 mantissa)
- Constantes usadas: PI, E, 1.0, 0.5, 0.0

### Newton-Raphson

- Convergência quadrática
- 4 iterações = ~16 bits de precisão

---

## 🛡️ Segurança

### Características de Segurança

- **No Stack Canaries** - Não necessário (código simples)
- **No ASLR Needed** - Position Independent by design
- **No Heap** - Apenas stack e .bss
- **No Dynamic Linking** - Static linkage apenas

### Auditoria

O código é pequeno o suficiente para auditoria manual completa:

- ~500 linhas de assembly
- ~50 instruções únicas
- Sem bibliotecas externas
- Fluxo de controle linear

---

## 📝 Licença

```
RAFAELIA MVP PURO
Copyright (c) 2024-2026 instituto-Rafael

Este programa é software livre: você pode redistribuí-lo e/ou modificá-lo
sob os termos da Licença Pública Geral GNU conforme publicada pela Free
Software Foundation, seja a versão 3 da Licença, ou (a seu critério)
qualquer versão posterior.

Este programa é distribuído na esperança de que seja útil, mas SEM
QUALQUER GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO
ou ADEQUAÇÃO A UM DETERMINADO FIM. Veja a Licença Pública Geral GNU
para mais detalhes.
```

---

## ✍️ Autoria

**Autor**: instituto-Rafael  
**Framework**: RAFAELIA (RAfael FrAmework for Ethical Linear and Iterative Analysis)  
**Projeto**: Termux RAFCODEΦ  

### Assinatura Digital

```
Arquivo: rafaelia_mvp_puro.s
Hash SHA-256: [calculado em build]
Assinatura: RAFA-ELIA-INST-RAFA
Data: Janeiro 2026
```

---

**FIAT RAFAELIA** - Que haja computação ética e de baixo nível.

**Φ_ethica** - Minimizar entropia, maximizar coerência, zero abstrações.


## Auditoria desta estrutura

Consulte [AUDITORIA.md](./AUDITORIA.md) para achados de inconsistência, ausência e pontos de manutenção deste diretório.
