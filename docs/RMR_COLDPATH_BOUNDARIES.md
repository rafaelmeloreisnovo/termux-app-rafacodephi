# RMR Cold Path Boundaries

## COLD_PATH
- boot/probe/log/debug/JNI/Android.
- `raf_clock_now_ns` usa `clock_gettime`: **MEASURE_PATH/COLD_PATH**.
- `raf_memory_layers_get` usa `sysconf`: **COLD_PATH**.
- `baremetal.c` classificado como `REFERENCE_COMPAT_NOT_PURE`.

## HOT_PATH
- BitRAF encode/decode/validate
- vCPU step/validate
- CRC32C step
- arena estática em `baremetal_nomalloc.c`
- ASM utilitário sem libc.

## Regra operacional
- Loop crítico **não chama** `clock_gettime`, `sysconf`, `dlopen`, `dlsym`, formatação de string.
