/**
 * Bare-metal low-level operations for Termux
 * RAFAELIA Framework Implementation
 * No external dependencies, architecture-optimized
 * 
 * RAFAELIA Methodology:
 * ---------------------
 * This module implements the RAFAELIA (RAfael FrAmework for Ethical Linear and 
 * Iterative Analysis) computational framework, which emphasizes:
 * 
 * 1. Matrix-based deterministic computation
 *    - All operations are expressed as matrix transformations
 *    - Flip operations (horizontal, vertical, diagonal) for solving systems
 *    - Linear algebra without external library dependencies
 * 
 * 2. Minimal abstraction
 *    - Variables use short names (m, r, c) to reduce overhead
 *    - Direct memory access for performance
 *    - No legacy function names
 * 
 * 3. Ethical computation (Φ_ethica)
 *    - Minimize entropy, maximize coherence
 *    - Deterministic outcomes for reproducibility
 *    - Clear, verifiable algorithms
 * 
 * 4. Hardware optimization
 *    - SIMD support (NEON, AVX, SSE)
 *    - Architecture-specific optimizations
 *    - Bare-metal implementations
 * 
 * 5. Self-contained implementation
 *    - Only stdlib for malloc/free
 *    - Custom math functions (fm_*)
 *    - Custom memory operations (bmem_*)
 *    - Custom string operations (bstr_*)
 * 
 * Mathematical Foundation:
 * ------------------------
 * R_Ω = Σ_n (ψ_n·χ_n·ρ_n·Δ_n·Σ_n·Ω_n)^{Φλ}
 * 
 * Where the ψχρΔΣΩ cycle represents:
 *   ψ (psi):   Perception - Input processing and validation
 *   χ (chi):   Feedback - Retroalignment and coherence check
 *   ρ (rho):   Expansion - Transformation and computation
 *   Δ (Delta): Validation - Verification of results
 *   Σ (Sigma): Execution - Synthesis and output
 *   Ω (Omega): Alignment - Ethical coherence (Φ_ethica)
 * 
 * Copyright (c) 2024-present instituto-Rafael
 * License: GPLv3
 * Attribution: RAFAELIA Framework - RAFCODE-Φ
 */

#include "baremetal.h"

#include <fcntl.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "TermuxBareMetal"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...)
#endif


/* ============================================================================
 * Runtime architecture detection (HWCAP/CPUID)
 * ========================================================================== */

typedef struct {
    uint32_t caps_runtime;
    uint32_t caps_binary;
    int runtime_valid;
    int initialized;
} arch_caps_rt_t;

static arch_caps_rt_t g_arch_caps = {0, 0, 0, 0};

static uint32_t get_binary_caps(void) {
    uint32_t caps = 0;

    #if HAS_NEON
    caps |= CAP_NEON;
    #endif

    #if defined(__ARM_NEON) || defined(__ARM_NEON__)
    caps |= CAP_ASIMD;
    #endif

    #if HAS_AVX2
    caps |= CAP_AVX2;
    #endif

    #if HAS_AVX
    caps |= CAP_AVX;
    #endif

    #if HAS_SSE42
    caps |= CAP_SSE42;
    #endif

    #if HAS_SSE2
    caps |= CAP_SSE2;
    #endif

    #if defined(__SSE__)
    caps |= CAP_SSE;
    #endif

    return caps;
}

#if defined(__linux__) && (defined(__aarch64__) || defined(__arm__) || defined(__arm64__))
typedef struct {
    unsigned long a_type;
    unsigned long a_val;
} auxv_ent_t;

#ifndef AT_HWCAP
#define AT_HWCAP 16
#endif

#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif

#ifndef HWCAP_ASIMD
#define HWCAP_ASIMD (1UL << 1)
#endif

#ifndef HWCAP_SVE
#define HWCAP_SVE (1UL << 22)
#endif

#ifndef HWCAP2_SVE2
#define HWCAP2_SVE2 (1UL << 1)
#endif

#ifndef HWCAP_NEON
#define HWCAP_NEON (1UL << 12)
#endif

static int read_auxv_hwcap(unsigned long* hwcap, unsigned long* hwcap2) {
    int fd = open("/proc/self/auxv", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return 0;
    }

    auxv_ent_t ent;
    int got = 0;
    *hwcap = 0;
    *hwcap2 = 0;

    while (read(fd, &ent, sizeof(ent)) == (ssize_t)sizeof(ent)) {
        if (ent.a_type == 0) break;
        if (ent.a_type == AT_HWCAP) {
            *hwcap = ent.a_val;
            got |= 1;
        } else if (ent.a_type == AT_HWCAP2) {
            *hwcap2 = ent.a_val;
            got |= 2;
        }
    }

    close(fd);
    return (got & 1) != 0;
}

static uint32_t detect_runtime_caps_arm(int* valid) {
    unsigned long hwcap = 0;
    unsigned long hwcap2 = 0;
    if (!read_auxv_hwcap(&hwcap, &hwcap2)) {
        *valid = 0;
        return 0;
    }

    uint32_t caps = 0;
    #if defined(__aarch64__) || defined(__arm64__)
    if (hwcap & HWCAP_ASIMD) {
        caps |= CAP_NEON;
        caps |= CAP_ASIMD;
    }
    #else
    if (hwcap & HWCAP_NEON) {
        caps |= CAP_NEON;
    }
    #endif
    if (hwcap & HWCAP_SVE) {
        caps |= CAP_SVE;
    }
    if (hwcap2 & HWCAP2_SVE2) {
        caps |= CAP_SVE2;
    }

    *valid = 1;
    return caps;
}
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
static void cpuid_query(uint32_t leaf, uint32_t subleaf,
                        uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    uint32_t a, b, c, d;
    #if defined(__i386__) && defined(__PIC__)
    __asm__ volatile(
        "xchgl %%ebx, %1\n\t"
        "cpuid\n\t"
        "xchgl %%ebx, %1\n\t"
        : "=a"(a), "=&r"(b), "=c"(c), "=d"(d)
        : "0"(leaf), "2"(subleaf));
    #else
    __asm__ volatile(
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "0"(leaf), "2"(subleaf));
    #endif
    *eax = a;
    *ebx = b;
    *ecx = c;
    *edx = d;
}

static uint64_t xgetbv_query(uint32_t xcr) {
    uint32_t eax, edx;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
    return ((uint64_t)edx << 32) | eax;
}

static uint32_t detect_runtime_caps_x86(int* valid) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    cpuid_query(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax == 0) {
        *valid = 0;
        return 0;
    }

    uint32_t max_leaf = eax;
    uint32_t caps = 0;

    cpuid_query(1, 0, &eax, &ebx, &ecx, &edx);
    if (edx & (1u << 25)) caps |= CAP_SSE;
    if (edx & (1u << 26)) caps |= CAP_SSE2;
    if (ecx & (1u << 20)) caps |= CAP_SSE42;

    int avx_hw = 0;
    if ((ecx & (1u << 27)) && (ecx & (1u << 28))) {
        uint64_t xcr0 = xgetbv_query(0);
        avx_hw = ((xcr0 & 0x6u) == 0x6u);
        if (avx_hw) {
            caps |= CAP_AVX;
        }
    }

    if (max_leaf >= 7) {
        cpuid_query(7, 0, &eax, &ebx, &ecx, &edx);
        if (avx_hw && (ebx & (1u << 5))) {
            caps |= CAP_AVX2;
        }
    }

    *valid = 1;
    return caps;
}
#endif

static void init_runtime_caps_once(void) {
    if (g_arch_caps.initialized) {
        return;
    }

    g_arch_caps.caps_binary = get_binary_caps();
    g_arch_caps.caps_runtime = 0;
    g_arch_caps.runtime_valid = 0;

    #if defined(__linux__) && (defined(__aarch64__) || defined(__arm__) || defined(__arm64__))
    g_arch_caps.caps_runtime = detect_runtime_caps_arm(&g_arch_caps.runtime_valid);
    #elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
    g_arch_caps.caps_runtime = detect_runtime_caps_x86(&g_arch_caps.runtime_valid);
    #endif

    g_arch_caps.initialized = 1;
}

/* ============================================================================
 * Vector Operations - Optimized for each architecture
 * ========================================================================== */

void vop_add(const float* a, const float* b, float* r, uint32_t n) {
    if (!a || !b || !r || n == 0) return;

#if defined(HAS_BM_NEON_ASM)
    if (bm_can_use_neon_asm()) {
        const uint32_t step = bm_simd_step_f32();
        const uint32_t simd_n = n - (n % step);
        if (simd_n != 0) {
            bm_vadd_neon(a, b, r, simd_n);
        }
        for (uint32_t i = simd_n; i < n; i++) {
            r[i] = a[i] + b[i];
        }
        return;
    }
#endif

    for (uint32_t i = 0; i < n; i++) {
        r[i] = a[i] + b[i];
    }
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

void vop_scale(float* a, float s, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        a[i] *= s;
    }
}

void vop_copy(const float* a, float* r, uint32_t n) {
    if (!a || !r) return;
    bmem_cpy(r, a, n * sizeof(float));
}

void vop_fill(float* a, float v, uint32_t n) {
    if (!a) return;
    for (uint32_t i = 0; i < n; i++) {
        a[i] = v;
    }
}

float vop_sum(const float* a, uint32_t n) {
    if (!a) return 0.0f;
    float s = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        s += a[i];
    }
    return s;
}

float vop_min(const float* a, uint32_t n) {
    if (!a || n == 0) return 0.0f;
    float v = a[0];
    for (uint32_t i = 1; i < n; i++) {
        if (a[i] < v) v = a[i];
    }
    return v;
}

float vop_max(const float* a, uint32_t n) {
    if (!a || n == 0) return 0.0f;
    float v = a[0];
    for (uint32_t i = 1; i < n; i++) {
        if (a[i] > v) v = a[i];
    }
    return v;
}

float vop_dot(const float* a, const float* b, uint32_t n) {
    if (!a || !b || n == 0) return 0.0f;

#if defined(HAS_BM_NEON_ASM)
    if (bm_can_use_neon_asm()) {
        const uint32_t step = bm_simd_step_f32();
        const uint32_t simd_n = n - (n % step);
        float s = 0.0f;
        if (simd_n != 0) {
            s = bm_dot_neon(a, b, simd_n);
        }
        for (uint32_t i = simd_n; i < n; i++) {
            s += a[i] * b[i];
        }
        return s;
    }
#endif

    float s = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        s += a[i] * b[i];
    }
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

static inline float fm_abs(float x) {
    return (x < 0.0f) ? -x : x;
}

mx_t* mx_create(uint32_t r, uint32_t c) {
    mx_t* m = (mx_t*)malloc(sizeof(mx_t));
    if (!m) return NULL;
    
    /* Allocate matrix data */
    m->m = (float*)malloc(r * c * sizeof(float));
    if (!m->m) {
        free(m);
        return NULL;
    }
    bmem_zero(m->m, r * c * sizeof(float));
    
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

void mx_zero(mx_t* m) {
    if (!m || !m->m) return;
    bmem_zero(m->m, m->r * m->c * sizeof(float));
}

void mx_copy(const mx_t* a, mx_t* r) {
    if (!a || !r) return;
    if (a->r != r->r || a->c != r->c) return;
    bmem_cpy(r->m, a->m, a->r * a->c * sizeof(float));
}

void mx_fill(mx_t* m, float v) {
    if (!m || !m->m) return;
    uint32_t n = m->r * m->c;
    for (uint32_t i = 0; i < n; i++) {
        m->m[i] = v;
    }
}

/* Determinant calculation - deterministic approach using RAFAELIA method */
float mx_det(const mx_t* m) {
    if (!m || m->r != m->c) return 0.0f;
    
    if (m->r == 1) {
        return m->m[0];
    }
    
    if (m->r == 2) {
        return m->m[0] * m->m[3] - m->m[1] * m->m[2];
    }
    
    if (m->r == 3) {
        /* Sarrus rule for 3x3 matrices - deterministic */
        float a = m->m[0], b = m->m[1], c = m->m[2];
        float d = m->m[3], e = m->m[4], f = m->m[5];
        float g = m->m[6], h = m->m[7], i = m->m[8];
        return a*e*i + b*f*g + c*d*h - c*e*g - b*d*i - a*f*h;
    }
    
    /* For larger matrices, use Gaussian elimination - O(n^3) */
    /* Create working copy */
    mx_t* w = mx_create(m->r, m->c);
    if (!w) return 0.0f;
    
    bmem_cpy(w->m, m->m, m->r * m->c * sizeof(float));
    
    float det = 1.0f;
    
    /* Forward elimination with partial pivoting */
    for (uint32_t k = 0; k < m->r; k++) {
        /* Find pivot */
        uint32_t pivot = k;
        float max = fm_abs(w->m[k * w->c + k]);
        
        for (uint32_t i = k + 1; i < w->r; i++) {
            float val = fm_abs(w->m[i * w->c + k]);
            if (val > max) {
                max = val;
                pivot = i;
            }
        }
        
        /* Swap rows if needed */
        if (pivot != k) {
            for (uint32_t j = 0; j < w->c; j++) {
                float tmp = w->m[k * w->c + j];
                w->m[k * w->c + j] = w->m[pivot * w->c + j];
                w->m[pivot * w->c + j] = tmp;
            }
            det = -det; /* Row swap changes sign */
        }
        
        float diag = w->m[k * w->c + k];
        if (diag == 0.0f) {
            mx_free(w);
            return 0.0f; /* Singular matrix */
        }
        
        det *= diag;
        
        /* Eliminate column */
        for (uint32_t i = k + 1; i < w->r; i++) {
            float factor = w->m[i * w->c + k] / diag;
            for (uint32_t j = k; j < w->c; j++) {
                w->m[i * w->c + j] -= factor * w->m[k * w->c + j];
            }
        }
    }
    
    mx_free(w);
    return det;
}

/* Matrix inversion using Gauss-Jordan elimination - RAFAELIA deterministic method */
int mx_inv(const mx_t* m, mx_t* r) {
    if (!m || !r) return -1;
    if (m->r != m->c || r->r != m->r || r->c != m->c) return -1;
    
    uint32_t n = m->r;
    
    /* Create augmented matrix [M | I] */
    mx_t* aug = mx_create(n, 2 * n);
    if (!aug) return -1;
    
    /* Copy M to left half */
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < n; j++) {
            aug->m[i * aug->c + j] = m->m[i * m->c + j];
        }
    }
    
    /* Set identity matrix in right half */
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < n; j++) {
            aug->m[i * aug->c + (n + j)] = (i == j) ? 1.0f : 0.0f;
        }
    }
    
    /* Gauss-Jordan elimination with partial pivoting */
    for (uint32_t k = 0; k < n; k++) {
        /* Find pivot */
        uint32_t pivot = k;
        float max = 0.0f;
        
        for (uint32_t i = k; i < n; i++) {
            float val = aug->m[i * aug->c + k];
            float abs_val = fm_abs(val);
            if (abs_val > max) {
                max = abs_val;
                pivot = i;
            }
        }
        
        /* Check for singular matrix */
        if (max < 1e-10f) {
            mx_free(aug);
            return -1;
        }
        
        /* Swap rows if needed */
        if (pivot != k) {
            for (uint32_t j = 0; j < aug->c; j++) {
                float tmp = aug->m[k * aug->c + j];
                aug->m[k * aug->c + j] = aug->m[pivot * aug->c + j];
                aug->m[pivot * aug->c + j] = tmp;
            }
        }
        
        /* Scale pivot row */
        float diag = aug->m[k * aug->c + k];
        for (uint32_t j = 0; j < aug->c; j++) {
            aug->m[k * aug->c + j] /= diag;
        }
        
        /* Eliminate column in all other rows */
        for (uint32_t i = 0; i < n; i++) {
            if (i != k) {
                float factor = aug->m[i * aug->c + k];
                for (uint32_t j = 0; j < aug->c; j++) {
                    aug->m[i * aug->c + j] -= factor * aug->m[k * aug->c + j];
                }
            }
        }
    }
    
    /* Extract inverse from right half */
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < n; j++) {
            r->m[i * r->c + j] = aug->m[i * aug->c + (n + j)];
        }
    }
    
    mx_free(aug);
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
 * Advanced Matrix Operations - RAFAELIA extended methods
 * ========================================================================== */

/* Element-wise matrix addition */
void mx_add(const mx_t* a, const mx_t* b, mx_t* r) {
    if (!a || !b || !r) return;
    if (a->r != b->r || a->c != b->c) return;
    if (r->r != a->r || r->c != a->c) return;
    
    uint32_t n = a->r * a->c;
    for (uint32_t i = 0; i < n; i++) {
        r->m[i] = a->m[i] + b->m[i];
    }
}

/* Element-wise matrix subtraction */
void mx_sub(const mx_t* a, const mx_t* b, mx_t* r) {
    if (!a || !b || !r) return;
    if (a->r != b->r || a->c != b->c) return;
    if (r->r != a->r || r->c != a->c) return;
    
    uint32_t n = a->r * a->c;
    for (uint32_t i = 0; i < n; i++) {
        r->m[i] = a->m[i] - b->m[i];
    }
}

/* Scalar multiplication */
void mx_scale(mx_t* m, float s) {
    if (!m) return;
    
    uint32_t n = m->r * m->c;
    for (uint32_t i = 0; i < n; i++) {
        m->m[i] *= s;
    }
}

/* Trace - sum of diagonal elements */
float mx_trace(const mx_t* m) {
    if (!m || m->r != m->c) return 0.0f;
    
    float trace = 0.0f;
    for (uint32_t i = 0; i < m->r; i++) {
        trace += m->m[i * m->c + i];
    }
    return trace;
}

/* Set matrix to identity */
void mx_identity(mx_t* m) {
    if (!m || m->r != m->c) return;

    mx_zero(m);
    for (uint32_t i = 0; i < m->r; i++) {
        m->m[i * m->c + i] = 1.0f;
    }
}

/* Solve linear system Ax = b using Gaussian elimination with back substitution
 * RAFAELIA deterministic method - no external dependencies
 * Returns 0 on success, -1 on failure (singular matrix)
 */
int mx_solve_linear(const mx_t* a, const float* b, float* x) {
    if (!a || !b || !x) return -1;
    if (a->r != a->c) return -1;  /* Must be square */
    
    uint32_t n = a->r;
    
    /* Create augmented matrix [A | b] */
    mx_t* aug = mx_create(n, n + 1);
    if (!aug) return -1;
    
    /* Copy A to left part */
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < n; j++) {
            aug->m[i * aug->c + j] = a->m[i * a->c + j];
        }
        /* Copy b to right column */
        aug->m[i * aug->c + n] = b[i];
    }
    
    /* Forward elimination with partial pivoting */
    for (uint32_t k = 0; k < n; k++) {
        /* Find pivot */
        uint32_t pivot = k;
        float max = 0.0f;
        
        for (uint32_t i = k; i < n; i++) {
            float val = aug->m[i * aug->c + k];
            float abs_val = fm_abs(val);
            if (abs_val > max) {
                max = abs_val;
                pivot = i;
            }
        }
        
        /* Check for singular matrix */
        if (max < 1e-10f) {
            mx_free(aug);
            return -1;
        }
        
        /* Swap rows if needed */
        if (pivot != k) {
            for (uint32_t j = k; j <= n; j++) {
                float tmp = aug->m[k * aug->c + j];
                aug->m[k * aug->c + j] = aug->m[pivot * aug->c + j];
                aug->m[pivot * aug->c + j] = tmp;
            }
        }
        
        /* Eliminate column below pivot */
        for (uint32_t i = k + 1; i < n; i++) {
            float factor = aug->m[i * aug->c + k] / aug->m[k * aug->c + k];
            for (uint32_t j = k; j <= n; j++) {
                aug->m[i * aug->c + j] -= factor * aug->m[k * aug->c + j];
            }
        }
    }
    
    /* Back substitution */
    for (int i = n - 1; i >= 0; i--) {
        float sum = aug->m[i * aug->c + n];
        for (uint32_t j = i + 1; j < n; j++) {
            sum -= aug->m[i * aug->c + j] * x[j];
        }
        x[i] = sum / aug->m[i * aug->c + i];
    }
    
    mx_free(aug);
    return 0;
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
    if (!d || !s || n == 0) return d;

    char* pd = (char*)d;
    const char* ps = (const char*)s;

#if defined(HAS_BM_NEON_ASM)
    if (bm_can_use_neon_asm()) {
        const size_t step = bm_simd_step_u8();
        while (n != 0 && ((((uintptr_t)pd & (step - 1u)) != 0u) || (((uintptr_t)ps & (step - 1u)) != 0u))) {
            *pd++ = *ps++;
            n--;
        }

        const size_t simd_n = n - (n % step);
        if (simd_n != 0) {
            bm_memcpy_neon(pd, ps, simd_n);
            pd += simd_n;
            ps += simd_n;
            n -= simd_n;
        }

        while (n != 0) {
            *pd++ = *ps++;
            n--;
        }
        return d;
    }
#endif

    while (n != 0) {
        *pd++ = *ps++;
        n--;
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

void* bmem_zero(void* d, size_t n) {
    return bmem_set(d, 0, n);
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
    init_runtime_caps_once();

    const uint32_t caps = g_arch_caps.runtime_valid
        ? g_arch_caps.caps_runtime
        : g_arch_caps.caps_binary;

    LOGD("Architecture: %s, RuntimeValid: %d, RuntimeCaps: 0x%08x, BinaryCaps: 0x%08x, EffectiveCaps: 0x%08x",
         ARCH_NAME,
         g_arch_caps.runtime_valid,
         g_arch_caps.caps_runtime,
         g_arch_caps.caps_binary,
         caps);

    return caps;
}

uint32_t get_arch_runtime_caps(void) {
    init_runtime_caps_once();
    return g_arch_caps.caps_runtime;
}

uint32_t get_arch_binary_caps(void) {
    init_runtime_caps_once();
    return g_arch_caps.caps_binary;
}

int get_arch_runtime_caps_valid(void) {
    init_runtime_caps_once();
    return g_arch_caps.runtime_valid;
}
