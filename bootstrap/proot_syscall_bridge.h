#ifndef PROOT_SYSCALL_BRIDGE_H
#define PROOT_SYSCALL_BRIDGE_H

#include <stdint.h>
#include "proot_config.h"

/* ARM64 Syscall Invocation Layer (freestanding, no libc) */
/* SVC #0 instruction: ARM64 syscall ABI */
/* Arguments: x0-x5 (syscall args), x8 (syscall number) */
/* Return: x0 (result) */

typedef int64_t syscall_result_t;

/* ARM64 Syscall Numbers (relevant to bootstrap) */
#define SYSCALL_OPEN           56
#define SYSCALL_CLOSE          57
#define SYSCALL_READ           63
#define SYSCALL_WRITE          64
#define SYSCALL_LSEEK          62
#define SYSCALL_STAT           106
#define SYSCALL_FSTAT          80
#define SYSCALL_LSTAT          107
#define SYSCALL_POLL           72
#define SYSCALL_IOCTL          29
#define SYSCALL_FCNTL          25
#define SYSCALL_MKDIR          34
#define SYSCALL_RMDIR          35
#define SYSCALL_UNLINK         35
#define SYSCALL_UNLINKAT       35
#define SYSCALL_CHDIR          49
#define SYSCALL_GETCWD         17
#define SYSCALL_MMAP           222
#define SYSCALL_MPROTECT       226
#define SYSCALL_MUNMAP         215
#define SYSCALL_BRKS           214  /* brk — not used in freestanding */
#define SYSCALL_RT_SIGACTION   134
#define SYSCALL_RT_SIGPROCMASK 135
#define SYSCALL_PRCTL          167
#define SYSCALL_ARCH_PRCTL     158
#define SYSCALL_EXIT           93
#define SYSCALL_EXIT_GROUP     94
#define SYSCALL_FUTEX          98
#define SYSCALL_SCHED_GETAFFINITY 123
#define SYSCALL_SCHED_SETAFFINITY 122
#define SYSCALL_GETPID         39
#define SYSCALL_GETTID         224
#define SYSCALL_GETTIMEOFDAY   169
#define SYSCALL_CLOCK_GETTIME  113
#define SYSCALL_NANOSLEEP      101
#define SYSCALL_SEM_TIMEDWAIT  (SEM_TIMEDWAIT_FUTEX_BASED)  /* futex-backed */

/* ARM64 Direct Syscall Macro */
/* Inline assembler: call syscall via SVC #0 */
/* Clobbers: x0-x15, cc (condition codes) */
static inline syscall_result_t __proot_syscall(
    uint64_t nr,      /* syscall number (x8) */
    uint64_t a0,      /* arg 1 (x0) */
    uint64_t a1,      /* arg 2 (x1) */
    uint64_t a2,      /* arg 3 (x2) */
    uint64_t a3,      /* arg 4 (x3) */
    uint64_t a4,      /* arg 5 (x4) */
    uint64_t a5       /* arg 6 (x5) */
) {
    syscall_result_t result;

    __asm__ volatile (
        "mov x8, %[nr]\n"
        "mov x0, %[a0]\n"
        "mov x1, %[a1]\n"
        "mov x2, %[a2]\n"
        "mov x3, %[a3]\n"
        "mov x4, %[a4]\n"
        "mov x5, %[a5]\n"
        "svc #0\n"
        "mov %[result], x0\n"
        : [result] "=r" (result)
        : [nr] "r" (nr), [a0] "r" (a0), [a1] "r" (a1),
          [a2] "r" (a2), [a3] "r" (a3), [a4] "r" (a4), [a5] "r" (a5)
        : "x0", "x1", "x2", "x3", "x4", "x5", "x8", "cc"
    );

    return result;
}

/* Convenience wrappers for common bootstrap syscalls */

static inline syscall_result_t proot_sys_open(const char *path, int flags, int mode) {
    return __proot_syscall(SYSCALL_OPEN, (uint64_t)path, flags, mode, 0, 0, 0);
}

static inline syscall_result_t proot_sys_close(int fd) {
    return __proot_syscall(SYSCALL_CLOSE, fd, 0, 0, 0, 0, 0);
}

static inline syscall_result_t proot_sys_read(int fd, void *buf, uint64_t count) {
    return __proot_syscall(SYSCALL_READ, fd, (uint64_t)buf, count, 0, 0, 0);
}

static inline syscall_result_t proot_sys_write(int fd, const void *buf, uint64_t count) {
    return __proot_syscall(SYSCALL_WRITE, fd, (uint64_t)buf, count, 0, 0, 0);
}

static inline syscall_result_t proot_sys_mkdir(const char *path, int mode) {
    return __proot_syscall(SYSCALL_MKDIR, (uint64_t)path, mode, 0, 0, 0, 0);
}

static inline syscall_result_t proot_sys_chdir(const char *path) {
    return __proot_syscall(SYSCALL_CHDIR, (uint64_t)path, 0, 0, 0, 0, 0);
}

static inline syscall_result_t proot_sys_exit(int code) {
    return __proot_syscall(SYSCALL_EXIT_GROUP, code, 0, 0, 0, 0, 0);
}

static inline syscall_result_t proot_sys_clock_gettime(int clockid, void *tp) {
    return __proot_syscall(SYSCALL_CLOCK_GETTIME, clockid, (uint64_t)tp, 0, 0, 0, 0);
}

static inline syscall_result_t proot_sys_prctl(int op, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    return __proot_syscall(SYSCALL_PRCTL, op, arg2, arg3, arg4, arg5, 0);
}

static inline syscall_result_t proot_sys_mmap(void *addr, uint64_t length, int prot, int flags, int fd, uint64_t offset) {
    return __proot_syscall(SYSCALL_MMAP, (uint64_t)addr, length, prot, flags, fd, offset);
}

static inline syscall_result_t proot_sys_mprotect(void *addr, uint64_t length, int prot) {
    return __proot_syscall(SYSCALL_MPROTECT, (uint64_t)addr, length, prot, 0, 0, 0);
}

static inline syscall_result_t proot_sys_munmap(void *addr, uint64_t length) {
    return __proot_syscall(SYSCALL_MUNMAP, (uint64_t)addr, length, 0, 0, 0, 0);
}

/* Error code detection */
#define IS_SYSCALL_ERROR(result) ((int64_t)(result) < 0 && (int64_t)(result) >= -4096)

#endif /* PROOT_SYSCALL_BRIDGE_H */
