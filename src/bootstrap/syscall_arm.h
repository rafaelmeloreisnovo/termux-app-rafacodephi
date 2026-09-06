#ifndef RAFCODEPHI_SYSCALL_ARM_H
#define RAFCODEPHI_SYSCALL_ARM_H

/*
 * RAFCODEPHI direct Linux/Android syscall adapter.
 *
 * Scope:
 *   - ARM32 EABI and AArch64 only;
 *   - no libc, malloc, pthread, stdio, POSIX wrapper or dynamic loader;
 *   - platform adapter for the freestanding bootstrap controller;
 *   - this file does NOT make payload programs (apt/dpkg/proot.real) freestanding.
 *
 * The public syscall_* names intentionally match the historical bootstrap
 * adapter so callers can migrate without parallel runtime state.
 */

#include "freestanding.h"

#define RAF_AT_FDCWD      (-100)
#define RAF_AT_REMOVEDIR  0x200
#define RAF_SIGCHLD       17

#define RAF_O_RDONLY      0x0000
#define RAF_O_WRONLY      0x0001
#define RAF_O_RDWR        0x0002
#define RAF_O_CREAT       0x0040
#define RAF_O_EXCL        0x0080
#define RAF_O_TRUNC       0x0200
#define RAF_O_APPEND      0x0400
#define RAF_O_CLOEXEC     0x80000

#if defined(__aarch64__)

#define RAF_NR_FCNTL            25
#define RAF_NR_MKDIRAT          34
#define RAF_NR_UNLINKAT         35
#define RAF_NR_SYMLINKAT        36
#define RAF_NR_FACCESSAT        48
#define RAF_NR_CHDIR            49
#define RAF_NR_FCHMODAT         53
#define RAF_NR_OPENAT           56
#define RAF_NR_CLOSE            57
#define RAF_NR_PIPE2            59
#define RAF_NR_LSEEK            62
#define RAF_NR_READ             63
#define RAF_NR_WRITE            64
#define RAF_NR_READLINKAT       78
#define RAF_NR_NEWFSTATAT       79
#define RAF_NR_FSTAT            80
#define RAF_NR_FSYNC            82
#define RAF_NR_EXIT             93
#define RAF_NR_EXIT_GROUP       94
#define RAF_NR_CLOCK_GETTIME    113
#define RAF_NR_KILL             129
#define RAF_NR_TKILL            130
#define RAF_NR_TGKILL           131
#define RAF_NR_PRCTL            167
#define RAF_NR_GETPID           172
#define RAF_NR_GETUID           174
#define RAF_NR_GETGID           176
#define RAF_NR_GETTID           178
#define RAF_NR_MUNMAP           215
#define RAF_NR_CLONE            220
#define RAF_NR_EXECVE           221
#define RAF_NR_MMAP             222
#define RAF_NR_MPROTECT         226
#define RAF_NR_WAIT4            260

static __inline__ int64_t raf_sc1(int64_t n, int64_t a0) {
    register int64_t x0 __asm__("x0") = a0;
    register int64_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory", "cc");
    return x0;
}

static __inline__ int64_t raf_sc2(int64_t n, int64_t a0, int64_t a1) {
    register int64_t x0 __asm__("x0") = a0;
    register int64_t x1 __asm__("x1") = a1;
    register int64_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory", "cc");
    return x0;
}

static __inline__ int64_t raf_sc3(int64_t n, int64_t a0, int64_t a1, int64_t a2) {
    register int64_t x0 __asm__("x0") = a0;
    register int64_t x1 __asm__("x1") = a1;
    register int64_t x2 __asm__("x2") = a2;
    register int64_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory", "cc");
    return x0;
}

static __inline__ int64_t raf_sc4(int64_t n, int64_t a0, int64_t a1, int64_t a2, int64_t a3) {
    register int64_t x0 __asm__("x0") = a0;
    register int64_t x1 __asm__("x1") = a1;
    register int64_t x2 __asm__("x2") = a2;
    register int64_t x3 __asm__("x3") = a3;
    register int64_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory", "cc");
    return x0;
}

static __inline__ int64_t raf_sc5(int64_t n, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4) {
    register int64_t x0 __asm__("x0") = a0;
    register int64_t x1 __asm__("x1") = a1;
    register int64_t x2 __asm__("x2") = a2;
    register int64_t x3 __asm__("x3") = a3;
    register int64_t x4 __asm__("x4") = a4;
    register int64_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8) : "memory", "cc");
    return x0;
}

#define RAF_PTR(v) ((int64_t)(intptr_t)(v))

#elif defined(__arm__)

/* Linux ARM EABI syscall numbers. clock_gettime64 is used to keep the
 * timespec contract 64-bit on 32-bit Android kernels. */
#define RAF_NR_EXIT             1
#define RAF_NR_FORK             2
#define RAF_NR_READ             3
#define RAF_NR_WRITE            4
#define RAF_NR_OPEN             5
#define RAF_NR_CLOSE            6
#define RAF_NR_UNLINK           10
#define RAF_NR_EXECVE           11
#define RAF_NR_CHDIR            12
#define RAF_NR_CHMOD            15
#define RAF_NR_LSEEK            19
#define RAF_NR_GETPID           20
#define RAF_NR_ACCESS           33
#define RAF_NR_KILL             37
#define RAF_NR_MKDIR            39
#define RAF_NR_RMDIR            40
#define RAF_NR_DUP              41
#define RAF_NR_FCNTL            55
#define RAF_NR_UMASK            60
#define RAF_NR_DUP2             63
#define RAF_NR_SYMLINK          83
#define RAF_NR_READLINK         85
#define RAF_NR_MUNMAP           91
#define RAF_NR_WAIT4            114
#define RAF_NR_FSYNC            118
#define RAF_NR_CLONE            120
#define RAF_NR_MPROTECT         125
#define RAF_NR_PRCTL            172
#define RAF_NR_MMAP2            192
#define RAF_NR_FSTAT64          197
#define RAF_NR_GETUID32         199
#define RAF_NR_GETGID32         200
#define RAF_NR_GETTID           224
#define RAF_NR_EXIT_GROUP       248
#define RAF_NR_TKILL            238
#define RAF_NR_TGKILL           268
#define RAF_NR_DUP3             358
#define RAF_NR_PIPE2            359
#define RAF_NR_CLOCK_GETTIME64  403

static __inline__ int64_t raf_sc1(int32_t n, int32_t a0) {
    register int32_t r0 __asm__("r0") = a0;
    register int32_t r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r7) : "memory", "cc");
    return (int64_t)r0;
}

static __inline__ int64_t raf_sc2(int32_t n, int32_t a0, int32_t a1) {
    register int32_t r0 __asm__("r0") = a0;
    register int32_t r1 __asm__("r1") = a1;
    register int32_t r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r7) : "memory", "cc");
    return (int64_t)r0;
}

static __inline__ int64_t raf_sc3(int32_t n, int32_t a0, int32_t a1, int32_t a2) {
    register int32_t r0 __asm__("r0") = a0;
    register int32_t r1 __asm__("r1") = a1;
    register int32_t r2 __asm__("r2") = a2;
    register int32_t r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory", "cc");
    return (int64_t)r0;
}

static __inline__ int64_t raf_sc4(int32_t n, int32_t a0, int32_t a1, int32_t a2, int32_t a3) {
    register int32_t r0 __asm__("r0") = a0;
    register int32_t r1 __asm__("r1") = a1;
    register int32_t r2 __asm__("r2") = a2;
    register int32_t r3 __asm__("r3") = a3;
    register int32_t r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r7) : "memory", "cc");
    return (int64_t)r0;
}

static __inline__ int64_t raf_sc5(int32_t n, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4) {
    register int32_t r0 __asm__("r0") = a0;
    register int32_t r1 __asm__("r1") = a1;
    register int32_t r2 __asm__("r2") = a2;
    register int32_t r3 __asm__("r3") = a3;
    register int32_t r4 __asm__("r4") = a4;
    register int32_t r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r7) : "memory", "cc");
    return (int64_t)r0;
}

#define RAF_PTR(v) ((int32_t)(intptr_t)(v))

#else
#error "RAFCODEPHI freestanding bootstrap supports ARM32 or AArch64 only"
#endif

static __inline__ int64_t syscall_read(int fd, void *buf, uint64_t count) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_READ, fd, RAF_PTR(buf), (int64_t)count);
#else
    return raf_sc3(RAF_NR_READ, fd, RAF_PTR(buf), (int32_t)count);
#endif
}

static __inline__ int64_t syscall_write(int fd, const void *buf, uint64_t count) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_WRITE, fd, RAF_PTR(buf), (int64_t)count);
#else
    return raf_sc3(RAF_NR_WRITE, fd, RAF_PTR(buf), (int32_t)count);
#endif
}

static __inline__ int64_t syscall_open(const char *path, int flags, int mode) {
#if defined(__aarch64__)
    return raf_sc4(RAF_NR_OPENAT, RAF_AT_FDCWD, RAF_PTR(path), flags, mode);
#else
    return raf_sc3(RAF_NR_OPEN, RAF_PTR(path), flags, mode);
#endif
}

static __inline__ int64_t syscall_close(int fd) {
    return raf_sc1(RAF_NR_CLOSE, fd);
}

static __inline__ int64_t syscall_clock_gettime(int clockid, void *tp) {
#if defined(__aarch64__)
    return raf_sc2(RAF_NR_CLOCK_GETTIME, clockid, RAF_PTR(tp));
#else
    return raf_sc2(RAF_NR_CLOCK_GETTIME64, clockid, RAF_PTR(tp));
#endif
}

static __inline__ __attribute__((noreturn)) void syscall_exit(int status) {
    (void)raf_sc1(RAF_NR_EXIT, status);
    for (;;) { __asm__ volatile("" ::: "memory"); }
}

static __inline__ __attribute__((noreturn)) void syscall_exit_group(int status) {
    (void)raf_sc1(RAF_NR_EXIT_GROUP, status);
    for (;;) { __asm__ volatile("" ::: "memory"); }
}

static __inline__ int64_t syscall_execve(const char *filename, char *const argv[], char *const envp[]) {
    return raf_sc3(RAF_NR_EXECVE, RAF_PTR(filename), RAF_PTR(argv), RAF_PTR(envp));
}

/* AArch64 has no fork syscall. clone(SIGCHLD, 0, 0, 0, 0) supplies fork-like
 * semantics without CLONE_VM and works for the ARM32 path too. */
static __inline__ int64_t syscall_fork(void) {
    return raf_sc5(RAF_NR_CLONE, RAF_SIGCHLD, 0, 0, 0, 0);
}

static __inline__ int64_t syscall_wait4(int pid, int *wstatus, int options, void *rusage) {
    return raf_sc4(RAF_NR_WAIT4, pid, RAF_PTR(wstatus), options, RAF_PTR(rusage));
}

static __inline__ int64_t syscall_kill(int pid, int sig) {
    return raf_sc2(RAF_NR_KILL, pid, sig);
}

static __inline__ int64_t syscall_mkdir(const char *path, int mode) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_MKDIRAT, RAF_AT_FDCWD, RAF_PTR(path), mode);
#else
    return raf_sc2(RAF_NR_MKDIR, RAF_PTR(path), mode);
#endif
}

static __inline__ int64_t syscall_unlink(const char *path) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_UNLINKAT, RAF_AT_FDCWD, RAF_PTR(path), 0);
#else
    return raf_sc1(RAF_NR_UNLINK, RAF_PTR(path));
#endif
}

static __inline__ int64_t syscall_rmdir(const char *path) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_UNLINKAT, RAF_AT_FDCWD, RAF_PTR(path), RAF_AT_REMOVEDIR);
#else
    return raf_sc1(RAF_NR_RMDIR, RAF_PTR(path));
#endif
}

static __inline__ int64_t syscall_chmod(const char *path, int mode) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_FCHMODAT, RAF_AT_FDCWD, RAF_PTR(path), mode);
#else
    return raf_sc2(RAF_NR_CHMOD, RAF_PTR(path), mode);
#endif
}

static __inline__ int64_t syscall_symlink(const char *target, const char *linkpath) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_SYMLINKAT, RAF_PTR(target), RAF_AT_FDCWD, RAF_PTR(linkpath));
#else
    return raf_sc2(RAF_NR_SYMLINK, RAF_PTR(target), RAF_PTR(linkpath));
#endif
}

static __inline__ int64_t syscall_readlink(const char *path, char *buf, uint32_t size) {
#if defined(__aarch64__)
    return raf_sc4(RAF_NR_READLINKAT, RAF_AT_FDCWD, RAF_PTR(path), RAF_PTR(buf), size);
#else
    return raf_sc3(RAF_NR_READLINK, RAF_PTR(path), RAF_PTR(buf), size);
#endif
}

static __inline__ int64_t syscall_access(const char *path, int mode) {
#if defined(__aarch64__)
    return raf_sc3(RAF_NR_FACCESSAT, RAF_AT_FDCWD, RAF_PTR(path), mode);
#else
    return raf_sc2(RAF_NR_ACCESS, RAF_PTR(path), mode);
#endif
}

static __inline__ int64_t syscall_fsync(int fd) {
    return raf_sc1(RAF_NR_FSYNC, fd);
}

static __inline__ int64_t syscall_prctl(int option, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5) {
#if defined(__aarch64__)
    return raf_sc5(RAF_NR_PRCTL, option, arg2, arg3, arg4, arg5);
#else
    return raf_sc5(RAF_NR_PRCTL, option, (int32_t)arg2, (int32_t)arg3, (int32_t)arg4, (int32_t)arg5);
#endif
}

#undef RAF_PTR

#endif /* RAFCODEPHI_SYSCALL_ARM_H */
