/*
 * freestanding_syscalls.h — Freestanding syscall wrappers for ARM64
 *
 * Direct syscall wrappers (no libc layer) for common operations:
 * read, write, open, close, stat, fstat, mmap, brk, etc.
 *
 * Replaces unistd.h, fcntl.h, sys/stat.h syscall abstractions.
 * No external dependencies: pure ARM64 SVC-based syscalls.
 */

#ifndef FREESTANDING_SYSCALLS_H
#define FREESTANDING_SYSCALLS_H

#pragma once
#include <stdint.h>
#include <stddef.h>

/* RAFCODEPHI direct Linux syscall layer.
 * Runtime-libc independent. Android ABIs: arm64, arm32, x86_64, x86.
 * Syscall numbering follows Linux UAPI tables; file-stat layout is ABI-specific.
 */

#define FS_AT_FDCWD (-100)
#define FS_ENOSYS   38

#if defined(__aarch64__)
static inline intptr_t fs_sc0(intptr_t n) {
    register intptr_t x0 __asm__("x0");
    register intptr_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "=r"(x0) : "r"(x8) : "cc", "memory");
    return x0;
}
static inline intptr_t fs_sc1(intptr_t n, intptr_t a0) {
    register intptr_t x0 __asm__("x0") = a0;
    register intptr_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "cc", "memory"); return x0;
}
static inline intptr_t fs_sc2(intptr_t n, intptr_t a0, intptr_t a1) {
    register intptr_t x0 __asm__("x0") = a0; register intptr_t x1 __asm__("x1") = a1;
    register intptr_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "cc", "memory"); return x0;
}
static inline intptr_t fs_sc3(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2) {
    register intptr_t x0 __asm__("x0") = a0; register intptr_t x1 __asm__("x1") = a1;
    register intptr_t x2 __asm__("x2") = a2; register intptr_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "cc", "memory"); return x0;
}
static inline intptr_t fs_sc4(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) {
    register intptr_t x0 __asm__("x0") = a0; register intptr_t x1 __asm__("x1") = a1;
    register intptr_t x2 __asm__("x2") = a2; register intptr_t x3 __asm__("x3") = a3;
    register intptr_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "cc", "memory"); return x0;
}
static inline intptr_t fs_sc5(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4) {
    register intptr_t x0 __asm__("x0") = a0; register intptr_t x1 __asm__("x1") = a1;
    register intptr_t x2 __asm__("x2") = a2; register intptr_t x3 __asm__("x3") = a3;
    register intptr_t x4 __asm__("x4") = a4; register intptr_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8) : "cc", "memory"); return x0;
}
static inline intptr_t fs_sc6(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5) {
    register intptr_t x0 __asm__("x0") = a0; register intptr_t x1 __asm__("x1") = a1;
    register intptr_t x2 __asm__("x2") = a2; register intptr_t x3 __asm__("x3") = a3;
    register intptr_t x4 __asm__("x4") = a4; register intptr_t x5 __asm__("x5") = a5;
    register intptr_t x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8) : "cc", "memory"); return x0;
}
#define FS_NR_read 63
#define FS_NR_write 64
#define FS_NR_openat 56
#define FS_NR_close 57
#define FS_NR_newfstatat 79
#define FS_NR_fstat 80
#define FS_NR_lseek 62
#define FS_NR_mmap 222
#define FS_NR_munmap 215
#define FS_NR_mprotect 226
#define FS_NR_brk 214
#define FS_NR_exit 93
#define FS_NR_exit_group 94
#define FS_NR_faccessat 48
#define FS_NR_getpid 172
#define FS_NR_gettid 178
#define FS_NR_sync 81
#define FS_NR_fsync 82
#define FS_NR_fdatasync 83
#define FS_USE_OPENAT 1
#define FS_USE_FSTATAT 1
#define FS_USE_FACCESSAT 1
#define FS_MMAP2 0
struct freestanding_stat {
    uint64_t st_dev; uint64_t st_ino; uint32_t st_mode; uint32_t st_nlink;
    uint32_t st_uid; uint32_t st_gid; uint64_t st_rdev; uint64_t pad1;
    int64_t st_size; int32_t st_blksize; int32_t pad2; int64_t st_blocks;
    int64_t st_atime; uint64_t st_atime_nsec; int64_t st_mtime; uint64_t st_mtime_nsec;
    int64_t st_ctime; uint64_t st_ctime_nsec; uint32_t reserved_tail[2];
};

#elif defined(__arm__)
#define FS_ARM_BODY(out, nr, inputs...) \
    __asm__ volatile("push {r7}\n\tmov r7, %[sysno]\n\tsvc #0\n\tpop {r7}" : out : [sysno] "r"(nr), ##inputs : "cc", "memory")
static inline intptr_t fs_sc0(intptr_t n) { register intptr_t r0 __asm__("r0"); FS_ARM_BODY("=r"(r0), n); return r0; }
static inline intptr_t fs_sc1(intptr_t n, intptr_t a0) { register intptr_t r0 __asm__("r0")=a0; FS_ARM_BODY("+r"(r0), n); return r0; }
static inline intptr_t fs_sc2(intptr_t n, intptr_t a0, intptr_t a1) { register intptr_t r0 __asm__("r0")=a0; register intptr_t r1 __asm__("r1")=a1; FS_ARM_BODY("+r"(r0), n, "r"(r1)); return r0; }
static inline intptr_t fs_sc3(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2) { register intptr_t r0 __asm__("r0")=a0; register intptr_t r1 __asm__("r1")=a1; register intptr_t r2 __asm__("r2")=a2; FS_ARM_BODY("+r"(r0), n, "r"(r1), "r"(r2)); return r0; }
static inline intptr_t fs_sc4(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) { register intptr_t r0 __asm__("r0")=a0; register intptr_t r1 __asm__("r1")=a1; register intptr_t r2 __asm__("r2")=a2; register intptr_t r3 __asm__("r3")=a3; FS_ARM_BODY("+r"(r0), n, "r"(r1), "r"(r2), "r"(r3)); return r0; }
static inline intptr_t fs_sc5(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4) { register intptr_t r0 __asm__("r0")=a0; register intptr_t r1 __asm__("r1")=a1; register intptr_t r2 __asm__("r2")=a2; register intptr_t r3 __asm__("r3")=a3; register intptr_t r4 __asm__("r4")=a4; FS_ARM_BODY("+r"(r0), n, "r"(r1), "r"(r2), "r"(r3), "r"(r4)); return r0; }
static inline intptr_t fs_sc6(intptr_t n, intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5) { register intptr_t r0 __asm__("r0")=a0; register intptr_t r1 __asm__("r1")=a1; register intptr_t r2 __asm__("r2")=a2; register intptr_t r3 __asm__("r3")=a3; register intptr_t r4 __asm__("r4")=a4; register intptr_t r5 __asm__("r5")=a5; FS_ARM_BODY("+r"(r0), n, "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)); return r0; }
#undef FS_ARM_BODY
#define FS_NR_read 3
#define FS_NR_write 4
#define FS_NR_open 5
#define FS_NR_close 6
#define FS_NR_stat64 195
#define FS_NR_fstat64 197
#define FS_NR_lseek 19
#define FS_NR_mmap2 192
#define FS_NR_munmap 91
#define FS_NR_mprotect 125
#define FS_NR_brk 45
#define FS_NR_exit 1
#define FS_NR_exit_group 248
#define FS_NR_access 33
#define FS_NR_getpid 20
#define FS_NR_gettid 224
#define FS_NR_sync 36
#define FS_NR_fsync 118
#define FS_NR_fdatasync 148
#define FS_USE_OPENAT 0
#define FS_USE_FSTATAT 0
#define FS_USE_FACCESSAT 0
#define FS_MMAP2 1
struct freestanding_stat {
    uint64_t st_dev; uint8_t pad0[4]; uint32_t old_ino;
    uint32_t st_mode; uint32_t st_nlink; uint32_t st_uid; uint32_t st_gid;
    uint64_t st_rdev; uint8_t pad3[4]; int64_t st_size; uint32_t st_blksize;
    uint64_t st_blocks; uint32_t st_atime; uint32_t st_atime_nsec;
    uint32_t st_mtime; uint32_t st_mtime_nsec; uint32_t st_ctime; uint32_t st_ctime_nsec;
    uint64_t st_ino;
};

#elif defined(__x86_64__)
static inline intptr_t fs_sc0(intptr_t n) { register intptr_t rax __asm__("rax")=n; __asm__ volatile("syscall" : "+a"(rax) :: "rcx","r11","cc","memory"); return rax; }
static inline intptr_t fs_sc1(intptr_t n,intptr_t a0) { register intptr_t rax __asm__("rax")=n; __asm__ volatile("syscall" : "+a"(rax) : "D"(a0) : "rcx","r11","cc","memory"); return rax; }
static inline intptr_t fs_sc2(intptr_t n,intptr_t a0,intptr_t a1) { register intptr_t rax __asm__("rax")=n; __asm__ volatile("syscall" : "+a"(rax) : "D"(a0),"S"(a1) : "rcx","r11","cc","memory"); return rax; }
static inline intptr_t fs_sc3(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2) { register intptr_t rax __asm__("rax")=n; __asm__ volatile("syscall" : "+a"(rax) : "D"(a0),"S"(a1),"d"(a2) : "rcx","r11","cc","memory"); return rax; }
static inline intptr_t fs_sc4(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3) { register intptr_t rax __asm__("rax")=n; register intptr_t r10 __asm__("r10")=a3; __asm__ volatile("syscall" : "+a"(rax) : "D"(a0),"S"(a1),"d"(a2),"r"(r10) : "rcx","r11","cc","memory"); return rax; }
static inline intptr_t fs_sc5(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4) { register intptr_t rax __asm__("rax")=n; register intptr_t r10 __asm__("r10")=a3; register intptr_t r8 __asm__("r8")=a4; __asm__ volatile("syscall" : "+a"(rax) : "D"(a0),"S"(a1),"d"(a2),"r"(r10),"r"(r8) : "rcx","r11","cc","memory"); return rax; }
static inline intptr_t fs_sc6(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5) { register intptr_t rax __asm__("rax")=n; register intptr_t r10 __asm__("r10")=a3; register intptr_t r8 __asm__("r8")=a4; register intptr_t r9 __asm__("r9")=a5; __asm__ volatile("syscall" : "+a"(rax) : "D"(a0),"S"(a1),"d"(a2),"r"(r10),"r"(r8),"r"(r9) : "rcx","r11","cc","memory"); return rax; }
#define FS_NR_read 0
#define FS_NR_write 1
#define FS_NR_open 2
#define FS_NR_close 3
#define FS_NR_stat 4
#define FS_NR_fstat 5
#define FS_NR_lseek 8
#define FS_NR_mmap 9
#define FS_NR_mprotect 10
#define FS_NR_munmap 11
#define FS_NR_brk 12
#define FS_NR_access 21
#define FS_NR_getpid 39
#define FS_NR_exit 60
#define FS_NR_fsync 74
#define FS_NR_fdatasync 75
#define FS_NR_sync 162
#define FS_NR_gettid 186
#define FS_NR_exit_group 231
#define FS_USE_OPENAT 0
#define FS_USE_FSTATAT 0
#define FS_USE_FACCESSAT 0
#define FS_MMAP2 0
struct freestanding_stat {
    uint64_t st_dev; uint64_t st_ino; uint64_t st_nlink; uint32_t st_mode;
    uint32_t st_uid; uint32_t st_gid; uint32_t pad0; uint64_t st_rdev;
    int64_t st_size; int64_t st_blksize; int64_t st_blocks;
    uint64_t st_atime; uint64_t st_atime_nsec; uint64_t st_mtime; uint64_t st_mtime_nsec;
    uint64_t st_ctime; uint64_t st_ctime_nsec; int64_t reserved_tail[3];
};

#elif defined(__i386__)
/* Preserve EBX (PIC base) by swapping the first syscall argument through a compiler-owned GPR. */
static inline intptr_t fs_sc0(intptr_t n) { register intptr_t eax __asm__("eax")=n; __asm__ volatile("int $0x80" : "+a"(eax) :: "cc","memory"); return eax; }
static inline intptr_t fs_sc1(intptr_t n,intptr_t a0) { register intptr_t eax __asm__("eax")=n; intptr_t b=a0; __asm__ volatile("xchgl %%ebx, %1\n\tint $0x80\n\txchgl %%ebx, %1" : "+a"(eax), "+r"(b) :: "cc", "memory"); return eax; }
static inline intptr_t fs_sc2(intptr_t n,intptr_t a0,intptr_t a1) { register intptr_t eax __asm__("eax")=n; intptr_t b=a0; __asm__ volatile("xchgl %%ebx, %1\n\tint $0x80\n\txchgl %%ebx, %1" : "+a"(eax), "+r"(b) : "c"(a1) : "cc", "memory"); return eax; }
static inline intptr_t fs_sc3(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2) { register intptr_t eax __asm__("eax")=n; intptr_t b=a0; __asm__ volatile("xchgl %%ebx, %1\n\tint $0x80\n\txchgl %%ebx, %1" : "+a"(eax), "+r"(b) : "c"(a1), "d"(a2) : "cc", "memory"); return eax; }
static inline intptr_t fs_sc4(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3) { register intptr_t eax __asm__("eax")=n; intptr_t b=a0; __asm__ volatile("xchgl %%ebx, %1\n\tint $0x80\n\txchgl %%ebx, %1" : "+a"(eax), "+r"(b) : "c"(a1), "d"(a2), "S"(a3) : "cc", "memory"); return eax; }
static inline intptr_t fs_sc5(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4) { register intptr_t eax __asm__("eax")=n; intptr_t b=a0; __asm__ volatile("xchgl %%ebx, %1\n\tint $0x80\n\txchgl %%ebx, %1" : "+a"(eax), "+r"(b) : "c"(a1), "d"(a2), "S"(a3), "D"(a4) : "cc", "memory"); return eax; }
static inline intptr_t fs_sc6(intptr_t n,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5) { (void)n;(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return -FS_ENOSYS; }
#define FS_NR_read 3
#define FS_NR_write 4
#define FS_NR_open 5
#define FS_NR_close 6
#define FS_NR_lseek 19
#define FS_NR_access 33
#define FS_NR_getpid 20
#define FS_NR_brk 45
#define FS_NR_mmap_old 90
#define FS_NR_munmap 91
#define FS_NR_stat64 195
#define FS_NR_fstat64 197
#define FS_NR_mprotect 125
#define FS_NR_fsync 118
#define FS_NR_fdatasync 148
#define FS_NR_gettid 224
#define FS_NR_exit_group 252
#define FS_NR_exit 1
#define FS_NR_sync 36
#define FS_USE_OPENAT 0
#define FS_USE_FSTATAT 0
#define FS_USE_FACCESSAT 0
#define FS_MMAP2 2
struct freestanding_stat {
    uint64_t st_dev; uint8_t pad0[4]; uint32_t old_ino;
    uint32_t st_mode; uint32_t st_nlink; uint32_t st_uid; uint32_t st_gid;
    uint64_t st_rdev; uint8_t pad3[4]; int64_t st_size; uint32_t st_blksize;
    uint64_t st_blocks; uint32_t st_atime; uint32_t st_atime_nsec;
    uint32_t st_mtime; uint32_t st_mtime_nsec; uint32_t st_ctime; uint32_t st_ctime_nsec;
    uint64_t st_ino;
};
#else
#error "freestanding_syscalls.h: unsupported architecture"
#endif

#define O_RDONLY      0x00000000
#define O_WRONLY      0x00000001
#define O_RDWR        0x00000002
#define O_CREAT       0x00000040
#define O_EXCL        0x00000080
#define O_TRUNC       0x00000200
#define O_APPEND      0x00000400
#define PROT_NONE     0x0
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_ANON MAP_ANONYMOUS
#ifndef S_IRUSR
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#endif

static inline int64_t freestanding_read(int fd, void *buf, uint32_t count) { return (int64_t)fs_sc3(FS_NR_read, fd, (intptr_t)buf, count); }
static inline int64_t freestanding_write(int fd, const void *buf, uint32_t count) { return (int64_t)fs_sc3(FS_NR_write, fd, (intptr_t)buf, count); }
static inline int64_t freestanding_open(const char *path, uint32_t flags, uint32_t mode) {
#if FS_USE_OPENAT
    return (int64_t)fs_sc4(FS_NR_openat, FS_AT_FDCWD, (intptr_t)path, flags, mode);
#else
    return (int64_t)fs_sc3(FS_NR_open, (intptr_t)path, flags, mode);
#endif
}
static inline int64_t freestanding_close(int fd) { return (int64_t)fs_sc1(FS_NR_close, fd); }
static inline int64_t freestanding_stat(const char *path, struct freestanding_stat *st) {
#if FS_USE_FSTATAT
    return (int64_t)fs_sc4(FS_NR_newfstatat, FS_AT_FDCWD, (intptr_t)path, (intptr_t)st, 0);
#elif defined(__arm__) || defined(__i386__)
    return (int64_t)fs_sc2(FS_NR_stat64, (intptr_t)path, (intptr_t)st);
#else
    return (int64_t)fs_sc2(FS_NR_stat, (intptr_t)path, (intptr_t)st);
#endif
}
static inline int64_t freestanding_fstat(int fd, struct freestanding_stat *st) {
#if defined(__arm__) || defined(__i386__)
    return (int64_t)fs_sc2(FS_NR_fstat64, fd, (intptr_t)st);
#else
    return (int64_t)fs_sc2(FS_NR_fstat, fd, (intptr_t)st);
#endif
}
static inline int64_t freestanding_lseek(int fd, int64_t offset, int whence) { return (int64_t)fs_sc3(FS_NR_lseek, fd, (intptr_t)offset, whence); }
static inline int64_t freestanding_brk(void *addr) { return (int64_t)fs_sc1(FS_NR_brk, (intptr_t)addr); }
static inline void *freestanding_mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset) {
#if FS_MMAP2 == 1
    if ((offset & 4095u) != 0u) return (void *)(intptr_t)-22;
    return (void *)fs_sc6(FS_NR_mmap2, (intptr_t)addr, length, prot, flags, fd, offset >> 12);
#elif FS_MMAP2 == 2
    struct fs_mmap_old_args { uintptr_t addr; uint32_t len; int32_t prot; int32_t flags; int32_t fd; uint32_t offset; } args;
    args.addr=(uintptr_t)addr; args.len=length; args.prot=prot; args.flags=flags; args.fd=fd; args.offset=offset;
    return (void *)fs_sc1(FS_NR_mmap_old, (intptr_t)&args);
#else
    return (void *)fs_sc6(FS_NR_mmap, (intptr_t)addr, length, prot, flags, fd, offset);
#endif
}
static inline int64_t freestanding_munmap(void *addr, uint32_t length) { return (int64_t)fs_sc2(FS_NR_munmap, (intptr_t)addr, length); }
static inline int64_t freestanding_mprotect(void *addr, uint32_t length, int prot) { return (int64_t)fs_sc3(FS_NR_mprotect, (intptr_t)addr, length, prot); }
static inline void freestanding_exit(int status) { (void)fs_sc1(FS_NR_exit, status); for (;;) { __asm__ volatile("" ::: "memory"); } }
static inline void freestanding_exit_group(int status) { (void)fs_sc1(FS_NR_exit_group, status); for (;;) { __asm__ volatile("" ::: "memory"); } }
static inline int64_t freestanding_getpid(void) { return (int64_t)fs_sc0(FS_NR_getpid); }
static inline int64_t freestanding_gettid(void) { return (int64_t)fs_sc0(FS_NR_gettid); }
static inline int64_t freestanding_access(const char *path, int mode) {
#if FS_USE_FACCESSAT
    return (int64_t)fs_sc3(FS_NR_faccessat, FS_AT_FDCWD, (intptr_t)path, mode);
#else
    return (int64_t)fs_sc2(FS_NR_access, (intptr_t)path, mode);
#endif
}
static inline int64_t freestanding_sync(void) { return (int64_t)fs_sc0(FS_NR_sync); }
static inline int64_t freestanding_fsync(int fd) { return (int64_t)fs_sc1(FS_NR_fsync, fd); }
static inline int64_t freestanding_fdatasync(int fd) { return (int64_t)fs_sc1(FS_NR_fdatasync, fd); }

#endif /* FREESTANDING_SYSCALLS_H */
