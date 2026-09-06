#ifndef FREESTANDING_H
#define FREESTANDING_H

/*
 * RAFCODEPHI freestanding primitive types.
 * No libc, POSIX, Android, NDK or hosted headers.
 */

typedef __UINT8_TYPE__   uint8_t;
typedef __UINT16_TYPE__  uint16_t;
typedef __UINT32_TYPE__  uint32_t;
typedef __UINT64_TYPE__  uint64_t;
typedef __INT8_TYPE__    int8_t;
typedef __INT16_TYPE__   int16_t;
typedef __INT32_TYPE__   int32_t;
typedef __INT64_TYPE__   int64_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __INTPTR_TYPE__  intptr_t;
typedef __SIZE_TYPE__    size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __INTPTR_TYPE__  ssize_t;
typedef int32_t           pid_t;
typedef uint32_t          mode_t;
typedef int64_t           off_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

struct timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

#define EFAIL (-1)
#define EOK   (0)

#define MAX_RECEIPT_JSON    2048u
#define MAX_DEVICE_INFO     512u
#define MAX_SCENARIO_RESULT 1024u
#define MAX_PATHS           256u
#define MAX_COMMAND_ARGS    32u

struct Receipt {
    uint32_t magic;
    uint32_t stage;
    uint64_t timestamp;
    uint32_t crc32c;
    uint8_t  sha256[32];
    uint32_t phi_fst;
    uint32_t attractor;
    uint32_t entropy_norm;
    uint32_t coherence_norm;
    int32_t  exit_code;
    char     log[MAX_RECEIPT_JSON];
};

struct BootstrapProgress {
    int extracted;
    int dpkg_installed;
    int apt_configured;
    int restart_count;
    int skip_count;
};

#define SYSCALL_OK(x)       ((x) >= 0)
#define SYSCALL_ERR(x)      ((x) < 0)
#define SYSCALL_ERR_VAL(x)  (-(x))

#define LIKELY(x)     __builtin_expect(!!(x), 1)
#define UNLIKELY(x)   __builtin_expect(!!(x), 0)

#define LE32(x) (x)
#define LE64(x) (x)

#endif /* FREESTANDING_H */
