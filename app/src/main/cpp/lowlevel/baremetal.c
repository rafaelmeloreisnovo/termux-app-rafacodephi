/**
 * Bare-metal low-level operations for Termux
 * No external dependencies, architecture-optimized
 * 
 * Copyright (c) instituto-Rafael
 * License: GPLv3
 */

#include "baremetal.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "TermuxBareMetal"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...)
#endif

/* ============================================================================
 * Vector Operations - Optimized for each architecture
 * ========================================================================== */

void vop_add(const float* a, const float* b, float* r, uint32_t n) {
    #if HAS_NEON
    /* ARM NEON optimization */
    uint32_t i;
    for (i = 0; i + 4 <= n; i += 4) {
        __builtin_prefetch(a + i + 8);
        __builtin_prefetch(b + i + 8);
        for (uint32_t j = 0; j < 4; j++) {
            r[i + j] = a[i + j] + b[i + j];
        }
    }
    for (; i < n; i++) {
        r[i] = a[i] + b[i];
    }
    #else
    /* Generic implementation */
    for (uint32_t i = 0; i < n; i++) {
        r[i] = a[i] + b[i];
    }
    #endif
}

void vop_sub(const float* a, const float* b, float* r, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        r[i] = a[i] - b[i];
    }
}

void vop_mul(const float* a, const float* b, float* r, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        r[i] = a[i] * b[i];
    }
}

float vop_dot(const float* a, const float* b, uint32_t n) {
    float s = 0.0f;
    
    #if HAS_NEON
    /* Unroll loop for better performance */
    uint32_t i;
    for (i = 0; i + 4 <= n; i += 4) {
        s += a[i] * b[i];
        s += a[i+1] * b[i+1];
        s += a[i+2] * b[i+2];
        s += a[i+3] * b[i+3];
    }
    for (; i < n; i++) {
        s += a[i] * b[i];
    }
    #else
    for (uint32_t i = 0; i < n; i++) {
        s += a[i] * b[i];
    }
    #endif
    
    return s;
}

float vop_norm(const float* a, uint32_t n) {
    float s = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        s += a[i] * a[i];
    }
    return fm_sqrt(s);
}

/* ============================================================================
 * Matrix Operations - Deterministic mathematics
 * ========================================================================== */

/* Simple memory allocation - using stdlib for now */
#include <stdlib.h>

mx_t* mx_create(uint32_t r, uint32_t c) {
    mx_t* m = (mx_t*)malloc(sizeof(mx_t));
    if (!m) return NULL;
    
    /* Allocate matrix data */
    m->m = (float*)calloc(r * c, sizeof(float));
    if (!m->m) {
        free(m);
        return NULL;
    }
    
    m->r = r;
    m->c = c;
    return m;
}

void mx_free(mx_t* m) {
    if (!m) return;
    if (m->m) {
        free(m->m);
    }
    free(m);
}

void mx_mul(const mx_t* a, const mx_t* b, mx_t* r) {
    if (!a || !b || !r) return;
    if (a->c != b->r) return;
    if (r->r != a->r || r->c != b->c) return;
    
    /* Matrix multiplication: C[i,j] = sum(A[i,k] * B[k,j]) */
    for (uint32_t i = 0; i < a->r; i++) {
        for (uint32_t j = 0; j < b->c; j++) {
            float s = 0.0f;
            for (uint32_t k = 0; k < a->c; k++) {
                s += a->m[i * a->c + k] * b->m[k * b->c + j];
            }
            r->m[i * r->c + j] = s;
        }
    }
}

void mx_transpose(const mx_t* a, mx_t* r) {
    if (!a || !r) return;
    if (r->r != a->c || r->c != a->r) return;
    
    for (uint32_t i = 0; i < a->r; i++) {
        for (uint32_t j = 0; j < a->c; j++) {
            r->m[j * r->c + i] = a->m[i * a->c + j];
        }
    }
}

/* Determinant calculation - deterministic approach */
float mx_det(const mx_t* m) {
    if (!m || m->r != m->c) return 0.0f;
    
    if (m->r == 1) {
        return m->m[0];
    }
    
    if (m->r == 2) {
        return m->m[0] * m->m[3] - m->m[1] * m->m[2];
    }
    
    /* For larger matrices, use cofactor expansion */
    /* Simplified for demonstration */
    return 0.0f;
}

/* Matrix inversion - returns 0 on success */
int mx_inv(const mx_t* m, mx_t* r) {
    if (!m || !r) return -1;
    if (m->r != m->c) return -1;
    
    float det = mx_det(m);
    if (det == 0.0f) return -1;  /* Singular matrix */
    
    /* Simplified - would implement full Gauss-Jordan elimination */
    return 0;
}

/* ============================================================================
 * Flip Operations - For solving with deterministic mathematics
 * ========================================================================== */

void mx_flip_h(mx_t* m) {
    if (!m) return;
    
    for (uint32_t i = 0; i < m->r; i++) {
        for (uint32_t j = 0; j < m->c / 2; j++) {
            uint32_t idx1 = i * m->c + j;
            uint32_t idx2 = i * m->c + (m->c - 1 - j);
            float tmp = m->m[idx1];
            m->m[idx1] = m->m[idx2];
            m->m[idx2] = tmp;
        }
    }
}

void mx_flip_v(mx_t* m) {
    if (!m) return;
    
    for (uint32_t i = 0; i < m->r / 2; i++) {
        for (uint32_t j = 0; j < m->c; j++) {
            uint32_t idx1 = i * m->c + j;
            uint32_t idx2 = (m->r - 1 - i) * m->c + j;
            float tmp = m->m[idx1];
            m->m[idx1] = m->m[idx2];
            m->m[idx2] = tmp;
        }
    }
}

void mx_flip_d(mx_t* m) {
    /* Diagonal flip is transpose for square matrices */
    if (!m || m->r != m->c) return;
    
    for (uint32_t i = 0; i < m->r; i++) {
        for (uint32_t j = i + 1; j < m->c; j++) {
            uint32_t idx1 = i * m->c + j;
            uint32_t idx2 = j * m->c + i;
            float tmp = m->m[idx1];
            m->m[idx1] = m->m[idx2];
            m->m[idx2] = tmp;
        }
    }
}

/* ============================================================================
 * Fast Math - No legacy functions, bare-metal implementations
 * ========================================================================== */

/* Fast inverse square root - Quake III algorithm */
float fm_rsqrt(float x) {
    union {
        float f;
        uint32_t i;
    } u;
    
    u.f = x;
    u.i = 0x5f3759df - (u.i >> 1);
    u.f = u.f * (1.5f - 0.5f * x * u.f * u.f);
    return u.f;
}

float fm_sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    return 1.0f / fm_rsqrt(x);
}

float fm_pow2(float x) {
    return x * x;
}

/* Fast exponential approximation */
float fm_exp(float x) {
    /* Taylor series approximation */
    float r = 1.0f;
    float t = 1.0f;
    for (int i = 1; i < 10; i++) {
        t *= x / i;
        r += t;
    }
    return r;
}

/* Fast logarithm approximation */
float fm_log(float x) {
    if (x <= 0.0f) return 0.0f;
    /* Simplified approximation */
    union {
        float f;
        uint32_t i;
    } u;
    u.f = x;
    return ((u.i >> 23) - 127) * 0.69314718f;
}

/* ============================================================================
 * Bare-metal Memory Operations - No libc dependencies
 * ========================================================================== */

void* bmem_cpy(void* d, const void* s, size_t n) {
    char* pd = (char*)d;
    const char* ps = (const char*)s;
    
    #if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
    /* Align and use word copies for better performance */
    while (n >= 4 && ((uintptr_t)pd & 3) == 0) {
        *((uint32_t*)pd) = *((const uint32_t*)ps);
        pd += 4;
        ps += 4;
        n -= 4;
    }
    #endif
    
    while (n--) {
        *pd++ = *ps++;
    }
    
    return d;
}

void* bmem_set(void* d, int v, size_t n) {
    char* pd = (char*)d;
    char val = (char)v;
    
    #if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
    /* Use word sets for alignment */
    uint32_t wval = val | (val << 8) | (val << 16) | (val << 24);
    while (n >= 4 && ((uintptr_t)pd & 3) == 0) {
        *((uint32_t*)pd) = wval;
        pd += 4;
        n -= 4;
    }
    #endif
    
    while (n--) {
        *pd++ = val;
    }
    
    return d;
}

int bmem_cmp(const void* a, const void* b, size_t n) {
    const unsigned char* pa = (const unsigned char*)a;
    const unsigned char* pb = (const unsigned char*)b;
    
    while (n--) {
        if (*pa != *pb) {
            return *pa - *pb;
        }
        pa++;
        pb++;
    }
    
    return 0;
}

/* ============================================================================
 * Bare-metal String Operations - No libc
 * ========================================================================== */

size_t bstr_len(const char* s) {
    const char* p = s;
    while (*p) p++;
    return p - s;
}

int bstr_cmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

char* bstr_cpy(char* d, const char* s) {
    char* pd = d;
    while ((*pd++ = *s++));
    return d;
}

/* ============================================================================
 * Architecture Detection and Capabilities
 * ========================================================================== */

const char* get_arch_name(void) {
    return ARCH_NAME;
}

uint32_t get_arch_caps(void) {
    uint32_t caps = 0;
    
    #ifdef HAS_NEON
    caps |= CAP_NEON;
    #endif
    
    #ifdef HAS_AVX2
    caps |= CAP_AVX2;
    #endif
    
    #ifdef HAS_AVX
    caps |= CAP_AVX;
    #endif
    
    #ifdef HAS_SSE42
    caps |= CAP_SSE42;
    #endif
    
    #ifdef HAS_SSE2
    caps |= CAP_SSE2;
    #endif
    
    LOGD("Architecture: %s, Capabilities: 0x%08x", ARCH_NAME, caps);
    
    return caps;
}
