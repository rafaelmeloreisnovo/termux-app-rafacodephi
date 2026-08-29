#include "proot_config.h"
#include "proot_syscall_bridge.h"
#include <stdint.h>
#include <string.h>
#include <time.h>

/* Watchdog: 30s Timeout + Automatic Restart */
/* Single-threaded, named-semaphore backed (futex), freestanding */

typedef struct {
    uint32_t timeout_seconds;
    uint32_t max_restarts;
    uint32_t restart_count;
    uint64_t start_time_sec;
    uint64_t start_time_nsec;
    int is_running;
} watchdog_t;

static watchdog_t _watchdog = {
    .timeout_seconds = PROOT_WATCHDOG_TIMEOUT_SECONDS,
    .max_restarts = PROOT_MAX_RESTART_ATTEMPTS,
    .restart_count = 0,
    .start_time_sec = 0,
    .start_time_nsec = 0,
    .is_running = 0
};

/* Get current time (CLOCK_MONOTONIC for timeout measurement) */
static int _get_monotonic_time(uint64_t *sec, uint64_t *nsec) {
    struct timespec ts;
    int result = proot_sys_clock_gettime(1, &ts);  /* CLOCK_MONOTONIC = 1 */
    if (result < 0) {
        return -1;
    }
    *sec = ts.tv_sec;
    *nsec = ts.tv_nsec;
    return 0;
}

/* Start watchdog timer */
int watchdog_start(void) {
    if (_get_monotonic_time(&_watchdog.start_time_sec, &_watchdog.start_time_nsec) < 0) {
        return -1;  /* Failed to get time */
    }

    _watchdog.is_running = 1;
    _watchdog.restart_count = 0;
    return 0;
}

/* Check if watchdog has timed out */
int watchdog_check_timeout(void) {
    if (!_watchdog.is_running) {
        return 0;  /* Not running */
    }

    uint64_t current_sec, current_nsec;
    if (_get_monotonic_time(&current_sec, &current_nsec) < 0) {
        return -1;  /* Failed to get time */
    }

    uint64_t elapsed_sec = current_sec - _watchdog.start_time_sec;
    if (elapsed_sec >= _watchdog.timeout_seconds) {
        return 1;  /* TIMEOUT */
    }

    return 0;  /* Still running */
}

/* Get remaining time in watchdog */
uint64_t watchdog_get_remaining_seconds(void) {
    if (!_watchdog.is_running) {
        return 0;
    }

    uint64_t current_sec, current_nsec;
    if (_get_monotonic_time(&current_sec, &current_nsec) < 0) {
        return 0;
    }

    uint64_t elapsed = current_sec - _watchdog.start_time_sec;
    if (elapsed >= _watchdog.timeout_seconds) {
        return 0;
    }

    return _watchdog.timeout_seconds - elapsed;
}

/* Handle timeout: attempt restart */
int watchdog_handle_timeout(void) {
    if (_watchdog.restart_count >= _watchdog.max_restarts) {
        /* Max restarts exceeded: FATAL */
        return PROOT_FAILOVER_EXIT_CODE;
    }

    _watchdog.restart_count++;
    return 0;  /* OK, can restart */
}

/* Reset watchdog (continue operation) */
int watchdog_reset(void) {
    if (!_watchdog.is_running) {
        return -1;
    }

    return watchdog_start();  /* Restart timer */
}

/* Stop watchdog */
int watchdog_stop(void) {
    _watchdog.is_running = 0;
    _watchdog.restart_count = 0;
    return 0;
}

/* Watchdog status for logging */
typedef struct {
    int is_running;
    uint32_t restart_count;
    uint64_t remaining_seconds;
    uint32_t timeout_seconds;
} watchdog_status_t;

watchdog_status_t watchdog_get_status(void) {
    return (watchdog_status_t) {
        .is_running = _watchdog.is_running,
        .restart_count = _watchdog.restart_count,
        .remaining_seconds = watchdog_get_remaining_seconds(),
        .timeout_seconds = _watchdog.timeout_seconds
    };
}

/* Write watchdog status to file */
int watchdog_write_status(const char *path) {
    int fd = proot_sys_open(path, 0x241, 0644);  /* O_CREAT | O_WRONLY | O_TRUNC */
    if (fd < 0) {
        return -1;
    }

    watchdog_status_t status = watchdog_get_status();
    char buf[256];
    int len = snprintf(buf, sizeof(buf),
        "{\"schema\":\"raf.watchdog-status.v1\","
        "\"is_running\":%d,"
        "\"restart_count\":%u,"
        "\"remaining_seconds\":%lu,"
        "\"timeout_seconds\":%u}",
        status.is_running,
        status.restart_count,
        status.remaining_seconds,
        status.timeout_seconds
    );

    if (len > 0) {
        proot_sys_write(fd, buf, len);
    }

    proot_sys_close(fd);
    return 0;
}

/* Proot process restart routine (simplified stub) */
int restart_proot_process(void) {
    /* This is a placeholder for the actual proot restart logic */
    /* In a real implementation, this would:
     * 1. Kill existing proot process
     * 2. Reset prefix state
     * 3. Reinitialize proot
     */
    int restart_status = watchdog_handle_timeout();
    if (restart_status != 0) {
        /* Max retries exceeded, fatal */
        return restart_status;
    }

    /* Reset watchdog for next attempt */
    watchdog_reset();
    return 0;
}
