#ifndef FREESTANDING_H
#define FREESTANDING_H

/* Freestanding C bootstrap — no libc, no malloc, syscalls only */

#include <stdint.h>
#include <stddef.h>

/* Standard type aliases (avoid conflicts with system headers) */
typedef int32_t ssize_t;
typedef int32_t pid_t;
typedef uint32_t mode_t;
typedef uint64_t off_t;

/* Minimal timespec structure */
struct timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

/* Error convention */
#define EFAIL (-1)
#define EOK   (0)

/* Memory layout — stack-only buffers, no heap */
#define MAX_RECEIPT_JSON    2048
#define MAX_DEVICE_INFO     512
#define MAX_SCENARIO_RESULT 1024
#define MAX_PATHS           256
#define MAX_COMMAND_ARGS    32

/* Receipt structure — stack-allocated */
struct Receipt {
    uint32_t magic;           /* 0xDEADBEEF */
    uint32_t stage;
    uint64_t timestamp;       /* CLOCK_MONOTONIC */
    uint32_t crc32c;          /* Castagnoli polynomial */
    uint8_t  sha256[32];      /* SHA-256 digest */
    uint32_t phi_fst;         /* Coherence Q16 fixed-point */
    uint32_t attractor;       /* T^7 attractor slot [0..40] (41-state toroid) */
    uint32_t entropy_norm;    /* H_norm Q16 */
    uint32_t coherence_norm;  /* C_norm Q16 */
    int32_t  exit_code;
    char     log[MAX_RECEIPT_JSON];
};

/* Bootstrap progress tracking */
struct BootstrapProgress {
    int extracted;
    int dpkg_installed;
    int apt_configured;
    int restart_count;
    int skip_count;
};

/* Syscall return value macro */
#define SYSCALL_OK(x)       ((x) >= 0)
#define SYSCALL_ERR(x)      ((x) < 0)
#define SYSCALL_ERR_VAL(x)  (-(x))

/* Assembly markers (ARM64/ARM32 neutral) */
#define LIKELY(x)     __builtin_expect(!!(x), 1)
#define UNLIKELY(x)   __builtin_expect(!!(x), 0)

/* Byte-order neutral (assume little-endian ARM) */
#define LE32(x) (x)
#define LE64(x) (x)

#endif /* FREESTANDING_H */
