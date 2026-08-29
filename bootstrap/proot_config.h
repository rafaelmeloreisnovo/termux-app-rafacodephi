#ifndef PROOT_CONFIG_H
#define PROOT_CONFIG_H

#include <stdint.h>

/* RAFCODEΦ Freestanding proot Configuration */
/* No fork, no threads, no shadows, single-threaded only */

#define PROOT_CONFIG_VERSION    1
#define PROOT_CONFIG_SCHEMA     "raf.proot-config.v1"

/* Static Configuration Flags */
#define PROOT_NO_FORK           1  /* Disable fork syscall handling */
#define PROOT_NO_THREADS        1  /* Disable pthread tracking */
#define PROOT_SINGLE_THREADED   1  /* Enforce single-threaded mode */
#define PROOT_NO_SHADOW_PROCS   1  /* No background/shadow processes */
#define PROOT_NO_TAIL_PROCS     1  /* No tail processes or deferred execution */

/* Timeout & Watchdog Configuration */
#define PROOT_WATCHDOG_TIMEOUT_SECONDS    30
#define PROOT_MAX_RESTART_ATTEMPTS        2
#define PROOT_FAILOVER_EXIT_CODE          128  /* Fatal error, no retry loop */

/* Bootstrap Atomic State Machine */
typedef enum {
    PROOT_STATE_PREFIX_EMPTY = 0,
    PROOT_STATE_INITIALIZED = 1,
    PROOT_STATE_PAYLOAD_EXTRACTED = 2,
    PROOT_STATE_DPKG_INSTALLED = 3,
    PROOT_STATE_APT_CONFIGURED = 4,
    PROOT_STATE_USER_PACKAGES_READY = 5
} proot_state_t;

/* Syscall Bridge Configuration */
#define PROOT_SYSCALL_BRIDGE_ARM64      1
#define PROOT_SYSCALL_CUSTOM            1  /* Custom ARM64 syscall encoder */
#define PROOT_NO_GLIBC_SYSCALL_WRAPPER  1  /* Direct syscall, no libc */

/* Receipt Sealing Configuration */
#define PROOT_RECEIPT_SCHEMA         "raf.bootstrap-receipt.v1"
#define PROOT_RECEIPT_CRC_ALGORITHM  "CRC32C"  /* Castagnoli polynomial */
#define PROOT_RECEIPT_MAX_STAGES     6

/* State Hash Configuration */
#define PROOT_STATE_HASH_ALGORITHM   "sha256"
#define PROOT_STATE_HASH_SIZE        32  /* bytes */

/* Rollback Configuration */
#define PROOT_ROLLBACK_ENABLED       1
#define PROOT_ROLLBACK_MAX_DEPTH     PROOT_STATE_USER_PACKAGES_READY

/* Determinism Configuration */
#define PROOT_DETERMINISTIC_STATE    1  /* State transitions are deterministic */
#define PROOT_NO_RANDOMIZATION       1  /* No random seeds in state machine */

/* Freestanding Guarantees */
#define PROOT_NO_MALLOC              1
#define PROOT_NO_CALLOC              1
#define PROOT_NO_FREE                1
#define PROOT_NO_LIBC_STDIO          1  /* No printf/fprintf in hot path */
#define PROOT_STACK_ALLOCATED_ONLY   1  /* All buffers on stack */

/* Buffer Sizes (stack-allocated, no heap) */
#define PROOT_MAX_SYSLOG_SIZE        512
#define PROOT_MAX_RECEIPT_SIZE       2048
#define PROOT_MAX_STATE_HASH_SIZE    32
#define PROOT_MAX_CRC_BUFFER         4096

#endif /* PROOT_CONFIG_H */
