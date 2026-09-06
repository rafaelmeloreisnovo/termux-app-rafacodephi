/*
 * RAFCODEPHI RAF-NINJA proot controller.
 *
 * This translation unit is freestanding.  It launches and supervises the
 * source-built proot payload but does not misclassify proot.real itself as a
 * freestanding artifact.
 */

#include "freestanding.h"
#include "syscall_arm.h"

#ifndef RAFCODEPHI_PREFIX
#define RAFCODEPHI_PREFIX "/data/data/com.termux.rafacodephi/files/usr"
#endif

#define PROOT_REAL_BIN RAFCODEPHI_PREFIX "/bin/proot.real"
#define PROOT_BIN      RAFCODEPHI_PREFIX "/bin/proot"
#define RAF_CLOCK_MONOTONIC 1
#define RAF_WNOHANG 1
#define RAF_SIGKILL 9

struct ProotNinjaReceipt {
    int64_t pid;
    uint64_t start_ns;
    uint64_t finish_ns;
    int32_t raw_wait_status;
    int32_t timed_out;
    int32_t used_real_binary;
};

static int64_t log_message(const char *msg, uint32_t len) {
    return syscall_write(2, msg, len);
}

static int64_t get_monotonic_ns(void) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    if (SYSCALL_ERR(syscall_clock_gettime(RAF_CLOCK_MONOTONIC, &ts))) return -1;
    if (ts.tv_sec < 0 || ts.tv_nsec < 0 || ts.tv_nsec >= 1000000000LL) return -1;
    return (ts.tv_sec * 1000000000LL) + ts.tv_nsec;
}

static void cpu_relax_bounded(void) {
    volatile uint32_t i;
    for (i = 0; i < 32768u; ++i) {
#if defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("yield" ::: "memory");
#else
        __asm__ volatile("" ::: "memory");
#endif
    }
}

static const char *select_proot_binary(int *used_real) {
    if (syscall_access(PROOT_REAL_BIN, 1) == 0) {
        *used_real = 1;
        return PROOT_REAL_BIN;
    }
    if (syscall_access(PROOT_BIN, 1) == 0) {
        *used_real = 0;
        return PROOT_BIN;
    }
    return (const char *)0;
}

static int run_version_probe(const char *binary, struct ProotNinjaReceipt *receipt, uint32_t timeout_s) {
    char *argv[] = {
        (char *)binary,
        (char *)"--version",
        (char *)0
    };
    char *envp[] = {
        (char *)"PATH=" RAFCODEPHI_PREFIX "/bin:/system/bin",
        (char *)"PREFIX=" RAFCODEPHI_PREFIX,
        (char *)"TMPDIR=" RAFCODEPHI_PREFIX "/tmp",
        (char *)"HOME=/data/data/com.termux.rafacodephi/files/home",
        (char *)0
    };

    int64_t start = get_monotonic_ns();
    if (start < 0) return -2;

    int64_t pid = syscall_fork();
    if (SYSCALL_ERR(pid)) return -3;

    if (pid == 0) {
        int64_t exec_rc = syscall_execve(binary, argv, envp);
        (void)exec_rc;
        syscall_exit(127);
    }

    receipt->pid = pid;
    receipt->start_ns = (uint64_t)start;
    receipt->finish_ns = 0;
    receipt->raw_wait_status = -1;
    receipt->timed_out = 0;

    const uint64_t timeout_ns = (uint64_t)timeout_s * 1000000000ULL;
    for (;;) {
        int status = 0;
        int64_t waited = syscall_wait4((int)pid, &status, RAF_WNOHANG, (void *)0);
        if (waited == pid) {
            int64_t finish = get_monotonic_ns();
            receipt->finish_ns = finish < 0 ? 0u : (uint64_t)finish;
            receipt->raw_wait_status = status;
            return status == 0 ? 0 : -4;
        }
        if (SYSCALL_ERR(waited)) return -5;

        int64_t now = get_monotonic_ns();
        if (now < 0 || (uint64_t)(now - start) >= timeout_ns) {
            receipt->timed_out = 1;
            (void)syscall_kill((int)pid, RAF_SIGKILL);
            (void)syscall_wait4((int)pid, &status, 0, (void *)0);
            receipt->raw_wait_status = status;
            receipt->finish_ns = now < 0 ? 0u : (uint64_t)now;
            return -6;
        }
        cpu_relax_bounded();
    }
}

int proot_ninja_probe_real(struct ProotNinjaReceipt *receipt) {
    if (!receipt) return -1;
    receipt->pid = 0;
    receipt->start_ns = 0;
    receipt->finish_ns = 0;
    receipt->raw_wait_status = -1;
    receipt->timed_out = 0;
    receipt->used_real_binary = 0;

    int used_real = 0;
    const char *binary = select_proot_binary(&used_real);
    if (!binary) {
        log_message("RAF-NINJA: proot payload missing\n", 33);
        return -7;
    }
    receipt->used_real_binary = used_real;

    int rc = run_version_probe(binary, receipt, 10u);
    if (rc == 0) {
        log_message("RAF-NINJA: proot payload probe PASS\n", 36);
    } else {
        log_message("RAF-NINJA: proot payload probe FAIL\n", 36);
    }
    return rc;
}

/* Historical entry point retained as an adapter.  Payload extraction and dpkg
 * initialization are intentionally owned by the orchestrator and happen first. */
int bootstrap_main(void) {
    struct ProotNinjaReceipt receipt;
    return proot_ninja_probe_real(&receipt);
}

void _exit_bootstrap(int code) {
    syscall_exit(code);
}
