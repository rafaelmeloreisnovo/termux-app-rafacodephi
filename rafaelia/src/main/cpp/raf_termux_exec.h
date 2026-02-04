/* raf_termux_exec.h
   Execucao real low-level (syscall direto, sem libc)
*/
#ifndef RAF_TERMUX_EXEC_H
#define RAF_TERMUX_EXEC_H

typedef unsigned char      u8;
typedef unsigned int       u32;

typedef struct {
  u32 status;
} raf_exec_result_t;

u32 RmR_exec_host_termux(const u8 *name, u32 len, raf_exec_result_t *out);

#endif
