# RMR ASM Audit

## app/src/main/cpp/lowlevel/baremetal_asm.S
- Símbolos globais mínimos para rotinas exportadas de memcpy/set.
- Sem chamada libc.
- Caminho hot com loads/stores diretos.

## rmr/Rrr/rafaelia_b1.S
- Usa `_start` e múltiplos símbolos de boot/IO: classificado como híbrido demo-runtime.
- Tem stack frames em subrotinas; aceitável no contexto, mas não ideal para hot core minimal.
- Usa syscalls diretas, sem libc.
- Recomenda-se split entre core asm puro e runner de boot.

## mvp/rafaelia_mvp_puro.s
- AArch64 separado corretamente de ARM32.
- Usa syscalls diretas (write/exit/brk), sem libc.
- Não é módulo hotpath de produção JNI; manter como MVP/lab.
