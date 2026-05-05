# RMR ASM Audit

Escopo auditado:
- `app/src/main/cpp/lowlevel/baremetal_asm.S`
- `rmr/Rrr/rafaelia_b1.S`
- `mvp/rafaelia_mvp_puro.s`

## 1) app/src/main/cpp/lowlevel/baremetal_asm.S

### Símbolos `.global` indispensáveis
- `bm_dot_neon`
- `bm_vadd_neon`
- `bm_memcpy_neon`

Observação: os três símbolos são ponto de entrada externo esperado para integração via C/C++ (JNI/native bridge), e aparecem tanto no bloco `__arm__` quanto `__aarch64__`.

### Alinhamento (16/64) real em seções
- ARM32 (`__arm__`): `.text` com `.align 4` (alinhamento de 16 bytes).
- ARM64 (`__aarch64__`): `.text` com `.align 4` (alinhamento de 16 bytes).
- Não há seção explicitamente alinhada em 64 bytes neste arquivo.

### Stack frame necessário vs desnecessário
- `bm_dot_neon` (ARM32 e ARM64): sem frame de stack; adequado (leaf puro).
- `bm_vadd_neon`:
  - ARM32: `push {r4, lr}` / `pop {r4, pc}` sem uso efetivo de `r4` fora de byte-temporário em outro símbolo; aqui o frame é **desnecessário** (pode reduzir para leaf puro).
  - ARM64: sem frame; adequado.
- `bm_memcpy_neon`:
  - ARM32: `push {r4, lr}` necessário pelo uso de `r4` como registrador temporário no loop de bytes e retorno via `pop {r4, pc}`.
  - ARM64: sem frame; adequado (`w4` caller-saved).

### Laços `subs/bne` quando aplicável
- ARM32:
  - `bm_dot_neon` remainder usa `subs r2, r2, #1` + `bne`.
  - `bm_vadd_neon` remainder usa `subs r3, r3, #1` + `bne`.
  - `bm_memcpy_neon` byte loop usa `subs r2, r2, #1` + `bne`.
- ARM64:
  - `bm_dot_neon` remainder usa `subs w2, w2, #1` + `b.ne`.
  - `bm_vadd_neon` remainder usa `subs w3, w3, #1` + `b.ne`.
  - `bm_memcpy_neon` byte loop usa `subs x2, x2, #1` + `b.ne`.

### Ausência de chamadas libc
- Confirmado: não há `bl` para símbolos libc (`memcpy`, `printf`, etc.). Código é puro asm com instruções SIMD/load-store/branch.

### Separação ARM32/ARM64 e ABI correta
- Separação por pré-processador `#ifdef __arm__` e `#ifdef __aarch64__`: correta.
- ABI:
  - ARM32 AAPCS: argumentos em `r0-r3`, retorno em `r0` (inteiro/padrão). **Risco**: `bm_dot_neon` retorna float, mas termina com `vmov r0, s0` (padrão softfp-like). Para hard-float estrito, retorno de `float` deveria estar em `s0` sem remap para `r0`.
  - ARM64 AAPCS64: argumentos em `x0-x3`, retorno float em `s0`; implementação usa `fmov w0, s0`, o que sugere retorno inteiro-bitcast e pode conflitar com assinatura C de `float`.

### Classificação HOT/COLD/DEMO e bloqueios para pure core
- `bm_dot_neon`: **HOT** (núcleo matemático).
- `bm_vadd_neon`: **HOT** (vetorização fundamental).
- `bm_memcpy_neon`: **HOT** (movimentação de memória).
- Bloqueios pure core:
  1. Sem alinhamento 64-byte para pontos críticos de fetch/cache.
  2. Ambiguidade de ABI de retorno float em ARM32/ARM64 (potencial incompatibilidade em chamadas C).
  3. Frame extra em `bm_vadd_neon` ARM32 (fricção micro-otimização).

---

## 2) rmr/Rrr/rafaelia_b1.S

### Símbolos `.global` indispensáveis
- `_start` (entrypoint do binário).

Observação: os demais símbolos estão com `.type` mas sem `.global`, coerente para rotinas internas locais (não exportadas).

### Alinhamento (16/64) real em seções
- `.rodata` com `.align 6` (64 bytes).
- `attractor_table` com `.align 5` (32 bytes).
- `.bss` com `.align 6` (64 bytes), com blocos internos `.align 5` e `.align 6`.
- `.text` com `.align 2` (4 bytes) para instrução ARM32.

Conclusão: dados críticos já têm uso forte de 64 bytes em áreas globais; seção de código não está em 16/64.

### Stack frame necessário vs desnecessário
- `_start`: sem frame formal, aceitável para entrypoint.
- Funções com `push/pop` extensivo (`arena_init`, `arena_alloc`, `crc32c_gentable`, etc.) aparentam coerentes ao uso de callee-saved e chamadas internas.
- O padrão geral indica frame **majoritariamente necessário** por alta densidade de sub-rotinas e preservação explícita de registradores.

### Laços `subs/bne` quando aplicável
- Presente em múltiplos loops internos (ex.: geração de tabela CRC com `subs r7, r7, #1` + `bne`).
- Estilo consistente com countdown loop de baixo overhead para ARM32.

### Ausência de chamadas libc
- Confirmado: syscalls diretas (`swi #0`) com números `SYS_*`, sem dependência de libc.

### Separação ARM32/ARM64 e ABI correta
- Arquivo é estritamente ARM32 AArch32 (comentário + sintaxe + registradores `r*` + `swi`).
- ABI operacional de binário standalone está coerente com entrypoint `_start` Linux ARM32.

### Classificação HOT/COLD/DEMO e bloqueios para pure core
- Classificação global: **DEMO/HYBRID CORE** (runtime completo com mensagens, selftests e orquestração).
- Hotspots internos:
  - `crc32c_gentable`: **HOT**
  - `rafaelia_run_42`/núcleo iterativo (quando presente no restante do arquivo): **HOT**
- Blocos de I/O e boot/status: **COLD**
- Bloqueios pure core:
  1. Forte acoplamento de core com telemetria textual (`sys_write_stdout`).
  2. Entrypoint monolítico `_start` mistura boot, validação e execução.
  3. Não há split claro entre kernel matemático e camada de observabilidade.

---

## 3) mvp/rafaelia_mvp_puro.s

### Símbolos `.global` indispensáveis
- `_start` (entrypoint principal).

Observação: demais rotinas são locais, chamadas via `bl` interno.

### Alinhamento (16/64) real em seções
- `.rodata` com `.align 6` (64 bytes).
- `.bss` com `.align 6` (64 bytes).
- `.text` com `.align 4` (16 bytes).

Conclusão: política de alinhamento está explícita e consistente com objetivo de cache line nos dados.

### Stack frame necessário vs desnecessário
- `_start`: sem frame formal; aceitável para entrypoint.
- `verify_authorship`: frame explícito `stp x29, x30` / `ldp` necessário (não-leaf com syscalls e branches de falha).
- `fast_sqrt`: leaf sem frame; adequado.
- Padrão geral é limpo, com frame apenas onde há necessidade de preservar LR/FP.

### Laços `subs/bne` quando aplicável
- Exemplo claro em `fast_sqrt`: `subs x3, x3, #1` + `b.ne sqrt_loop`.
- Padrão eficiente para controle iterativo.

### Ausência de chamadas libc
- Confirmado: apenas syscalls via `svc #0`; sem símbolos libc.

### Separação ARM32/ARM64 e ABI correta
- Arquivo é AArch64-only (registradores `x*`, `w*`, `svc`, `stp/ldp`).
- Convenções de chamada internas coerentes com AAPCS64 para rotinas locais.

### Classificação HOT/COLD/DEMO e bloqueios para pure core
- Classificação global: **DEMO** (banner, mensagens, validação de autoria e testes encadeados).
- Núcleos computacionais (`fast_sqrt`, testes matemáticos): **HOT local**.
- I/O textual e sequência de apresentação: **COLD**.
- Bloqueios pure core:
  1. Forte presença de fluxo demonstrativo em `_start`.
  2. Rotinas de teste e output intercaladas com computação.
  3. Falta de entrypoint alternativo silencioso para integração em pipeline puro.

---

## Matriz de bloqueio para evolução PURE CORE

| Arquivo | Classe atual | Pronto p/ pure core? | Bloqueio principal |
|---|---|---:|---|
| `app/src/main/cpp/lowlevel/baremetal_asm.S` | HOT | Parcial | ABI de retorno float e ausência de alinhamento 64 em código |
| `rmr/Rrr/rafaelia_b1.S` | DEMO/HYBRID CORE | Não | Monolito `_start` + I/O acoplado |
| `mvp/rafaelia_mvp_puro.s` | DEMO | Não | Fluxo demonstrativo obrigatório em runtime |

## Gate mínimo recomendado (objetivo pure core)

1. Corrigir retorno float ABI em `bm_dot_neon` (ARM32/ARM64) para assinatura C inequívoca.
2. Extrair camada de I/O/prints dos entrypoints `_start` em B1/MVP para wrapper opcional.
3. Introduzir símbolo core dedicado (ex.: `rafaelia_core_run`) sem side-effects de stdout.
4. Preservar build demo em paralelo (sem quebrar trilha atual).
