#ifndef SYSCALL_ARM64_H
#define SYSCALL_ARM64_H

/* ARM64 direct syscalls via SVC #0 — freestanding */

#include "freestanding.h"

/* ARM64 syscall numbers (EABI) */
#define SYS_read            63
#define SYS_write           64
#define SYS_open            1024
#define SYS_close           57
#define SYS_lseek           62
#define SYS_mmap            222
#define SYS_mprotect        226
#define SYS_clock_gettime   113
#define SYS_clock_nanosleep 115
#define SYS_exit            93
#define SYS_exit_group      94
#define SYS_fork            1079
#define SYS_clone           220
#define SYS_execve          221
#define SYS_wait4           114
#define SYS_kill            129
#define SYS_tkill           268
#define SYS_tgkill          268
#define SYS_prctl           167
#define SYS_getcwd          17
#define SYS_chdir           49
#define SYS_access          21
#define SYS_stat            106
#define SYS_lstat           107
#define SYS_chmod           90
#define SYS_umask           95
#define SYS_unlink          23
#define SYS_rmdir           82
#define SYS_mkdir           34
#define SYS_brk             214
#define SYS_getpid          172
#define SYS_gettid          224
#define SYS_getuid          174
#define SYS_getgid          176
#define SYS_dup             23
#define SYS_dup2            33
#define SYS_dup3            32
#define SYS_pipe2           59
#define SYS_fcntl           25

/* Inline syscall wrappers — no calling convention overhead */

static inline int64_t syscall_read(int fd, void *buf, uint64_t count) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)fd;
    register int64_t x1 asm("x1") = (int64_t)buf;
    register int64_t x2 asm("x2") = (int64_t)count;
    register int64_t x8 asm("x8") = SYS_read;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_write(int fd, const void *buf, uint64_t count) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)fd;
    register int64_t x1 asm("x1") = (int64_t)buf;
    register int64_t x2 asm("x2") = (int64_t)count;
    register int64_t x8 asm("x8") = SYS_write;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_open(const char *path, int flags, int mode) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)path;
    register int64_t x1 asm("x1") = (int64_t)flags;
    register int64_t x2 asm("x2") = (int64_t)mode;
    register int64_t x8 asm("x8") = SYS_open;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_close(int fd) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)fd;
    register int64_t x8 asm("x8") = SYS_close;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_lseek(int fd, int64_t offset, int whence) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)fd;
    register int64_t x1 asm("x1") = (int64_t)offset;
    register int64_t x2 asm("x2") = (int64_t)whence;
    register int64_t x8 asm("x8") = SYS_lseek;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_clock_gettime(int clockid, void *tp) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)clockid;
    register int64_t x1 asm("x1") = (int64_t)tp;
    register int64_t x8 asm("x8") = SYS_clock_gettime;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_exit(int status) {
    register int64_t x0 asm("x0") = (int64_t)status;
    register int64_t x8 asm("x8") = SYS_exit;
    asm volatile("svc #0" : : "r"(x0), "r"(x8) : "memory");
    return -1; /* unreachable */
}

static inline int64_t syscall_execve(const char *filename, char *const argv[], char *const envp[]) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)filename;
    register int64_t x1 asm("x1") = (int64_t)argv;
    register int64_t x2 asm("x2") = (int64_t)envp;
    register int64_t x8 asm("x8") = SYS_execve;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_fork(void) {
    int64_t ret;
    register int64_t x0 asm("x0");
    register int64_t x8 asm("x8") = SYS_fork;
    asm volatile("svc #0" : "=r"(x0) : "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_wait4(int pid, int *wstatus, int options, void *rusage) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)pid;
    register int64_t x1 asm("x1") = (int64_t)wstatus;
    register int64_t x2 asm("x2") = (int64_t)options;
    register int64_t x3 asm("x3") = (int64_t)rusage;
    register int64_t x8 asm("x8") = SYS_wait4;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_kill(int pid, int sig) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)pid;
    register int64_t x1 asm("x1") = (int64_t)sig;
    register int64_t x8 asm("x8") = SYS_kill;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

static inline int64_t syscall_prctl(int option, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5) {
    int64_t ret;
    register int64_t x0 asm("x0") = (int64_t)option;
    register int64_t x1 asm("x1") = (int64_t)arg2;
    register int64_t x2 asm("x2") = (int64_t)arg3;
    register int64_t x3 asm("x3") = (int64_t)arg4;
    register int64_t x4 asm("x4") = (int64_t)arg5;
    register int64_t x8 asm("x8") = SYS_prctl;
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8) : "memory");
    ret = x0;
    return ret;
}

#endif /* SYSCALL_ARM64_H */
