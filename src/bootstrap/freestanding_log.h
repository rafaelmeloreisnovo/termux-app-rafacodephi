/*
 * freestanding_log.h — Freestanding logging via the canonical syscall layer
 *
 * Replaces stdio.h printf() and android/log.h without external runtime
 * dependencies. write(2) is provided by freestanding_syscalls.h so logging
 * follows the same ABI-correct ARM32/ARM64/x86/x86_64 path as the rest of the
 * bootstrap instead of carrying a second ARM64-only syscall implementation.
 */

#ifndef FREESTANDING_LOG_H
#define FREESTANDING_LOG_H

#include <stdint.h>
#include <stddef.h>

#include "freestanding_syscalls.h"

/* Log to stderr (fd 2) */
static inline int64_t freestanding_log(const char *msg, uint32_t len) {
    if (!msg || len == 0u) return 0;
    return freestanding_write(2, msg, len);
}

/* Formatted logging with pre-computed string (va_args not supported).
 * Caller must pre-format the message into a buffer.
 */
static inline int64_t freestanding_logf(const char *prefix, const char *msg,
                                        uint32_t msg_len) {
    int64_t ret = 0;

    if (prefix) {
        uint32_t prefix_len = 0u;
        const char *p = prefix;
        while (*p && prefix_len < 256u) {
            prefix_len++;
            p++;
        }
        if (prefix_len > 0u) {
            ret = freestanding_write(2, prefix, prefix_len);
            if (ret < 0) return ret;
        }
    }

    if (msg && msg_len > 0u) {
        ret = freestanding_write(2, msg, msg_len);
    }

    return ret;
}

static inline int64_t freestanding_log_stage(const char *stage_tag, const char *msg,
                                             uint32_t msg_len) {
    return freestanding_logf(stage_tag, msg, msg_len);
}

static inline int64_t freestanding_log_error(const char *msg, uint32_t msg_len) {
    return freestanding_logf("[ERROR] ", msg, msg_len);
}

static inline int64_t freestanding_log_warn(const char *msg, uint32_t msg_len) {
    return freestanding_logf("[WARN] ", msg, msg_len);
}

static inline int64_t freestanding_log_info(const char *msg, uint32_t msg_len) {
    return freestanding_logf("[INFO] ", msg, msg_len);
}

static inline int64_t freestanding_log_debug(const char *msg, uint32_t msg_len) {
#ifdef FREESTANDING_DEBUG
    return freestanding_logf("[DEBUG] ", msg, msg_len);
#else
    (void)msg;
    (void)msg_len;
    return 0;
#endif
}

static inline void freestanding_hexdump(const uint8_t *buf, uint32_t len) {
    if (!buf || len == 0u) return;

    static const char hex_chars[] = "0123456789ABCDEF";
    char line[80];
    uint32_t line_idx = 0u;

    for (uint32_t i = 0u; i < len; i++) {
        const uint8_t b = buf[i];
        const uint32_t hex_high = (uint32_t)((b >> 4) & 0x0Fu);
        const uint32_t hex_low = (uint32_t)(b & 0x0Fu);

        if (line_idx < 77u) {
            line[line_idx++] = hex_chars[hex_high];
            line[line_idx++] = hex_chars[hex_low];
            if ((i + 1u) % 16u == 0u) {
                line[line_idx++] = '\n';
                (void)freestanding_write(2, line, line_idx);
                line_idx = 0u;
            } else {
                line[line_idx++] = ' ';
            }
        }
    }

    if (line_idx > 0u) {
        line[line_idx++] = '\n';
        (void)freestanding_write(2, line, line_idx);
    }
}

#endif /* FREESTANDING_LOG_H */
