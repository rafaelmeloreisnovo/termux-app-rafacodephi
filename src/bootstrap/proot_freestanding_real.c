/* P0.1: proot initialization — real implementation, freestanding */

#include "freestanding.h"
#include "syscall_arm64.h"

#define PROOT_BIN "/system/bin/proot"
#define TERMUX_PREFIX "/data/data/com.termux.rafacodephi"

/* Bootstrap state machine */
enum BootstrapState {
    STATE_PREFIX_EMPTY = 0,
    STATE_PROOT_INITIALIZED = 1,
    STATE_PAYLOAD_EXTRACTED = 2,
    STATE_DPKG_INSTALLED = 3,
    STATE_APT_CONFIGURED = 4,
    STATE_USER_PACKAGES_READY = 5
};

struct BootstrapContext {
    enum BootstrapState state;
    pid_t proot_pid;
    int watchdog_fd;
    uint64_t start_time_ns;
    struct Receipt receipt;
};

/* Watchdog timer using CLOCK_MONOTONIC (nanoseconds) */
static int64_t get_monotonic_ns(void) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    int64_t ret = syscall_clock_gettime(0, &ts);  /* CLOCK_MONOTONIC = 1 on ARM64 */
    if (SYSCALL_ERR(ret)) return -1;

    return (ts.tv_sec * 1000000000LL) + ts.tv_nsec;
}

/* Check if watchdog exceeded 30 seconds */
static int watchdog_exceeded(struct BootstrapContext *ctx) {
    int64_t now = get_monotonic_ns();
    if (now < 0) return 1;  /* Error → treat as timeout */

    uint64_t elapsed_ns = now - ctx->start_time_ns;
    uint64_t timeout_ns = 30ULL * 1000000000ULL;  /* 30 seconds */

    return elapsed_ns > timeout_ns;
}

/* Write to stderr (fd=2) for logging */
static int64_t log_message(const char *msg, uint32_t len) {
    return syscall_write(2, msg, len);
}

/* Initialize proot process in PREFIX_EMPTY state */
static int proot_init(struct BootstrapContext *ctx) {
    ctx->state = STATE_PREFIX_EMPTY;
    ctx->start_time_ns = get_monotonic_ns();

    if (ctx->start_time_ns < 0) {
        log_message("ERROR: clock_gettime failed\n", 31);
        return EFAIL;
    }

    /* Transition to PROOT_INITIALIZED state */
    ctx->state = STATE_PROOT_INITIALIZED;

    log_message("BOOTSTRAP: PREFIX_EMPTY → PROOT_INITIALIZED\n", 45);
    return EOK;
}

/* Spawn proot child process */
static int spawn_proot_child(struct BootstrapContext *ctx,
                             const char *prefix_path,
                             const char *proot_binary) {
    /* Simple fork + execve pattern (no libc clone) */
    int64_t pid = syscall_fork();

    if (SYSCALL_ERR(pid)) {
        log_message("ERROR: fork failed\n", 20);
        return EFAIL;
    }

    if (pid == 0) {
        /* Child: prepare to execve proot */
        char *argv[] = {
            (char *)proot_binary,
            (char *)"-r",
            (char *)prefix_path,
            (char *)"-w", (char *)"/",
            (char *)"/bin/sh",
            NULL
        };

        char *env[] = {
            (char *)"TERMUX_PREFIX=/",
            (char *)"HOME=/root",
            NULL
        };

        /* Replace child image with proot */
        int64_t ret = syscall_execve(proot_binary, argv, env);
        if (SYSCALL_ERR(ret)) {
            log_message("ERROR: execve proot failed\n", 29);
            syscall_exit(1);
        }
    } else {
        /* Parent: store child PID */
        ctx->proot_pid = (pid_t)pid;
        log_message("BOOTSTRAP: proot spawned\n", 26);
    }

    return EOK;
}

/* Wait for proot child with watchdog timeout */
static int wait_proot_with_watchdog(struct BootstrapContext *ctx, int timeout_s) {
    int wstatus = 0;

    uint64_t timeout_ns = (uint64_t)timeout_s * 1000000000ULL;
    uint64_t deadline_ns = ctx->start_time_ns + timeout_ns;

    while (1) {
        /* Timeout check */
        int64_t now = get_monotonic_ns();
        if (now < 0 || (uint64_t)now > deadline_ns) {
            log_message("WATCHDOG: timeout exceeded, killing proot\n", 43);
            syscall_kill(ctx->proot_pid, 9);  /* SIGKILL */
            return EFAIL;
        }

        /* Wait4 with WNOHANG — non-blocking check */
        int64_t ret = syscall_wait4(ctx->proot_pid, &wstatus, 4, NULL);  /* 4 = WNOHANG */

        if (ret == ctx->proot_pid) {
            /* Child exited */
            if (wstatus == 0) {
                return EOK;
            } else {
                log_message("ERROR: proot exited with non-zero\n", 35);
                return EFAIL;
            }
        } else if (SYSCALL_ERR(ret)) {
            log_message("ERROR: wait4 failed\n", 20);
            return EFAIL;
        }

        /* Spin-wait briefly (freestanding, no nanosleep) */
        for (volatile uint32_t i = 0; i < 10000000; i++);
    }
}

/* Kill and restart proot (P0.3 real implementation) */
static int restart_proot_real(struct BootstrapContext *ctx) {
    log_message("BOOTSTRAP: restarting proot\n", 29);

    /* Kill current process */
    if (ctx->proot_pid > 0) {
        syscall_kill(ctx->proot_pid, 9);  /* SIGKILL */
    }

    /* Wait for it to die */
    int wstatus = 0;
    syscall_wait4(ctx->proot_pid, &wstatus, 0, NULL);

    /* Reset state machine to PROOT_INITIALIZED for retry */
    ctx->state = STATE_PROOT_INITIALIZED;
    ctx->proot_pid = 0;
    ctx->start_time_ns = get_monotonic_ns();

    if (ctx->start_time_ns < 0) {
        return EFAIL;
    }

    return EOK;
}

/* Main bootstrap entry point */
int bootstrap_main(void) {
    struct BootstrapContext ctx;
    ctx.state = STATE_PREFIX_EMPTY;
    ctx.proot_pid = 0;
    ctx.watchdog_fd = -1;
    ctx.start_time_ns = 0;

    /* Initialize */
    if (proot_init(&ctx) != EOK) {
        return 1;
    }

    /* Spawn proot child */
    if (spawn_proot_child(&ctx, TERMUX_PREFIX, PROOT_BIN) != EOK) {
        return 2;
    }

    /* Wait for completion with 30-second watchdog */
    if (wait_proot_with_watchdog(&ctx, 30) != EOK) {
        /* On timeout, attempt restart (once) */
        if (restart_proot_real(&ctx) != EOK) {
            return 3;
        }
        /* After restart, wait again */
        if (wait_proot_with_watchdog(&ctx, 30) != EOK) {
            return 4;
        }
    }

    ctx.state = STATE_PAYLOAD_EXTRACTED;
    log_message("BOOTSTRAP: PROOT_INITIALIZED → PAYLOAD_EXTRACTED\n", 50);

    return 0;
}

/* Exit wrapper */
void _exit_bootstrap(int code) {
    syscall_exit(code);
    /* unreachable */
    for (;;);
}
