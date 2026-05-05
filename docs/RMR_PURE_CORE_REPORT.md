# RMR Pure Core Report

## F_ok
- `baremetal_nomalloc.c` habilitado no modo puro.
- `raf_bitraf.c` hotpath sem stdio/snprintf.
- Constantes críticas centralizadas em `rmr/include/rmr_hex_const.h`.
- Flags de pureza adicionadas ao build.

## F_gap
- `rmr/Rrr/rafaelia_core.c` ainda mistura funções de probe/boot com lógica de core.
- Alguns módulos ASM ainda estão em perfil demo/runner e não somente core lib.

## F_error
- Divergência histórica Q16 sqrt3/2: 0xDDB3 vs 0xDDB4.
- Decisão oficial fixada: **0xDDB4** (mais próximo de 0.8660254 em Q16).

## F_next
- Patch mínimo beta: pronto com flags, isolamento bitraf e selftest.
- Patch ideal pure core: split total de `rafaelia_core.c` em hot/cold unidades.
- Benchmark necessário: latência vCPU step + CRC32C em armv7 e arm64.
