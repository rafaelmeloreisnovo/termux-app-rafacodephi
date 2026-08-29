/*
 * freestanding_syscalls.h — Freestanding syscall wrappers for ARM64
 *
 * Direct syscall wrappers (no libc layer) for common operations:
 * read, write, open, close, stat, fstat, mmap, brk, etc.
 *
 * Replaces unistd.h, fcntl.h, sys/stat.h syscall abstractions.
 * No external dependencies: pure ARM64 SVC-based syscalls.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* ARM64 syscall convention:
 * - Arguments: x0-x5 (register args), stack for additional
 * - Syscall number: x8
 * - Return value: x0 (may be int, may be pointer, on error: -errno)
 * - Clobber: x16, x17, x30 (lr)
 */

/* Macro for inline ARM64 syscall (1 arg) */
#define SYSCALL_1(num, a) \
    ({ \
        register int64_t x0 __asm__("x0") = (int64_t)(a); \
        register int64_t x8 __asm__("x8") = (num); \
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "cc", "memory"); \
        x0; \
    })

/* Macro for inline ARM64 syscall (2 args) */
#define SYSCALL_2(num, a, b) \
    ({ \
        register int64_t x0 __asm__("x0") = (int64_t)(a); \
        register int64_t x1 __asm__("x1") = (int64_t)(b); \
        register int64_t x8 __asm__("x8") = (num); \
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "cc", "memory"); \
        x0; \
    })

/* Macro for inline ARM64 syscall (3 args) */
#define SYSCALL_3(num, a, b, c) \
    ({ \
        register int64_t x0 __asm__("x0") = (int64_t)(a); \
        register int64_t x1 __asm__("x1") = (int64_t)(b); \
        register int64_t x2 __asm__("x2") = (int64_t)(c); \
        register int64_t x8 __asm__("x8") = (num); \
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "cc", "memory"); \
        x0; \
    })

/* Macro for inline ARM64 syscall (4 args) */
#define SYSCALL_4(num, a, b, c, d) \
    ({ \
        register int64_t x0 __asm__("x0") = (int64_t)(a); \
        register int64_t x1 __asm__("x1") = (int64_t)(b); \
        register int64_t x2 __asm__("x2") = (int64_t)(c); \
        register int64_t x3 __asm__("x3") = (int64_t)(d); \
        register int64_t x8 __asm__("x8") = (num); \
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "cc", "memory"); \
        x0; \
    })

/* ARM64 Linux syscall numbers */
#define SYS_read            63
#define SYS_write           64
#define SYS_open            1024
#define SYS_close           57
#define SYS_stat            106
#define SYS_fstat           80
#define SYS_stat_at         79
#define SYS_mmap            222
#define SYS_munmap          215
#define SYS_mprotect        226
#define SYS_brk             214
#define SYS_exit            93
#define SYS_exit_group      94
#define SYS_fork            220
#define SYS_clone           220
#define SYS_execve          221
#define SYS_wait4           114
#define SYS_chmod           47
#define SYS_lseek           62
#define SYS_sync            81
#define SYS_fsync           82
#define SYS_fdatasync       83
#define SYS_access          21
#define SYS_faccessat       49
#define SYS_getpid          172
#define SYS_gettid          224

/* O_* flags for open() */
#define O_RDONLY            0x00000000
#define O_WRONLY            0x00000001
#define O_RDWR              0x00000002
#define O_CREAT             0x00000040
#define O_EXCL              0x00000080
#define O_TRUNC             0x00000200
#define O_APPEND            0x00000400

/* PROT_* flags for mmap/mprotect */
#define PROT_NONE           0x00000000
#define PROT_READ           0x00000001
#define PROT_WRITE          0x00000002
#define PROT_EXEC           0x00000004

/* MAP_* flags for mmap */
#define MAP_SHARED          0x00000001
#define MAP_PRIVATE         0x00000002
#define MAP_FIXED           0x00000010
#define MAP_ANONYMOUS       0x00000020
#define MAP_ANON            MAP_ANONYMOUS

/* File permission bits */
#define S_IRUSR             0400
#define S_IWUSR             0200
#define S_IXUSR             0100
#define S_IRGRP             040
#define S_IWGRP             020
#define S_IXGRP             010
#define S_IROTH             04
#define S_IWOTH             02
#define S_IXOTH             01

/* stat structure (minimal) */
struct freestanding_stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t __pad0;
    int64_t  st_size;
    int32_t  st_blksize;
    int32_t  __pad1;
    int64_t  st_blocks;
    int64_t  st_atime;
    uint64_t st_atime_nsec;
    int64_t  st_mtime;
    uint64_t st_mtime_nsec;
    int64_t  st_ctime;
    uint64_t st_ctime_nsec;
    int32_t  _unused[2];
};

/* read(fd, buf, count) → bytes read */
static inline int64_t freestanding_read(int fd, void *buf, uint32_t count) {
    return SYSCALL_3(SYS_read, fd, buf, count);
}

/* write(fd, buf, count) → bytes written */
static inline int64_t freestanding_write(int fd, const void *buf, uint32_t count) {
    return SYSCALL_3(SYS_write, fd, buf, count);
}

/* open(path, flags, mode) → fd */
static inline int64_t freestanding_open(const char *path, uint32_t flags, uint32_t mode) {
    return SYSCALL_3(SYS_open, path, flags, mode);
}

/* close(fd) → status */
static inline int64_t freestanding_close(int fd) {
    return SYSCALL_1(SYS_close, fd);
}

/* stat(path, statbuf) → status */
static inline int64_t freestanding_stat(const char *path, struct freestanding_stat *statbuf) {
    return SYSCALL_2(SYS_stat, path, statbuf);
}

/* fstat(fd, statbuf) → status */
static inline int64_t freestanding_fstat(int fd, struct freestanding_stat *statbuf) {
    return SYSCALL_2(SYS_fstat, fd, statbuf);
}

/* lseek(fd, offset, whence) → new offset */
static inline int64_t freestanding_lseek(int fd, int64_t offset, int whence) {
    return SYSCALL_3(SYS_lseek, fd, offset, whence);
}

/* brk(addr) → new brk address */
static inline int64_t freestanding_brk(void *addr) {
    return SYSCALL_1(SYS_brk, addr);
}

/* mmap(addr, length, prot, flags, fd, offset) → mapped address */
static inline void *freestanding_mmap(void *addr, uint32_t length, int prot,
                                     int flags, int fd, uint32_t offset) {
    int64_t result = SYSCALL_4(SYS_mmap, addr, length, prot, flags);
    return (void *)result;
}

/* munmap(addr, length) → status */
static inline int64_t freestanding_munmap(void *addr, uint32_t length) {
    return SYSCALL_2(SYS_munmap, addr, length);
}

/* mprotect(addr, length, prot) → status */
static inline int64_t freestanding_mprotect(void *addr, uint32_t length, int prot) {
    return SYSCALL_3(SYS_mprotect, addr, length, prot);
}

/* exit(status) → never returns */
static inline void freestanding_exit(int status) {
    SYSCALL_1(SYS_exit, status);
    /* Unreachable */
    while (1) {
        asm("svc #0");
    }
}

/* exit_group(status) → never returns */
static inline void freestanding_exit_group(int status) {
    SYSCALL_1(SYS_exit_group, status);
    /* Unreachable */
    while (1) {
        asm("svc #0");
    }
}

/* getpid() → process id */
static inline int64_t freestanding_getpid(void) {
    return SYSCALL_1(SYS_getpid, 0);
}

/* gettid() → thread id */
static inline int64_t freestanding_gettid(void) {
    return SYSCALL_1(SYS_gettid, 0);
}

/* access(path, mode) → status */
static inline int64_t freestanding_access(const char *path, int mode) {
    return SYSCALL_2(SYS_access, path, mode);
}

/* sync() → void (always succeeds) */
static inline int64_t freestanding_sync(void) {
    return SYSCALL_1(SYS_sync, 0);
}

/* fsync(fd) → status */
static inline int64_t freestanding_fsync(int fd) {
    return SYSCALL_1(SYS_fsync, fd);
}

/* fdatasync(fd) → status */
static inline int64_t freestanding_fdatasync(int fd) {
    return SYSCALL_1(SYS_fdatasync, fd);
}

#endif /* FREESTANDING_SYSCALLS_H */
