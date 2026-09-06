#ifndef PROOT_SYSCALL_BRIDGE_H
#define PROOT_SYSCALL_BRIDGE_H

/*
 * RAFCODEPHI freestanding Linux syscall bridge.
 * Contract:
 *   - no libc
 *   - no heap
 *   - no TLS
 *   - no crt
 *   - architecture-specific syscall numbers
 * Supported: AArch64 Android/Linux and ARM EABI (armv7).
 */

typedef signed long raf_sysret_t;
typedef unsigned long raf_word_t;

#define RAF_AT_FDCWD (-100)
#define RAF_X_OK 1

#if defined(__aarch64__)

#define RAF_NR_OPENAT          56
#define RAF_NR_CLOSE           57
#define RAF_NR_READ            63
#define RAF_NR_WRITE           64
#define RAF_NR_MKDIRAT         34
#define RAF_NR_UNLINKAT        35
#define RAF_NR_FACCESSAT       48
#define RAF_NR_CHDIR           49
#define RAF_NR_CLOCK_GETTIME  113
#define RAF_NR_PRCTL          167
#define RAF_NR_GETPID         172
#define RAF_NR_MMAP           222
#define RAF_NR_MPROTECT       226
#define RAF_NR_MUNMAP         215
#define RAF_NR_EXECVE         221
#define RAF_NR_EXIT_GROUP      94

static __inline__ __attribute__((always_inline))
raf_sysret_t raf_syscall6(raf_word_t nr,
                          raf_word_t a0, raf_word_t a1, raf_word_t a2,
                          raf_word_t a3, raf_word_t a4, raf_word_t a5) {
    register raf_word_t x0 __asm__("x0") = a0;
    register raf_word_t x1 __asm__("x1") = a1;
    register raf_word_t x2 __asm__("x2") = a2;
    register raf_word_t x3 __asm__("x3") = a3;
    register raf_word_t x4 __asm__("x4") = a4;
    register raf_word_t x5 __asm__("x5") = a5;
    register raf_word_t x8 __asm__("x8") = nr;
    __asm__ volatile(
        "svc 0"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
        : "memory", "cc");
    return (raf_sysret_t)x0;
}

#elif defined(__arm__)

#define RAF_NR_OPENAT         322
#define RAF_NR_CLOSE            6
#define RAF_NR_READ             3
#define RAF_NR_WRITE            4
#define RAF_NR_MKDIRAT        323
#define RAF_NR_UNLINKAT       328
#define RAF_NR_FACCESSAT      334
#define RAF_NR_CHDIR           12
#define RAF_NR_CLOCK_GETTIME  263
#define RAF_NR_PRCTL          172
#define RAF_NR_GETPID          20
#define RAF_NR_MMAP2          192
#define RAF_NR_MPROTECT       125
#define RAF_NR_MUNMAP          91
#define RAF_NR_EXECVE          11
#define RAF_NR_EXIT_GROUP     248

static __inline__ __attribute__((always_inline))
raf_sysret_t raf_syscall6(raf_word_t nr,
                          raf_word_t a0, raf_word_t a1, raf_word_t a2,
                          raf_word_t a3, raf_word_t a4, raf_word_t a5) {
    register raf_word_t r0 __asm__("r0") = a0;
    register raf_word_t r1 __asm__("r1") = a1;
    register raf_word_t r2 __asm__("r2") = a2;
    register raf_word_t r3 __asm__("r3") = a3;
    register raf_word_t r4 __asm__("r4") = a4;
    register raf_word_t r5 __asm__("r5") = a5;
    register raf_word_t r7 __asm__("r7") = nr;
    __asm__ volatile(
        "svc 0"
        : "+r"(r0)
        : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7)
        : "memory", "cc");
    return (raf_sysret_t)r0;
}

#else
#error "RAFCODEPHI freestanding syscall bridge supports only __aarch64__ or __arm__"
#endif

#define RAF_PTR(p) ((raf_word_t)(p))

static __inline__ raf_sysret_t proot_sys_open(const char *path, int flags, int mode) {
    return raf_syscall6(RAF_NR_OPENAT, (raf_word_t)RAF_AT_FDCWD, RAF_PTR(path),
                        (raf_word_t)flags, (raf_word_t)mode, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_close(int fd) {
    return raf_syscall6(RAF_NR_CLOSE, (raf_word_t)fd, 0, 0, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_read(int fd, void *buf, raf_word_t count) {
    return raf_syscall6(RAF_NR_READ, (raf_word_t)fd, RAF_PTR(buf), count, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_write(int fd, const void *buf, raf_word_t count) {
    return raf_syscall6(RAF_NR_WRITE, (raf_word_t)fd, RAF_PTR(buf), count, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_mkdir(const char *path, int mode) {
    return raf_syscall6(RAF_NR_MKDIRAT, (raf_word_t)RAF_AT_FDCWD, RAF_PTR(path),
                        (raf_word_t)mode, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_unlink(const char *path, int flags) {
    return raf_syscall6(RAF_NR_UNLINKAT, (raf_word_t)RAF_AT_FDCWD, RAF_PTR(path),
                        (raf_word_t)flags, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_access_exec(const char *path) {
    return raf_syscall6(RAF_NR_FACCESSAT, (raf_word_t)RAF_AT_FDCWD, RAF_PTR(path),
                        RAF_X_OK, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_chdir(const char *path) {
    return raf_syscall6(RAF_NR_CHDIR, RAF_PTR(path), 0, 0, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_clock_gettime(int clockid, void *tp) {
    return raf_syscall6(RAF_NR_CLOCK_GETTIME, (raf_word_t)clockid, RAF_PTR(tp),
                        0, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_prctl(int op, raf_word_t arg2,
                                               raf_word_t arg3, raf_word_t arg4,
                                               raf_word_t arg5) {
    return raf_syscall6(RAF_NR_PRCTL, (raf_word_t)op, arg2, arg3, arg4, arg5, 0);
}

static __inline__ raf_sysret_t proot_sys_getpid(void) {
    return raf_syscall6(RAF_NR_GETPID, 0, 0, 0, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_mmap(void *addr, raf_word_t length,
                                              int prot, int flags, int fd,
                                              raf_word_t offset) {
#if defined(__aarch64__)
    return raf_syscall6(RAF_NR_MMAP, RAF_PTR(addr), length, (raf_word_t)prot,
                        (raf_word_t)flags, (raf_word_t)fd, offset);
#else
    if (offset & 4095UL) return -22; /* EINVAL: mmap2 offset is in 4 KiB pages. */
    return raf_syscall6(RAF_NR_MMAP2, RAF_PTR(addr), length, (raf_word_t)prot,
                        (raf_word_t)flags, (raf_word_t)fd, offset >> 12);
#endif
}

static __inline__ raf_sysret_t proot_sys_mprotect(void *addr, raf_word_t length, int prot) {
    return raf_syscall6(RAF_NR_MPROTECT, RAF_PTR(addr), length, (raf_word_t)prot,
                        0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_munmap(void *addr, raf_word_t length) {
    return raf_syscall6(RAF_NR_MUNMAP, RAF_PTR(addr), length, 0, 0, 0, 0);
}

static __inline__ raf_sysret_t proot_sys_execve(const char *path,
                                                char *const argv[],
                                                char *const envp[]) {
    return raf_syscall6(RAF_NR_EXECVE, RAF_PTR(path), RAF_PTR(argv), RAF_PTR(envp),
                        0, 0, 0);
}

__attribute__((noreturn))
static __inline__ void proot_sys_exit(int code) {
    (void)raf_syscall6(RAF_NR_EXIT_GROUP, (raf_word_t)code, 0, 0, 0, 0, 0);
    for (;;) { __asm__ volatile("" ::: "memory"); }
}

#define IS_SYSCALL_ERROR(result) ((raf_sysret_t)(result) < 0 && (raf_sysret_t)(result) >= -4095)

#endif /* PROOT_SYSCALL_BRIDGE_H */
