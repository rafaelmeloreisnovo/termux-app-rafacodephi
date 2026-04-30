# TAIL / SHADOW / RESERVOIR MODEL (Termux RAFCODEΦ)

## Tails (último estado dependente)

### simd_tail
- `app/src/main/cpp/lowlevel/baremetal.c`
  - `vop_add()` usa `simd_n = n - (n % step)` e processa resto no loop escalar.
  - `vop_dot()` usa `simd_n = n - (n % step)` e acumula resto no loop escalar.
  - `bmem_cpy()` usa `simd_n = n - (n % step)` e completa bytes restantes no loop escalar.
- `app/src/main/cpp/lowlevel/baremetal_asm.S`
  - `bm_dot_neon`, `bm_vadd_neon`, `bm_memcpy_neon` têm loop vetorial + loop remainder.

### scalar_tail
- `app/src/main/cpp/lowlevel/baremetal.c`
  - fallback completo em `vop_add`, `vop_dot`, `bmem_cpy` quando ASM/SIMD indisponível.

### remainder_tail
- `app/src/main/cpp/lowlevel/baremetal.c`: loops `for (i = simd_n; i < n; i++)`.
- `app/src/main/cpp/lowlevel/baremetal_asm.S`: labels `*_remainder` e `*_rem_loop`.

### transcript_tail
- `terminal-emulator/.../TerminalBuffer.java`
  - `resize()` preserva transcript e cursor durante reflow, incluindo linhas em branco skip/insert.

### scrollback_tail
- `terminal-emulator/.../TerminalBuffer.java`
  - `scrollDownOneLine()` atualiza ring buffer, transcript ativo e linha recém-exposta.

### process_tail
- `terminal-emulator/.../TerminalSession.java`
  - `cleanupResources()` e threads de I/O encerram estado do processo até notificação final.
- `termux-shared/.../AppShell.java`
  - `executeInner()` drena STDOUT/STDERR e espera término com timeout.

### cleanup_tail
- `app/src/main/cpp/lowlevel/baremetal.c`
  - `mx_free()`, `arena_destroy()`, limpeza em caminhos de erro de inversão/solve.

## Shadows (estado provisório/isolado)

### bootstrap_shadow
- `app/src/main/java/com/termux/app/TermuxInstaller.java`
  - extração em `TERMUX_STAGING_PREFIX_DIR_PATH` + rename atômico para `$PREFIX`.

### staging_shadow
- `TermuxInstaller`: validação de caminho canônico e guard de ZipEntry/SYMLINKS no staging.

### prefix_shadow
- `TermuxInstaller`: somente promove para prefix após verificação de binários runtime.

### resize_shadow
- `TerminalBuffer.resize()`: estado antigo copiado para buffer novo antes de publicar cursor/screen.

### row_shadow
- `TerminalBuffer`: `mScreenFirstRow`, `mActiveTranscriptRows`, `externalToInternalRow()` como shadow de coordenadas.

### capability_shadow
- `app/src/main/cpp/lowlevel/baremetal.c`
  - `init_runtime_caps_once()` publica snapshot único (`binary/runtime/valid`) após detecção completa.
- `app/src/main/java/com/termux/lowlevel/BareMetal.java`
  - `CapabilitiesDetail` espelha snapshot nativo sem publicar parcial.

### matrix_shadow
- `baremetal.c` + `BareMetal.java`
  - wrappers de matriz como shadow de ownership de ponteiros nativos.

### arena_shadow (reservoir)
- `baremetal.c`
  - `mx_arena_t` controla alocação monotônica; consumidores vivem até reset/destroy explícito.

## Verificações de integridade
1. SIMD remainder (`n % step`) está coberto por tail escalar.
2. ASM remainder usa labels de resto sem perda de elemento.
3. Resize terminal mantém cursor/linhas com clamps finais.
4. Staging shadow não escapa prefix por canonical-path guard.
5. Capability shadow publica snapshot completo via `pthread_once`.
6. Matrix/arena ownership: sem `double-free` quando arena-backed matrices não chamam `mx_free` individual.

## Último estado sustentado
- O sistema sustenta consistência ao publicar apenas estado final de:
  - capacidades de hardware,
  - bootstrap staging,
  - reflow de terminal,
  - ownership de arena/matriz.
