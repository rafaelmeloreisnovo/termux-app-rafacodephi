/*
 * freestanding_log.h — Freestanding logging via direct syscalls
 *
 * Replaces stdio.h printf() and android/log.h without external dependencies.
 * Uses direct write(2) to stderr (fd 2) for all logging.
 *
 * No external dependencies: pure freestanding C, no libc/malloc/syscalls wrapper.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* Direct syscall: write(fd, buf, count) → ssize_t
 * ARM64 SVC: x8=64, x0=fd, x1=buf, x2=count
 * Returns number of bytes written, or negative on error
 */
static inline int64_t freestanding_write(int fd, const void *buf, uint32_t count) {
    register int64_t x0 asm("x0") = (int64_t)fd;
    register int64_t x1 asm("x1") = (int64_t)buf;
    register int64_t x2 asm("x2") = (int64_t)count;
    register int64_t x8 asm("x8") = 64;  /* write syscall number */

    asm volatile (
        "svc #0"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x8)
        : "cc", "memory"
    );

    return x0;
}

/* Log to stderr (fd 2) */
static inline int64_t freestanding_log(const char *msg, uint32_t len) {
    if (!msg || len == 0) return 0;
    return freestanding_write(2, msg, len);
}

/* Formatted logging with pre-computed string (va_args not supported)
 * Caller must pre-format the message into a buffer
 */
static inline int64_t freestanding_logf(const char *prefix, const char *msg,
                                        uint32_t msg_len) {
    int64_t ret = 0;

    if (prefix) {
        uint32_t prefix_len = 0;
        const char *p = prefix;
        while (*p && prefix_len < 256) {
            prefix_len++;
            p++;
        }
        if (prefix_len > 0) {
            ret = freestanding_write(2, prefix, prefix_len);
            if (ret < 0) return ret;
        }
    }

    if (msg && msg_len > 0) {
        ret = freestanding_write(2, msg, msg_len);
    }

    return ret;
}

/* Log stage transition with leading tag
 * Usage: freestanding_log_stage("[STAGE-X]", msg, len)
 */
static inline int64_t freestanding_log_stage(const char *stage_tag, const char *msg,
                                             uint32_t msg_len) {
    return freestanding_logf(stage_tag, msg, msg_len);
}

/* Log error with prefix */
static inline int64_t freestanding_log_error(const char *msg, uint32_t msg_len) {
    return freestanding_logf("[ERROR] ", msg, msg_len);
}

/* Log warning with prefix */
static inline int64_t freestanding_log_warn(const char *msg, uint32_t msg_len) {
    return freestanding_logf("[WARN] ", msg, msg_len);
}

/* Log info with prefix */
static inline int64_t freestanding_log_info(const char *msg, uint32_t msg_len) {
    return freestanding_logf("[INFO] ", msg, msg_len);
}

/* Log debug with prefix (typically disabled in production) */
static inline int64_t freestanding_log_debug(const char *msg, uint32_t msg_len) {
#ifdef FREESTANDING_DEBUG
    return freestanding_logf("[DEBUG] ", msg, msg_len);
#else
    (void)msg;
    (void)msg_len;
    return 0;
#endif
}

/* Hexdump buffer to stderr (diagnostic) */
static inline void freestanding_hexdump(const uint8_t *buf, uint32_t len) {
    if (!buf || len == 0) return;

    char hex_chars[16] = "0123456789ABCDEF";
    char line[80];
    uint32_t line_idx = 0;

    for (uint32_t i = 0; i < len; i++) {
        uint8_t b = buf[i];
        uint32_t hex_high = (b >> 4) & 0xF;
        uint32_t hex_low = b & 0xF;

        if (line_idx < 77) {
            line[line_idx++] = hex_chars[hex_high];
            line[line_idx++] = hex_chars[hex_low];
            if ((i + 1) % 16 == 0) {
                line[line_idx++] = '\n';
                freestanding_write(2, line, line_idx);
                line_idx = 0;
            } else {
                line[line_idx++] = ' ';
            }
        }
    }

    if (line_idx > 0) {
        line[line_idx++] = '\n';
        freestanding_write(2, line, line_idx);
    }
}

#endif /* FREESTANDING_LOG_H */
