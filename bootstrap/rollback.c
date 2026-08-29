#include "proot_config.h"
#include "proot_syscall_bridge.h"
#include "receipt_sealer.c"
#include <stdint.h>
#include <string.h>

/* Atomic State Machine with Deterministic Rollback */
/* No malloc, freestanding, single-threaded only */

typedef struct {
    proot_state_t current_state;
    int last_error;
    uint32_t retry_count;
    uint8_t rollback_stack[PROOT_ROLLBACK_MAX_DEPTH + 1];
    uint32_t rollback_depth;
} state_machine_t;

static state_machine_t _state_machine = {
    .current_state = PROOT_STATE_PREFIX_EMPTY,
    .last_error = 0,
    .retry_count = 0,
    .rollback_depth = 0
};

/* State name mapping for logging */
static const char *_state_names[] = {
    "PREFIX_EMPTY",
    "INITIALIZED",
    "PAYLOAD_EXTRACTED",
    "DPKG_INSTALLED",
    "APT_CONFIGURED",
    "USER_PACKAGES_READY"
};

/* Get current state */
proot_state_t get_current_state(void) {
    return _state_machine.current_state;
}

/* Get state name for logging */
const char *get_state_name(proot_state_t state) {
    if (state < 0 || state >= PROOT_RECEIPT_MAX_STAGES) {
        return "UNKNOWN";
    }
    return _state_names[state];
}

/* Verify state transition is valid (only forward or reset) */
static int _is_valid_transition(proot_state_t from, proot_state_t to) {
    if (to == PROOT_STATE_PREFIX_EMPTY) {
        return 1;  /* Rollback to empty always allowed */
    }
    if (to > from && to <= PROOT_STATE_USER_PACKAGES_READY) {
        return 1;  /* Forward progression allowed */
    }
    return 0;  /* Invalid transition */
}

/* Execute state machine transition */
int transition_state(
    proot_state_t next_state,
    const void *state_snapshot,
    uint64_t state_size
) {
    if (!_is_valid_transition(_state_machine.current_state, next_state)) {
        _state_machine.last_error = -1;  /* Invalid transition */
        return -1;
    }

    /* Seal receipt for current state before transition */
    receipt_t receipt;
    int seal_result = seal_receipt(
        _state_machine.current_state,
        1,  /* status: PASS */
        state_snapshot,
        state_size,
        &receipt
    );

    if (seal_result != 0) {
        _state_machine.last_error = seal_result;
        return seal_result;
    }

    /* Verify and log receipt */
    if (verify_receipt(&receipt) != 0) {
        _state_machine.last_error = -2;  /* Receipt verification failed */
        return -2;
    }

    if (log_receipt(&receipt) != 0) {
        _state_machine.last_error = -3;  /* Receipt logging failed */
        return -3;
    }

    /* Push previous state to rollback stack */
    if (_state_machine.rollback_depth < PROOT_ROLLBACK_MAX_DEPTH) {
        _state_machine.rollback_stack[_state_machine.rollback_depth++] = _state_machine.current_state;
    }

    /* Perform state transition */
    _state_machine.current_state = next_state;
    _state_machine.retry_count = 0;
    return 0;
}

/* Handle failure in current state: trigger atomic rollback */
int on_state_failure(int error_code) {
    if (error_code == 0) {
        return 0;  /* Not a failure */
    }

    _state_machine.last_error = error_code;
    _state_machine.retry_count++;

    /* Check if we should retry or rollback */
    if (_state_machine.retry_count < PROOT_MAX_RESTART_ATTEMPTS) {
        return 1;  /* Signal: caller should retry current state */
    }

    /* Max retries exceeded: trigger rollback */
    return rollback_to_empty();
}

/* Rollback to PREFIX_EMPTY state (atomic reset) */
int rollback_to_empty(void) {
    /* Seal failure receipt for current state */
    receipt_t failure_receipt;
    int seal_result = seal_receipt(
        _state_machine.current_state,
        0,  /* status: FAIL */
        NULL,
        0,
        &failure_receipt
    );

    if (seal_result == 0) {
        log_receipt(&failure_receipt);  /* Log failure even if sealing failed */
    }

    /* Clear rollback stack and reset to PREFIX_EMPTY */
    _state_machine.rollback_depth = 0;
    _state_machine.current_state = PROOT_STATE_PREFIX_EMPTY;
    _state_machine.retry_count = 0;
    return 0;
}

/* Get last error code */
int get_last_error(void) {
    return _state_machine.last_error;
}

/* Get retry count */
uint32_t get_retry_count(void) {
    return _state_machine.retry_count;
}

/* State machine snapshot for debugging */
typedef struct {
    proot_state_t current;
    int last_error;
    uint32_t retry_count;
    uint32_t rollback_depth;
} state_snapshot_t;

state_snapshot_t get_state_snapshot(void) {
    return (state_snapshot_t) {
        .current = _state_machine.current_state,
        .last_error = _state_machine.last_error,
        .retry_count = _state_machine.retry_count,
        .rollback_depth = _state_machine.rollback_depth
    };
}

/* Write state machine state to syslog-like output */
int write_state_log(const char *path) {
    int fd = proot_sys_open(path, 0x241, 0644);  /* O_CREAT | O_WRONLY | O_TRUNC */
    if (fd < 0) {
        return -1;
    }

    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "{\"schema\":\"raf.state-machine-log.v1\","
        "\"current_state\":\"%s\","
        "\"state_value\":%d,"
        "\"last_error\":%d,"
        "\"retry_count\":%u,"
        "\"rollback_depth\":%u}",
        get_state_name(_state_machine.current_state),
        _state_machine.current_state,
        _state_machine.last_error,
        _state_machine.retry_count,
        _state_machine.rollback_depth
    );

    if (len > 0) {
        proot_sys_write(fd, buf, len);
    }

    proot_sys_close(fd);
    return 0;
}
