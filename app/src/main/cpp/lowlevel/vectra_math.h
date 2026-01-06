#ifndef TERMUX_VECTRA_MATH_H
#define TERMUX_VECTRA_MATH_H

#include <stdint.h>
#include <stddef.h>

/**
 * Low-level bare-metal mathematical operations for Vectra Analysis (VA)
 * Optimized for minimal dependencies and maximum performance
 */

// VA valid structure (Vectra pairing with 5 fixed items)
typedef struct {
    // Space: fixed dimension n
    uint32_t space_dim;
    
    // Features: base channel (HASH, TEXT, PHONEME, IMAGE FFT)
    uint8_t feature_type; // 0=HASH, 1=TEXT, 2=PHONEME, 3=IMAGE_FFT
    
    // Pairing mode data
    float cosine_similarity;
    float euclidean_distance;
    float delta_h;
    
    // Invariance test flags
    uint8_t reversal_invariant;
    uint8_t permutation_invariant;
    uint8_t noise_invariant;
    
    // Output specification
    uint8_t output_format; // 0=JSON, 1=CSV
} va_context_t;

// ANOVA bridge structures for derivatives/antiderivatives
typedef struct {
    float* coefficients;
    size_t coeff_count;
    float ss_total;    // Sum of squares total
    float ss_model;    // Sum of squares model
    float ss_error;    // Sum of squares error
} anova_result_t;

/**
 * Initialize VA context with bare-metal optimization
 * No external dependencies, direct memory manipulation
 */
void va_init(va_context_t* ctx, uint32_t space_dim, uint8_t feature_type);

/**
 * Compute pairing similarity using optimized bare-metal operations
 * Uses direct FPU instructions where available
 */
float va_compute_cosine_similarity(const float* v1, const float* v2, size_t dim);

/**
 * Compute Euclidean distance with SIMD optimization hints
 */
float va_compute_euclidean_distance(const float* v1, const float* v2, size_t dim);

/**
 * Test reversal invariance (vector reversal stability test)
 */
uint8_t va_test_reversal_invariance(const float* v, size_t dim, float threshold);

/**
 * ANOVA least squares parameter estimation
 * Minimizes: d/d_beta sum_i (y_i - y_hat_i(beta))^2 = 0
 */
int anova_fit_least_squares(const float* x, const float* y, size_t n, 
                            anova_result_t* result);

/**
 * Compute ANOVA decomposition: SS_T = SS_M + SS_E
 */
void anova_compute_ss_decomposition(const float* y, const float* y_pred, 
                                    size_t n, float* ss_t, float* ss_m, float* ss_e);

/**
 * Low-level memory operations optimized for bare-metal
 * Direct assembly hints for critical sections
 */
void* va_memcpy_optimized(void* dest, const void* src, size_t n);
void* va_memset_optimized(void* s, int c, size_t n);

/**
 * Fast mathematical operations with minimal library dependencies
 */
float va_sqrt_fast(float x);
float va_pow_fast(float base, int exp);

/**
 * Fallback implementation for non-ARM architectures
 */
float va_dot_product_fallback(const float* v1, const float* v2, size_t n);

#endif // TERMUX_VECTRA_MATH_H
