# RMR Lowlevel Purity Audit

| Arquivo | Hot path? | Usa malloc? | Usa stdio? | Usa libm? | Usa heap? | Usa ASM? | Usa Q16? | Status |
|---|---|---:|---:|---:|---:|---:|---:|---|
| app/src/main/cpp/Android.mk | N/A | N/A | N/A | N/A | N/A | N/A | N/A | NEEDS_ISOLATION |
| app/src/main/cpp/lowlevel/baremetal.c | Não | Sim | Sim | Indireto | Sim | Sim | Parcial | BLOCKER_PURE_CORE |
| app/src/main/cpp/lowlevel/baremetal_nomalloc.c | Sim | Não | Não | Não | Não | Sim | Parcial | PURE_HOTPATH |
| app/src/main/cpp/lowlevel/baremetal_asm.S | Sim | Não | Não | Não | Não | Sim | Não | PURE_HOTPATH |
| app/src/main/cpp/lowlevel/raf_bitraf.c | Sim | Não | Não | Não | Não | Não | Não | PURE_HOTPATH |
| app/src/main/cpp/lowlevel/raf_bitraf_debug.c | Não | Não | Sim | Não | Não | Não | Não | DEBUG_ONLY |
| app/src/main/cpp/lowlevel/raf_vcpu.c | Sim | Não | Não | Não | Não | Não | Sim | PURE_HOTPATH |
| app/src/main/cpp/lowlevel/raf_clock.c | Não | Não | Não | Não | Não | Não | Sim | COLD_PATH_OK |
| app/src/main/cpp/lowlevel/raf_memory_layers.c | Não | Não | Não | Não | Não | Não | Não | COLD_PATH_OK |
| rmr/Rrr/rafaelia_core.c | Misto | Não | Não | Sim (link) | Arena | Não | Sim | NEEDS_ISOLATION |
| rmr/Rrr/rafaelia_types.h | Sim | Não | Não | Não | Não | Não | Sim | PURE_HOTPATH |
| rmr/Rrr/rafaelia_b1.S | Sim | Não | Não | Não | mmap/syscall | Sim | Sim | NEEDS_REWRITE |
| mvp/rafaelia_mvp_puro.s | Não (MVP) | Não | Não | Não | brk syscall | Sim | Sim | COLD_PATH_OK |
