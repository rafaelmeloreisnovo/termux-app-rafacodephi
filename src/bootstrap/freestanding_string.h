/*
 * freestanding_string.h — Freestanding string utilities
 *
 * Replaces string.h (memcpy, memset, strlen, etc.) without external dependencies.
 * Minimal, branchless implementations suitable for bootstrap context.
 *
 * No external dependencies: pure freestanding C, no libc.
 */

#ifndef FREESTANDING_STRING_H
#define FREESTANDING_STRING_H

#include <stdint.h>
#include <stddef.h>

/* Memory copy: src → dst (assumes non-overlapping)
 * Returns dst
 */
static inline void *freestanding_memcpy(void *dst, const void *src, uint32_t len) {
    if (!dst || !src) return dst;

    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    for (uint32_t i = 0; i < len; i++) {
        d[i] = s[i];
    }

    return dst;
}

/* Memory move: src → dst (handles overlapping regions)
 * Returns dst
 */
static inline void *freestanding_memmove(void *dst, const void *src, uint32_t len) {
    if (!dst || !src) return dst;

    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    /* If src < dst, copy backward to avoid overwrite */
    if (s < d && d < s + len) {
        for (int32_t i = (int32_t)len - 1; i >= 0; i--) {
            d[i] = s[i];
        }
    } else {
        for (uint32_t i = 0; i < len; i++) {
            d[i] = s[i];
        }
    }

    return dst;
}

/* Memory set: fill dst with byte value
 * Returns dst
 */
static inline void *freestanding_memset(void *dst, int value, uint32_t len) {
    if (!dst) return dst;

    uint8_t *d = (uint8_t *)dst;
    uint8_t byte = (uint8_t)(value & 0xFF);

    for (uint32_t i = 0; i < len; i++) {
        d[i] = byte;
    }

    return dst;
}

/* Memory compare: compare len bytes of a and b
 * Returns: 0 if equal, <0 if a<b, >0 if a>b
 */
static inline int32_t freestanding_memcmp(const void *a, const void *b, uint32_t len) {
    if (!a || !b) return 0;

    const uint8_t *p_a = (const uint8_t *)a;
    const uint8_t *p_b = (const uint8_t *)b;

    for (uint32_t i = 0; i < len; i++) {
        if (p_a[i] != p_b[i]) {
            return (int32_t)p_a[i] - (int32_t)p_b[i];
        }
    }

    return 0;
}

/* String length: count bytes until null terminator
 * Returns length in bytes (not including null terminator)
 */
static inline uint32_t freestanding_strlen(const char *s) {
    if (!s) return 0;

    uint32_t len = 0;
    while (s[len] != '\0') {
        len++;
    }

    return len;
}

/* String copy: src → dst (src must be null-terminated)
 * Returns dst
 */
static inline char *freestanding_strcpy(char *dst, const char *src) {
    if (!dst || !src) return dst;

    uint32_t i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';

    return dst;
}

/* String copy with length limit: src → dst (null-terminated)
 * Copies at most (max_len - 1) bytes, always null-terminates
 * Returns dst
 */
static inline char *freestanding_strncpy(char *dst, const char *src, uint32_t max_len) {
    if (!dst || !src || max_len == 0) return dst;

    uint32_t i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';

    return dst;
}

/* String compare: compare until null terminator
 * Returns: 0 if equal, <0 if a<b, >0 if a>b
 */
static inline int32_t freestanding_strcmp(const char *a, const char *b) {
    if (!a || !b) return 0;

    uint32_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return (int32_t)(uint8_t)a[i] - (int32_t)(uint8_t)b[i];
        }
        i++;
    }

    return (int32_t)(uint8_t)a[i] - (int32_t)(uint8_t)b[i];
}

/* String compare with length limit
 * Returns: 0 if equal (up to max_len), <0 if a<b, >0 if a>b
 */
static inline int32_t freestanding_strncmp(const char *a, const char *b, uint32_t max_len) {
    if (!a || !b || max_len == 0) return 0;

    for (uint32_t i = 0; i < max_len; i++) {
        if (a[i] == '\0' || b[i] == '\0') {
            return (int32_t)(uint8_t)a[i] - (int32_t)(uint8_t)b[i];
        }
        if (a[i] != b[i]) {
            return (int32_t)(uint8_t)a[i] - (int32_t)(uint8_t)b[i];
        }
    }

    return 0;
}

/* Find first occurrence of character in string
 * Returns pointer to char, or NULL if not found
 */
static inline char *freestanding_strchr(const char *s, int ch) {
    if (!s) return NULL;

    uint8_t target = (uint8_t)(ch & 0xFF);

    for (uint32_t i = 0; s[i] != '\0'; i++) {
        if ((uint8_t)s[i] == target) {
            return (char *)&s[i];
        }
    }

    /* Check for null terminator match */
    if (target == 0) {
        return (char *)&s[freestanding_strlen(s)];
    }

    return NULL;
}

/* Find last occurrence of character in string
 * Returns pointer to char, or NULL if not found
 */
static inline char *freestanding_strrchr(const char *s, int ch) {
    if (!s) return NULL;

    uint8_t target = (uint8_t)(ch & 0xFF);
    char *last = NULL;

    for (uint32_t i = 0; s[i] != '\0'; i++) {
        if ((uint8_t)s[i] == target) {
            last = (char *)&s[i];
        }
    }

    if (target == 0) {
        return (char *)&s[freestanding_strlen(s)];
    }

    return last;
}

/* String concatenation: append src to dst
 * Assumes dst has enough space
 * Returns dst
 */
static inline char *freestanding_strcat(char *dst, const char *src) {
    if (!dst || !src) return dst;

    uint32_t dst_len = freestanding_strlen(dst);
    freestanding_strcpy(&dst[dst_len], src);

    return dst;
}

/* String concatenation with length limit
 * Appends at most (max_len - 1 - dst_len) bytes
 * Returns dst
 */
static inline char *freestanding_strncat(char *dst, const char *src, uint32_t max_len) {
    if (!dst || !src || max_len == 0) return dst;

    uint32_t dst_len = freestanding_strlen(dst);
    if (dst_len >= max_len) return dst;

    uint32_t remaining = max_len - dst_len;
    freestanding_strncpy(&dst[dst_len], src, remaining);

    return dst;
}

/* Convert string to integer
 * Parses decimal numbers, stops at first non-digit
 * Returns parsed value, or 0 on error
 */
static inline int32_t freestanding_atoi(const char *s) {
    if (!s) return 0;

    int32_t value = 0;
    int32_t sign = 1;
    uint32_t i = 0;

    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }

    while (s[i] >= '0' && s[i] <= '9') {
        value = value * 10 + (s[i] - '0');
        i++;
    }

    return value * sign;
}

#endif /* FREESTANDING_STRING_H */
